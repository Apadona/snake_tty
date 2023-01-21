#!/bin/sh

cd ..

if [ -d build_files ]; then
	cd build_files
else
	mkdir build_files
	cd build_files
fi

cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug"
if [ $? -eq 0 ]; then
	echo "cmake has generated the build files. now onto building the project."

	cmake --build . -j$(nproc)
	if [ $? -eq 0 ]; then
		echo "building is complete."
	fi
fi
