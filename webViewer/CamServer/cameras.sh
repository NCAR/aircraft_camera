#!/bin/bash

#set these vars correctly for your setup
dbHOST=lenado.eol.ucar.edu	# real-time postgres db hostname
capture=./capture			# path to capture program

start() {
	LOC='/scr/rafcam/flight_number_' 	#location where images will be stored (should be on webserver)
	CONF='cameras.conf'					#location of camera configuration file

	#start capture program
	($capture -c $CONF -f $LOC -d $dbHOST ) &

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
	psql -U ads -d real-time -h $dbHOST -c "UPDATE camera SET status=0,message='Recording Stopped';"
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
