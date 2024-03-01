#!/bin/bash

if [ $# -lt 2 ]; then 
    echo "Un-valid arguments!."
    echo "please enter first argument is a path to a directory on the filesystem, second argument is a text string which will be searched within these files"
    exit 1
fi

# Get the first argument
filesdir="$1"
# Get the second argument
searchstr="$2"
# Check if writefile is specified
if [ -z "$filesdir" ]; then
    echo "Error: Please provide a full path to the directory."
    exit 1
fi

# Check if writestr is specified
if [ -z "$searchstr" ]; then
    echo "Error: Please provide a text string to search in the directory."
    exit 1
fi

if [ ! -d "$filesdir" ]; then

    echo "$filesdir is not a valid directory or does not exist."
    exit 1
fi

# Count the number of files in the specified directory and its subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of matching lines containing the filename
match_count=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Check if the files are counted successfully
if [ $? -eq 0 ]; then
    echo "The number of files are $file_count and the number of matching lines are $match_count"
else
    echo "Error: Unable to find the file"
    exit 1
fi

