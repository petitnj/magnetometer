#! /bin/bash
PATH=$PATH\:/home/pi/perl5/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export PATH
cd /home/pi/code/gmag

sudo ./GBOMag_Test1 -i ./gbo.ini >> ./GBOMag.log 


