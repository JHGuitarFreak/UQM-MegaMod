#!/usr/bin/env python

# Created with help from DeepSeek R1 (Mainly to help me understand how to do things but not generating the entire code)
# Lists out fonts to choose from in the current working directory to export each individual glyph as PNG in your chosen point size
# PNG named after their unicode desgnations in hex format E.G. "0004e.png"
# Requires the FontForge and Pillow packages

import sys
import glob
import math
import argparse
import fontforge
from PIL import Image
from PIL import ImageOps
from pathlib import Path

cRed     = "\033[31m"
cGreen   = "\033[32m"
cYellow  = "\033[33m"
cBlue    = "\033[34m"
cMagenta = "\033[35m"
cCyan    = "\033[36m"
cGray    = "\033[90m"
tReset   = "\033[0m"
tBold    = "\033[1m"

Files = glob.glob('*.ttf') + glob.glob('*.otf') + glob.glob('*.woff')
NumFiles = len(Files)

if NumFiles == 0:
	sys.exit(f"{cRed}Terminating script because there are no usable font files in the current directory.")

parser = argparse.ArgumentParser(description="Export font glyphs as individual PNG files", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--ansi", action="store_true", help="export only ANSI character set")
parser.add_argument("--ascii", action="store_true", help="export only the basic ASCII character set")
parser.add_argument("--invert", action="store_false", help="export PNG as black on white")
parser.add_argument("--ascender", type=int, help="Custom ascender value for stubbornly cut off tops of fonts")
parser.add_argument("--descender", type=int, help="Custom descender value for stubbornly cut off bottoms of fonts")
parser.add_argument("--uqm", action="store_true", help="Takes 100 off the right side bearing of countour-less glyphs (Removes all side bearings otherwise), takes 1 pixel off the final width")
args = parser.parse_args()
config = vars(args)

if args.ansi and args.ascii:
	sys.exit(f"{tBold}{cRed}ERROR! {cYellow}Arguments --ansi and --ascii can not be used together. Script will now exit.")

print(f'{cCyan}\nPick a font from the list to work with...\n{tReset}')

for i in range (0, NumFiles):
	colour = cGray if i % 2 == 0 else tReset
	print(f'{colour}\t{i:>3}) {Files[i]}{tReset}')

print('')

UserInput = -1
while not int(UserInput) in range(0, NumFiles):
	UserInput = input(f"{cCyan}Choose between ({tReset}0 - {NumFiles - 1}{cCyan}) : {tReset}")

FontPath = Path(Files[int(UserInput)])

print('')

FontSize = int(input(f"{cCyan}Enter Font Size In Point: {tReset}"))

print(f"{cGreen}\nExporting {tReset}{Files[int(UserInput)]} {cGreen}with a size of {tReset}{FontSize}{cGreen}pt\n{cYellow}")

FontStem = Path(FontPath.stem)
FontStem.mkdir(parents=True, exist_ok=True)
Font = fontforge.open(FontPath.name, 1)

emSize = Font.em
ScaleFactor = FontSize / emSize
LowestOffs = None
WidthAdjust = 1 if args.uqm else 0

if args.ascender is not None and args.ascender < Font.hhea_ascent:
	sys.exit(f"{tBold}{cRed}\nERROR! {cYellow}Argument --ascender ({args.ascender}) shorter than the selected font's Ascender ({Font.hhea_ascent}). Script will now exit.")
if args.descender is not None and args.descender > Font.hhea_descent:
	sys.exit(f"{tBold}{cRed}\nERROR! {cYellow}Argument --descender ({args.descender}) taller than the selected font's Descender ({Font.hhea_descent}). Script will now exit.")

Ascender = args.ascender if args.ascender else Font.hhea_ascent
Descender = args.descender if args.descender else Font.hhea_descent

ImgHeight = int((Ascender - Descender) * ScaleFactor)

values = [Ascender, Descender, ImgHeight]
max_width = max(len(str(x)) for x in values)

print(f"\n"
		f"{cMagenta}Ascender:  {tReset}{Ascender:>{max_width}}\n"
		f"{cMagenta}Descender: {tReset}{Descender:>{max_width}}\n"
		f"{cMagenta}ImgHeight: {tReset}{ImgHeight:>{max_width}}\n"
		f"{tReset}"
)

for Glyph in Font.glyphs():
	if not Glyph.isWorthOutputting():
		continue

	UnicodeValue = Glyph.unicode

	if UnicodeValue < 0x20 or (args.ascii and UnicodeValue > 0x7F) or (args.ansi and UnicodeValue > 0xFF):
		continue

	if args.uqm:
		if not Glyph.layers['Fore'].isEmpty():
			Glyph.left_side_bearing = 0
			Glyph.right_side_bearing = 0
		else:
			if Glyph.right_side_bearing > 100:
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