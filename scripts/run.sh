#!/bin/sh

system_architecture=$(getconf LONG_BIT)

cd ..

../bin/linux/${system_architecture}/snake $@
