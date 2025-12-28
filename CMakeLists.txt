cmake_minimum_required(VERSION 3.15)
project(UQM-MegaMod VERSION 0.8.4 LANGUAGES C)

include(helpers.cmake)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

message (STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_DEBUG "-g -Og -Wall -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -s -DNDEBUG")
set(COMMON_C_FLAGS "")

set(USE_WINSOCK FALSE)
set(IS_MINGW FALSE)
set(IS_WINDOWS FALSE)

if(WIN32 OR MINGW OR MSYS)
    set(IS_WINDOWS TRUE)

    if (MSYS OR MINGW)
        set(IS_MINGW TRUE)
    endif()

    if (UQM_NETPLAY)
        set(USE_WINSOCK TRUE)
    endif()
endif()

if (CMAKE_BUILD_TYPE)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(UQM_DEBUG ON)
    else()
        set(UQM_DEBUG OFF)
    endif()
endif()

if (NOT CMAKE_BUILD_TYPE)
    option(UQM_DEBUG           "Enable debug build" ON)
endif()
option(UQM_STRICT_DEBUG        "Enable strict debug checks" OFF)
option(MEMDEBUG                "Use MEMDEBUG" OFF)
option(UQM_GFX_BACKEND         "Use SDL2 with modern graphics support" ON)
option(UQM_OPENGL              "Enable OpenGL support (SDL1 only)" OFF)
option(UQM_SOUND_BACKEND       "Use MixSDL for sound (internal)" ON)
option(UQM_USE_INTERNAL_MIKMOD "Use included libmikmod" ON)
option(UQM_USE_INTERNAL_LUA    "Use included Lua library" ON)
option(UQM_NETPLAY             "Enable IPv4 and IPv6 network play" ON)
option(UQM_JOYSTICK            "Enable joystick support" ON)
option(UQM_ZIP_IO              "Enable direct & .zip file I/O" ON)
option(UQM_PLATFORM_ACCEL      "Enable platform acceleration" ON)
option(UQM_THREAD_LIB          "Use SDL controlled thread library" ON)

# Ogg Vorbis codec selection (standard, tremor, none)
#        edit here ↓
set(UQM_OGG_CODEC "standard" CACHE STRING "Ogg Vorbis codec (standard, tremor, none)")
set_property(CACHE UQM_OGG_CODEC PROPERTY STRINGS standard tremor none)

#set_if_else function defined in helpers.cmake
# condition ↓      var to set ↓          if true ↓   else ↓
set_if_else(UQM_GFX_BACKEND   UQM_GFX_BACKEND   "SDL2"   "SDL")
set_if_else(UQM_SOUND_BACKEND UQM_SOUND_BACKEND "mixsdl" "openal")
set_if_else(UQM_THREAD_LIB    UQM_THREAD_LIB    "SDL"    "PTHREAD")

if (UQM_NETPLAY AND IS_WINDOWS)
    set(USE_WINSOCK TRUE)
endif()

find_package(PkgConfig REQUIRED)

if(UQM_GFX_BACKEND STREQUAL "SDL2")
    pkg_check_modules(SDL2 QUIET REQUIRED sdl2)
    message(STATUS "Found SDL2: ${SDL2_VERSION}")
    set(SDL_DIR "SDL2")
    set(HAVE_OPENGL 0)
    
    if(UQM_OPENGL)
        set(UQM_OPENGL OFF)
    endif()
    
elseif(UQM_GFX_BACKEND STREQUAL "SDL1")
    pkg_check_modules(SDL QUIET REQUIRED sdl)
    message(STATUS "Found SDL1: ${SDL_VERSION}")
    set(SDL_DIR "SDL")

    if(UQM_OPENGL)
        find_package(OpenGL REQUIRED)
        set(HAVE_OPENGL 1)
        message(STATUS "Using OpenGL with SDL1")
    else()
        set(HAVE_OPENGL 0)
        message(STATUS "Using SDL1 without OpenGL")
    endif()    
else()
    message(FATAL_ERROR "Unsupported graphics backend: ${UQM_GFX_BACKEND}")
endif()

