#!/bin/bash
#if cat $1 | grep "iteration 10"; 
#then
for i in  {1..3};
do
bash get_single_time.sh $1  $i
done
#fi
