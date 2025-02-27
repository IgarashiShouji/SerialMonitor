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

This is extended class on mruby for smon program.

| Extened class | <center>Overview</center>       |
|:--------------|:--------------------------------|
| Core          | System methods.                 |
| BinEdit       | Binary editor control           |
| CppRegexp     | Regexp contorl                  |
| WorkerThread  | Worker thread                   |
| Smon          | Serial monitor control          |
| OpenXLSX      | spread sheet read/write control |

## Core Class

### Core.gets

Get standard input. You can listen for Smon and standard input.
~~~
ex1)
str = Core.gets()
ex2)
Smon.open(Array.new(list_port)) do |smon|
  bin = BinEdit.new()
  str = Core.gets(smon, bin) do |idx, state|
    case state
    when Smon::GAP then
      ...
    else
      ...
    end
  end
  print str, "\n"
  ...
end
~~~

### Core.tick

Get the execution time from the previous execution point.
~~~
ex)
Core.tick
...
print Core.tick

~~~

### Core.date

Get the date and time.
~~~
ex)
print Core.date
~~~

### Core.exists

Check for the existence of a file.
~~~
ex)
if Core.exists('smon.exe') then
~~~

### Core.timestamp

Get the file timestamp.
~~~
ex)
print Core.timestamp('smon.exe')
~~~

### Core.makeQR

Generate a QR code in SVG.
~~~
ex)
print Core.makeQR('test')
~~~

### Core.args

Gets the list of startup arguments.
~~~
ex)
arg = Core.args
print arg[0]
~~~

### Core.opts

Get a list of startup options.
~~~
ex)
opts = Core.opts
print opts['gap']
~~~

### Core.prog

Get the launch program path.
~~~
ex)
print Core.prog
 => ./smon
~~~

### Core.opt_send

serial data send of one line command
~~~
ex)
 # smon -1 com10 010203 112233
~~~

### Core.bin_cmd_editor

binary edit mode
~~~
ex)
 # smon
~~~

### Core.checkOptions
Runs at startup, checks startup options, and performs functions according to the options.


## BinEdit

### BinEdit.new, bin.save

create a Binary Editor Resource.
And save to file.
~~~
bin = BinEdit.new
bin = BinEdit.new(num)
bin = BinEdit.new('010203')
bin = BinEdit.new(bin)              # copy from bin
bin = BinEdit.new(['112233', bin])  # copy from '112233' and bin
bin = BinEdit.new(bin, 2)           # 2 byte copy from bin
bin = BinEdit.new(16, 0xff)         # 16 byte and fill data 0xff
bin = BinEdit.new(bin, 2, 3)        # copy from bin[2..5]

bin = BinEdit.new('file:test.bin')  # load binary file('test.bin')
bin.save('test.bin')                # binary save to file('test.bin')
~~~

### bin.length

get binary length.
~~~
bin = BinEdit.new('010203')
print bin.length
~~~

### bin.resize

change binary length.
~~~
bin = BinEdit.new('010203')
bin.resize(10)
~~~

### bin.dump

get binary hex data string.
~~~
bin = BinEdit.new('010203040506070809')
print bin.dump          => 010203040506070809
print bin.dump(2)       => 0102
print bin.dump(2, 3)    => 030405
~~~

### bin.write

write binary data.
~~~
bin.write('112233')
bin.write(2, '112233')      # write from address 2
bin.write('112233', 3)      # write size 3
bin.write(2, '112233', 3)   # write from address 2 and size 3
~~~

### bin.memset

set data to memory.
~~~
    bin.memset(0xff)          # set data(0xff)
    bin.memset(0xAA, 10)      # set data(0xAA) and size(10)
    bin.memset(5, 0x55, 3)    # set data(0x55) and size(3) from address(5)
~~~

### bin.memcpy

data copy from binary data.
~~~
bin1 = BinEdit.new    # binary data 1
bin2 = BinEdit.new    # binary data 2
bin1.memcpy(bin2)             # copy binary data(bin2) to binary data(bin1).
bin1.memcpy(16, bin2)         # copy binary data(bin2) to binary data(bin1) of address(16).
bin1.memcpy(bin2, 10)         # copy binary data(bin2) of copy size(10) to binary data(bin1).
bin1.memcpy(16, 2, bin2)      # copy binary data(bin2) of address(2) to binary data(bin1) of address(16).
bin1.memcpy(16, bin2, 10)     # copy binary data(bin2) of of copy size(10) to binary data(bin1) of address(16).
bin1.memcpy(16, 2, bin2, 10)  # copy binary data(bin2) of address(2) of copy size(10) to binary data(bin1) of address(16).
~~~

### bin.memcmp

