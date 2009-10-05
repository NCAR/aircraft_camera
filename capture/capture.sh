#!/bin/bash

#This is the initscript for the capture program

#set these vars correctly for your setup
dbHOST="acserver"                      # real-time postgres db hostname
capture="/usr/bin/capture"                       # path to capture program
LOC='/mnt/acserver/r2/camera_images/flight_number_'      #location where images will be stored
CONF='/etc/capture.conf'                   #location of camera configuration file
monitor_script="/usr/sbin/capture_monitor.sh $dbHOST $0"
logit="logger -t capture_init -s -p local1.notice"         # command to send message to syslog & stderr

start() {

	# make sure no other capture process is running 
	if ! ps h -C $capture > /dev/null
	then
		#start capture program
		($capture -c $CONF -f $LOC -d $dbHOST $logfile) &

		cap_pid=$!
		$logit "started capture [pid: $cap_pid]"

	        #launch a monitor script, unless start was called by an already running monitor
	        if [ "$1" = "launch" ]; then
        	    $monitor_script $cap_pid &	 
				mon_pid=$!
		    $logit "started monitor [pid: $mon_pid]"
	        fi

	else
		$logit "capture already running"
		exit 1
	fi	

}

stop() {

	#send ctrl-c signal to allow program to clean up
	ps h -C capture > /dev/null && killall -HUP capture

	$logit "stopped cams"
}

restart() {
	stop
	sleep 3
	start $1
}

status(){
	STATUS=`psql -U ads -h $dbHOST -d real-time -t -c "Select status from camera;"`
	MESSAGE=`psql -U ads -h $dbHOST -d real-time -t -c "Select message from camera;"`
	
	echo -e "\nInitScript Variables:"
	echo -e "\tLatest Server Message:\t$MESSAGE"
	echo -e "\tDataBase:\t\t$dbHOST"
	echo -e "\tCapture Program:\t$capture"
	echo -e "\tPID file:\t\t$pidfile"
	echo -e "\tImage output directory:\t$LOC"
	echo -e "\tCapture Config file:\t$CONF"

}

case "$1" in
  start) #standard init, we want to launch a monitor process as well
  	start launch
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
  start_no_mon) #called by capture-monitor to restart the process, dont start another monitor
	start
	;;
  *)
	echo $"Usage: $0 {start|start_no_mon|stop|restart|status}"
	exit 1
	;;
esac