pkg_check_modules(PNG QUIET REQUIRED libpng)
message(STATUS "Found libPNG: ${PNG_VERSION}")

if(UQM_SOUND_BACKEND STREQUAL "openal")
    pkg_check_modules(OPENAL QUIET REQUIRED openal)
    message(STATUS "Found OpenAL: ${OPENAL_VERSION}")
    set(HAVE_OPENAL 1)
else()
    message(STATUS "Using MixSDL for sound")
endif()

if(UQM_OGG_CODEC STREQUAL "standard")
    pkg_check_modules(VORBISFILE QUIET REQUIRED vorbisfile)
    pkg_check_modules(VORBIS QUIET REQUIRED vorbis)
    pkg_check_modules(OGG QUIET REQUIRED ogg)
    message(STATUS "Using standard Ogg Vorbis codec")
    set(OGGVORBIS "vorbisfile")
elseif(UQM_OGG_CODEC STREQUAL "tremor")
    pkg_check_modules(TREMOR QUIET vorbisidec)
    if(TREMOR_FOUND)
        message(STATUS "Using Tremor Ogg Vorbis codec")
        set(OVCODEC_TREMOR 1)
        set(OGGVORBIS "tremor")
    else()
        message(WARNING "Tremor not found, falling back to standard Vorbis")
        pkg_check_modules(VORBISFILE QUIET REQUIRED vorbisfile)
        pkg_check_modules(VORBIS QUIET REQUIRED vorbis)
        pkg_check_modules(OGG QUIET REQUIRED ogg)
        set(OGGVORBIS "vorbisfile")
    endif()
else()
    message(STATUS "Ogg Vorbis support disabled")
    set(OVCODEC_NONE 1)
    set(OGGVORBIS "none")
endif()

if(UQM_ZIP_IO)
    pkg_check_modules(ZLIB QUIET REQUIRED zlib)
    message(STATUS "Found ZLIB: ${ZLIB_VERSION}")
    set(HAVE_ZIP 1)
    set(USE_ZIP_IO 1)
else()
    message(STATUS "ZIP I/O support disabled")
    set(HAVE_ZIP 0)
    set(USE_ZIP_IO 0)
endif()

if(UQM_THREAD_LIB STREQUAL "PTHREAD")
    find_package(Threads REQUIRED)
endif()
set(THREADLIB "${UQM_THREAD_LIB}")

include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckCSourceCompiles)

include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
    set(WORDS_BIGENDIAN_DEF 1)
    message(STATUS "Big-endian machine detected.")
else()
    message(STATUS "Little-endian machine detected.")
endif()

#quieter_check_symbol function defined in helpers.cmake
#    symbol to check ↓     header ↓   save to var ↓
quieter_check_symbol(readdir_r   "dirent.h"  HAVE_READDIR_R)
quieter_check_symbol(setenv      "stdlib.h"  HAVE_SETENV)
quieter_check_symbol(strupr      "string.h"  HAVE_STRUPR)
quieter_check_symbol(strcasecmp  "strings.h" HAVE_STRCASECMP_UQM)
quieter_check_symbol(stricmp     "string.h"  HAVE_STRICMP)
quieter_check_symbol(acos        "math.h"    HAVE_ACOS)
quieter_check_symbol(iswgraph    "wctype.h"  HAVE_ISWGRAPH)
quieter_check_symbol(getopt_long "getopt.h"  HAVE_GETOPT_LONG)
#check_type function defined in helpers.cmake
# type to check ↓ save to var ↓
check_type("wchar_t" HAVE_WCHAR_T)
check_type("wint_t"  HAVE_WINT_T)
check_type("_Bool"   HAVE_WINT_T)
#check_include function defined in helpers.cmake
#       header ↓   save to var ↓
check_include("regex.h" HAVE_REGEX_H)

if(NOT HAVE_ACOS)
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${MATH_LIBRARY})
        message(STATUS "Math library needed: ${MATH_LIBRARY}")
    endif()
endif()

