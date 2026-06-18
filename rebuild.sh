#!/bin/bash
rm -rf ./build/ ./install_test/
cmake -S. build
cmake --build build
sudo cmake --install build
