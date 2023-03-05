#!/bin/sh


mkdir -p build


echo "======================"
echo "    Building Tools    "
echo "======================"
CC=clang
CFLAGS="-std=c99 -Wall -Isrc -Ilibs/include"
LFLAGS="-lm"
SRC="src/tools/asset_generator.c"
$CC $SRC $CFLAGS $LFLAGS -o build/asset_generator



echo "======================"
echo "   Building fscord    "
echo "======================"
./build/asset_generator
CC=clang
CFLAGS="-std=c99 -c -Wall -Ilibs/include/libressl -Isrc -g"
LFLAGS="-Llibs/static/libressl -lcrypto -lX11 -lGL -lasound"
SRC_FSCORD="src/client/fscord.c"
SRC_SYS_LIN="src/client/sys/lin/sys_lin.c"

$CC $SRC_FSCORD  $CFLAGS -o build/fscord.o
$CC $SRC_SYS_LIN $CFLAGS -o build/sys_lin.o
$CC build/fscord.o build/sys_lin.o $LFLAGS -o build/fscord
echo ""; echo "";



echo "======================"
echo "Building fscord_server"
echo "======================"
mkdir -p build/cmake_fscord_server
cd build/cmake_fscord_server
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ../../cmake/CMakeLists.txt
make
cd ../..