if(UQM_DEBUG)
    if(UQM_STRICT_DEBUG)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-qual -Wmissing-declarations -Wwrite-strings -Wimplicit -Wreturn-type -Wformat -Wswitch -Wcomment -Wchar-subscripts -Wparentheses -Wcast-align -Wuninitialized")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wbad-function-cast -Wmissing-prototypes -Wstrict-prototypes -Wdeclaration-after-statement")
    endif()
    set(DEBUG 1)
endif()

if(UQM_NETPLAY)
    if(IS_WINDOWS)
        set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32)
    endif()
    set(NETPLAY "FULL")
else()
    set(NETPLAY "")
endif()

set_if_else(UQM_PLATFORM_ACCEL      USE_PLATFORM_ACCEL  "1" "0")
#set_if_else function defined in helpers.cmake
# condition ↓            var to set ↓          set value ↓
set_if(     UQM_JOYSTICK            HAVE_JOYSTICK       "1")
set_if(     UQM_USE_INTERNAL_MIKMOD USE_INTERNAL_MIKMOD "1")
set_if(     UQM_USE_INTERNAL_LUA    USE_INTERNAL_LUA    "1")

#function to parse the Makeinfo in every subdirectory using the existing recurse script
function(parse_makeinfo_with_recurse target_sources)
    set(ENV{BUILD_PROJECT} "uqm")
    set(ENV{BUILD_ROOT}    "${CMAKE_SOURCE_DIR}/")

    #set_makeinfo_env function defined in helpers.cmake
    # env_var to set ↓                    ↓ conditional
    set_makeinfo_env(uqm_HAVE_READDIR_R   HAVE_READDIR_R)
    set_makeinfo_env(uqm_HAVE_SETENV      HAVE_SETENV)
    set_makeinfo_env(uqm_HAVE_STRUPR      HAVE_STRUPR)
    set_makeinfo_env(uqm_HAVE_STRCASECMP  HAVE_STRCASECMP_UQM)
    set_makeinfo_env(uqm_HAVE_STRICMP     HAVE_STRICMP)
    set_makeinfo_env(uqm_HAVE_GETOPT_LONG HAVE_GETOPT_LONG)
    set_makeinfo_env(uqm_HAVE_REGEX       HAVE_REGEX_H)
    set_makeinfo_env(uqm_USE_ZIP_IO       USE_ZIP_IO)

    set(ENV{uqm_THREADLIB} "${UQM_THREAD_LIB}")

    #set_env_if function defined in helpers.cmake
    #          ↓ env_var to set  set to ↓    ↓ conditional
    set_env_if(MEMDEBUG                "1"   MEMDEBUG)
    set_env_if(DEBUG                   "1"   UQM_DEBUG)
    set_env_if(uqm_USE_INTERNAL_MIKMOD "1"   UQM_USE_INTERNAL_MIKMOD)
    set_env_if(uqm_USE_INTERNAL_LUA    "1"   UQM_USE_INTERNAL_LUA)
    set_env_if(uqm_NETPLAY             "1"   UQM_NETPLAY)
    set_env_if(MACRO___MINGW32__       "1"   IS_MINGW)
    set_env_if(uqm_USE_WINSOCK         "1"   USE_WINSOCK)
    set_env_if(uqm_GFXMODULE           "sdl" UQM_GFX_BACKEND)

    if(APPLE)
        set(ENV{HOST_SYSTEM} "Darwin")
    elseif(WIN32 OR IS_MINGW)
        set(ENV{HOST_SYSTEM} "MINGW32_NT")
    else()
        set(ENV{HOST_SYSTEM} "Linux")
    endif()

    set(ENV{BUILD_WORK} "${CMAKE_SOURCE_DIR}")

    execute_process(
            COMMAND sh "${CMAKE_SOURCE_DIR}/build/unix/recurse" "uqm" "${CMAKE_SOURCE_DIR}"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE recurse_output
            ERROR_VARIABLE recurse_error
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE recurse_result
    )

    if(recurse_error)
        message(STATUS "Recurse stderr: ${recurse_error}")
    endif()
    
    if(NOT recurse_result EQUAL 0)
        message(FATAL_ERROR "recurse script failed with exit code: ${recurse_result}")
    endif()

    string(REPLACE "\n" ";" file_lines "${recurse_output}")
    set(source_files "")
    
    foreach(line IN LISTS file_lines)
        if(line MATCHES "^(C|CXX|M|RC)\t(.*)$")
            set(file_type "${CMAKE_MATCH_1}")
            set(source_path "${CMAKE_MATCH_2}")
            set(full_source_path "${CMAKE_SOURCE_DIR}/${source_path}")
            
            if(EXISTS "${full_source_path}")
                list(APPEND source_files "${full_source_path}")
            endif()
        endif()
    endforeach()
    
    list(REMOVE_DUPLICATES source_files)
    set(${target_sources} ${source_files} PARENT_SCOPE)
