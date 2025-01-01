# Serial Monitor & Serial Test Utinitis (half duplex)

This is a data monitor for half duplex serial comminication running on respi, linux and Windows.

## Build

- Linux enviroment

~~~
 # make -f Makefile.linux
~~~

- Rasbery Pi
~~~
 # make -f Makefile.raspi
~~~

- WSL + MXE
~~~
 # make -f Makefile.cross-wsl
~~~

When Permission Error; execution of access append command.

~~~
 # sudo chmod a+rw /dev/ttyUSB0
~~~


# Use Case

'smon -h' is help option of command functions.<br>
see:
~~~
smon.exe [Options] [Device File]:
smon.exe [Options]:
  --comlist                 print com port list
  --pipelist                print pipe name list
  -g [ --gap ] arg          time out tick. Default   30 ( 30 [ms])
  -t [ --timer ] arg        time out tick. Default  300 (300 [ms])
  --timer2 arg              time out tick. Default  500 (500 [ms])
  --timer3 arg              time out tick. Default 1000 (  1 [s])
  -c [ --crc ] arg          calclate modbus RTU CRC
  --crc8 arg                calclate CRC8
  -s [ --sum ] arg          calclate checksum of XOR
  -F [ --FLOAT ] arg        hex to float value
  -f [ --float ] arg        litle endian hex to float value
  -1 [ --oneline ]          1 line command
  -2 [ --bin-edit ]         binary command editor
  -3 [ --makeQR ] arg       make QR code of svg
  --read-bin-to-xlsx        read binary to xlsx file
  -m [ --mruby-script ] arg execute mruby script
  -v [ --version ]          print version
  -h [ --help ]             help
  --help-misc               display of exsample and commet, ext class ...etc
~~~

## Serial Monitor

Multi Monitor of Serial Port

ex)
~~~
smon.exe COM10,1200,odd,one COM12,19200,none,Two COM14,9600,Odd,One,Gap=20,TO1=100,TO2=200,TO3=500
or
smon /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyUSB3
~~~

~~~
smon /dev/pts/9 /dev/pts/11                     │ smon /dev/pts/10 /dev/pts/12 
Date: 2025/01/01 17:39:41.513                   │ Date: 2025/01/01 17:40:15.218
0:/dev/pts/9 :         30[ms]: GAP              │ 1:/dev/pts/12:         30[ms]: GAP
1:/dev/pts/11:         30[ms]: GAP              │ 0:/dev/pts/10:         30[ms]: GAP
0:/dev/pts/9 :        300[ms]: TO1              │ 1:/dev/pts/12:        300[ms]: TO1
1:/dev/pts/11:        300[ms]: TO1              │ 0:/dev/pts/10:        300[ms]: TO1
0:/dev/pts/9 :        500[ms]: TO2              │ 1:/dev/pts/12:        499[ms]: TO2
1:/dev/pts/11:        500[ms]: TO2              │ 0:/dev/pts/10:        499[ms]: TO2
1:/dev/pts/11:       1000[ms]: TO3              │ 0:/dev/pts/10:        999[ms]: TO3
0:/dev/pts/9 :       1000[ms]: TO3              │ 1:/dev/pts/12:        999[ms]: TO3
11223344Date: 2025/01/01 17:40:31.563           │ 0:/dev/pts/10:      16860[ms]: 11223344
                                                │ 0:/dev/pts/10:      17130[ms]: TO1
0:/dev/pts/9 :      50580[ms]: Send: 11223344   │ 0:/dev/pts/10:      17330[ms]: TO2
0:/dev/pts/9 :      50610[ms]: GAP              │ 0:/dev/pts/10:      17830[ms]: TO3
0:/dev/pts/9 :      50880[ms]: TO1              │ 1:/dev/pts/12:      39898[ms]: 74657374313233
0:/dev/pts/9 :      51080[ms]: TO2              │ 1:/dev/pts/12:      40168[ms]: TO1
0:/dev/pts/9 :      51580[ms]: TO3              │ 1:/dev/pts/12:      40368[ms]: TO2
1:tx:test123                                    │ 1:/dev/pts/12:      40868[ms]: TO3
1:/dev/pts/11:      73645[ms]: Send: tx:test123 │ Date: 2025/01/01 17:41:05.269
1:/dev/pts/11:      73675[ms]: GAP              │ 0:123456
1:/dev/pts/11:      73945[ms]: TO1              │ 0:/dev/pts/10:      55523[ms]: Send: 123456
1:/dev/pts/11:      74145[ms]: TO2              │ 0:/dev/pts/10:      55553[ms]: GAP
1:/dev/pts/11:      74645[ms]: TO3              │ 0:/dev/pts/10:      55823[ms]: TO1
0:/dev/pts/9 :      89221[ms]: 123456           │ 0:/dev/pts/10:      56023[ms]: TO2
0:/dev/pts/9 :      89491[ms]: TO1              │ 0:/dev/pts/10:      56523[ms]: TO3
0:/dev/pts/9 :      89691[ms]: TO2              │ 11 22 33 445566 09
0:/dev/pts/9 :      90191[ms]: TO3              │ 0:/dev/pts/10:      76075[ms]: Send: 11 22 33 445566
Date: 2025/01/01 17:41:21.612                   │  09
0:/dev/pts/9 :     109736[ms]: 11223344556609   │ 0:/dev/pts/10:      76105[ms]: GAP
0:/dev/pts/9 :     110006[ms]: TO1              │ 0:/dev/pts/10:      76375[ms]: TO1
0:/dev/pts/9 :     110206[ms]: TO2              │ 0:/dev/pts/10:      76575[ms]: TO2
0:/dev/pts/9 :     110706[ms]: TO3              │ 0:/dev/pts/10:      77075[ms]: TO3
quit                                            │ quit
~~~

