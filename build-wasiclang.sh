#!/bin/sh

if [ ! -e wasi-sdk-27-src/build/install/opt/wasi-sdk/bin/clang ]; then
    git clone --depth 1 https://github.com/WebAssembly/wasi-sdk.git --branch wasi-sdk-27 --single-branch wasi-sdk-27-src
    cd wasi-sdk-27-src
    git submodule update --init --depth 100
    gmake -j
fi
