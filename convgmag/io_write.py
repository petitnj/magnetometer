
#!/usr/bin/python3
# io_write.py
# takes anti-spiked file btest3.csv
# Writes Falcon magnetometer data to io.adafruit.com ucla group 
# NJP 2/11/21
#2-18-21 NJP accounted for day change in input file
  
from datetime import datetime
from Adafruit_IO import Client, Feed
import signal
import schedule
import sched
import time
import sys

if len(sys.argv)< 2:
    print('enter filename to monitor')
    sys.exit(0)
TIME_ZONE = 'America/Chicago'
OPENWEATHER_TOKEN ='0dac8d87dfe8997cfeb046d133416040'
ADAFRUIT_IO_USERNAME = 'petitnoel'
ADAFRUIT_IO_KEY = 'aio_wveE73iOoYGETZ0iTV6ID8wlsT2A'

# Create an instance of the REST client.
aio = Client(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)

# Set up Adafruit IO Feeds.
xmag_feed = aio.feeds('ucla.xmag')
ymag_feed = aio.feeds('ucla.ymag')
zmag_feed = aio.feeds('ucla.zmag')
ftemp_feed = aio.feeds('ucla.ftemp')
sam_time = 10.0

def setup():
    global lasttime
    lasttime = ' '

# read thru the despiked and merged file to get to the end 
# assume that all have been read to this time
def first_time():
#    print('entering first_time()')
    global lasttime
    global fin
    filename = "/home/pi/code/convgmag/"+sys.argv[1]
#    print(filename)
    fin = open(filename,"r",1)
    line = fin.readline() # read the header line and ignore
    while True:
        line = fin.readline() # read thru the file
        if line:
            fields = line.split(',')
            lasttime = fields[0]
        else:
            break
    fin.close()
#    print('found end of the file '+lasttime)
#    print('exit first_time()')
    return

def check_data():
    global lasttime
    global fin
    fin = open("/home/pi/code/convgmag/"+sys.argv[1],"r",1)
    line = fin.readline() # read the header line discard
    if not line: # empty file
        fin.close()
        return
    line = fin.readline() # read the first line
    if not line: 
        fin.close()
        return   # no data
    fields = line.split(',')
    while (fields[0] != lasttime):  # read to lasttime
        line = fin.readline()
        if not line:
            print(fields)
            fin.close()
            return
        fields = line.split(',')
#    print('found lasttime line')
    line = fin.readline() # read the next line
    if not line:
        fin.close()
#        print('no new data')
        return # no new data
    fields = line.split(',')
#    print('adding new data')
#  read new line to io_feed
    aio.send(ftemp_feed.key,fields[1])
    aio.send(xmag_feed.key,fields[4])
    aio.send(ymag_feed.key,fields[5])
    aio.send(zmag_feed.key,fields[6])
    lasttime = fields[0] # advance the last time 
    print('entered new data '+lasttime)
    fin.close()

def sig_handler(sig,frame): #gracefully exit on contro-C
    global fin
    print ('  exiting')
    try:
        fin.close()
    except:
        pass
    sys.exit()

signal.signal(signal.SIGINT,sig_handler) # wait
setup()
first_time()
schedule.every(sam_time).seconds.do(check_data)


while True:
    schedule.run_pending()
    time.sleep(sam_time/4)

