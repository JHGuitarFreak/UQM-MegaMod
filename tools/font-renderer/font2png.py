#!/usr/bin/env python

# Lists out fonts to choose from in the current working directory to export
# each individual glyph as PNG in your chosen point size
# PNG named after their unicode designations in hex format E.G. "0004e.png"
# Requires the FontForge and Pillow packages

import sys
import glob
import argparse
import fontforge
from PIL import Image
from PIL import ImageOps
from pathlib import Path

RED_COLOR     = "\033[31m"
GREEN_COLOR   = "\033[32m"
YELLOW_COLOR  = "\033[33m"
MAGENTA_COLOR = "\033[35m"
CYAN_COLOR    = "\033[36m"
GRAY_COLOR    = "\033[90m"
TEXT_RESET    = "\033[0m"
TEXT_BOLD     = "\033[1m"

Files = glob.glob('*.ttf') + glob.glob('*.otf') + glob.glob('*.woff')
NumFiles = len(Files)

if NumFiles == 0:
		sys.exit(f"{RED_COLOR}Terminating script because there are no "
				f"usable font files in the current directory.")

parser = argparse.ArgumentParser(
		description="Export font glyphs as individual PNG files",
		formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--ansi", action="store_true",
		help="export only ANSI character set")
parser.add_argument("--ascii", action="store_true",
		help="export only the basic ASCII character set")
parser.add_argument("--invert", action="store_false",
		help="export PNG as black on white")
parser.add_argument("--ascender", type=int,
		help="Custom ascender value for stubbornly cut off tops of fonts")
parser.add_argument("--descender", type=int,
		help="Custom descender value for stubbornly cut off bottoms of fonts")
parser.add_argument("--uqm", action="store_true",
		help=("Sets side bearings to 0 for non-empty glyphs and removes 100 "
				"from the right side bearing if it's equal to or over 100 for "
				"empty glyphs. Useful for importing fonts into The Ur-Quan Masters"))
args = parser.parse_args()

if args.ansi and args.ascii:
		sys.exit(f"{TEXT_BOLD}{RED_COLOR}ERROR! {YELLOW_COLOR}"
				f"Arguments --ansi and --ascii can not be used together. "
				f"Script will now exit.")

print(f'{CYAN_COLOR}\nPick a font from the list to work with...\n{TEXT_RESET}')

for i in range(0, NumFiles):
	colour = GRAY_COLOR if i % 2 == 0 else TEXT_RESET
	print(f'{colour}\t{i:>3}) {Files[i]}{TEXT_RESET}')

print('')

UserInput = -1
while not int(UserInput) in range(0, NumFiles):
		UserInput = input(f"{CYAN_COLOR}Choose between ({TEXT_RESET}0 - "
				f"{NumFiles - 1}{CYAN_COLOR}) : {TEXT_RESET}")

FontPath = Path(Files[int(UserInput)])

print('')

FontSize = int(input(f"{CYAN_COLOR}Enter Font Size In Point: {TEXT_RESET}"))

print(f"{GREEN_COLOR}\nExporting {TEXT_RESET}{Files[int(UserInput)]} "
		f"{GREEN_COLOR}with a size of {TEXT_RESET}{FontSize}{GREEN_COLOR}pt\n"
		f"{YELLOW_COLOR}")

FontStem = Path(FontPath.stem)
FontStem.mkdir(parents=True, exist_ok=True)
Font = fontforge.open(FontPath.name, 1)

emSize = Font.em
ScaleFactor = FontSize / emSize
LowestOffs = None
WidthAdjust = 1 if args.uqm else 0

if args.ascender is not None and args.ascender < Font.hhea_ascent:
	sys.exit(f"{TEXT_BOLD}{RED_COLOR}\nERROR! {YELLOW_COLOR}"
			f"Argument --ascender ({args.ascender}) shorter than the "
			f"selected font's Ascender ({Font.hhea_ascent}). "
			f"Script will now exit.")
if args.descender is not None and args.descender > Font.hhea_descent:
	sys.exit(f"{TEXT_BOLD}{RED_COLOR}\nERROR! {YELLOW_COLOR}"
			f"Argument --descender ({args.descender}) taller than the "
			f"selected font's Descender ({Font.hhea_descent}). "
			f"Script will now exit.")

Ascender = args.ascender if args.ascender else Font.hhea_ascent
Descender = args.descender if args.descender else Font.hhea_descent

ImgHeight = int((Ascender - Descender) * ScaleFactor)

values = [Ascender, Descender, ImgHeight]
max_width = max(len(str(x)) for x in values)

print(f"{MAGENTA_COLOR}Ascender:  {TEXT_RESET}{Ascender:>{max_width}}\n"
		f"{MAGENTA_COLOR}Descender: {TEXT_RESET}{Descender:>{max_width}}\n"
		f"{MAGENTA_COLOR}ImgHeight: {TEXT_RESET}{ImgHeight:>{max_width}}\n"
		f"{TEXT_RESET}")

for Glyph in Font.glyphs():
	if not Glyph.isWorthOutputting():
		continue

	UnicodeValue = Glyph.unicode

	if (UnicodeValue < 0x20 or
			(args.ascii and UnicodeValue > 0x7F) or
			(args.ansi and UnicodeValue > 0xFF)):
		continue

	if args.uqm:
		if not Glyph.layers['Fore'].isEmpty():
			Glyph.left_side_bearing = 0
			Glyph.right_side_bearing = 0
		else:
			if Glyph.right_side_bearing >= 100:
				Glyph.right_side_bearing = int(Glyph.right_side_bearing) - 100

	OutputFile = str(FontStem.joinpath(f"{UnicodeValue:05x}.png"))
	Glyph.export(OutputFile, FontSize)

	Img = Image.open(OutputFile)
	Img = Img.convert("L")

	if args.invert:
		Img = ImageOps.invert(Img)
		Color = "Black"
	else:
		Color = "White"

	Offset = (ImgHeight - Img.height) // 2
	if LowestOffs is None or Offset < LowestOffs:
		LowestOffs = Offset

	NewImg = Image.new("L", (Img.width - WidthAdjust, ImgHeight), Color)
	NewImg.paste(Img, (0, Offset - abs(LowestOffs)))
	NewImg.save(OutputFile)

Font.close()