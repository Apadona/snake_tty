#!/bin/sh

system_architecture=$(getconf LONG_BIT)

cd ..

gdbtui bin/linux/${system_architecture}/snake $@
