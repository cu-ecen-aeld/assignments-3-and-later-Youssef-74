#!/bin/bash

# Check if both arguments are provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
fi

# Extract arguments
writefile="$1"
writestr="$2"

# Check if writefile is specified
if [ -z "$writefile" ]; then
    echo "Error: Please provide a full path to the file (including filename)."
    exit 1
fi

# Check if writestr is specified
if [ -z "$writestr" ]; then
    echo "Error: Please provide a text string to be written in the file."
    exit 1
fi

# Create the directory if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Write content to the file
echo "$writestr" > "$writefile"

# Check if the file was created successfully
if [ $? -ne 0 ]; then
    echo "Error: Unable to create the file."
    exit 1
fi
