#!/bin/bash
cd /home/pi/code/convgmag
fin='btest0.csv'
echo $fin
fout='btest2.csv'
echo $fout
python3 ./spike_check.py  $fin $fout
fin='btest1.csv'
echo $fin
fout='btest3.csv'
python3 ./spike_check.py $fin $fout
exit 0

