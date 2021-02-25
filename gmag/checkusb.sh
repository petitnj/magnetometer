#! /bin/bash
PATH=/home/pi/perl5/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PATH
if pgrep "GBOMag_Test1" > /dev/null
then
	if [ -e /dev/ttyUSB1 ] ;
	then
		if [ -s /home/pi/code/gmag/checkusb.txt ]
		then
			if tail -20 /home/pi/code/gmag/checkusb.txt | grep -q 'G' ;
			then
				if tail -20 /home/pi/code/gmag/checkusb.txt | grep -q 'L00000' ;
				then
					echo "$(date) The magnetic sensor was not found. Attempting to restart the GBOMag program."
					sudo pkill GBOMag_Test1
					sleep 20
					nohup /home/pi/code/gmag/GBOMagStartup.sh 0<&- 2>/home/pi/code/gmag/gmag.txt &
				else
					echo "$(date) Everything seems to be working."
				fi
			else
				echo "$(date) The GPS antenna was not found. Attempting to restart the GBOMag program."
				sudo pkill GBOMag_Test1
				sleep 20
				nohup /home/pi/code/gmag/GBOMagStartup.sh 0<&- 2>/home/pi/code/gmag/gmag.txt &
			fi
		else
			echo "$(date) The USB connection has hung. Must restart computer."
			sudo reboot
		fi
	else
		echo "$(date) The USB connection to the electronics box was not found. Attempting to restart the GBOMag program. If the USB connection is still not found, the program will fail to restart."
		sudo pkill GBOMag_Test1
		sleep 20
		/home/pi/code/gmag/GBOMagStartup.sh 0<&- 2>/home/pi/code/gmag/gmag.txt &
	fi
else
	echo "$(date) The GBOMag program was not running. Attempting to restart, if USB connection is not found, the program will fail to restart."
	nohup /home/pi/code/gmag/GBOMagStartup.sh 0<&- 2>/home/pi/code/gmag/gmag.txt &
fi
sleep 5
cp /dev/null /home/pi/code/gmag/checkusb.txt


