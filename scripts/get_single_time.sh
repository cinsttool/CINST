#!/bin/bash
set -e
file_name=`basename $1`
TIME=`cat $1 | grep "completed (" | awk -v FS='(' '{print $NF}' | awk -v FS='ms' '{print $1}' | tail -n $2 | head -n 1`
MODE=`echo $file_name | awk -v FS='-' '{print  $1}'`
CASE=`echo $file_name | sed -r 's/^\w+-(.*)-[^-]+$/\1/'`
MARK=`echo $file_name | awk -v FS='-' '{print  $NF}'`
echo $MODE, $CASE, $TIME, $MARK@$2 
