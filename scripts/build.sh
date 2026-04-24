#!/bin/bash
BUILD_TYPE=${BUILD_TYPE:-Release}
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --config "$BUILD_TYPE" --parallel
