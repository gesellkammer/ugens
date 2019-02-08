#!/bin/bash
base=klib
pluginsdir=/usr/local/lib/csound/plugins64-6.0
includedir=/usr/local/include/csound
libdir=/usr/local/lib/csound


lib=lib$base.so

mkdir -p build
cd build
cp ../$base.c .
cp ../*.h .

cmd="gcc -O2 -shared -msse2 -o $lib -Wall -std=c99 -fPIC -mfpmath=sse -ffast-math -Wall -DUSE_DOUBLE -DB64BIT -I$includedir $base.c"
# cmd="gcc -O1 -shared -o $lib -Wall -fPIC -DUSE_DOUBLE -I$includedir $base.c"

echo $cmd
$cmd

cmd="sudo cp $lib $pluginsdir"
echo $cmd
$cmd

cd ..
# To check installation, do csound -z ^&1 | grep <ugen>"
