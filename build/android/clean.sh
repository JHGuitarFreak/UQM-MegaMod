#!/bin/bash

[ -d ".gradle" ] && rm -rf ".gradle" && echo "Deleted .gradle"
[ -d "build" ] && rm -rf "build" && echo "Deleted build"

if [ -d "composeApp" ]; then
    [ -d "composeApp/.cxx" ] && rm -rf "composeApp/.cxx" && echo "Deleted composeApp/.cxx"
    [ -d "composeApp/build" ] && rm -rf "composeApp/build" && echo "Deleted composeApp/build"
fi

PARENT_DIR=$(cd ../.. && pwd)
[ -d "$PARENT_DIR/thirdparty" ] && rm -rf "$PARENT_DIR/thirdparty" && echo "Deleted $PARENT_DIR/thirdparty"