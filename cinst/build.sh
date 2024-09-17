#!/bin/bash
set -e
if [ ! -d thirdparty ];
then
    mkdir thirdparty
fi
./scripts/build_spdlog.sh


BUILD_DIR=`pwd`
rm -rf build
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../install .. -DENABLE_CSV_OUTPUT=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=True
make -j
make install
