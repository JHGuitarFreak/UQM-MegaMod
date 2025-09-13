#!/usr/bin/sh

UQM_VERSION="0.8.4"

if [ -f ./keyjam.exe ]; then
	for i in UrQuanMasters.exe keyjam.exe dlls.nsi undlls.nsi AUTHORS.txt COPYING.txt MegaMod-README.txt UQM-README.txt README-SDL.txt UQM-Manual.txt CHANGELOG.txt uqm-$UQM_VERSION-installer.exe; do
		rm "$i"
		echo "$i has been removed"
	done
fi