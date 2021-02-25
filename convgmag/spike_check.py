#!/usr/bin/python3
# remove data spikes from btest2.csv
# first data line from incoming recenth have been removed and time converted 
# files are csv files
#  writes to btest3.csv
from datetime import datetime
import re
import sys
if len(sys.argv) != 3:
    print('enter incoming file and outgoing file names')
    sys.exit()
infile = sys.argv[1]
outfile = sys.argv[2] 
oldetemp = oldstemp = oldgtemp = 0.0
oldxmag = oldymag = oldzmag = 0.0
fin = open('/home/pi/code/convgmag/'+infile,"r",1)
line = fin.readline()  #read the header line, discard
fout = open('/home/pi/code/convgmag/'+outfile,"w",1)
# write the header line (comma separated)
fout.write(line)  # write out the header line
# get an average of each data field
count = 0
while True:  # get raw average values
    oldline = fin.readline() # read the first data line and set up data values
    if not oldline:
        break
    count += 1
    fields = oldline.split(',')
    oldetemp += float(fields[1])
    oldstemp += float(fields[2])
    oldgtemp += float(fields[3])
    oldxmag +=  float(fields[4])
    oldymag +=  float(fields[5])
    oldzmag +=  float(fields[6])
oldetemp = oldetemp/count
oldstemp = oldstemp/count
oldgtemp = oldgtemp/count
oldxmag = oldxmag/count
oldymag = oldymag/count
oldzmag = oldzmag/count
# print (str(oldetemp), str(oldstemp),str(oldstemp))
# print (str(oldxmag), str(oldymag), str(oldzmag))
# print ('count = ' +str(count))
fin.close() # to start again
# now get the average with the spikes thrown out
count = 0
etot = stot = gtot = 0.0
xtot = ytot = ztot = 0.0
fin = open('/home/pi/code/convgmag/'+infile,"r",1)
line = fin.readline() # read the header line and ignore
while True:
    line = fin.readline()
#    print(line)
    if not line:
        break
    fields = line.split(',')
    try:
        etemp = float(fields[1])
        stemp = float(fields[2])
        gtemp = float(fields[3])
        xmag =  float(fields[4])
        ymag =  float(fields[5])
        zmag =  float(fields[6])
        ediff = abs(oldetemp - etemp)
        sdiff = abs(oldstemp - stemp)
        gdiff = abs(oldgtemp - gtemp)
        xdiff = abs(oldxmag - xmag)
        ydiff = abs(oldymag - ymag)
        zdiff = abs(oldzmag - zmag)
        tdiff = max(ediff, sdiff, gdiff)
        mdiff = max(xdiff, ydiff, zdiff)
        if (tdiff <20.0 and mdiff < 4000):
            etot += etemp
            stot += stemp
            gtot += gtemp
            xtot += xmag
            ytot += ymag
            ztot += zmag
            count += 1
    except:
        pass
oldetemp = etot/count
oldstemp = stot/count
oldgtemp = gtot/count
oldxmag =  xtot/count
oldymag =  ytot/count
oldzmag =  ztot/count
# print('after filter')
# print(str(oldetemp), str(oldstemp), str(oldgtemp))
# print(str(oldxmag), str(oldymag), str(oldzmag))
fin = open('/home/pi/code/convgmag/'+infile,"r",1)
while True: #scan thru btest0.csv line by line to remove spikes
    line = fin.readline()
    if not line:
        break
    fields=line.split(',')
#    print(line)
    try:
        etemp = float(fields[1])
        stemp = float(fields[2])
        gtemp = float(fields[3])
        xmag =  float(fields[4])
        ymag =  float(fields[5])
        zmag =  float(fields[6])
        ediff=  abs(oldetemp - etemp)
        sdiff = abs(oldstemp - stemp)
        gdiff = abs(oldgtemp - gtemp)
        xdiff = abs(oldxmag - xmag)
        ydiff = abs(oldymag - ymag)
        zdiff = abs(oldzmag - zmag)
        mmax = max(xdiff,ydiff,zdiff)
        tmax = max(ediff, sdiff, gdiff)
        if ((mmax < 200) and (tmax < 5.0)):
            fout.write(line)  # write if no spike
        else:
            pass
#            print('found spike at ' +  fields[0])
    except:
        pass
# close everything
fin.flush()
fin.close()
fout.close()
