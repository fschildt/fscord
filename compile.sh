#!/bin/sh

mkdir -p build
cd build

if [[ "$0" == "Release" ]]; then
    echo "Release"
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../cmake
else
    echo "Debug"
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../cmake
    cp 
fi

make
cd ..
