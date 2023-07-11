#!/usr/bin/env fontforge

import fontforge
from os import system
from pathlib import Path

FontName = Path(input("Enter Font Name: "))
FontSize = int(input("Enter Font Size: "))

FontStem = Path(FontName.stem)
FontStem.mkdir(parents=True, exist_ok=True)

F = fontforge.open(FontName.name, 1)
for name in F:
	unicodeName = fontforge.unicodeFromName(name)
	filename = '{:05x}'.format(unicodeName) + ".png"
	if '-' not in filename:
		FullDir = str(FontStem.joinpath(filename))
		print("Exporting " + filename)
		F[name].export(FullDir, FontSize)
		system('mogrify -negate -define png:color-type=0 "'+FullDir+'"')