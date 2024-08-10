#!/bin/sh

# Define the target directory
directory="./src"

# Check if the target is not a directory
if [ ! -d "$directory" ]; then
    echo "Error: $directory is not a valid directory"
    exit 1
fi

# Use find to iterate over all files in the directory and subdirectories
find "$directory" -type f | while read -r file; do
    if [[ "${file##*/}" =~ ^[a-z_]+\.c|h$ ]]; then
      echo "Formatting file: $file..."

      clang-format -i "$file"
    fi
done
