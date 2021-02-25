#!/usr/bin/python3
# merge a day of .HKD to one file
from datetime import datetime, time, date
import re
from os import listdir
from os.path import isfile, join
import glob
import sys
if len(sys.argv)< 3:
    print('enter date of input and output file name -- eg 2021-15-02 btest0.csv')
    sys.exit()
dater = sys.argv[1]
fnameout = sys.argv[2]
print (dater)
print (fnameout)
date_obj = datetime.strptime(dater, '%Y-%d-%m')
yrstring = date_obj.strftime('%Y/')
mostring = date_obj.strftime('%m/')
daystring = date_obj.strftime('%d/')
# determine the date and directory that holds the actual data
# NJP  2-15-21
today = date.today()
filename = '/home/pi/code/gmag/themis/data/'+yrstring+'/'+mostring+'/'+daystring
outfile = '/home/pi/code/convgmag/'+fnameout
fout = open(outfile,"w")
files0 = glob.glob(filename+'/*.HKD')
header = 'TIME,ETemp,STemp,GTemp,CH1,CH2,CH3,GPS\n'
fout.write(header)
files0.sort()

for f in files0:
    fin = open(f)
    line1 = fin.readline() # read and discard the header line
    while True:
        line = fin.readline()
        if not line:
            break
        timer = line[:16]
        rest = line[16:]
        timer_object = datetime.strptime(timer,'%m%d%Y %H:%M')
        timer_str = timer_object.strftime('%Y-%m-%d %H:%M:%S')
        restc = re.sub("\s+", ",", rest.strip())
        fout.write(timer_str+"," + restc +"\n")
    fin.close()
fout.close()

