#!/bin/sh

echo -e "\nThis is a helper script for MinGW\n"

echo "Exporting C_INCLUDE_PATH to ./dev-lib/MINGW/include"
export C_INCLUDE_PATH=$PWD/dev-lib/MINGW/include

echo "Exporting LIBRARY_PATH to ./dev-lib/MINGW/lib"
export LIBRARY_PATH=$PWD/dev-lib/MINGW/lib

echo "Exporting PATH to ./dev-lib/MINGW/bin"
export PATH=$PATH:$PWD/dev-lib/MINGW/bin

echo -e "Done exporting\n"

echo -e "Invoking build.sh $@\n"

build.sh "$@"