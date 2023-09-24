# Serial Monitor & Serial Test Utinitis (half duplex)

This is a data monitor for half duplex serial comminication running on respi, linux and msys2.

## command line

~~~
 # smon --help
~~~
## options

* --help
~~~
smon.exe [Device File] [Options]:
  -b [ --baud ] arg     baud rate ex) -b B9600E1
  -g [ --gap ] arg      time out tick. Default 3   ( 30 [ms])
  -t [ --timer ] arg    time out tick. Default 30  (300 [ms])
  --timer2 arg          time out tick. Default 50  (500 [ms])
  --timer3 arg          time out tick. Default 100 (  1 [s])
  --no-rts              no control RTS signal
  -c [ --crc ] arg      calclate modbus RTU CRC
  -s [ --sum ] arg      calclate checksum of XOR
  -f [ --float ] arg    hex to float value
  -F [ --floatl ] arg   litle endian hex to float value
  -h [ --help ]         help
~~~

* -b [ --baud ] arg     baud rate ex) -b B9600E1
* -g [ --gap ] arg      time out tick. Default 3   ( 30 [ms])
* -t [ --timer ] arg    time out tick. Default 30  (300 [ms])
* --timer2 arg          time out tick. Default 50  (500 [ms])
* --timer3 arg          time out tick. Default 100 (  1 [s])
* --no-rts              no control RTS signal
* -c [ --crc ] arg      calclate modbus RTU CRC<br>
* -s [ --sum ] arg      calclate checksum of XOR<br>
* -f [ --float ] arg    hex to float value<br>
* -F [ --floatl ] arg   litle endian hex to float value<br>
* -h [ --help ]         help<br>

## Build

### linux
~~~
 # make -f Makefile.linux
~~~

permission エラーになるため、以下のコマンドでアクセス権の追加が必要。
~~~
 # sudo chmod a+rw /dev/ttyUSB0
~~~

### Mingw

Step1 is build for boost on WSL. Step 2 is build for mruby and other sources.

- step 1
~~~
 # build-cross-wsl-boost.sh
~~~
- step 2
~~~
 # make -f Makefile.mingw
~~~

WSL environment cannot handle the line feed code of the mruby library correctly.<br>
boost on mingw cannot open of any USB Serial Driver.<br>
Therefore, build for boost is WSL, build for mruby is mingw, it so.

### RasPi
~~~
 # make -f Makefile.raspi
~~~
