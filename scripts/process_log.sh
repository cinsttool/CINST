#!/bin/bash
rm -f real.log real_cinst.log
python3 time2real.py ../logs/dacapo >> real.log
./log2time.sh ../logs/renaissance/ >> real.log
python3 time2real.py ../logs/spec >> real.log
python3 rm_@1.py real.log >> real_cinst.log

rm -f mem.log

python3 log2mem.py ../logs/dacapo >> mem.log
python3 log2mem.py ../logs/renaissance >> mem.log
python3 log2mem.py ../logs/spec >> mem.log
sed -i 's/-small//' mem.log

python3 overhead_merge.py > merge.log
