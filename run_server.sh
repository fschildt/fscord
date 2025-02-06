#!/bin/sh

cd ./data/fscord_server
if [ "$1" = "debug" ]; then
    gdb --args ./fscord_server -port 1905
else
    ./fscord_server -port 1905
fi
cd ../..