endfunction()

set(UQM_SOURCES)
message(STATUS "Parsing Makeinfo for source files...")
parse_makeinfo_with_recurse(UQM_SOURCES)

list(LENGTH UQM_SOURCES UQM_SOURCES_COUNT)
message(STATUS "Found ${UQM_SOURCES_COUNT} files")

if(NOT UQM_SOURCES)
    message(FATAL_ERROR "No source files found!")
endif()

#set_config_define function defined in helpers.cmake
set_config_define(WORDS_BIGENDIAN)
set_config_define(HAVE_READDIR_R)
set_config_define(HAVE_SETENV)
set_config_define(HAVE_STRUPR)
set_config_define(HAVE_STRCASECMP_UQM)
set_config_define(HAVE_STRICMP)
set_config_define(HAVE_GETOPT_LONG)

set(HAVE_ISWGRAPH "#define HAVE_ISWGRAPH")
set(HAVE_WCHAR_T  "#define HAVE_WCHAR_T")
set(HAVE_WINT_T   "#define HAVE_WINT_T")
set(HAVE__BOOL    "#define HAVE__BOOL")

if(IS_WINDOWS)
    configure_file(
        ${CMAKE_SOURCE_DIR}/src/config_win.h.in
        ${CMAKE_SOURCE_DIR}/src/config_win.h
        @ONLY
    )
    message(STATUS "Generated config_win.h in src directory")
else()
    configure_file(
        ${CMAKE_SOURCE_DIR}/src/config_unix.h.in
        ${CMAKE_SOURCE_DIR}/src/config_unix.h
        @ONLY
    )
    message(STATUS "Generated config_unix.h in src directory")
endif()

add_executable(UrQuanMasters ${UQM_SOURCES})

target_include_directories(UrQuanMasters PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${VORBISFILE_INCLUDE_DIRS}
)

if(UQM_DEBUG)
    set_target_properties(UrQuanMasters PROPERTIES OUTPUT_NAME "UrQuanMastersDebug")
endif()

if(IS_WINDOWS)
    set_target_properties(UrQuanMasters PROPERTIES
            WIN32_EXECUTABLE FALSE
            LINK_FLAGS "-mconsole"
    )
endif()

set(UQM_COMPILE_OPTIONS
        $<$<COMPILE_LANGUAGE:C>:${COMMON_C_FLAGS}>
)
#append_list macro defined in helpers.cmake
#           ↓ list to append     ↓ var to include      ↓ condition
append_list(UQM_COMPILE_OPTIONS "${SDL2_CFLAGS_OTHER}" SDL2_CFLAGS_OTHER)
append_list(UQM_COMPILE_OPTIONS "${PNG_CFLAGS_OTHER}"  PNG_CFLAGS_OTHER)

target_compile_options(UrQuanMasters PRIVATE ${UQM_COMPILE_OPTIONS})

set(UQM_DEFINITIONS
        GFXMODULE_SDL
        SDL_DIR="${SDL_DIR}")

