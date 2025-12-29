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

function(FetchAndroid)

    find_package(ZLIB REQUIRED)
    set(ZLIB_FOUND TRUE PARENT_SCOPE)
    set(ZLIB_LIBRARIES ZLIB::ZLIB PARENT_SCOPE)
    set(ZLIB_VERSION ${ZLIB_VERSION_STRING} PARENT_SCOPE)

    set(HAVE_ZIP 1 PARENT_SCOPE)
    set(SDL_DIR "SDL2" PARENT_SCOPE)
    set(HAVE_OPENGL 0 PARENT_SCOPE)
    
    if(UQM_OPENGL)
        set(UQM_OPENGL OFF PARENT_SCOPE)
    endif()
    
    include(FetchContent)
    
    # SDL2
    set(SDL_SHARED ON CACHE BOOL "Build SDL2 as a shared library")
    set(SDL_STATIC ON CACHE BOOL "Build SDL2 as a static library")
    
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.4
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2
    )
    FetchContent_MakeAvailable(SDL2)
    
    # Set SDL2 variables
    set(SDL2_FOUND TRUE PARENT_SCOPE)
    set(SDL2_LIBRARIES SDL2 PARENT_SCOPE)
    set(SDL2_INCLUDE_DIRS 
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/SDL2/include
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2/include
        PARENT_SCOPE
    )
    set(SDL2_CFLAGS_OTHER "" PARENT_SCOPE)
    set(SDL2_VERSION "2.32.4" PARENT_SCOPE)
    
    # libogg
    FetchContent_Declare(
        libogg
        URL https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.tar.gz
        URL_HASH SHA256=0eb4b4b9420a0f51db142ba3f9c64b333f826532dc0f48c6410ae51f4799b664
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg
    )
    FetchContent_MakeAvailable(libogg)
    
    # Set OGG variables
    set(OGG_FOUND TRUE PARENT_SCOPE)
    set(OGG_LIBRARIES ogg PARENT_SCOPE)
    set(OGG_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libogg/")
    set(OGG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg/include/ogg/")
    set(OGG_INCLUDE_DIRS 
        ${OGG_LIBRARY}/include
        ${OGG_INCLUDE_DIR}
        PARENT_SCOPE
    )
    set(OGG_VERSION "1.3.5" PARENT_SCOPE)
    
    # libvorbis
    FetchContent_Declare(
        libvorbis
        URL https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.gz
        URL_HASH SHA256=0e982409a9c3fc82ee06e08205b1355e5c6aa4c36bca58146ef399621b0ce5ab
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis
    )
    FetchContent_MakeAvailable(libvorbis)
    
    # Set Vorbis variables
    set(VORBIS_FOUND TRUE PARENT_SCOPE)
    set(VORBIS_LIBRARIES vorbis PARENT_SCOPE)
    set(VORBIS_INCLUDE_DIRS 
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libvorbis/include
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/include
        PARENT_SCOPE
    )
    set(VORBIS_VERSION "1.3.7" PARENT_SCOPE)
    
    set(VORBISFILE_FOUND TRUE PARENT_SCOPE)
    set(VORBISFILE_LIBRARIES vorbisfile PARENT_SCOPE)
    set(VORBISFILE_INCLUDE_DIRS 
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libvorbis/include
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/include
        PARENT_SCOPE
    )
    
    set(OGGVORBIS "vorbisfile" PARENT_SCOPE)
    
    # libpng
    set(PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library" FORCE)
    set(PNG_STATIC ON CACHE BOOL "Build libpng as a static library" FORCE)
    
    FetchContent_Declare(
        png
        URL http://prdownloads.sourceforge.net/libpng/libpng-1.6.48.tar.gz
        URL_HASH SHA256=68f3d83a79d81dfcb0a439d62b411aa257bb4973d7c67cd1ff8bdf8d011538cd
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng
    )
    FetchContent_MakeAvailable(png)
    
    # Set PNG variables
    set(PNG_FOUND TRUE PARENT_SCOPE)
    set(PNG_LIBRARIES png_static PARENT_SCOPE)
    set(PNG_INCLUDE_DIRS 
        ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libpng
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng
        PARENT_SCOPE
    )
    set(PNG_CFLAGS_OTHER "" PARENT_SCOPE)
    set(PNG_VERSION "1.6.48" PARENT_SCOPE)

    if(UQM_SOUND_BACKEND STREQUAL "openal")
        # OpenAL
        set(ALSOFT_EXAMPLES OFF CACHE BOOL "Disable OpenAL Soft examples")
        set(ALSOFT_TESTS OFF CACHE BOOL "Disable OpenAL Soft tests")
        set(ALSOFT_UTILS OFF CACHE BOOL "Disable OpenAL Soft utilities")
        set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "Disable OpenAL Soft config utility")
        set(ALSOFT_BACKEND_OPENSL OFF CACHE BOOL "Disable OpenSL backend")
        set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "Disable Wave backend")
        
        FetchContent_Declare(
            OpenAL
            GIT_REPOSITORY https://github.com/kcat/openal-soft.git
            GIT_TAG 1.24.3
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openal-soft
        )
        FetchContent_MakeAvailable(OpenAL)
        
        # Set OpenAL variables
        set(OPENAL_FOUND FALSE PARENT_SCOPE)
        set(OPENAL_LIBRARIES OpenAL PARENT_SCOPE)
        set(OPENAL_INCLUDE_DIRS 
            ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/openal-soft/include
            ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openal-soft/include
            PARENT_SCOPE
        )
        set(OPENAL_VERSION "1.24.3" PARENT_SCOPE)
        set(HAVE_OPENAL 1 PARENT_SCOPE)
    endif()
    
endfunction()