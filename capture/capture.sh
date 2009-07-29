#!/bin/bash

#set these vars correctly for your setup
dbHOST=acserver                   # real-time postgres db hostname
capture=/usr/bin/capture          # path to capture program
pidfile=/var/run/capture/capture.pid      # path to pid file
LOC='/scr/rafcam/flight_number_'  #location where images will be stored (should be on webserver)
CONF='/etc/capture.conf'          #location of camera configuration file

start() {

	#check to see if file exists
	if [ ! -s "$pidfile" ]
	then
		#start capture program
		($capture -c $CONF -f $LOC -d $dbHOST ) &

		#store pid for stop script
		cap_pid=$!
		retval=$?
		echo $cap_pid > $pidfile
		echo started cams

		return $retval
	else
		ps aux | grep -v grep | grep $capture > /dev/null
		if [ $? -eq 0 ]
		then
			echo already running
			exit 1
		else
			restart
		fi
	fi	

}

stop() {
	cap_pid=`cat $pidfile`
	kill -s INT $cap_pid  #send ctrl-c signal to allow program to clean up
	RETVAL=$?
	
	cat /dev/null > $pidfile #clear pid file
	echo stopped cams
	psql -U ads -d real-time -h $dbHOST -c "UPDATE camera SET status=0,message='Recording Stopped';"
	return $RETVAL
}

restart() {
	stop
	sleep 3
	start
}

status(){
	STATUS=`psql -U ads -h $dbHOST -d real-time -t -c "Select status from camera;"`
	MESSAGE=`psql -U ads -h $dbHOST -d real-time -t -c "Select message from camera;"`
	
	if [ -s "$pidfile" ]
	then
		ps aux | grep -v grep | grep $capture > /dev/null
		if [ $? -eq 0 ]
		then
			if [ ! "$STATUS" = "0" ]
			then
				echo "Capture process running correctly"
			else
				echo "Capture process is running but not correctly updating the database, could be caused by a network issue or incorrectly specified database"
			fi
		else
			echo "Capture process ended without calling this init script, could be triggered by night detection or error on the bus"
		fi
	else
		echo "Capture process is not running"
	fi
	
	echo -e "\nInitScript Variables:"
	echo -e "\tLatest Server Message:\t$MESSAGE"
	echo -e "\tDataBase:\t\t$dbHOST"
	echo -e "\tCapture Program:\t$capture"
	echo -e "\tPID file:\t\t$pidfile"
	echo -e "\tImage output directory:\t$LOC"
	echo -e "\tCapture Config file:\t$CONF"

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
  status)
  	status	
	;;
  *)
	echo $"Usage: $0 {start|stop|restart|status}"
	exit 1
	;;
esac
