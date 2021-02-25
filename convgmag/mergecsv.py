#!/usr/bin/python3
# mergecsv.py
# merge yesterday's (btest0.csv) csv with today's (btest1.csv) output btest2.csv
#  NJP 2-11-21
# merge the two input .csv files into one output file
from datetime import datetime
import re
import sys
if len(sys.argv) != 4:
    print('enter two incoming file nams and one outgoing')
    print(' -- btest2.csv btest3.csv themis.csv')
    sys.exit()
# 
fin0 = open('/home/pi/code/convgmag/'+sys.argv[1],"r",1)
fin1 = open('/home/pi/code/convgmag/'+sys.argv[2],"r",1)
fout = open('/home/pi/code/convgmag/'+sys.argv[3],"w",1)
fout.write('TIME,ETemp,STemp,GTemp,CH1,CH2,CH3,GPS\n') # write out a new header
line = fin0.readline() # read the first line of 0 discard
while True: #scan thru btest0.csv line by line add it to output
    line = fin0.readline()
    if not line:
        break
    fout.write(line)  # write out the data line
# close recenth.old
fin0.flush()
fin0.close()
#output file is still open for more appending

line = fin1.readline() # read off the header line of 2, throw it away (output already has header)
while True:  #read thru line by line and convert time and substitute commas for spaces
    line=fin1.readline()
    if not line:
        break
    fout.write(line)
#write out today's data to the end of the output file
fin1.close()
fout.flush()
fout.close()
#close recenth and the output file (btest2.csv)
