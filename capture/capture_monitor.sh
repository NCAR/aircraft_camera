#!/bin/bash

if [ "$1" = "" ]; then 
	echo "this script gets called automatically from the capture initscript"
	echo "to use manually call:"
	echo "$0 <dbHost> <path-to-capture-binary> <path-to-initscript>"
	exit
fi

#set these vars correctly for your setup
numRetries=2
dbHOST=$1
init_capture=$2
#reset_bus='/usr/sbin/capture_reset_bus'    # path to software ieee1394 bus reset program
reset_bus='./capture_reset_bus'    # path to software ieee1394 bus reset program
num_deaths=0
logit="logger -t capture_monitor -s -p local1.notice"         # command to send message to syslog & stderr

#echo "dbHost: $dbHOST, init_capture: $init_capture, logit: $logit"

#wait for program to update database
sleep 5

#loop forever
while true; do

	#check to see if another proc set the status to 0 (clean shutdown)
	STATUS=$(psql -U ads -h $dbHOST -d real-time -t -c "Select status from camera;" | tr -d " \n")
	if [ "$STATUS" = "0" ];
	then
		$logit "clean shutdown detected. exiting."
		exit	
	else

		#is process running?
		if ps h -C capture > /dev/null
		then
			sleep 5
		else 
			#how many times has it died?
			if [ $num_deaths -lt $numRetries ];
			then
				#increment a counter
				num_deaths=`expr $num_deaths + 1`
				$logit "Capture process ended un-cleanly [$num_deaths time(s)], restarting."

				#update database w/ error status
				junk=`psql -U ads -d real-time -h $dbHOST -c "UPDATE camera SET status=2,message='restarting...';"`

				#run dc1394_reset_bus
				$reset_bus > /dev/null

				#wait 2 seconds
				sleep 2

				#re-start capture process
				$init_capture start_no_mon
				
				#wait 3 seconds
				sleep 3
			else
				#log / display error
				$logit "Capture process has ended $numRetries times, no longer attempting to restart. Please check configuration."

				#give up
				junk=`psql -U ads -d real-time -h $dbHOST -c "UPDATE camera SET status=2,message='Fatal Error';" `
				exit
			fi
		fi
	fi
done


exit

