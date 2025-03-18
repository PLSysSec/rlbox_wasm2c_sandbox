#!/bin/sh

if [ ! -e wasi-sdk-21-src/build/install/opt/wasi-sdk/bin/clang ]; then
    git clone --depth 1 git@github.com:WebAssembly/wasi-sdk.git --branch wasi-sdk-21 --single-branch wasi-sdk-21-src
    cd wasi-sdk-21-src
    git submodule update --init --depth 100
    gmake -j
fi
