function(quieter_check_symbol symbol header result_var)
    set(CMAKE_REQUIRED_QUIET TRUE)  # Suppress all check output
    check_symbol_exists(${symbol} "${header}" ${result_var})
    if(${result_var})
        message(STATUS "Symbol '${symbol}' found")
    else()
        message(STATUS "Symbol '${symbol}' not found")
    endif()
endfunction()

function(check_type type result_var)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_type_size(${type} ${result_var})
    if(${result_var})
        message(STATUS "Type '${type}' found")
    else()
        message(STATUS "Type '${type}' not found")
    endif()
endfunction()

function(check_include header result_var)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_include_file(${header} ${result_var})
    if(${result_var})
        message(STATUS "Header '${header}' found")
    else()
        message(STATUS "Header '${header}' not found")
    endif()
endfunction()

function(set_if condition var value)
    if(${condition})
        set(${var} "${value}" PARENT_SCOPE)
    endif()
endfunction()

function(set_if_else condition var var_1 var_2)
    if(${condition})
        set(${var} "${var_1}" PARENT_SCOPE)
    else()
        set(${var} "${var_2}" PARENT_SCOPE)
    endif()
endfunction()

function(set_makeinfo_env env_var cmake_var)
    if(${cmake_var})
        set(ENV{${env_var}} "1")
    else()
        set(ENV{${env_var}} "0")
    endif()
endfunction()

function(set_env_if env_var value condition)
    if(${condition})
        set(ENV{${env_var}} "${value}")
    endif()
endfunction()

function(set_config_define var)
    if(${var})
        set(${var} "#define ${var}" PARENT_SCOPE)
    else()
        set(${var} "#undef ${var}" PARENT_SCOPE)
    endif()
endfunction()

macro(append_list list libraries condition)
    if(${condition})
        list(APPEND ${list} ${libraries})
    endif()
endmacro()

function (FetchSDL)
	message (STATUS "Fetching SDL2...")

	set (SDL_SHARED ON CACHE BOOL "Build SDL2 as a shared library")
	set (SDL_STATIC ON CACHE BOOL "Build SDL2 as a static library")

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING") # Disable status messages

	FetchContent_Declare (
			SDL2
			URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.8/SDL2-2.32.8.tar.gz
			URL_HASH SHA256=0ca83e9c9b31e18288c7ec811108e58bac1f1bb5ec6577ad386830eac51c787e
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2
			DOWNLOAD_NO_PROGRESS TRUE
	)
	FetchContent_MakeAvailable (SDL2)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS") # Re-enable status messages

	# Set variables
	set (SDL2_FOUND TRUE PARENT_SCOPE)
	set (SDL2_LIBRARIES SDL2 PARENT_SCOPE)
	set (SDL2_INCLUDE_DIRS
			${CMAKE_CURRENT_BINARY_DIR}/thirdparty/SDL2/include
			${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2/include
			PARENT_SCOPE
	)

	file (RELATIVE_PATH SDL2_REL_PATH
			${CMAKE_SOURCE_DIR}
			${sdl2_SOURCE_DIR}
	)
	set (SDL_DIR           "${SDL2_REL_PATH}" PARENT_SCOPE)
	set (SDL2_CFLAGS_OTHER ""                 PARENT_SCOPE)
	set (SDL2_VERSION      "2.32.8"           PARENT_SCOPE)
endfunction ()

function (FetchPNG)
	message (STATUS "Fetching libPNG...")

	set (PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library" FORCE)
	set (PNG_STATIC ON  CACHE BOOL "Build libpng as a static library" FORCE)

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING")

	FetchContent_Declare (
			png
			URL https://github.com/pnggroup/libpng/archive/refs/tags/v1.6.54.tar.gz
			URL_HASH SHA256=ba7efce137409079989df4667706c339bebfbb10e9f413474718012a13c8cd4c
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng
			DOWNLOAD_NO_PROGRESS TRUE
	)
	FetchContent_MakeAvailable (png)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS")

	set (PNG_FOUND TRUE PARENT_SCOPE)
	set (PNG_LIBRARIES png_static PARENT_SCOPE)
	set (PNG_INCLUDE_DIRS
			${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libpng
			${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng
			PARENT_SCOPE
	)
	set (PNG_CFLAGS_OTHER ""       PARENT_SCOPE)
	set (PNG_VERSION      "1.6.54" PARENT_SCOPE)
endfunction ()

function (FetchOgg)
	message (STATUS "Fetching libogg...")
	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING")

	FetchContent_Declare (
			libogg
			URL https://github.com/xiph/ogg/releases/download/v1.3.6/libogg-1.3.6.tar.gz
			URL_HASH SHA256=83e6704730683d004d20e21b8f7f55dcb3383cdf84c0daedf30bde175f774638
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg
			DOWNLOAD_NO_PROGRESS TRUE
	)
	FetchContent_MakeAvailable (libogg)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS")

	# Set OGG variables
	set (OGG_FOUND TRUE PARENT_SCOPE)
	set (OGG_LIBRARIES ogg PARENT_SCOPE)
	set (OGG_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libogg/")
	set (OGG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg/include/ogg/")
	set (OGG_INCLUDE_DIRS
			${OGG_LIBRARY}/include
			${OGG_INCLUDE_DIR}
			PARENT_SCOPE
	)
	set (OGG_VERSION "1.3.6" PARENT_SCOPE)
endfunction ()

function (FetchVorbis)
	message (STATUS "Fetching libvorbis...")
	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING")

	FetchContent_Declare (
			libvorbis
			URL https://github.com/JHGuitarFreak/vorbis/archive/refs/tags/uqm-megamod.tar.gz
			URL_HASH SHA256=1f51792ad82269317866c1f347ca4aa35bfbaade9ddab80dd3db9fd3d8fe094c
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis
			DOWNLOAD_NO_PROGRESS TRUE
	)
	FetchContent_MakeAvailable (libvorbis)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS")

	set (VORBIS_FOUND TRUE PARENT_SCOPE)
	set (VORBIS_LIBRARIES vorbis PARENT_SCOPE)
	set (VORBIS_INCLUDE_DIRS
			${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libvorbis/include
			${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/include
			PARENT_SCOPE
	)
	set (VORBIS_VERSION "1.3.7" PARENT_SCOPE)

	set (VORBISFILE_FOUND TRUE PARENT_SCOPE)
	set (VORBISFILE_LIBRARIES vorbisfile PARENT_SCOPE)
	set (VORBISFILE_INCLUDE_DIRS
			${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libvorbis/include
			${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/include
			PARENT_SCOPE
	)

	set (OGGVORBIS     "vorbisfile" PARENT_SCOPE)
	set (UQM_OGG_CODEC "standard"   PARENT_SCOPE)
endfunction ()

function (FetchZLIB)
	message (STATUS "Fetching ZLIB...")

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING")

	FetchContent_Declare (
			zlib
			URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
			URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlib
			DOWNLOAD_NO_PROGRESS TRUE
			CMAKE_ARGS
			-DBUILD_SHARED_LIBS=OFF
			-DZLIB_BUILD_EXAMPLES=OFF
	)
	FetchContent_MakeAvailable (zlib)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS")

	if (TARGET zlibstatic)
		set (ZLIB_TARGET zlibstatic)
	else ()
		set (ZLIB_TARGET zlib)
	endif ()

	set (ZLIB_FOUND TRUE PARENT_SCOPE)
	set (ZLIB_LIBRARIES ${ZLIB_TARGET} PARENT_SCOPE)
	set (ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR} PARENT_SCOPE)
	set (ZLIB_VERSION "1.3.1" PARENT_SCOPE)
endfunction ()