cmake_minimum_required (VERSION 4.1.2)
project (UQM-MegaMod VERSION 0.8.4 LANGUAGES C CXX)

include (FetchContent)
include (helpers.cmake)

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED ON)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

message (STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set (CMAKE_C_FLAGS_DEBUG "-g -Og -Wall -DDEBUG")
set (CMAKE_C_FLAGS_RELEASE "-O2 -s -DNDEBUG")
set (COMMON_C_FLAGS "")

set (CXX_EXTRA_FLAGS "-fno-rtti -fno-exceptions -nostdinc++")

set (USE_WINSOCK FALSE)
set (IS_MINGW FALSE)
set (IS_WINDOWS FALSE)
set (ZLIB_FETCHED FALSE)

if (CMAKE_HOST_WIN32)
	set (IS_WINDOWS TRUE)
	if (CMAKE_C_COMPILER_ID MATCHES "GNU")
		set (IS_MINGW TRUE)
	endif ()
endif ()

if (CMAKE_BUILD_TYPE)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set (UQM_DEBUG ON)
	else ()
		set (UQM_DEBUG OFF)
	endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
	option (UQM_DEBUG           "Enable debug build"                ON)
endif ()
option (UQM_STRICT_DEBUG        "Enable strict debug checks"        OFF)
option (UQM_USE_INTERNAL_MIKMOD "Use included libmikmod"            ON)
option (UQM_USE_INTERNAL_LUA    "Use included Lua library"          ON)
option (UQM_NETPLAY             "Enable IPv4 and IPv6 network play" ON)
option (UQM_JOYSTICK            "Enable joystick support"           ON)
option (UQM_ZIP_IO              "Enable direct & .zip file I/O"     ON)
option (UQM_PLATFORM_ACCEL      "Enable platform acceleration"      ON)

set (UQM_GFX_BACKEND   "SDL2")
set (UQM_THREAD_LIB    "SDL")
set (UQM_SOUND_BACKEND "mixsdl")

if (UQM_NETPLAY AND IS_WINDOWS)
	set (USE_WINSOCK TRUE)
endif ()

# Find/Fetch the dependencies

find_package (PkgConfig QUIET)
if (PkgConfig_FOUND)
	message (STATUS "Found pkg-config: ${PkgConfig_VERSION}")
else ()
	message (STATUS "pkg-config not found")
endif ()

if (PkgConfig_FOUND)
	pkg_check_modules (SDL2 QUIET sdl2)
else ()
	find_package (SDL2 QUIET)
endif ()

if (SDL2_FOUND)
	message (STATUS "Found SDL2: ${SDL2_VERSION}")
else ()
	FetchSDL ()
	message (STATUS "Fetched SDL2: ${SDL2_VERSION}")
endif ()

if (PkgConfig_FOUND)
	pkg_check_modules (ZLIB QUIET zlib)
else ()
	find_package (ZLIB QUIET)
endif ()

if (ZLIB_FOUND)
	message (STATUS "Found ZLIB: ${ZLIB_VERSION}")
else ()
	FetchZLIB ()
	message (STATUS "Fetched ZLIB: ${ZLIB_VERSION}")
endif ()

if (UQM_ZIP_IO)
	set (HAVE_ZIP 1)
	set (USE_ZIP_IO 1)
else ()
	message (STATUS "ZIP I/O support disabled")
	set (HAVE_ZIP 0)
	set (USE_ZIP_IO 0)
endif ()

if (PkgConfig_FOUND)
	pkg_check_modules (PNG QUIET libpng)
else ()
	find_package (PNG QUIET)
endif ()

if (PNG_FOUND)
	message (STATUS "Found libPNG: ${PNG_VERSION}")
else ()
	FetchPNG (${UQM_ZIP_IO} ${ZLIB_FETCHED} ${ZLIB_LIBRARIES})
	message (STATUS "Fetched libPNG: ${PNG_VERSION}")
endif ()

if (PkgConfig_FOUND)
	pkg_check_modules (OGG QUIET ogg)
else ()
	find_package (OGG QUIET)
endif ()

if (OGG_FOUND)
	message (STATUS "Found ogg: ${OGG_VERSION}")
else ()
	FetchOgg ()
	message (STATUS "Fetched ogg: ${OGG_VERSION}")
endif ()

if (PkgConfig_FOUND)
	pkg_check_modules (VORBIS QUIET vorbis)
	pkg_check_modules (VORBISFILE QUIET vorbisfile)
	if (NOT VORBISFILE_FOUND)
		set (VORBIS_FOUND 0)
		find_package (VORBIS QUIET)
	endif ()
else ()
	find_package (VORBIS QUIET)
endif ()

if (VORBIS_FOUND)
	message (STATUS "Found Vorbis: ${VORBIS_VERSION}")
	set (UQM_OGG_CODEC "standard")
else ()
	FetchVorbis ()
	message (STATUS "Fetched Vorbis: ${VORBIS_VERSION}")
endif ()

include (CheckSymbolExists)
include (CheckIncludeFile)
include (CheckTypeSize)

include (TestBigEndian)
test_big_endian (WORDS_BIGENDIAN)
if (WORDS_BIGENDIAN)
	set (WORDS_BIGENDIAN_DEF 1)
	message (STATUS "Big-endian machine detected.")
else ()
	message (STATUS "Little-endian machine detected.")
endif ()

#quieter_check_symbol function defined in helpers.cmake
#     symbol to check ↓     header ↓   save to var ↓
quieter_check_symbol (readdir_r   "dirent.h"  HAVE_READDIR_R)
quieter_check_symbol (setenv      "stdlib.h"  HAVE_SETENV)
quieter_check_symbol (strupr      "string.h"  HAVE_STRUPR)
quieter_check_symbol (strcasecmp  "strings.h" HAVE_STRCASECMP_UQM)
quieter_check_symbol (stricmp     "string.h"  HAVE_STRICMP)
quieter_check_symbol (acos        "math.h"    HAVE_ACOS)
quieter_check_symbol (iswgraph    "wctype.h"  HAVE_ISWGRAPH)
quieter_check_symbol (getopt_long "getopt.h"  HAVE_GETOPT_LONG)
#check_type function defined in helpers.cmake
# type to check ↓ save to var ↓
check_type ("wchar_t" HAVE_WCHAR_T)
check_type ("wint_t"  HAVE_WINT_T)
check_type ("_Bool"   HAVE__BOOL)
#check_include function defined in helpers.cmake
#        header ↓   save to var ↓
check_include ("regex.h" HAVE_REGEX_H)

if (NOT HAVE_ACOS)
	find_library (MATH_LIBRARY m)
	if (MATH_LIBRARY)
		set (EXTRA_LIBS ${EXTRA_LIBS} ${MATH_LIBRARY})
		message (STATUS "Math library needed: ${MATH_LIBRARY}")
	endif ()
endif ()

if (UQM_DEBUG)
	if (UQM_STRICT_DEBUG)
		list (APPEND WARNING_FLAGS
				-Wcast-qual -Wmissing-declarations -Wwrite-strings
				-Wimplicit -Wreturn-type -Wformat -Wswitch -Wcomment
				-Wchar-subscripts -Wparentheses -Wcast-align -Wuninitialized
				-Wbad-function-cast -Wmissing-prototypes -Wstrict-prototypes
				-Wdeclaration-after-statement
		)
		string (REPLACE ";" " " WARNING_FLAGS_DELIM "${WARNING_FLAGS}")
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_DELIM}")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_DELIM}")
	endif ()
	set (DEBUG 1)
