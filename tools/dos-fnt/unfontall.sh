#!/bin/bash

FILES=$(find . -type f -iname '*.fon')

for file in $FILES; do
    ./unfont -o out/"$file" -m -x -0 5 -c data.cfg "$file"
done