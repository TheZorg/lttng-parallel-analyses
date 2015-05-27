#!/bin/bash

pushd ..
git submodule init
git submodule update
popd

pushd tigerbeetle
./bootstrap.sh
scons
popd
