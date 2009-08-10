#!/bin/bash

#set these vars correctly for your setup
dbHOST=acserver                                 # real-time postgres db hostname
pidfile=/var/run/capture/capture.pid            # path to pid file
init_capture=/etc/init.d/capture                # path to capture initscript
reset_bus=/usr/sbin/capture_reset_bus           # path to software ieee1394 bus reset program
find_pid="ps -C capture -o pid="
num_deaths=0

#wait for program to update database
sleep 5

#loop forever
while true; do

	#check to see if another proc set the status to 0 (clean shutdown)
	STATUS=`psql -U ads -h $dbHOST -d real-time -t -c "Select status from camera;" | tr -d " \n"`
#	echo "status = $STATUS"
	if [ "$STATUS" = "0" ];
	then
		echo "clean shutdown detected. exiting."
		exit	
	else
		file_pid=`cat $pidfile`
		junk=`ps aux | grep -v grep | grep $file_pid`
		alive_file_pid=$?

		#is pid running?
#		cap_pid=`$find_pid`
#		alive_pid=$?
#		echo "File pid: $file_pid, pid found: $cap_pid"
		if [ "$alive_file_pid" = "0" ];
		then
#			echo "found capture pid: $file_pid running."
			sleep 5
		else 
			echo "Capture process ended un-cleanly. This is probably caused by bus reset, Attempting to restart process..."
			#update database w/ error status
			junk=`psql -U ads -d real-time -h $dbHOST -c "UPDATE camera SET status=2,message='Bus reset, restarting...';"`

				#how many times has it died?
				if [ $num_deaths -lt 3 ];
				then
					#run dc1394_reset_bus
					$reset_bus
					
					#increment a counter
					num_deaths=`expr $num_deaths + 1`
#					echo "numdeaths: $num_deaths"

					#wait 2 seconds
					sleep 2

					#re-start capture process
					$init_capture start_no_mon
					
					#wait 3 seconds
					sleep 3
				else
					#log / display error
					echo "Capture process has died 3 times, please check configuration"

					#give up
					junk=`psql -U ads -d real-time -h $dbHOST -c "UPDATE camera SET status=2,message='Fatal Error';" `
					exit
				fi
		fi
	fi
done


exit