endif ()

if (UQM_NETPLAY)
	if (IS_WINDOWS)
		set (EXTRA_LIBS ${EXTRA_LIBS} ws2_32)
	endif ()
	set (NETPLAY "FULL")
else ()
	set (NETPLAY "")
endif ()

#set_if_else function defined in helpers.cmake
#  condition ↓       var to set ↓           if true ↓   ↓ else
set_if_else (UQM_PLATFORM_ACCEL USE_PLATFORM_ACCEL "1" "0")

#set_if function defined in helpers.cmake
# condition ↓            var to set ↓      set value ↓
set_if (UQM_JOYSTICK            HAVE_JOYSTICK       "1")
set_if (UQM_USE_INTERNAL_MIKMOD USE_INTERNAL_MIKMOD "1")
set_if (UQM_USE_INTERNAL_LUA    USE_INTERNAL_LUA    "1")

#function to parse the Makeinfo in every subdirectory using the existing
# recurse script
function (parse_makeinfo_with_recurse target_sources)
	set (ENV{BUILD_PROJECT} "uqm")
	set (ENV{BUILD_ROOT} "${CMAKE_SOURCE_DIR}/")

	#set_makeinfo_env function defined in helpers.cmake
	#  env_var to set ↓                    ↓ conditional
	set_makeinfo_env (uqm_HAVE_READDIR_R   HAVE_READDIR_R)
	set_makeinfo_env (uqm_HAVE_SETENV      HAVE_SETENV)
	set_makeinfo_env (uqm_HAVE_STRUPR      HAVE_STRUPR)
	set_makeinfo_env (uqm_HAVE_STRCASECMP  HAVE_STRCASECMP_UQM)
	set_makeinfo_env (uqm_HAVE_STRICMP     HAVE_STRICMP)
	set_makeinfo_env (uqm_HAVE_GETOPT_LONG HAVE_GETOPT_LONG)
	set_makeinfo_env (uqm_HAVE_REGEX       HAVE_REGEX_H)
	set_makeinfo_env (uqm_USE_ZIP_IO       USE_ZIP_IO)

	set (ENV{uqm_THREADLIB} "${UQM_THREAD_LIB}")

	#set_env_if function defined in helpers.cmake
	#           ↓ env_var to set  set to ↓    ↓ conditional
	set_env_if (DEBUG                   "1"   UQM_DEBUG)
	set_env_if (uqm_USE_INTERNAL_MIKMOD "1"   UQM_USE_INTERNAL_MIKMOD)
	set_env_if (uqm_USE_INTERNAL_LUA    "1"   UQM_USE_INTERNAL_LUA)
	set_env_if (uqm_NETPLAY             "1"   UQM_NETPLAY)
	set_env_if (MACRO___MINGW32__       "1"   IS_MINGW)
	set_env_if (uqm_USE_WINSOCK         "1"   USE_WINSOCK)
	set_env_if (uqm_GFXMODULE           "sdl" UQM_GFX_BACKEND)

	if (APPLE)
		set (ENV{HOST_SYSTEM} "Darwin")
	elseif (WIN32 OR IS_MINGW)
		set (ENV{HOST_SYSTEM} "MINGW32_NT")
	else ()
		set (ENV{HOST_SYSTEM} "Linux")
	endif ()

	set (ENV{BUILD_WORK} "${CMAKE_SOURCE_DIR}")

	execute_process (
			COMMAND sh "${CMAKE_SOURCE_DIR}/build/unix/recurse" "uqm"
					"${CMAKE_SOURCE_DIR}"
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE recurse_output
			ERROR_VARIABLE recurse_error
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE recurse_result
	)

	if (recurse_error)
		message (STATUS "Recurse stderr: ${recurse_error}")
	endif ()

	if (NOT recurse_result EQUAL 0)
		message (FATAL_ERROR "recurse script failed with exit code:
				${recurse_result}")
	endif ()

	string (REPLACE "\n" ";" file_lines "${recurse_output}")
	set (source_files "")

	foreach (line IN LISTS file_lines)
		if (line MATCHES "^(C|CXX|M|RC)\t(.*)$")
			set (file_type "${CMAKE_MATCH_1}")
			set (source_path "${CMAKE_MATCH_2}")
			set (full_source_path "${CMAKE_SOURCE_DIR}/${source_path}")

			if (EXISTS "${full_source_path}")
				list (APPEND source_files "${full_source_path}")
			endif ()
		endif ()
	endforeach ()

	list (REMOVE_DUPLICATES source_files)
	set (${target_sources} ${source_files} PARENT_SCOPE)
endfunction ()

set (UQM_SOURCES)
message (STATUS "Parsing Makeinfo for source files...")
parse_makeinfo_with_recurse (UQM_SOURCES)

list (LENGTH UQM_SOURCES UQM_SOURCES_COUNT)
message (STATUS "Found ${UQM_SOURCES_COUNT} files")

if (NOT UQM_SOURCES)
	message (FATAL_ERROR "No source files found!")
endif ()

#set_config_define function defined in helpers.cmake
set_config_define (WORDS_BIGENDIAN)
set_config_define (HAVE_READDIR_R)
set_config_define (HAVE_SETENV)
set_config_define (HAVE_STRUPR)
set_config_define (HAVE_STRCASECMP_UQM)
set_config_define (HAVE_STRICMP)
set_config_define (HAVE_GETOPT_LONG)

set (HAVE_ISWGRAPH "#define HAVE_ISWGRAPH")
set (HAVE_WCHAR_T  "#define HAVE_WCHAR_T")
set (HAVE_WINT_T   "#define HAVE_WINT_T")
set (HAVE__BOOL    "#define HAVE__BOOL")

if (IS_WINDOWS)
	configure_file (
			${CMAKE_SOURCE_DIR}/src/config_win.h.in
			${CMAKE_SOURCE_DIR}/src/config_win.h
			@ONLY
	)
	message (STATUS "Generated config_win.h in src directory")
else ()
	configure_file (
			${CMAKE_SOURCE_DIR}/src/config_unix.h.in
			${CMAKE_SOURCE_DIR}/src/config_unix.h
			@ONLY
	)
	message (STATUS "Generated config_unix.h in src directory")
endif ()

add_executable (UrQuanMasters ${UQM_SOURCES})

target_include_directories (UrQuanMasters PRIVATE
		${CMAKE_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/src
		${SDL2_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${VORBISFILE_INCLUDE_DIRS}
)

if (UQM_DEBUG)
	set_target_properties (UrQuanMasters PROPERTIES OUTPUT_NAME
			"UrQuanMastersDebug")
endif ()

if (IS_WINDOWS)
	set_target_properties (UrQuanMasters PROPERTIES
			WIN32_EXECUTABLE FALSE
			LINK_FLAGS "-mconsole"
	)
endif ()

set (UQM_COMPILE_OPTIONS
		$<$<COMPILE_LANGUAGE:C>:${COMMON_C_FLAGS}>
		$<$<COMPILE_LANGUAGE:CXX>:${CXX_EXTRA_FLAGS}>
)

target_compile_options (UrQuanMasters PRIVATE ${UQM_COMPILE_OPTIONS})

append_list (UQM_DEFINITIONS GFXMODULE_SDL          IS_WINDOWS)
append_list (UQM_DEFINITIONS HAVE_OPENAL            HAVE_OPENAL)
append_list (UQM_DEFINITIONS USE_INTERNAL_MIKMOD    USE_INTERNAL_MIKMOD)
append_list (UQM_DEFINITIONS USE_INTERNAL_LUA       USE_INTERNAL_LUA)
append_list (UQM_DEFINITIONS HAVE_JOYSTICK          HAVE_JOYSTICK)
append_list (UQM_DEFINITIONS USE_PLATFORM_ACCEL     USE_PLATFORM_ACCEL)
append_list (UQM_DEFINITIONS HAVE_ZIP               HAVE_ZIP)
append_list (UQM_DEFINITIONS DEBUG                  DEBUG)
append_list (UQM_DEFINITIONS "NETPLAY=NETPLAY_FULL" UQM_NETPLAY)

list (APPEND UQM_DEFINITIONS THREADLIB_SDL)

target_compile_definitions (UrQuanMasters PRIVATE ${UQM_DEFINITIONS})

set (UQM_LINK_LIBRARIES
		${SDL2_LIBRARIES}
		${PNG_LIBRARIES}
		${OGG_LIBRARIES}
		${VORBIS_LIBRARIES}
		${VORBISFILE_LIBRARIES}
		${EXTRA_LIBS}
)

append_list (UQM_LINK_LIBRARIES "${ZLIB_LIBRARIES}" USE_ZIP_IO)

if (HAVE_REGEX_H AND IS_MINGW OR NOT HAVE_REGEX_H)
	if (PkgConfig_FOUND)
		pkg_check_modules (REGEX QUIET regex)
	else ()
		find_package (REGEX QUIET)
	endif ()

	if (REGEX_FOUND)
		list (APPEND UQM_LINK_LIBRARIES ${REGEX_LIBRARIES})
		message (STATUS "Using regex: ${REGEX_LIBRARIES}")
	else ()
		find_library (REGEX_LIBRARY regex)
		if (REGEX_LIBRARY)
			list (APPEND UQM_LINK_LIBRARIES ${REGEX_LIBRARY})
			message (STATUS "Using regex library: ${REGEX_LIBRARY}")
		else ()
			message (FATAL_ERROR "REGEX NOT FOUND!")
		endif ()
	endif ()
endif ()

target_link_libraries (UrQuanMasters ${UQM_LINK_LIBRARIES})

message (STATUS "")
message (STATUS "=== UQM-MegaMod Configuration ===")
message (STATUS "Build Type:       ${CMAKE_BUILD_TYPE}")
message (STATUS "Graphics Engine:  ${UQM_GFX_BACKEND} "
		"(SDL_DIR: ${SDL_DIR})")
message (STATUS "Sound Backend:    ${UQM_SOUND_BACKEND}")
message (STATUS "Ogg Vorbis Codec: ${UQM_OGG_CODEC}")
message (STATUS "Thread Library:   ${UQM_THREAD_LIB}")
message (STATUS "Features:")
message (STATUS "  Debug:        ${UQM_DEBUG}")
message (STATUS "  Strict Debug: ${UQM_STRICT_DEBUG}")
message (STATUS "  Joystick:     ${UQM_JOYSTICK}")
message (STATUS "  Netplay:      ${UQM_NETPLAY}")
message (STATUS "  ZIP I/O:      ${UQM_ZIP_IO}")
message (STATUS "  Platform Acceleration: ${UQM_PLATFORM_ACCEL}")
message (STATUS "  Internal MikMod:       ${UQM_USE_INTERNAL_MIKMOD}")
message (STATUS "  Internal Lua:          ${UQM_USE_INTERNAL_LUA}")
message (STATUS "Source Files: ${UQM_SOURCES_COUNT}")
message (STATUS "================================")