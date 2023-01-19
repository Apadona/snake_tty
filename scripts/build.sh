#!/bin/sh

cd ..

if [ -d build_files ]; then
	cd build_files
else
	mkdir build_files
	cd build_files
fi

cmake .. -G "Unix Makefiles"
echo "cmake has generated the build files. now onto building the project."


cmake --build . -j$(nproc)
echo "building is complete."
