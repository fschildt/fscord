#!/bin/sh

if [ "$1" = "server" ]; then
    cd ./data/fscord_server
    ./fscord_server -port 1905 -keyfile key.pem
    cd ../..
else
    if [ "$1" = "debug" ]; then
        cd ./data/fscord
        gdb ./fscord
        cd ../..
    else
        cd ./data/fscord
        ./fscord
        cd ../..
    fi
fi
