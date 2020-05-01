#!/usr/bin/env fontforge

import os
import fontforge

FontName = input("Enter Font Name: ")
FontSize = int(input("Enter Font Size: "))

# small StarCon HD font = 28
# large StarCon HD font = 36
# MicroFont HD font = 39

ExportDir= os.path.splitext(FontName)[0]+"/"
try:
    os.makedirs(ExportDir)
except FileExistsError:
    pass

F = fontforge.open(FontName, 1)
for name in F:
	unicodeName = fontforge.unicodeFromName(name)
	filename = '{:05x}'.format(unicodeName) + ".png"
	print("Exporting " + filename)
	FullDir = ExportDir + filename
	F[name].export(FullDir, FontSize)
	os.system('mogrify -negate '+FullDir)