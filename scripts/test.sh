#!/bin/bash
./scripts/build.sh
cd build
ctest --output-on-failure -C "${BUILD_TYPE:-Release}"