data compere of binary data.
~~~
bin1 = BinEdit.new    # binary data 1
bin2 = BinEdit.new    # binary data 2
bin1.memcmp(bin2)             # compare to binary data(bin2) to binary data(bin1).
bin1.memcmp(16, bin2)         # compare to binary data(bin2) to binary data(bin1) of address(16).
bin1.memcmp(bin2, 10)         # compare to binary data(bin2) of copy size(10) to binary data(bin1).
bin1.memcmp(16, 2, bin2)      # compare to binary data(bin2) of address(2) to binary data(bin1) of address(16).
bin1.memcmp(16, bin2, 10)     # compare to binary data(bin2) of of copy size(10) to binary data(bin1) of address(16).
bin1.memcmp(16, 2, bin2, 10)  # compare to binary data(bin2) of address(2) of copy size(10) to binary data(bin1) of address(16).
~~~

### bin.set, bin.get, bin.pos

set data with data format.
And get data with data format.
pos is set or get start position.
~~~
bin.set(0, 'cbsSwWiIdDfFaAhH', [85, 85, -2, -2, 258, 258, -4, -4, 0x01020304, 0x01020304, 7.0, 7.0, '01234', '01234', 'aa55', 'aa55'])
bin.pos(0)
arry = bin.get(0, 'cbsSwWiIdDfFa5A5h2H2')

Format:
c: signed char
b: unsigned char
s: signed short for litle endian
S: signed short for big endian
w: unsigned short for litle endian
W: unsigned short for big endian
i: signed long for litle endian
I: signed long for big endian
d: unsigned long for litle endian
D: unsigned long for big endian
f: float for litle endian
F: float for big endian
a: ascii data for litle endian
A: ascii data for big endian
h: hex string for litle endian
H: hex string for big endian
~~~

### bin.crc32, bin.crc16, bin.crc8, bin.sum, bin.xsum

Below is the checksum calculator.

CRC32, CRC16(modbus), CRC8, Check Sum, XOR sum

|   Function    |                   <center>Overview</center>                                       |
|:--------------|:----------------------------------------------------------------------------------|
| CRC32         | x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1    |
| CRC16(modbus) | CRC16 for MODBUS-RTU communication.                                               |
| CRC8          | x8 + x7 + x6 + x4 + x2 + 1                                                        |
| Check Sum     | byte sum (for srec, ihex)                                                         |
| XOR sum       | modem communication 1 byte sum. for XMODEM (Checksum mode), HART etc.             |

~~~
bin = BinEdit.new('010203040506070809')
print bin.crc32
print bin.crc16
print bin.crc8
print bin.sum
print bin.xsum
~~~

### bin.compress, bin.uncompress

data compress and uncompress.
complress is lz4 algolizm.
~~~
bin = BinEdit.new('010203040506070809FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF010203040506070809')
bin.compress();
bin.uncompress();
~~~

### BinEdit.toArray, BinEdit.toHexString

data convert functions.

| Function            | Overview                              |
|:--------------------|:--------------------------------------|
| BinEdit.toArray     | convert of hex string to value array. |
| BinEdit.toHexString | convert of value array to hex string. |

~~~
ex)
print BinEdit.toArray('0102 FEFF FFFE 0100 0100 FCFFFFFF FFFFFFFC 01000000 00000001 00002041 41200000 5566 303132', 'bb sS wW iI dD fF h2 A3')
print BinEdit.toHexString([1, 2, -2, -2, 1, 256, -4, -4, 1, 1, 10.0, 10.0, "5566", "210"], 'bbsSwWiIdDfFh2A3')
~~~

### Now testing.

~~~
BinEdit.readBinToXlsx
~~~

## CppRegexp

This class is regexp utilities.
Regexp is c++ algorithm.

### CppRegexp.new

