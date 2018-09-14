#!/bin/bash
base=emugens
pluginsdir=/usr/local/lib/csound/plugins64-6.0
includedir=/usr/local/include/csound


lib=lib$base.so

mkdir -p build
cd build
cp ../$base.c .

cmd="gcc -O2 -shared -o $lib -Wall -fPIC $base.c -DUSE_DOUBLE -I$includedir"
echo $cmd
$cmd

cmd="cp $lib $pluginsdir"
echo $cmd
$cmd

cd ..
# To check installation, do csound -z ^&1 | grep <ugen>"
