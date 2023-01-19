#!/bin/sh

cd ..

if [ -d bin ]; then
	echo "bin directory exists, attempting to remove it..."
	rm -rf bin
	echo "succesfully deleted the bin directory."
fi

if [ -d build_files ]; then
	echo "build_files directory exists, attemting to remove it..."
	rm -rf build_files
	echo "succesfully deleted the build_files directory"
fi