~~~
CppRegexp.new('reg')
CppRegexp.new(['reg1', 'reg2'])
CppRegexp.new('reg1, 'reg2')
~~~

### reg.length

Return value is regexp array count.

~~~
reg = CppRegexp.new('reg')
print reg.length
  ==> 1
reg = CppRegexp.new(['reg1', 'reg2'])
print reg.length
  ==> 2
reg = CppRegexp.new('reg1, 'reg2')
print reg.length
  ==> 2
~~~

### reg.match

This is search of string list of regex array match(matches any one).

~~~
reg = CppRegexp.new('reg')
print reg.match('reg')
  ==> true
print reg.match('test')
  ==> false

reg = CppRegexp.new('reg1', 'reg2')
or
reg = CppRegexp.new(['reg1', 'reg2'])
print reg.match(['test', 'test1 reg1', 'test2 reg1 reg2', 'test3 reg2', 'test4 reg'])
  ==> ['test1 reg1', 'test2 reg1 reg2', 'test3 reg2']
print reg.match('test', 'test1 reg1', 'test2 reg1 reg2', 'test3 reg2', 'test4 reg')
  ==> ['test1 reg1', 'test2 reg1 reg2', 'test3 reg2']
print reg.match('test', ['test1 reg1', 'test2 reg1 reg2'] , ['test3 reg2', 'test4 reg'])
  ==> ['test1 reg1', 'test2 reg1 reg2', 'test3 reg2']
~~~

### reg.grep

This is search of string list of regex array match(matches all).

~~~
reg = CppRegexp.new('reg')
print reg.grep('reg')
  ==> ['reg']
print reg.grep('test')
  ==> []

reg = CppRegexp.new('reg1', 'reg2')
or
reg = CppRegexp.new(['reg1', 'reg2'])
print reg.grep(['test', 'test1 reg1', 'test2 reg1 reg2', 'test3 reg2', 'test4 reg'])
  ==> ['test2 reg1 reg2']
print reg.grep('test', 'test1 reg', 'test2 reg1 reg2', 'test3 reg2', 'test4 reg')
  ==> ['test2 reg1 reg2']
print reg.grep('test', ['test1 reg', 'test2 reg1 reg2'] , ['test3 reg2', 'test4 reg'])
  ==> ['test2 reg1 reg2']
~~~

### reg.replace

This is replace string list to arg[0] by regex array match(mathes any one).

~~~
reg = CppRegexp.new('reg1', 'reg2')
or
reg = CppRegexp.new(['reg1', 'reg2'])

print reg.replace('reg', ['test', 'test1 reg1', 'test2 reg1 reg2', 'test3 reg2', 'test4 reg'])
  ==> ['test', 'test1 reg', 'test2 reg reg', 'test3 reg', 'test4 reg']
print reg.replace('reg', 'test', ['test1 reg1', 'test2 reg1 reg2'], ['test3 reg2', 'test4 reg'])
  ==> ['test', 'test1 reg', 'test2 reg reg', 'test3 reg', 'test4 reg']
~~~

### reg.split

get string split by regex arry.

~~~
reg = CppRegexp.new([',', ':'])

print reg.select('123,456:789')
  ==> ['123', '456', '789']
~~~

### reg.select

get matched index by regex.

~~~
reg = CppRegexp.new('reg1', 'reg2')
or
reg = CppRegexp.new(['reg1', 'reg2'])

print reg.select('reg1')
  ==> 0
print reg.select('reg2')
  ==> 1
print reg.select('reg')
  ==> 2
print reg.select()
  ==> 2
~~~

### CppRegexp.reg_match

This is search of string list of regex array match(matches any one).

~~~
print CppRegexp.reg_match('test reg 123', ['A', 'B', '1'])              # -> true
print CppRegexp.reg_match('test reg 123', ['A', 'B'], '2')              # -> true
print CppRegexp.reg_match(['test', 'reg', '123'], ['A', 're', '2'])     # => ['reg', '123']
print CppRegexp.reg_match(['test', 'reg', '123'], ['A', 'e'], '3')      # => ['reg', '123']
~~~

### CppRegexp.reg_grep

This is search of string list of regex array match(matches all).

~~~
print CppRegexp.reg_match('test reg 123', ['A', 'B', '1'])              # -> true
print CppRegexp.reg_match('test reg 123', ['A', 'B'], '2')              # -> true
print CppRegexp.reg_match(['test', 'reg', '123'], ['A', 're', '2'])     # => ['reg', '123']
print CppRegexp.reg_match(['test', 'reg', '123'], ['A', 'e'], '3')      # => ['reg', '123']
~~~

### CppRegexp.reg_replace

This is replace string list to arg[0] by regex array match(mathes any one).

~~~
print CppRegexp.reg_replace('test', 'ex', 'A')                              # => tAt
                                    ^^^  ^^^
                                    |||  +++-  Replace String
                                    +++------- Regex list

print CppRegexp.reg_replace(['test', 'reg', '123'], 'A', ['e', 's'])        # => [ 'tAAt', 'rAg', '123' ]
print CppRegexp.reg_replace(['test', 'reg', '123'], '-', ['e', 's'], '1')   # => [ 't--t', 'r-g', '-23' ]
print CppRegexp.reg_replace(['test', 'reg', '123'], '-', 'A')               # => [ 'test', 'reg', '123' ]
                                                    ^^^  ^^^
                                                    |||  +++-  Regex list
                                                    +++------- Replace String
~~~

### CppRegexp.reg_split

get string split by regex arry.

~~~
print CppRegexp.reg_split('123,456:789', ',', ':')                      # => [ '123', '456', '789' ]
print CppRegexp.reg_split('123,456:789', [',', ':'])                    # => [ '123', '456', '789' ]
~~~

### CppRegexp.reg_select

get matched index by regex.

~~~
print CppRegexp.reg_split('cmd1', 'cmd1', 'cmd2', 'cmd3')               # => 0
print CppRegexp.reg_split('cmd2', ['cmd1', 'cmd2', 'cmd3'])             # => 1
print CppRegexp.reg_split('cmd4', ['cmd1', 'cmd2'], 'cmd3')             # => 3
~~~

### Now testting.

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
