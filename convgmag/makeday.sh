#!/bin/bash
cd /home/pi/code/convgmag
dod=$(date -u '+%Y-%d-%m' )
echo $dod
doy=$(date -u -d '-1 day' '+%Y-%d-%m')
echo $doy
fout='btest1.csv'
echo $fout
fout1='btest0.csv'
python3 ./makeday.py $dod $fout
python3 ./makeday.py $doy $fout1
exit 0

