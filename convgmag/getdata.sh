#!/bin/bash
cd /home/pi/code/convgmag
sudo cp /home/pi/code/gmag/themis/data/recenth .
sudo chown pi recenth
sudo chgrp pi recenth
python3 ./time_conv.py
python3 ./spike_check.py
python3 ./mergecsv.py
cp btest3.csv themis.csv
sudo cp themis.csv /var/www/html/convgmag/
exit 0

