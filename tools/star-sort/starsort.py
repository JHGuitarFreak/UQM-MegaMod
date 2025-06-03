#!/usr/bin/env python

# Created with help from DeepSeek R1
# Takes a text file with a custom star aray for plangen.c and arranges it by the Y coordinates
# Instructions - 
# Copy your star array that's in plangen.c from the first element to the element just before the first MAX_X/Y_UNIVERSE element and place it in a text file
# Run this script on the text file to sort the array using, as an example `python ./starsort.py tosort.txt sorted.txt`
# Take the results from the sorted text file and overwrite what you initially copied.

import argparse

# Function to extract both the first and second numbers from a line
def extract_sort_key(line):
	# Skip empty lines
	if not line.strip():
		return None

	parts = line.split(',')
	# Check if there are at least two parts (first and second numbers)
	if len(parts) < 2:
		return None

	try:
		# Extract the first number and remove any leading/trailing whitespace or braces
		first_number = int(parts[0].strip().lstrip('{').strip())
		# Extract the second number and remove any trailing characters (like '}')
		second_number = int(parts[1].strip().rstrip('}'))
		# Return a tuple (second_number, first_number) for sorting
		return (second_number, first_number)
	except ValueError:
		# Skip lines with invalid numbers
		return None

parser = argparse.ArgumentParser(description="Sort lines in a file by the second number, then by the first number.")
parser.add_argument('input_file', help="Path to the input text file")
parser.add_argument('output_file', help="Path to the output text file")
args = parser.parse_args()

try:
	with open(args.input_file, 'r') as file:
		lines = file.readlines()
except FileNotFoundError:
	print(f"Error: The file '{args.input_file}' does not exist.")
	exit()

# Extract valid lines with sort keys
valid_lines_with_keys = []
for line in lines:
	sort_key = extract_sort_key(line)
	if sort_key is not None:
		valid_lines_with_keys.append((sort_key, line))

# Sort the lines based on the second number, then the first number
try:
	sorted_lines = [line for _, line in sorted(valid_lines_with_keys, key=lambda x: x[0])]
except ValueError as e:
	print(f"Error: Failed to sort lines. Details: {e}")
	exit()

# Write the sorted lines to the output file
try:
	with open(args.output_file, 'w') as file:
		for line in sorted_lines:
			# Ensure each line ends with a newline character
			if not line.endswith('\n'):
				line += '\n'
			file.write(line)
	print(f"Lines sorted and saved to {args.output_file}")
except PermissionError:
	print(f"Error: You do not have permission to write to '{args.output_file}'.")
	exit()