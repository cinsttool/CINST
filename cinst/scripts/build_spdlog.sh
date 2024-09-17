#!/bin/bash
set -e
cd thirdparty
if [ -d spdlog/install ];
then
    exit
fi

if [ ! -d spdlog ];
then
    git clone https://github.com/gabime/spdlog.git
fi

cd spdlog && git checkout v1.13.0 && mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/../install && make -j install
