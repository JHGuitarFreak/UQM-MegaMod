#!/bin/bash

# Make "-big", "-med", and "-sml" scaled image sets from a larger set of images based on input percentage using ImageMagick

# Requires ImageMagick in the PATH to function

# Input files are required to be named like "filename.###.png"
# where "filename" can be whatever tickles your fancy

# How do I get the percentage?
# Say you want scale your ship to an Earthling Cruiser in HD...
# The Cruiser facing north is 13x35, so multiply 35 by 4 and you get 140.
# Now get the length of YOUR ship facing north from top pixel to bottom
# pixel disregarding the empty space around the ship image, if any.
# Let's say it's 770.
# 140 / 770 = 0.1818181818181818
# 0.1818181818181818 * 100 = 18.18181818181818 < this is your percentage.

Usage() {
	echo "Usage: $0 [percentage] [-t]"
	echo "  percentage     Largest percentage to scale (optional - will prompt if not provided)"
	echo "  -t             Trim each generated image after creation"
	echo "  -h             Show this help"
	exit 1
}

Resize() {
	local pct=$1
	local label=$2
	local count=0
	
	echo "Making $label set at $pct%..."
	
	for f in "${files[@]}"; do
		base="${f%.png}"
		name="${base%.*}"
		
		out="${name}-${label}-$(printf "%03d" $count).png"

		magick "$f" -resize "$pct%" "$out"
		
		if [ -f "$out" ]; then
			[ "$trim" = true ] && magick "$out" -trim +repage "$out" 2>/dev/null
			((count++))
		else
			echo "  Failed: $f"
		fi
	done
	
	echo "Made $count $label images"
	echo ""
}

trim=false
largest=""

while [[ $# -gt 0 ]]; do
	case $1 in
		-t) trim=true; shift ;;
		-h) Usage ;;
		-*) echo "Unknown option: $1"; Usage ;;
		*)
			if [ -z "$largest" ]; then
				largest="$1"
				shift
			else
				echo "Unexpected argument: $1"
				Usage
			fi
			;;
	esac
done

if [ -z "$largest" ]; then
	read -p "Enter the largest percentage: " largest
fi

if ! [[ $largest =~ ^[0-9]+\.?[0-9]*$ ]]; then
	echo "That's not a number, try again"
	exit 1
fi

med=$(awk "BEGIN {printf \"%.15f\", $largest / 2}")
sml=$(awk "BEGIN {printf \"%.15f\", $med / 2}")

echo ""

files=()
for f in *.png; do
	if [[ ! "$f" =~ -big- ]] && [[ ! "$f" =~ -med- ]] && [[ ! "$f" =~ -sml- ]]; then
		files+=("$f")
	fi
done

if [ ${#files[@]} -eq 0 ]; then
	echo "No PNG files found"
	exit 1
fi

echo "Found ${#files[@]} images to resize"
echo ""

Resize "$largest" "big"
Resize "$med" "med"
Resize "$sml" "sml"

echo "Done"