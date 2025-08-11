#!/bin/sh
 
writefile="$1"
writestr="$2"
 
filebasename=$(basename $writefile)
pathname=$(dirname $writefile)
 
if [ $# -eq 2 ]; then
	if [ ! -d "$1" ]; then
		echo "Path does not exist. Create a new one..."
        mkdir -p "$pathname"
	fi
else
	echo "Number of input arguments should be equal to 02"
	echo "the first argument is a full path to a file (including filename) on the filesystem"
	echo " the second argument is a text string which will be written within this file"
	exit 1
fi
 
mkfile () {
    # Go to the directory where the file will be created
    cd "$pathname"
    echo "$writestr" > "$filebasename"
    echo "File created successfully!"
}
 
# Call the mkfile() function to create file with predefined content
mkfile