append_list(UQM_DEFINITIONS HAVE_OPENGL            HAVE_OPENGL)
append_list(UQM_DEFINITIONS HAVE_OPENAL            HAVE_OPENAL)
append_list(UQM_DEFINITIONS OVCODEC_TREMOR         OVCODEC_TREMOR)
append_list(UQM_DEFINITIONS OVCODEC_NONE           OVCODEC_NONE)
append_list(UQM_DEFINITIONS USE_INTERNAL_MIKMOD    USE_INTERNAL_MIKMOD)
append_list(UQM_DEFINITIONS USE_INTERNAL_LUA       USE_INTERNAL_LUA)
append_list(UQM_DEFINITIONS HAVE_JOYSTICK          HAVE_JOYSTICK)
append_list(UQM_DEFINITIONS USE_PLATFORM_ACCEL     USE_PLATFORM_ACCEL)
append_list(UQM_DEFINITIONS HAVE_ZIP               HAVE_ZIP)
append_list(UQM_DEFINITIONS DEBUG                  DEBUG)
append_list(UQM_DEFINITIONS "NETPLAY=NETPLAY_FULL" UQM_NETPLAY)

if(UQM_THREAD_LIB STREQUAL "PTHREAD")
    list(APPEND UQM_DEFINITIONS THREADLIB_PTHREAD)
else()
    list(APPEND UQM_DEFINITIONS THREADLIB_SDL)
endif()

target_compile_definitions(UrQuanMasters PRIVATE ${UQM_DEFINITIONS})

set(UQM_LINK_LIBRARIES
        ${SDL2_LIBRARIES}
        ${PNG_LIBRARIES}
        ${VORBISFILE_LIBRARIES}
        ${VORBIS_LIBRARIES}
        ${OGG_LIBRARIES}
        ${EXTRA_LIBS}
)

append_list(UQM_LINK_LIBRARIES "${OPENAL_LIBRARIES}" "OPENAL_FOUND AND UQM_SOUND_BACKEND STREQUAL \"openal\"")
append_list(UQM_LINK_LIBRARIES "${ZLIB_LIBRARIES}"   USE_ZIP_IO)
append_list(UQM_LINK_LIBRARIES "Threads::Threads"    "Threads_FOUND AND UQM_THREAD_LIB STREQUAL \"PTHREAD\"")
append_list(UQM_LINK_LIBRARIES "OpenGL::GL"          HAVE_OPENGL)
append_list(UQM_LINK_LIBRARIES "gdi32"               IS_WINDOWS)

if(HAVE_REGEX_H AND IS_MINGW)
    pkg_check_modules(REGEX QUIET regex)
    if(REGEX_FOUND)
        list(APPEND UQM_LINK_LIBRARIES ${REGEX_LIBRARIES})
        message(STATUS "Using regex: ${REGEX_LIBRARIES}")
    else()
        find_library(REGEX_LIBRARY regex)
        if(REGEX_LIBRARY)
            list(APPEND UQM_LINK_LIBRARIES ${REGEX_LIBRARY})
            message(STATUS "Using regex: ${REGEX_LIBRARY}")
        endif()
    endif()
endif()

target_link_libraries(UrQuanMasters ${UQM_LINK_LIBRARIES})

message(STATUS "")
message(STATUS "=== UQM-MegaMod Configuration ===")
message(STATUS "Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "Graphics Engine:  ${UQM_GFX_BACKEND} (SDL_DIR: ${SDL_DIR})")
message(STATUS "OpenGL Support:   ${UQM_OPENGL}")
message(STATUS "Sound Backend:    ${UQM_SOUND_BACKEND}")
message(STATUS "Ogg Vorbis Codec: ${UQM_OGG_CODEC}")
message(STATUS "Thread Library:   ${UQM_THREAD_LIB}")
message(STATUS "Features:")
message(STATUS "  Debug:        ${UQM_DEBUG}")
message(STATUS "  Strict Debug: ${UQM_STRICT_DEBUG}")
message(STATUS "  Joystick:     ${UQM_JOYSTICK}")
message(STATUS "  Netplay:      ${UQM_NETPLAY}")
message(STATUS "  ZIP I/O:      ${UQM_ZIP_IO}")
message(STATUS "  Platform Acceleration: ${UQM_PLATFORM_ACCEL}")
message(STATUS "  Internal MikMod:       ${UQM_USE_INTERNAL_MIKMOD}")
message(STATUS "  Internal Lua:          ${UQM_USE_INTERNAL_LUA}")
message(STATUS "Source Files: ${UQM_SOURCES_COUNT}")
message(STATUS "================================")