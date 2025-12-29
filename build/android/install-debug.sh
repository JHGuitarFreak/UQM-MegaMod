#!/bin/bash

APK_PATH="composeApp/build/outputs/apk/debug/*.apk"

APK_FILE=$(ls $APK_PATH 2>/dev/null | head -1)

if [ -z "$APK_FILE" ]; then
    echo "No APK found. Build the project first."
    exit 1
fi

echo "Installing $(basename $APK_FILE)..."
adb install -r "$APK_FILE"