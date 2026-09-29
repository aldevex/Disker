#!/bin/bash

cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang
cmake --build "build"
mv "build/Disker" "a.out"
