#!/usr/bin/env python

# Created with help from DeepSeek R1
# Lists out fonts to choose from in the current working directory to export each individual glyph as PNG in your chosen point size
# PNG named after their unicode desgnations in hex format E.G. "0004e.png"

import sys
import glob
import math
import argparse
import fontforge
from PIL import Image
from PIL import ImageOps
from pathlib import Path

# Find all font files in the working directory
Files = glob.glob('*.ttf') + glob.glob('*.otf') + glob.glob('*.woff')
NumFiles = len(Files)

if NumFiles == 0:
	sys.exit("Terminating script because there are no usable font files in the current directory.")

parser = argparse.ArgumentParser(description="Export font glyphs as individual PNG files", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--ansi", action="store_true", help="export only ANSI character set")
parser.add_argument("--ascii", action="store_true", help="export only the basic ASCII character set")
parser.add_argument("--invert", action="store_false", help="export PNG as black on white")
args = parser.parse_args()
config = vars(args)

if args.ansi and args.ascii:
	sys.exit("Arguments --ansi and --ascii can not be used together. Script will now exit.")

print('\nPick a font from the list to work with...\n')

for i in range (0, NumFiles):
	print('\t' + str(i) + ") " + Files[i])

print('')

# Have the user pick from a list of found font files
UserInput = -1
while not int(UserInput) in range(0, NumFiles):
	UserInput = input("Choose between (0 - " + str(NumFiles - 1) + ") : ")

FontPath = Path(Files[int(UserInput)]) # Fill in FontPath with the user selected font

print('')

# Have the user enter the point size they want the font to be exported as
FontSize = int(input("Enter Font Size In Point: "))

print('\nExporting ' + Files[int(UserInput)] + ' with a size of ' + str(FontSize) + 'pt\n')

FontStem = Path(FontPath.stem)              # Set the font name
FontStem.mkdir(parents=True, exist_ok=True) # Create the output directory if it doesn't exist
Font = fontforge.open(FontPath.name, 1)     # Open the font file

# Calculate the scaling factor for the desired point size
emSize = Font.em
ScaleFactor = FontSize / emSize

print (ScaleFactor)

# Initialize variables to track the highest and lowest points
HighestPoint = None
LowestPoint = None
max_height = 0

# Iterate through all glyphs in the font
for Glyph in Font.glyphs():
	if not Glyph.isWorthOutputting():
		continue

	# Get the glyph's Unicode code point
	UnicodeValue = Glyph.unicode
	
	# Skip glyphs below the space character
	if UnicodeValue < 0x20:
		continue

	if (args.ascii and UnicodeValue > 0x7F) or (args.ansi and UnicodeValue > 0xFF):
		continue

	height = (Glyph.boundingBox()[3] - Glyph.boundingBox()[1]) * ScaleFactor

	#pixels_per_unit = (dpi * math.pow((point_size / 72), 2))

	if height is None or height > max_height:
		max_height = height

	#print (f"{UnicodeValue:05x} -> height: {height}")

	# Iterate through all contours in the glyph
	for Contour in Glyph.foreground:
		# Iterate through all points in the contour
		for Point in Contour:
			y = Point.y
			
			# Update highest point
			if HighestPoint is None or y > HighestPoint:
				HighestPoint = y
			
			# Update lowest point
			if LowestPoint is None or y < LowestPoint:
				LowestPoint = y


# Calculate the difference in height
HeightDiff = int((HighestPoint - LowestPoint) * ScaleFactor)

print (f"HighestPoint={HighestPoint}, LowestPoint={LowestPoint}, HeightDiff={HeightDiff}")

LowestOffs = None;

for Glyph in Font.glyphs():
	if Glyph.isWorthOutputting():

		# Get the glyph's Unicode code point
		UnicodeValue = Glyph.unicode
		
		# Skip glyphs below the space character
		if UnicodeValue < 0x20:
			continue

		if (args.ascii and UnicodeValue > 0x7F) or (args.ansi and UnicodeValue > 0xFF):
			continue

		# Remove left and right side bearings (side metrics) for glyphs with contours
		if not Glyph.layers['Fore'].isEmpty():
			Glyph.left_side_bearing = 0
			Glyph.right_side_bearing = 0
		
		# Generate the output file name
		OutputFile = str(FontStem.joinpath(f"{UnicodeValue:05x}.png"))
		
		# Export the glyph as a PNG image
		Glyph.export(OutputFile, FontSize)

		Img = Image.open(OutputFile) # Open the exported image for more operations

		#print (f"{UnicodeValue:05x} -> Img.height: {Img.height}")

		# Calculate vertical offset to center the glyph
		Offset = (HeightDiff - Img.height) // 2

		if LowestOffs is None or Offset < LowestOffs:
			LowestOffs = Offset;

for Glyph in Font.glyphs():
	if Glyph.isWorthOutputting():

		# Get the glyph's Unicode code point
		UnicodeValue = Glyph.unicode
		
		# Skip glyphs below the space character
		if UnicodeValue < 0x20:
			continue

		if (args.ascii and UnicodeValue > 0x7F) or (args.ansi and UnicodeValue > 0xFF):
			continue
		
		# Generate the output file name
		OutputFile = str(FontStem.joinpath(f"{UnicodeValue:05x}.png"))

		Img = Image.open(OutputFile)   # Open the exported PNG with Pillow
		Img = Img.convert("L")         # Convert the image to 8bit Grayscale

		if args.invert:
			Img = ImageOps.invert(Img) # invert the image
			Color = "Black"
		else:
			Color = "White"

		# Calculate vertical offset to center the glyph
		Offset = (HeightDiff - Img.height) // 2

		# Create a new 8bit grayscale canvas with the original width, calculated height, and black background
		NewImg = Image.new("L", (Img.width, HeightDiff), Color)

		# Paste the glyph onto the new canvas
		NewImg.paste(Img, (0, Offset - abs(LowestOffs)))

		# Save the final image
		NewImg.save(OutputFile)

# Close the font
Font.close()
