#!/bin/bash

compare_files() {
    file1="$1"
    file2="$2"
    echo "Comparing $file1 and $file2:"
    diff_output=$(diff "$file1" "$file2")
    if [ $? -eq 0 ]; then
        echo "Files are identical"
    else
        echo "Files are different"
    fi
    echo ""
}

# List of file pairs
file_pairs=(
    "6000.txt 6000r.txt"
    "6002.txt 6002r.txt"
    "6004.txt 6004r.txt"
    "6006.txt 6006r.txt"
    "6008.txt 6008r.txt"
    "6010.txt 6010r.txt"
    "6012.txt 6012r.txt"
    "6014.txt 6014r.txt"
    "6016.txt 6016r.txt"
    "6018.txt 6018r.txt"
    "6020.txt 6020r.txt"
    "6022.txt 6022r.txt"
)

# Iterate through each pair and compare
for pair in "${file_pairs[@]}"; do
    compare_files $pair
done