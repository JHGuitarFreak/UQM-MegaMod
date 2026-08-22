#!/bin/bash

# Takes a "-big.ani" file and creates -med and -sml versions by dividing the values in half successively
# Further adjustment needs to be done to the -med and -sml .ani files either manually or with
# Joris van de Donk's uqmanimationtool, which is included in the tools folder of this repository

# Usage: ./make-ani.sh [-f] [filename-big.ani]
#   -f    Overwrite existing files without asking

DivideByTwo()
{
	echo $(( ($1 + 1) / 2 ))
}

SetupOutput()
{
	local file=$1
	local var=$2
	
	if [ "$FORCE" = true ]; then
		eval "$var=true"
		> "$file" 2>/dev/null || true
		return
	fi
	
	if [ -f "$file" ]; then
		echo "Warning: '$file' already exists."
		read -p "Overwrite? (y/n): " -n 1 -r
		echo
		[[ ! $REPLY =~ ^[Yy]$ ]] && { eval "$var=false"; return; }
	fi
	
	eval "$var=true"
	> "$file"
}

ProcessLines()
{
	local input=$1
	local output=$2
	local suffix=$3
	local factor=$4

	local total_lines=$(grep -c '.' "$input" 2>/dev/null || echo 0)
	local processed=0

	echo "Output $suffix: $output"

	while IFS= read -r line || [ -n "$line" ]; do
		[ -z "$line" ] && continue

		processed=$((processed + 1))
		
		fields=($line)
		[ ${#fields[@]} -lt 5 ] && continue
		
		file="${fields[0]%.png}"
		nums="${fields[-2]} ${fields[-1]}"
		rest="${fields[@]:1:${#fields[@]}-3}"

		for _ in $(seq 1 $factor); do
			nums=$(printf "%s %s" $(DivideByTwo ${nums% *}) \
				$(DivideByTwo ${nums#* }))
		done
		
		echo "${file/-big/-$suffix}.png $rest $nums" >> "$output"
	done < "$input"
	
	echo "Finished $suffix: $processed lines processed"
}

FORCE=false
while getopts "f" opt; do
	case $opt in f) FORCE=true;; *) exit 1;; esac
done
shift $((OPTIND-1))

FILENAME="$1"
if [ -z "$FILENAME" ]; then
	echo "Enter the filename (e.g., eluder-big.ani):"
	read -r FILENAME
fi

[ -z "$FILENAME" ] && { echo "Error: No filename provided."; exit 1; }
[ ! -f "$FILENAME" ] && { echo "Error: File '$FILENAME' not found."; exit 1; }

BASE_NAME="${FILENAME%.ani}"
BASE_NAME="${BASE_NAME%-big}"
MED_FILE="${BASE_NAME}-med.ani"
SML_FILE="${BASE_NAME}-sml.ani"

SetupOutput "$MED_FILE" CREATE_MED
SetupOutput "$SML_FILE" CREATE_SML

[ "$CREATE_MED" = true ] && ProcessLines "$FILENAME" "$MED_FILE" "med" 1
[ "$CREATE_SML" = true ] && ProcessLines "$FILENAME" "$SML_FILE" "sml" 2