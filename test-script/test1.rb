# test script for smon on mruby

def test(proc)
  proc.call("hello")
end
def mkProc()
  msg="Message"
  p = Proc.new do |str|
    printf( "  %-10s check %s: %s, %s\n", 'Proc:', ('hello'==str ? 'ok' : 'ng'), msg, str);
  end
  return p
end
def test_base()
  print "test mruby\n"
  str = ""; IO.popen("ls test1.rb") { |pipe| pipe.each { |s| str=s.chop() } }
  printf("  %-10s check %s: %s\n", 'IO.popen', ('test1.rb'==str ? 'ok' : 'ng'), str)
  test(mkProc())
#  printf("  %-10s check %s: %s\n", 'String =~', (str=~/test/ ? 'ok' : 'ng'), str)
  print "test mruby end\n"
end

def test_options()
  print "test Args\n"
  opt = Args.new()
  printf("  program: %s\n", opt.prog)
  printf("  --mruby-script: %s\n", opt['mruby-script'])
  printf("  opt.size: %d\n", opt.size())
  IO.popen(sprintf("%s -v", opt.prog)) { |pipe| pipe.each { |s| print "  > ", s } }
  cmd = sprintf("%s -m %s arg1 arg2", opt.prog, opt['mruby-script'])
  print "  ", cmd, "\n"
  IO.popen(cmd) { |pipe| pipe.each { |s| print "  > ", s } }
  cmd=sprintf("%s -f 0000E040", opt.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('7.000000: 0000E040'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s -F 40E00000", opt.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('7.000000: 40E00000'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --crc  010203040506070809", opt.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('0EB2: 010203040506070809'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --crc8 010203040506070809", opt.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('98: 010203040506070809'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --sum  010203040506070809", opt.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('01: 010203040506070809'==result ? 'ok' : 'ng'), result)
  print "test Args end\n"
end

def test_core()
  print "Core test\n"
  printf("  tick: %d\n", Core.tick(1))
  data = '010203040506070809'
  printf("  file exists: %s\n", (Core.exists('test1.rb') ? 'ok' : 'ng'))
  printf("  file exists: %s\n", (Core.exists('test1.rb', 'test2.rb', 'test3.rb') ? 'ok' : 'ng'))
  printf("  file exists: %s\n", (Core.exists([ 'test1.rb', 'test2.rb', 'test3.rb' ]) ? 'ok' : 'ng'))
  printf("  file exists: %s\n", (!Core.exists('test1.rb', 'test2', 'test3.rb') ? 'ok' : 'ng'))
  printf("  file timestamp: %s", Core.timestamp('test1.rb'))
  print  "  file timestamp: ", Core.timestamp('test1.rb', 'test2.rb', 'test3.rb'), "\n"
  print  "  file timestamp: ", Core.timestamp( [ 'test1.rb', 'test2.rb', 'test3.rb' ] ), "\n"
  printf("  tick: %d\n", Core.tick(0))
  #printf("  date UTC           : %s\n", Core.date('UTC'))
  #printf("  date Asia/Singapore: %s\n", Core.date('Asia/Singapore'))
  printf("  date Asia/Tokyo    : %s\n", Core.date('Asia/Tokyo'))
  printf("  date: %s\n", Core.date())
  print Core.makeQR('Core.test')
  print "Core test end\n"
end

def test_bin_edit
  print "BinEdit test\n"

  bin = BinEdit.new('01 02-0304@#_05')
  printf("  %s: length check %s, dump check %s\n", bin.dump, (5 == bin.length ? 'OK' : 'NG'), ('0102030405' == bin.dump ? "OK" : "NG"))
  printf("  %s\n", bin.dump(3))
  printf("  %s\n", bin.dump(1, 2))
  printf("  %s\n", bin.dump(0,10))
  print "  ", BinEdit.hexToArray('bb sS wW iI dD fF h2 A3', '0102 FEFF FFFE 0100 0100 FCFFFFFF FFFFFFFC 01000000 00000001 12345678 12345678 5566 303132'), "\n"

  bin = BinEdit.new(3)
  printf("  wsize: %s\n", bin.write('11 22'))
  printf("  wsize: %s\n", bin.write(2, '03'))
  printf("  %d: %s\n", bin.length, bin.dump)

  bin = BinEdit.new('010203040506070809112233445566778899AABBCCDDEEFF010203040506070809')
  printf("  compress: \n")
  printf("  %d: %s\n", bin.length, bin.dump)
  bin.compress();   printf("  %d: %s\n", bin.length, bin.dump)
  bin.uncompress(); printf("  %d: %s\n", bin.length, bin.dump)

  bin = BinEdit.new('010203040506070809FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF010203040506070809')
  printf("  compress: \n")
  printf("  %d: %s\n", bin.length, bin.dump)
  bin.compress();   printf("  %d: %s\n", bin.length, bin.dump)
  bin.uncompress(); printf("  %d: %s\n", bin.length, bin.dump)

  begin
    bin = BinEdit.new('file:test1.rb')
    printf("  length: %d\n", bin.length)
    print "  ", bin.dump(0, 16), "\n"
    bin.compress()
    bin.save( 'test1.rb.compress' )
    bin2 = BinEdit.new( 'file:test1.rb.compress' )
    print "  compress check ", ( (0 == bin.memcmp(bin2)) ? "OK" : "NG"), "\n"
  end

  bin = BinEdit.new('00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ')
  print "  ", bin.dump(), "\n"
  data = [ 0x11223344, 0.1, 0xaabb, 0x99, -10, -3, -2, 0.1, '01234', 'aabbccdd' ];
  print "  ", data, "\n"
  bin.set(0, 'dfwbcsiFAH',  data)
  print "  ", bin.dump(), "\n"
  print "  ", bin.get(0, 'dfwbcsiFA5H4'), "\n"
  begin
    bin2 = BinEdit.new(bin)
    print "  ", bin2.dump(), "\n"
    bin3 = BinEdit.new(bin, 10)
    print "  ", bin3.dump(), "\n"
    bin4 = BinEdit.new(bin, 1, 3)
    print "  ", bin4.dump(), "\n"
  end

  bin = BinEdit.new(bin.length, 0x00)
  print "  ", bin.dump(), "\n"
  data = [ 0x11223344, 0.1, 0xaabb, 0x99, -10, -3, -2, 0.1, '01234', 'AABBCCDD' ];
  print "  ", data, "\n"
  bin.set(0, 'dfwbcsiFah',  data)
  print "  ", bin.dump(), "\n"
  print "  ", bin.get(0, 'dfwbcsiFa5h4'), "\n"

  bin = BinEdit.new('00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ')
  bin.set('DFWbcSIFA',  [ 0x11223344, 1.0, 0xaabb, 0x99, -10, -3, -2, 0.1, '987' ])
  print "  ", bin.dump(), "\n"
  print "  ", bin.get(0, 'DFWbcSIFA3'), "\n"

  bin = BinEdit.new(25);
  bin.memset(0xff)
  print "  ", bin.dump(), "\n"
  bin.memset(0xAA, 10)
  print "  ", bin.dump(), "\n"
  bin.memset(5, 0x55, 3)
  print "  ", bin.dump(), "\n"
  bin.memset(0x00)
  print "  ", bin.dump(), "\n"
  list = Array.new
  (3).times do |idx|
    list.push( [ idx, idx * 3, sprintf("%02x", idx + 0x10) ] )
  end
  list.each do |item|
    bin.set("bwA2", item)
  end
  print "  ", bin.dump(), "\n"
  bin.get(0, '')
  (3).times do |idx|
    print "  ", bin.get("bwA2"), "\n"
  end

  begin
    bin1 = BinEdit.new(32)
    bin1.memset(0);
    bin2 = BinEdit.new('AABBCCDD')
    print "  ", bin1.dump(), "\n"
    print "  ", bin2.dump(), "\n"
    bin1.memcpy(bin2)
    print "  ", bin1.dump(), "\n"
    bin1.memcpy(16, bin2)
    print "  ", bin1.dump(), "\n"
    bin1.memcpy(8, 2, bin2)
    print "  ", bin1.dump(), "\n"
    bin1.memcpy(24, 2, 2, bin2)
    print "  ", bin1.dump(), "\n"
  end

  begin
    bin1 = BinEdit.new('01020304')
    bin2 = BinEdit.new(bin1)
    bin3 = BinEdit.new(bin1, 3)
    bin4 = BinEdit.new(bin1, 1, 2)
    print "  bin1: ", bin1.dump(), "\n"
    print "  bin2: ", bin2.dump(), "\n"
    print "  bin3: ", bin3.dump(), "\n"
    print "  bin4: ", bin4.dump(), "\n"
  end

  print "  ", (BinEdit.new("00000001")).get('D'), "\n"
  print "  ", (BinEdit.new("000000010200")).get('Dw'), "\n"

  bin = BinEdit.new('00000000')
  num = 7.0
  bin.set('F', [num])
  printf("  FLOAT: %f: %s\n", num, bin.dump)
  printf("  float: %f: %s\n", num, BinEdit.hexFromArray('f', [7.0]))

  str = '0123456789012345'
  bin = BinEdit.new(str.length)
  bin.set('a', [str])
  print '  hex: ', bin.dump(), "\n"
  print '  tx:  ', bin.get(sprintf("a%d", bin.length())), "\n"

  str = 'tx:0123456'
  bin = BinEdit.new('tx:0123456')
  printf("  %s -> %s\n", str, bin.dump())

  bin = BinEdit.new('010203040506070809')
  printf("  modbus crc: %s -> %-6s : check %s\n",  bin.dump(), bin.crc16(), ('0EB2' == bin.crc16() ? 'OK' : 'NG'))
  printf("  crc8 crc  : %s -> %-6s : check %s\n",  bin.dump(), bin.crc8(),  ('98'   == bin.crc8()  ? 'OK' : 'NG'))
  printf("  checksum  : %s -> %-6s : check %s\n",  bin.dump(), bin.sum(),   ('01'   == bin.sum()   ? 'OK' : 'NG'))

  GC.start()
  print "BinEdit test end\n"
end

def test_cpp_regexp
  print "CppRegexp test\n"
  reg = CppRegexp.new('[abc]')
  print '  test: ',    reg.match('test')    ? "ok" : "ng" , "\n"
  print '  abcdecg: ', reg.match('abcdecg') ? "ok" : "ng" , "\n"
  reg = CppRegexp.new( [ '[abc]', '[def]', '[xyz]' ] )
  printf("  reg.length(): %d\n", reg.length())
  printf("  select:a -> %d: check %s\n", reg.select('a'), (0==reg.select('a') ? 'ok' : 'ng'))
  printf("  select:e -> %d: check %s\n", reg.select('e'), (1==reg.select('e') ? 'ok' : 'ng'))
  reg = CppRegexp.new( '[abc]', '[def]', '[xyz]' )
  printf("  select:z -> %d: check %s\n", reg.select('z'), (2==reg.select('z') ? 'ok' : 'ng'))
  printf("  select:  -> %d: check %s\n", reg.select(' '), (3==reg.select(' ') ? 'ok' : 'ng'))
  reg = CppRegexp.new('1', '[bc]')
  print "  grep:  ", reg.grep(  [ '10:a', '11:b', '00:ig', '12:c', '13:d' ] ), "\n"
  print "  match: ", reg.match( [ '10:a', '11:b', '00:ig', '12:c', '13:d' ] ), "\n"
  reg = CppRegexp.new('1', 'x')
  print "  grep: ", reg.grep('0', 's1as2bs3cs4ds5es6fs7xs8xs9zs10 s11', 's1as2bs3cs4ds5es6fs7    s11'), "\n"
  reg = CppRegexp.new(' ', 'x')
  print "  replace: ", reg.replace('--', 's1as2bs3cs4ds5es6fs7xs8xs9zs10 s11', 's1as2bs3cs4ds5es6fs7    s11'), "\n"

  reg = CppRegexp.new(':', ',')
  msg = '123,456,789:abc,dfg,hij'
  print "  split: ", reg.split(msg), "\n"

  printf("  reg_match: %s\n",   (CppRegexp.reg_match('123:456:789', ':') ? "OK" : "NG"))
  printf("  reg_replace: %s\n", CppRegexp.reg_replace('123:456:789', ':', ' '))
  print  "  reg_split: ", CppRegexp.reg_split('123:456:789', ':'), "\n"
  print "CppRegexp test end\n"
end

def test_thead
  print "thread test\n"
  list = Array.new
  th = WorkerThread.new
  th1 = WorkerThread.new
  th2 = WorkerThread.new
  th1.run do
    print "  run thread 1\n"
    th1.wait()
    th1.synchronize do
      th.synchronize do
        print "  list.pop: ", list.pop(), "\n"
      end
    end
    th1.stop()
    print "  run thread 1 end\n"
  end
  th2.run do
    print "  run thread\n"
    th2.wait()
    th2.synchronize do
      th.synchronize do
        print "  list.pop: ", list.pop(), "\n"
      end
    end
    th2.stop()
    print "  run thread end\n"
  end
  print "  notify \n"
  th2.notify do
    print "  push(2) + notiry\n"
    list.push(2)
  end
  th1.notify do
    print "  push(1) + notiry\n"
    list.push(1)
  end
  print "  thread join\n"
  th1.join
  th2.join
  print "thead test end\n"
end

opt = Args.new()
if 0 < opt.size() then
  (opt.size()).times { |idx| printf("  arg: %s\n", opt[idx]); }
else
  print "mruby test script 1\n"
  test_base(); print "\n"
  test_options(); print "\n"
  test_core(); print "\n"
  test_bin_edit(); print "\n"
  test_cpp_regexp(); print "\n"
  test_thead(); print "\n"
  print "mruby test script 1 end\n"
  print "\n"
end
