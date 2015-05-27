#!/usr/bin/env bash

# Build dependencies
pushd contrib
./build.sh
popd

# Make build directory
mkdir -p build
cd build
qmake ..
make
