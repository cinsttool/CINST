set -u
#rm real.log
ls $1/*-*-*.log | awk '{print "bash get_time.sh " $1}' | sh
