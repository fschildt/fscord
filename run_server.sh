#!/bin/sh

if [[ $1 == "debug" ]]; then
    gdb --args ./build/fscord_server -port 1905 -keyfile ./data/server/key.pem
else
    ./build/fscord_server -port 1905 -keyfile ./data/server/key.pem
fi
