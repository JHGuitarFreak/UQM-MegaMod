cmake_minimum_required(VERSION 3.14)

project(uqm)

include(CheckIncludeFile)
include(CheckLibraryExists)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBOGG QUIET libogg)
pkg_check_modules(PC_LIBVORBIS QUIET libvorbis)
pkg_check_modules(PC_LIBVORBISFILE QUIET libvorbisfile)

function(find_library_package return_name name)
    pkg_check_modules(PC_${name} QUIET ${name})
    find_path(${name}_INCLUDE_DIR NAMES ${name}/lua.h HINTS ${PC_${name}_INCLUDEDIR} ${PC_${name}_INCLUDE_DIRS})
    find_library(${name}_LIBRARY NAMES ${name} HINTS ${PC_${name}_LIBDIR} ${PC_${name}_LIBRARY_DIRS})
    set(${return_name} ${${name}_LIBRARY} PARENT_SCOPE)
    set(${name}_INCLUDE_DIRS ${${name}_INCLUDE_DIR} PARENT_SCOPE)
endfunction()

find_library_package(LIBOGG ogg)
find_library_package(LIBVORBIS vorbis)
find_library_package(LIBVORBISFILE vorbisfile)

find_package(SDL2 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(Threads REQUIRED)

include_directories(src)
include_directories(src/libs/lua)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${PNG_INCLUDE_DIRS})
include_directories(cmake)

add_definitions(-DDEBUG -DHAVE_JOYSTICK -DTHREADLIB_SDL -DUSE_PLATFORM_ACCEL=1 -DNETPLAY=NETPLAY_FULL -DHAVE_ZIP=1 -DGFXMODULE_SDL -DUSE_INTERNAL_MIKMOD -DUSE_INTERNAL_LUA -DTHREADLIB_PTHREAD -DHAVE_OPENAL)

file(GLOB gameFiles src/*.c src/uqm/*.c src/uqm/supermelee/*.c src/libs/uio/stdio/*.c)
file(GLOB_RECURSE sourceFiles src/uqm/ships/*.c src/uqm/planets/*.c src/uqm/supermelee/*.c src/uqm/lua/*.c src/uqm/comm/*.c)
# file(GLOB_RECURSE libFiles src/libs/*.c)
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

if (MSVC OR WIN32)
    set(THREAD_IMPLEMENTATION sdl)
else()
    set(THREAD_IMPLEMENTATION pthread)
endif()

file(GLOB logFiles src/libs/log/msgbox_stub.c src/libs/log/uqmlog.c)
file(GLOB threadFiles src/libs/threads/*.c src/libs/threads/${THREAD_IMPLEMENTATION}/*.c)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

#    set(linkFlags "-s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=2032MB --shell-file ${CMAKE_CURRENT_LIST_DIR}/src/tiny_chess_shell.html --js-library ${CMAKE_CURRENT_LIST_DIR}/src/library_unicode.js")
    set(linkFlags "${linkFlags} -s USE_PTHREADS=1 -s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=2032MB -s PTHREAD_POOL_SIZE=4 -s ASMFS=1 -s PROXY_TO_PTHREAD=1 -s OFFSCREENCANVAS_SUPPORT=0 -s OFFSCREEN_FRAMEBUFFER=1 --threadprofiler --profiling-funcs")

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

target_link_libraries(uqm PRIVATE SDL2::SDL2 ${LIBOGG} ${LIBVORBIS} ${LIBVORBISFILE} OpenAL::OpenAL ZLIB::ZLIB PNG::PNG m Threads::Threads)

if(ANDROID)
    target_link_libraries(uqm PRIVATE log)
endif()

if (NOT MSVC AND NOT ANDROID)
    target_link_libraries(uqm PRIVATE m)
endif()
