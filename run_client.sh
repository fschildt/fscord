#!/bin/sh

cd ./data/fscord
if [ "$1" = "debug" ]; then
    gdb ./fscord
else
    ./fscord
fi
cd ../..

