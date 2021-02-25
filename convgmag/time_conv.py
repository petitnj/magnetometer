#!/usr/bin/python3
# convert recenth format 09212020    0:06 to
# strftime format        2020-09-21 00:06:00
# for plotting
# opens recenth then append to btest1.csv  with NO header
# throw out header line and first data line as usually bogus NJP 2-11-21
from datetime import datetime
import re
# 
fin = open('/home/pi/code/convgmag/recenth',"r",1)
line = fin.readline()  #read the header line, discard
line = fin.readline() # read the first data line -- usually bogus
fout = open('/home/pi/code/convgmag/btest1.csv',"a",1)
# write the new header line (comma separated)
# fout.write('TIME,ETemp,STemp,GTemp,CH1,CH2,CH3,GPS\n')
while True: #scan thru recenth line by line add it to btest2.csv
    line = fin.readline()
    if not line:
        break
    timer = line[:16]  # get the time part of the string
    rest  = line[16:]  # get the rest (data)
    timer_object = datetime.strptime(timer,'%m%d%Y  %H:%M')  #convert string to datetime object
    timer_str = timer_object.strftime('%Y-%m-%d %H:%M:%S')  #convert to formatted datetime string
    restc = re.sub("\s+", ",", rest.strip())  #strip the data of spaces and convert to commas
    fout.write(timer_str+"," + restc+"\n")  # write out the data line
# close recenth.old
fin.flush()
fin.close()
fout.close()
