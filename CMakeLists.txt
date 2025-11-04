cmake_minimum_required(VERSION 3.14)

project(uqm)

find_package(ZLIB REQUIRED)
include(FetchContent)

set(SDL_SHARED ON CACHE BOOL "Build SDL2 as a shared library")
set(SDL_STATIC ON CACHE BOOL "Build SDL2 as a static library")
set(OGG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg/include/ogg/")
set(OGG_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libogg/")

FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.4
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2
)
FetchContent_MakeAvailable(SDL2)

FetchContent_Declare(
        libogg
        URL https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.tar.gz
        URL_HASH SHA256=0eb4b4b9420a0f51db142ba3f9c64b333f826532dc0f48c6410ae51f4799b664
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg
)
FetchContent_MakeAvailable(libogg)

FetchContent_Declare(
        libvorbis
        URL https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.gz
        URL_HASH SHA256=0e982409a9c3fc82ee06e08205b1355e5c6aa4c36bca58146ef399621b0ce5ab
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis
)
FetchContent_MakeAvailable(libvorbis)

set(PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library" FORCE)
set(PNG_STATIC ON CACHE BOOL "Build libpng as a static library" FORCE)

FetchContent_Declare(
        png
        URL http://prdownloads.sourceforge.net/libpng/libpng-1.6.48.tar.gz
        URL_HASH SHA256=68f3d83a79d81dfcb0a439d62b411aa257bb4973d7c67cd1ff8bdf8d011538cd
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng
)
FetchContent_MakeAvailable(png)

FetchContent_Declare(
        oboe
        GIT_REPOSITORY https://github.com/google/oboe.git
        GIT_TAG 1.10.0
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oboe
)
FetchContent_Populate(oboe)

set(DOBOE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oboe")

set(ALSOFT_EXAMPLES OFF CACHE BOOL "Disable OpenAL Soft examples")
set(ALSOFT_TESTS OFF CACHE BOOL "Disable OpenAL Soft tests")
set(ALSOFT_UTILS OFF CACHE BOOL "Disable OpenAL Soft utilities")
set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "Disable OpenAL Soft config utility")
set(ALSOFT_BACKEND_OPENSL OFF CACHE BOOL "Disable OpenSL backend")
set(ALSOFT_BACKEND_OBOE ON CACHE BOOL "Enable Oboe backend")
set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "Disable Wave backend")

FetchContent_Declare(
        OpenAL
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG 1.24.3
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openal-soft
)
FetchContent_MakeAvailable(OpenAL)

add_definitions(-DDEBUG -DHAVE_JOYSTICK -DTHREADLIB_SDL -DUSE_PLATFORM_ACCEL=1 -DNETPLAY=NETPLAY_FULL -DHAVE_ZIP=1 -DGFXMODULE_SDL -DUSE_INTERNAL_MIKMOD -DUSE_INTERNAL_LUA) # -DHAVE_OPENAL

file(GLOB gameFiles src/*.c src/uqm/*.c src/uqm/supermelee/*.c src/libs/uio/stdio/*.c)
file(GLOB_RECURSE sourceFiles src/uqm/ships/*.c src/uqm/planets/*.c src/uqm/supermelee/*.c src/uqm/lua/*.c src/uqm/comm/*.c)
file(GLOB_RECURSE libFiles src/libs/luauqm/*.c src/libs/video/*.c src/libs/mikmod/*.c src/libs/network/*.c src/libs/time/*.c src/libs/task/*.c src/libs/strings/*.c src/libs/sound/*.c src/libs/resource/*.c src/libs/memory/*.c src/libs/math/*.c src/libs/list/*.c src/libs/input/*.c src/libs/heap/*.c src/libs/graphics/*.c src/libs/file/*.c src/libs/decomp/*.c src/libs/callback/*.c src/libs/threads/*.c src/libs/uio/*.c)

if (ANDROID)
    list(FILTER libFiles EXCLUDE REGEX "src/libs/uio/hashtable.c|src/libs/uio/memdebug.c|src/libs/network/socket/socket_win.c|src/libs/network/network_win.c|src/libs/network/netmanager/netmanager_win.c|src/libs/cdp/cdpapi.c|src/libs/cdp/cdp.c|src/libs/cdp/windl.c|src/libs/lua/lua.c|src/libs/lua/luac.c")
endif()

if (UNIX)
    list(FILTER libFiles EXCLUDE REGEX "src/libs/network/socket/socket_win.c|src/libs/network/network_win.c|src/libs/network/netmanager/netmanager_win.c|src/libs/cdp/windl.c")
endif()

foreach(f lapi.c lbaselib.c lcode.c lctype.c ldebug.c ldump.c lgc.c liolib.c lmathlib.c loadlib.c lopcodes.c lparser.c lstring.c ltable.c ltm.c lvm.c lauxlib.c lbitlib.c lcorolib.c ldblib.c ldo.c lfunc.c linit.c llex.c lmem.c lobject.c loslib.c lstate.c lstrlib.c ltablib.c lundump.c lzio.c)
    list(APPEND libFiles "src/libs/lua/${f}")
endforeach()

foreach(f charhashtable.c defaultfs.c fstypes.c hashtable.c io.c mount.c paths.c uiostream.c utils.c debug.c fileblock.c gphys.c ioaux.c match.c mounttree.c physical.c uioutils.c)
    list(APPEND libFiles "src/libs/uio/${f}")
endforeach()

set(THREAD_IMPLEMENTATION sdl)

file(GLOB logFiles src/libs/log/msgbox_stub.c src/libs/log/uqmlog.c)
file(GLOB threadFiles src/libs/threads/*.c src/libs/threads/${THREAD_IMPLEMENTATION}/*.c)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    #    set(linkFlags "-s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=2032MB --shell-file ${CMAKE_CURRENT_LIST_DIR}/src/tiny_chess_shell.html --js-library ${CMAKE_CURRENT_LIST_DIR}/src/library_unicode.js")
    set(linkFlags "${linkFlags} -s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=2032MB -s PTHREAD_POOL_SIZE=4 -s ASMFS=1 -s PROXY_TO_PTHREAD=1 -s OFFSCREENCANVAS_SUPPORT=0 -s OFFSCREEN_FRAMEBUFFER=1 --threadprofiler --profiling-funcs")

    set(linkFlags "${linkFlags} -s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=256MB")
    #    set(linkFlagsDebug "-s GL_DEBUG=1 -g2")
endif()

# Check for Android
if (ANDROID)
    add_library(uqm SHARED ${HEADERS} ${sourceFiles} ${gameFiles} ${libFiles} ${threadFiles} ${logFiles})
else()
    add_executable(uqm ${HEADERS} ${sourceFiles} ${gameFiles} ${libFiles} ${threadFiles} ${logFiles})
    #add_library(uqmShared SHARED ${HEADERS} ${sourceFiles} ${gameFiles} ${libFiles} ${threadFiles} ${logFiles})
endif()

set_target_properties(uqm PROPERTIES LINK_FLAGS_DEBUG "${linkFlagsDebug} ${linkFlags}")
set_target_properties(uqm PROPERTIES LINK_FLAGS_RELEASE "${linkFlags}")

target_include_directories(uqm PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libogg/include
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libvorbis/include
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oboe/
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/openal-soft/include
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/SDL2/include
        src
        src/libs/lua
        cmake
)

target_link_libraries(uqm PRIVATE
        SDL2
        ogg
        vorbis
        vorbisfile
        OpenAL
        ZLIB::ZLIB
        png_static
)

if(ANDROID)
    target_link_libraries(uqm PRIVATE log)
endif()

if (NOT MSVC AND NOT ANDROID)
    target_link_libraries(uqm PRIVATE m)
endif()
