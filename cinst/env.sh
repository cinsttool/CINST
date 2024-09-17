#!/bin/bash
ROOT=`pwd`
ROOT=$ROOT/install
export CINST_ROOT=$ROOT
echo "Cinst Install Directory: $ROOT"
export PATH=$ROOT/bin:$PATH:$ROOT/../scripts
export LD_LIBRARY_PATH=$ROOT/lib:$LD_LIBRARY_PATH
export LIBNATIVE=$ROOT/lib/libagent.so
