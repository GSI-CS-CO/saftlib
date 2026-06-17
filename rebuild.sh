#!/bin/bash
rm -rf ./build/ ./install_test/
cmake -S. build
cmake --build build
cmake --install build --prefix=./install_test --config Release


