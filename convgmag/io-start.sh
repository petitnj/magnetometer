#!/bin/bash
export PATH=/usr/local/bin:$PATH
HOME=/home/pi/code/convgmag
export HOME
# sleep 3
LOGFILE=/home/pi/code/convgmag/error.log
#if started from crontab
if pgrep -f "python3 /home/pi/code/convgmag/io_write.py btest4.csv" &>/dev/null; 
then
  date >> $LOGFILE
  echo "io_write.py already running" >> $LOGFILE
  exit 0
fi
#if started from command line
if pgrep -f "python3 io_write.py btest4.csv" &>/dev/null;
then
  date >> $LOGFILE
  echo "io_write.py already running" >> $LOGFILE
  exit 0
fi

date >> $LOGFILE 
echo "Starting io_write.py." >>$LOGFILE
cd /home/pi/code/lis3mdl/
bash -c "python3 /home/pi/code/convgmag/io_write.py btest4.csv &"

exit 0

