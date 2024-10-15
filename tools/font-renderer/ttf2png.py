#!/usr/bin/env fontforge

import fontforge
import glob
import subprocess
from os import system
from pathlib import Path

Files = glob.glob('*.ttf') + glob.glob('*.otf') + glob.glob('*.woff')
num_files = len(Files)

print('\n\nPick a font from the list to work with...\n')

for i in range (0, num_files):
	print('\t' + str(i) + ") " + Files[i]),

print('')

user_input = -1
while not int(user_input) in range(0, num_files):
	user_input = input("Choose between (0 - " + str(num_files - 1) + ") : ")

FontPath = Path(Files[int(user_input)])

print('')

FontSize = int(input("Enter Font Size: "))

print('\nExporting ' + Files[int(user_input)] + ' with the size of ' + str(FontSize) + '\n')

FontStem = Path(FontPath.stem)
FontStem.mkdir(parents=True, exist_ok=True)

F = fontforge.open(FontPath.name, 1)
for name in F:
	unicodeName = fontforge.unicodeFromName(name)
	filename = '{:05x}'.format(unicodeName) + ".png"
	if int(unicodeName) > 0:
		FullPath = str(FontStem.joinpath(filename))
		print("Exporting " + filename + " -> ",  end='', flush=True)
		F[name].export(FullPath, FontSize)
		print("Inverting color and defining PNG color type",  end='', flush=True)
		system('mogrify -negate -define png:color-type=0 "'+FullPath+'"')
		if int(subprocess.check_output('convert "'+FullPath+'" -format "%k" info:', shell=True)) > 1:
			print(" -> Trimming east/west black space")
			system('mogrify -fuzz 30% -define trim:edges=west,east -trim +repage "'+FullPath+'"')
		else:
			print('')

print('\nExport complete!')