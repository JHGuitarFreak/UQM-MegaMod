#!/usr/bin/env bash
# Copy .dll dependencies of UQM-MegaMod from the MSYS2 bins to here

EXE="UrQuanMasters.exe"

if [ ! -f $EXE ]; then
    echo "$EXE not found!"
    EXE="UrQuanMastersDebug.exe"

    if [ ! -f $EXE ]; then
        echo "$EXE not found!"
        EXE="EXIT"
    fi
fi

if [ "$EXE" = "EXIT" ]; then
    echo "No UQM-MegaMod binaries found. Exiting..."
    exit 1
fi

DLLS=$(ntldd -R $EXE | awk '/\\bin\\/{print $3;}')
DLLS=$(for dll in $DLLS; do echo $dll; done | sort -u)

for dll in $DLLS; do
    echo "copying \"$dll\""
    cp $dll .
done