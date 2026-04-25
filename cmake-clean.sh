#!/bin/bash

rm -rf _deps CMakeFiles thirdparty build.ninja cmake_install.cmake CMakeCache.txt \
       CPackConfig.cmake CPackSourceConfig.cmake .ninja_log .ninja_deps UrQuanMastersDebug \
       UrQuanMastersDebug.exe UrQuanMasters UrQuanMasters.exe
echo "Cleaned CMake build files"