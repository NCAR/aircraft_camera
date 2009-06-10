#!/bin/bash


start() {
	#get flight number
	export PGHOST=lenado.eol.ucar.edu
	export PGDATABASE=real-time
	export PGUSER=ads
	num=`psql -A -t -c "SELECT value FROM global_attributes WHERE key='FlightNumber'"`;

	#start capture program
	(./capture $num ) &

	#store pid for stop script
	cap_pid=$!
	retval=$?
	echo $cap_pid > pid

	echo started cams
	return $RETVAL
}

stop() {
	cap_pid=`cat pid`
	kill -s INT $cap_pid  #send ctrl-c signal to allow program to clean up
	RETVAL=$?
	
	rm -f pid 
	echo stopped cams
	return $RETVAL
}

restart() {
	stop
	sleep 3
	start
}

case "$1" in
  start)
  	start
	;;
  stop)
  	stop
	;;
  restart)
  	restart
	;;
  *)
	echo $"Usage: $0 {start|stop|restart}"
	exit 1
	;;
esac
