#!/bin/bash
cd /home/pi/code/convgmag
fin0='btest2.csv'
echo $fin0
fin1='btest3.csv'
echo $fin1
fout='btest4.csv'
echo $fout
python3 ./mergecsv.py $fin0 $fin1  $fout
sudo cp $fout /var/www/html/convgmag/themis.csv
exit 0

