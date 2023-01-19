##### simple console snake game

snake game coded in C++ for console enviroment.

#### for compiling the project you need:
### gcc ( that supports C++17 )
### cmake( atleast 3.16 ) installed.

For compiling the project simply just run the **scripts/build.sh** file and it will build automatically. for compiling in debug mode, run **scripts/build_debug.bat**.

For running the project, simply just execute **run.sh** for normal build, and **run_debug.sh** for debugging ( debug build ).

**run.sh** will supply run the executable with project root directory as it's working directory, if you wish to copy the executable somewhere else or ship it, you need to also copy data directory into the executable directory itself, or if you want to use the .sh file, you need to copy them with respect to the relative path they have with the run.sh file or modify the script file.

---