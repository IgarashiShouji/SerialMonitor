# Serial Monitor & Serial Test Utinitis (half duplex)

This is a data monitor for half duplex serial comminication running on respi, linux and msys2.

## command line

~~~
 # smon --help
~~~
## options

* --help
~~~
-b [ --baud ] arg     baud rate ex) -b B9600E1<br>
-g [ --gap ] arg      time out tick. Default 3   ( 30 [ms])
-t [ --timer ] arg    time out tick. Default 30  (300 [ms])
--timer2 arg          time out tick. Default 50  (500 [ms])
--timer3 arg          time out tick. Default 100 (  1 [s])
-c [ --crc ] arg      calclate modbus RTU CRC
-s [ --sum ] arg      calclate checksum of XOR
-f [ --float ] arg    hex to float value
-h [ --help ]         help
~~~

* -b [ --baud ] arg     baud rate ex) -b B9600E1
* -g [ --gap ] arg      time out tick. Default 3   ( 30 [ms])
* -t [ --timer ] arg    time out tick. Default 30  (300 [ms])
* --timer2 arg          time out tick. Default 50  (500 [ms])
* --timer3 arg          time out tick. Default 100 (  1 [s])
* -c [ --crc ] arg      calclate modbus RTU CRC<br>
* -s [ --sum ] arg      calclate checksum of XOR<br>
* -f [ --float ] arg    hex to float value<br>
* -h [ --help ]         help<br>

## Build

* linux
~~~
 # make -f Makefile.linux
~~~

* Mingw
~~~
 # make -f Makefile.mingw
~~~

* RasPi
~~~
 # make -f Makefile.raspi
~~~