- Stop Bit Options

"one"<br>
"two"<br>

- Parity Options

"none"<br>
"even"<br>
"odd"<br>

- Timer Options

"GAP=[0-9]+"<br>
"TO1=[0-9]+"<br>
"TO2=[0-9]+"<br>
"TO3=[0-9]+"<br>

- RTS Control Option

"rts=true/false"<br>

- boulad Speed

"[0-9]+" <br>


## Serial Send Command

~~~
smon -1 COM10 11223344 tx:test-string 123456
~~~

~~~
~~~

## Data Convert


## Binary Edit Mode

- execut command

~~~
smon
or
smon -2
~~~

- command list

~~~
  quit
  echo
  sum
  xsum
  crc8
  crc16
  crc32
  dump
  offset
  len
  clear
  save
  load
  # xxx
~~~

ex)
~~~
smon
01 04 03E8 001A F1B1
crc16
0000: CRC16
dump
dump: 010403E8001AF1B1
clear
dump
dump: 
len
length: 0

01 04 03E8 001A
dump
dump: 010403E8001A
crc16 make
dump
dump: 010403E8001AF1B1
clear

010403E8001AF1B1
dump
dump: 010403E8001AF1B1
dump 16
00000000: 010403E8001AF1B1
offset 1000
dump 16 sum
00001000: 010403E8001AF1B1                 54
~~~

# mruby exection mode

- Core
- BinEdit
- CppRegexp
- WorkerThread
- Smon
- OpenXLSX

## Core

~~~
Core.tick
Core.date
Core.gets
Core.exists
Core.timestamp
Core.makeQR
arg = Core.new
arg.length
arg[]
arg.prog

Core.opt_send
Core.bin_cmd_editor
Core.checkOptions
~~~

## BinEdit

~~~
bin = BinEdit.new
bin.length
bin.dump
bin.write
bin.memset
bin.memcpy
bin.memcmp
bin.get
bin.set
bin.pos
bin.crc32
bin.crc16
bin.crc8
bin.sum
bin.xsum
bin.compress
bin.uncompress
bin.save

BinEdit.hexToArray
BinEdit.hexFromArray
~~~

Now testing.
~~~
BinEdit.readBinToXlsx
~~~

## CppRegexp

~~~
CppRegexp.reg_match
CppRegexp.reg_replace
CppRegexp.reg_split
reg = CppRegexp.new
reg.length
reg.match
reg.grep
reg.replace
reg.select
reg.split
~~~

Now testting.

~~~
CppRegexp.compile
CppRegexp.===
CppRegexp.===
CppRegexp.= 
~~~

## WorkerThread

~~~
WorkerThread.ms_sleep
th = WorkerThread.new
th.state
th.run
th.wait
th.notify
th.synchronize
th.join
th.stop
WorkerThread.STOP
WorkerThread.WAKEUP
WorkerThread.RUN
WorkerThread.WAIT_JOIN
~~~

## Smon

~~~
Smon.comlist
Smon.pipelist
mon = Smon.new
mon.send
mon.read
mon.read_wait
mon.close

Smon.GAP
Smon.TO1
Smon.TO2
Smon.TO3
Smon.CLOSE
Smon.CACHE_FULL
Smon.NONE
~~~

Now testting.

~~~
CppRegexp.send_wait
CppRegexp.wait_send
~~~

## OpenXLSX

Now testting.

~~~
xlsx = OpenXLSX.new
xlsx.create
xlsx.open
xlsx.sheet
xlsx.sheet_names
xlsx.setSheetName
xlsx.set_value
xlsx.cell
xlsx.save
~~~

## SvgSequence

Now testting.

~~~
log = SvgSequence.new
log.setTitle
log.setPage
log.length
log.add_actor
log.act_life
log.log_data
log.inteval
log.hedder
log.prn_data
log.fotter
log.body
~~~
