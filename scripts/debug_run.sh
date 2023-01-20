#!/bin/sh

system_architecture=$(getconf LONG_BIT)

cd ..

gdb bin/linux/${system_architecture}/snake $@