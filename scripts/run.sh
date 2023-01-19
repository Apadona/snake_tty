#!/bin/sh

system_architecture=$(getconf LONG_BIT)

../bin/linux/${system_architecture}/snake $@
