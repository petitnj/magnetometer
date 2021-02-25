from Adafruit_IO import Client, Feed
TIME_ZONE = 'America/Chicago'
OPENWEATHER_TOKEN ='0dac8d87dfe8997cfeb046d133416040'
ADAFRUIT_IO_USERNAME = 'petitnoel'
ADAFRUIT_IO_KEY = 'aio_wveE73iOoYGETZ0iTV6ID8wlsT2A'

# Create an instance of the REST client.
aio = Client(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)

# Set up Adafruit IO Feeds.
xmag_feed = aio.feeds('xmag')
ymag_feed = aio.feeds('ymag')
zmag_feed = aio.feeds('zmag')

def setup():
    global sam_time
    global tot_x
    global tot_y
    global tot_z
    global sensor
    global count
    tot_x = tot_y = tot_z = 0
    count = 0 # incase we write data immediately
    check_dir()
    sam_time = 10.0
        aio.send(xmag_feed.key,str(tot_x/count))
        aio.send(ymag_feed.key,str(tot_y/count))
        aio.send(zmag_feed.key,str(tot_z/count))

def sig_handler(sig,frame): #gracefully exit on contro-C
    print ('  exiting')
    try:
        fl.close()
    except:
        pass
    sys.exit()
      
signal.signal(signal.SIGINT,sig_handler) # wait 


