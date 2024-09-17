ps -ef | grep java | grep -v code | grep server | awk '{print "kill -9 " $2}' | sh
