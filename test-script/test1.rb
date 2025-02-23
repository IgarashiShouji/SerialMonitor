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
def test_core()
  print "Core & Option test\n"
  printf("  tick: %d\n", Core.tick(1))

  # mruby lang tenst
  test(mkProc())

  # smon Args test
  printf("  program: %s\n", Core.prog)
  opt = Core.opts()
  printf("  --mruby-script: %s\n", opt['mruby-script'])
  printf("  opt.length: %d\n", opt.length())

  IO.popen(sprintf("%s -v", Core.prog)) { |pipe| pipe.each { |s| print "  > ", s } }
  cmd = sprintf("%s -m %s arg1 arg2", Core.prog, opt['mruby-script'])
  print "  ", cmd, "\n"
  IO.popen(cmd) { |pipe| pipe.each { |s| print "  > ", s } }
  cmd=sprintf("%s -f 0000E040", Core.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('7.000000: 0000E040'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s -F 40E00000", Core.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('7.000000: 40E00000'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --crc    010203040506070809 ", Core.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('0EB2: 010203040506070809'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --crc8   010203040506070809 ", Core.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('98: 010203040506070809'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --sum    010203040506070809 ", Core.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('01: 010203040506070809'==result ? 'ok' : 'ng'), result)
  cmd=sprintf("%s --sum '~:010203040506070809'", Core.prog); result=''; IO.popen(cmd) { |pipe| pipe.each { |s| result=s.chop() } }
  printf("  %s -> check %s: %s\n", cmd, ('D3: 010203040506070809'==result ? 'ok' : 'ng'), result)

  io = IO.popen(sprintf("%s", Core.prog), "r+")
  io.puts "01020304"
  io.puts "dump"
  io.close_write
  result = io.gets; result.chop!()
  printf("  check %s: %s\n", ('dump: 01020304'==result ? 'ok' : 'ng'), result)

  io = IO.popen(sprintf("%s", Core.prog), "r+")
  io.puts "fmt:fF:7.0,7.0"
  io.puts "dump"
  io.close_write
  result = io.gets; result.chop!()
  printf("  check %s: %s\n", ('dump: 0000E04040E00000'==result ? 'ok' : 'ng'), result)

  io = IO.popen(sprintf("%s", Core.prog), "r+")
  io.puts "fmt:cbWD:1,2,3,4"
  io.puts "dump"
  io.close_write
  result = io.gets; result.chop!()
  printf("  check %s: %s\n", ('dump: 0102000300000004'==result ? 'ok' : 'ng'), result)

  io = IO.popen(sprintf("%s", Core.prog), "r+")
  io.puts "fmt:cbwd:1,2,3,4"
  io.puts "dump"
  io.close_write
  result = io.gets; result.chop!()
  printf("  check %s: %s\n", ('dump: 0102030004000000'==result ? 'ok' : 'ng'), result)

  # Core test
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

  print "Core & Option test end\n"
end

def test_bin_edit
  print "BinEdit test\n"

  begin
    bin = BinEdit.new('01 02-0304@#_05')
    printf("  %s: length check %s, dump check %s\n", bin.dump, (5 == bin.length ? 'OK' : 'NG'), ('0102030405' == bin.dump ? "OK" : "NG"))
    printf("  check %s: %s\n", ("010203" == bin.dump(3) ? "OK" : "NG"),  bin.dump(3))
    printf("  check %s: %s\n", ("0203" == bin.dump(1, 2) ? 'OK' : 'NG'), bin.dump(1, 2))
    printf("  check %s: %s\n", ("0102030405" == bin.dump(0,10) ? 'OK' : 'NG'), bin.dump(0,10))
    print "\n"
  end
  begin
    bin = BinEdit.new()
    printf("  bin.length check %s\n", (0 == bin.length ? 'OK' : 'NG'))
    data_set = [ 85, 85, -2, -2, 258, 258, -4, -4, 0x01020304, 0x01020304, 7.0, 7.0, '01234', '01234', 'aa55', 'aa55' ]
    print "  set: ", data_set, "\n"
    bin.set(0, 'cbsSwWiIdDfFaAhH',  data_set)
    printf("  dump check %s: dump: %s\n", ("5555FEFFFFFE02010102FCFFFFFFFFFFFFFC04030201010203040000E04040E0000030313233343433323130AA5555AA" == bin.dump() ? 'OK' : 'NG'), bin.dump())
    data = bin.get(0, 'cbsSwWiIdDfFa5A5h2H2')
    print "  get: ", data, "\n"
    check = 'OK'
    data_set.each_with_index do |d, idx|
      if d != data[idx] then
        check = 'NG'
        break;
      end
    end
    printf("  set -> get verify check %s\n\n", check)
  end

  begin
    print "  test BinEdit.toArray, BinEdit.toHexString\n"
    list = BinEdit.toArray('0102 FEFF FFFE 0100 0100 FCFFFFFF FFFFFFFC 01000000 00000001 00002041 41200000 5566 303132', 'bb sS wW iI dD fF h2 A3')
    check = 'OK'
    [1, 2, -2, -2, 1, 256, -4, -4, 1, 1, 10.0, 10.0, "5566", "210"].each_with_index do |data, idx|
      if data != list[idx] then
        check = 'NG'
        break;
      end
    end
    printf("  BinEdit.toArray check: %s: ", check)
    print list, "\n"
    hex = BinEdit.toHexString([1, 2, -2, -2, 1, 256, -4, -4, 1, 1, 10.0, 10.0, "5566", "210"], 'bbsSwWiIdDfFh2A3')
    printf("  BinEdit.toHexString check: %s: %s\n", ('0102FEFFFFFE01000100FCFFFFFFFFFFFFFC010000000000000100002041412000005566303132' == hex ? 'OK' : 'NG'), hex)
    print "\n"
  end

  begin
    print "  BinEdit: bin.set => bin.get\n"
    fmt = 'bwA2'
    bin = BinEdit.new(5*3, 0x00);
    (3).times do |idx|
      bin.set(fmt, [ idx, 1 + idx * 3, sprintf("%02x", idx + 0x10) ])
    end
    printf("  check %s: %s\n", ("000100303101040031310207003231" == bin.dump ? 'OK' : 'NG'), bin.dump)
    bin.pos(0)
    (3).times do |idx|
      print "  ", bin.get(fmt), "\n"
    end
    print "\n"
  end

  begin
    print "  BinEdit: bin.write\n"
    bin = BinEdit.new(3)
    printf("  check bin.length %s: %d: %s\n", (3 == bin.length ? 'OK' : 'NG' ), bin.length, bin.dump)
    bin.write('11 22')
    #printf("  %d: %s\n", bin.length, bin.dump)
    bin.write(2, '33')
    printf("  Write check %s: %d: %s\n", ("112233" == bin.dump ? 'OK' : 'NG'), bin.length, bin.dump)
    print "\n"
  end

  begin
    print "  BinEdit: test compress\n"
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
    print "\n"
  end

  begin
    print "  BinEdit: file load\n"
    bin = BinEdit.new('file:test1.rb')
    len = bin.length
    bin.compress()
    bin.save('test1.rb.compress')
    printf("  Original length: %d -> %d\n", len, bin.length)
    bin2 = BinEdit.new('file:test1.rb.compress')
    len2 = bin2.length
    bin2.uncompress()
    printf("  Compress file: %d -> %d\n", len2, bin2.length)
    printf("  Complress check %s\n", (len == bin2.length ? 'OK' : 'NG'))
    print "\n"
  end

  begin
    print "  BinEdit: copy constractor\n"
    bin  = BinEdit.new('010203040506070809112233445566778899AABBCCDDEEFF010203040506070809')
    bin2 = BinEdit.new(bin)
    printf("  bin2.dup check %s: %s\n", ('010203040506070809112233445566778899AABBCCDDEEFF010203040506070809' == bin2.dump() ? 'OK' : 'NG'), bin2.dump())
    bin3 = BinEdit.new(bin, 9)
    printf("  bin3.dup check %s: %s\n", ('010203040506070809' == bin3.dump() ? 'OK' : 'NG'), bin3.dump())
    bin4 = BinEdit.new(bin, 1, 3)
    printf("  bin4.dup check %s: %s\n", ('020304' == bin4.dump() ? 'OK' : 'NG'), bin4.dump())
    bin = BinEdit.new(bin.length, 0x00)
    printf("  bin. dup check %s: %s\n", ("000000000000000000000000000000000000000000000000000000000000000000" == bin.dump ? 'OK' : 'NG'), bin.dump)
    print "\n"
  end

  begin
    print "  BinEdit: memset\n"
    bin = BinEdit.new(25);
    bin.memset(0xff)
    printf("  memset check %s: %s\n", ("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" == bin.dump ? 'OK' : 'NG'), bin.dump)
    bin.memset(0xAA, 10)
    printf("  memset check %s: %s\n", ("AAAAAAAAAAAAAAAAAAAAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" == bin.dump ? 'OK' : 'NG'), bin.dump)
    bin.memset(5, 0x55, 3)
    printf("  memset check %s: %s\n", ("AAAAAAAAAA555555AAAAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" == bin.dump ? 'OK' : 'NG'), bin.dump)
    bin.memset(0x00)
    printf("  memset check %s: %s\n", ("00000000000000000000000000000000000000000000000000" == bin.dump ? 'OK' : 'NG'), bin.dump)
  end

  begin
    print "  BinEdit: memcpy\n"
    bin1 = BinEdit.new(32, 0)
    printf("  check %s: %s\n", ('0000000000000000000000000000000000000000000000000000000000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    bin2 = BinEdit.new('AABBCCDD')
    bin1.memcpy(bin2, 2)
    printf("  check %s: %s\n", ('AABB000000000000000000000000000000000000000000000000000000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    bin1.memcpy(bin2)
    printf("  check %s: %s\n", ('AABBCCDD00000000000000000000000000000000000000000000000000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    bin1.memcpy(16, bin2)
    printf("  check %s: %s\n", ('AABBCCDD000000000000000000000000AABBCCDD000000000000000000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    bin1.memcpy(5, bin2, 2)
    printf("  check %s: %s\n", ('AABBCCDD00AABB000000000000000000AABBCCDD000000000000000000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    bin1.memcpy(10, 1, bin2)
    printf("  check %s: %s\n", ('AABBCCDD00AABB000000BBCCDD000000AABBCCDD000000000000000000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    bin1.memcpy(25, 1, bin2, 2)
    printf("  check %s: %s\n", ('AABBCCDD00AABB000000BBCCDD000000AABBCCDD0000000000BBCC0000000000' == bin1.dump ? 'OK' : 'NG'), bin1.dump)
    print "\n"
  end

  begin
    print "  BinEdit: memcmp\n"
    bin1 = BinEdit.new('01020304')
    bin2 = BinEdit.new('01020304')
    bin3 = BinEdit.new('01020405')
    bin4 = BinEdit.new('01020200')
    printf("  check %s: %s\n", (0 == bin1.memcmp(bin2) ? 'OK' : 'NG'), bin1.memcmp(bin2))
    printf("  check %s: %s\n", (bin1.memcmp(bin3) < 0 ? 'OK' : 'NG'), bin1.memcmp(bin3))
    printf("  check %s: %s\n", (0 < bin1.memcmp(bin4) ? 'OK' : 'NG'), bin1.memcmp(bin4))
    bin1 = BinEdit.new('010203040102030401020304')
    printf("  check %s: %s\n", (0 == bin1.memcmp(4, bin2) ? 'OK' : 'NG'),     bin1.memcmp(4, bin2))
    printf("  check %s: %s\n", (0 != bin1.memcmp(3, bin2) ? 'OK' : 'NG'),     bin1.memcmp(3, bin2))
    printf("  check %s: %s\n", (     bin1.memcmp(4, bin3) < 0 ? 'OK' : 'NG'), bin1.memcmp(4, bin3))
    printf("  check %s: %s\n", (0 <  bin1.memcmp(4, bin4) ? 'OK' : 'NG'),     bin1.memcmp(4, bin4))

    printf("  check %s: %s\n", (0 == bin1.memcmp(5, 1, bin2) ? 'OK' : 'NG'),     bin1.memcmp(5, 1, bin2))
    printf("  check %s: %s\n", (0 != bin1.memcmp(4, 1, bin2) ? 'OK' : 'NG'),     bin1.memcmp(4, 1, bin2))
    printf("  check %s: %s\n", (     bin1.memcmp(5, 1, bin3) < 0 ? 'OK' : 'NG'), bin1.memcmp(5, 1, bin3))
    printf("  check %s: %s\n", (0 <  bin1.memcmp(5, 1, bin4) ? 'OK' : 'NG'),     bin1.memcmp(5, 1, bin4))

    printf("  check %s: %s\n", (0 == bin1.memcmp(5, 1, bin2, 2) ? 'OK' : 'NG'),     bin1.memcmp(5, 1, bin2, 2))
    printf("  check %s: %s\n", (0 != bin1.memcmp(4, 1, bin2, 2) ? 'OK' : 'NG'),     bin1.memcmp(4, 1, bin2, 2))
    printf("  check %s: %s\n", (     bin1.memcmp(5, 1, bin3, 2) < 0 ? 'OK' : 'NG'), bin1.memcmp(5, 1, bin3, 2))
    printf("  check %s: %s\n", (0 <  bin1.memcmp(5, 1, bin4, 2) ? 'OK' : 'NG'),     bin1.memcmp(5, 1, bin4, 2))
    print "\n"
  end

  begin
    print "  BinEdit: etc\n"
    print "  ", (BinEdit.new("00000001")).get('D'), "\n"
    print "  ", (BinEdit.new("000000010200")).get('Dw'), "\n"
    bin = BinEdit.new
    num = 7.0
    bin.set('F', [num])
    printf("  FLOAT: %f: %s\n", num, bin.dump)
    printf("  float: %f: %s\n", num, BinEdit.toHexString([7.0], 'f'))
    print "\n"
  end

  begin
    str = '0123456789012345'
    bin = BinEdit.new(str.length)
    bin.set('a', [str])
    print '  hex: ', bin.dump(), "\n"
    print '  tx:  ', bin.get(sprintf("a%d", bin.length())), "\n"
    print "\n"
  end

  begin
    str = 'tx:0123456'
    bin = BinEdit.new('tx:0123456')
    printf("  %s -> %s\n", str, bin.dump())
    print "\n"
  end

  begin
    bin = BinEdit.new('010203040506070809')
    printf("  crc32 crc : %s -> %-8s : check %s\n",  bin.dump(), bin.crc32(), ('40EFAB9E' == bin.crc32() ? 'OK' : 'NG'))
    printf("  modbus crc: %s -> %-8s : check %s\n",  bin.dump(), bin.crc16(), ('0EB2'     == bin.crc16() ? 'OK' : 'NG'))
    printf("  crc8 crc  : %s -> %-8s : check %s\n",  bin.dump(), bin.crc8(),  ('98'       == bin.crc8()  ? 'OK' : 'NG'))
    printf("  checksum  : %s -> %-8s : check %s\n",  bin.dump(), bin.sum(),   ('D3'       == bin.sum()   ? 'OK' : 'NG'))
    printf("  XOR check : %s -> %-8s : check %s\n",  bin.dump(), bin.xsum(),  ('01'       == bin.xsum()  ? 'OK' : 'NG'))
    print "\n"
  end

  GC.start()
  print "BinEdit test end\n"
end

def test_cpp_regexp
  print "CppRegexp test\n"
  # Class method test
  ## CppRegexp.reg_match
  printf("  reg_match  1: %s\n",   ( CppRegexp.reg_match('123:456:789', ':')                      ? "OK" : "NG"))
  printf("  reg_match  2: %s\n",   (!CppRegexp.reg_match('123:456:789', 'A')                      ? "OK" : "NG"))
  printf("  reg_match  3: %s\n",   (!CppRegexp.reg_match('123:456:789', ',', ';', 'i')            ? "OK" : "NG"))
  printf("  reg_match  4: %s\n",   ( CppRegexp.reg_match('123:456:789', 'g', ':')                 ? "OK" : "NG"))
  printf("  reg_match  5: %s\n",   ( CppRegexp.reg_match('123:456:789', ':', ';', 'o')            ? "OK" : "NG"))
  printf("  reg_match 10: %s\n",   ( CppRegexp.reg_match('123:456:789', ';', ':', 'j')            ? "OK" : "NG"))
  printf("  reg_match 11: %s\n",   ( CppRegexp.reg_match('123:456:789', ',', ';', ':')            ? "OK" : "NG"))
  printf("  reg_match 12: %s\n",   ( CppRegexp.reg_match('123:456:789', ['1', '5', '9'])          ? "OK" : "NG"))
  printf("  reg_match 13: %s\n",   ( CppRegexp.reg_match('123:456:789', ['a', 'A'], '7')          ? "OK" : "NG"))
  printf("  reg_match 14: %s\n",   ( CppRegexp.reg_match('123:456:789', '1', ['i', 'A'])          ? "OK" : "NG"))
  printf("  reg_match 15: %s\n",   (!CppRegexp.reg_match('123:456:789', ['a', 'A', '-'])          ? "OK" : "NG"))
  printf("  reg_match 16: %s\n",   ( CppRegexp.reg_match(['123', '456', '789'], '23')             ? "OK" : "NG"))
  printf("  reg_match 17: %s\n",   ( CppRegexp.reg_match(['123', '456', '789'], '6')              ? "OK" : "NG"))
  printf("  reg_match 18: %s\n",   ( CppRegexp.reg_match(['123', '456', '789'], '8')              ? "OK" : "NG"))
  printf("  reg_match 19: %s\n",   (!CppRegexp.reg_match(['123', '456', '789'], 'A')              ? "OK" : "NG"))
  printf("  reg_match 20: %s\n",   ( CppRegexp.reg_match(['123', '456', '789'], ['A', '23'])      ? "OK" : "NG"))
  printf("  reg_match 21: %s\n",   ( CppRegexp.reg_match(['123', '456', '789'], 'A', '6', 'B')    ? "OK" : "NG"))
  printf("  reg_match 22: %s\n",   ( CppRegexp.reg_match(['123', '456', '789'], 'A', ['B', '8'])  ? "OK" : "NG"))
  printf("  reg_match 23: %s\n",   (!CppRegexp.reg_match(['123', '456', '789'], ['A', 'B', 'C'])  ? "OK" : "NG"))
  ## CppRegexp.reg_grep
  arry = CppRegexp.reg_grep('123:456:789', ':'                           ); print "  reg_grep  1: ", sprintf("%s: ", (arry == ['123:456:789']       ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep('123:456:789', 'A'                           ); print "  reg_grep  2: ", sprintf("%s: ", (arry == []                    ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], '23'                  ); print "  reg_grep  3: ", sprintf("%s: ", (arry == ['123']               ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], '6'                   ); print "  reg_grep  4: ", sprintf("%s: ", (arry == ['456']               ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], '8'                   ); print "  reg_grep  5: ", sprintf("%s: ", (arry == ['789']               ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], 'A'                   ); print "  reg_grep  6: ", sprintf("%s: ", (arry == []                    ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], '[0-9]'               ); print "  reg_grep  7: ", sprintf("%s: ", (arry == ['123', '456', '789'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], ['[0-9]', '23']       ); print "  reg_grep  8: ", sprintf("%s: ", (arry == ['123']               ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], '.', '6', '4'         ); print "  reg_grep  9: ", sprintf("%s: ", (arry == ['456']               ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], '.', ['9', '8']       ); print "  reg_grep 10: ", sprintf("%s: ", (arry == ['789']               ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_grep(['123', '456', '789'], ['.', '[0-9]', '[29]']); print "  reg_grep 11: ", sprintf("%s: ", (arry == ['123', '789']        ? "OK" : "NG")), arry, "\n"
  ## CppRegexp.reg_replace
  arry = CppRegexp.reg_replace('123:456:789', ':', ' '                     ); print "  reg_replace  1: ", sprintf("%s: ", (arry == '123 456 789'         ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace('123:456:789', 'A', ' '                     ); print "  reg_replace  2: ", sprintf("%s: ", (arry == '123:456:789'         ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], '--', '23'           ); print "  reg_replace  3: ", sprintf("%s: ", (arry == ['1--', '456', '789'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], '-', '6'             ); print "  reg_replace  4: ", sprintf("%s: ", (arry == ['123', '45-', '789'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], 'A', '8'             ); print "  reg_replace  5: ", sprintf("%s: ", (arry == ['123', '456', '7A9'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], '*', 'A'             ); print "  reg_replace  6: ", sprintf("%s: ", (arry == ['123', '456', '789'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], 'i', '[A-Z]'         ); print "  reg_replace  7: ", sprintf("%s: ", (arry == ['123', '456', '789'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], '*', ['[23]', '7']   ); print "  reg_replace  8: ", sprintf("%s: ", (arry == ['1**', '456', '*89'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], '&', '7', '6', '4'   ); print "  reg_replace  9: ", sprintf("%s: ", (arry == ['123', '&5&', '&89'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], 'm', '7', ['9', '8'] ); print "  reg_replace 10: ", sprintf("%s: ", (arry == ['123', '456', 'mmm'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_replace(['123', '456', '789'], '?', ['2', '5', '8'] ); print "  reg_replace 11: ", sprintf("%s: ", (arry == ['1?3', '4?6', '7?9'] ? "OK" : "NG")), arry, "\n"
  ## CppRegexp.reg_split
  arry = CppRegexp.reg_split('123,456:789', ':'        ); print "  reg_split  1: ", sprintf("%s: ", (arry == ['123,456','789']   ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_split('123,456:789', ',', ':'   ); print "  reg_split  2: ", sprintf("%s: ", (arry == ['123','456','789'] ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_split('123,456:789', [',', ':'] ); print "  reg_split  3: ", sprintf("%s: ", (arry == ['123','456','789'] ? "OK" : "NG")), arry, "\n"
  ## CppRegexp.reg_select
  arry = CppRegexp.reg_select('cmd1', 'cmd1', 'cmd2', 'cmd3'   ); print "  reg_select  1: ", sprintf("%s: ", (arry == 0 ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_select('cmd2', 'cmd1', ['cmd2', 'cmd3'] ); print "  reg_select  2: ", sprintf("%s: ", (arry == 1 ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_select('cmd3', ['cmd1', 'cmd2', 'cmd3'] ); print "  reg_select  3: ", sprintf("%s: ", (arry == 2 ? "OK" : "NG")), arry, "\n"
  arry = CppRegexp.reg_select('cmd4', ['cmd1', 'cmd2', 'cmd3'] ); print "  reg_select  4: ", sprintf("%s: ", (arry == 3 ? "OK" : "NG")), arry, "\n"

  # Object method test
  reg = CppRegexp.new('[abc]')
  print '  abcdecg: check ',  reg.match('abcdecg') ? "OK" : "NG" , "\n"
  print '  test: check ',    !reg.match('test')   ? "OK" : "NG" , "\n"
  reg = CppRegexp.new( [ '[abc]', '[def]', '[xyz]' ] )
  printf("  reg.length(): check %s: %d\n", (3 == reg.length() ? 'OK' : 'NG'), reg.length())
  printf("  select:a -> %d: check %s\n", reg.select('a'), (0==reg.select('a') ? 'OK' : 'NG'))
  printf("  select:e -> %d: check %s\n", reg.select('e'), (1==reg.select('e') ? 'OK' : 'NG'))
  reg = CppRegexp.new( '[abc]', '[def]', '[xyz]' )
  printf("  select:z -> %d: check %s\n", reg.select('z'), (2==reg.select('z') ? 'OK' : 'NG'))
  printf("  select:  -> %d: check %s\n", reg.select(' '), (3==reg.select(' ') ? 'OK' : 'NG'))

  text = ['10:a','11:b','00:ig','12:c','13:d']
  reg = CppRegexp.new('1', '[bc]')
  print "  grep:  check ", (["11:b","12:c"] == reg.grep(Array.new(text)) ? 'OK' : 'NG'), ': ', reg.grep(Array.new(text)), "\n"
  print "  match: check ", (["10:a","11:b","12:c","13:d"] == reg.match(Array.new(text)) ? 'OK' : 'NG'), ': ', reg.match(Array.new(text)), "\n"

  text = ['0', 's1as2bs3cs4ds5es6fs7xs8xs9zs10 s11', 's1as2bs3cs4ds5es6fs7    s11']
  reg = CppRegexp.new('1', 'x')
  print "  grep: check ", (["s1as2bs3cs4ds5es6fs7xs8xs9zs10 s11"] == reg.grep(Array.new(text)) ? 'OK' : 'NG'), ': ', reg.grep(Array.new(text)), "\n"

  text = ['123', '456', '789']
  reg = CppRegexp.new('5', '8')
  print "  replace: check ", (['123', '4-6', '7-9'] == reg.replace('-', Array.new(text)) ? 'OK' : 'NG'), ': ', reg.replace('-', Array.new(text)), "\n"
  reg = CppRegexp.new(':', ',')
  text = '123,456,789:abc,dfg,hij'
  print "  split: check ", (["123", "456", "789", "abc", "dfg", "hij"] == reg.split(text) ? 'OK' : 'NG'), ': ', reg.split(text), "\n"

  print "CppRegexp test end\n"
end

def test_thead
  print "thread test\n"
  begin
    Core.tick
    WorkerThread.ms_sleep(200)
    tick = Core.tick
    printf("  WorkerThread.ms_sleep: check %s: result %d\n", (tick == 200 ? 'OK' : 'NG'), tick)
  end
  begin
    th = WorkerThread.new
    th1 = WorkerThread.run(1) do |th|
      th.synchronize do
        print '  Thread test 1: th1 = WorkerThread.run(1)', "\n"
      end
    end
    th2 = WorkerThread.run(1) do |th|
      th.synchronize do
        print '  Thread test 1: th2 = WorkerThread.run(1)', "\n"
      end
    end
    th1.join()
    th2.join()
    th1.start(5) do
      th.synchronize do
        print '  Thread test 1: th1.start(1)', "\n"
      end
    end
    th2.start(2) do
      th.synchronize do
        print '  Thread test 1: th2.start(1)', "\n"
      end
    end
    th1.join()
    th2.join()
    th1.start(1) do
      th1.wait do
        print '  Thread test 2: th1.wait', "\n"
      end
    end
    th2.start(1) do
      th1.notify do
        print '  Thread test 2: th2.nority', "\n"
      end
    end
    th1.join
    th2.join
    printf("  Thread test 3: th1: fifo.len=%d\n", th1.fifo_len);
    th1.start(1) do
      th1.wait do
        print '  Thread test 3: th1: str = th1.fifo_pop(bin)', "\n"
        printf("  Thread test 3: th1: fifo.len=%d\n", th1.fifo_len);
        bin = BinEdit.new
        while(0 < th1.fifo_len)
          str = th1.fifo_pop(bin)
          print '    str -> "', str, '"', "\n"
        end
      end
    end
    th2.start(1) do
      th1.notify do
        print "  Thread test 3: th2: th1.fifp_push('test')", "\n"
        th1.fifo_push('test 1')
        th1.fifo_push('test 2')
      end
    end
    th1.join
    th2.join
    printf("  Thread test 4: th1: fifo.len=%d\n", th1.fifo_len);
    th1.start(1) do
      print '  Thread test 4: th1: str = th1.fifo_pop(bin)', "\n"
      bin = BinEdit.new
      len = th1.fifo_wait()
      printf("  Thread test 4: th1: fifo.len=%d\n", th1.fifo_len);
      str = th1.fifo_pop(bin)
      printf("  Thread test 4: th1: fifo.len=%d\n", th1.fifo_len);
      print '    str -> "', str, '"', "\n"
      print "  Thread test 4: th1: th2.fifp_push('test2')", "\n"
      th2.fifo_push('test2')
      print "  Thread test 4: th1: end", "\n"
    end
    th2.start(1) do
      print "  Thread test 4: th2: th1.fifp_push('test1')", "\n"
      th1.fifo_push('test1')
      bin = BinEdit.new
      len = th2.fifo_wait()
      str = th2.fifo_pop(bin)
      print '  Thread test 4: th2: str = th2.fifo_pop(bin)', "\n"
      print '    str -> "', str, '"', "\n"
      print "  Thread test 4: th2: end", "\n"
    end
    th1.join
    th2.join
  end
  begin
    th1 = WorkerThread.run(1) do |th|
      bin = BinEdit.new
      len = th.fifo_wait()
      printf("    th1: len -> %d\n", len)
      str = th.fifo_pop(bin)
      print  '    th1: str -> "', str, '"', "\n"
    end
    th2 = WorkerThread.run(1) do |th|
      print "  Thread & fifo test 1: th2: Core.fifo_push('test')\n"
      th1.fifo_push('test')
    end
    th1.join
    th2.join
  end
  begin
    th1 = WorkerThread.run(1) do |th|
      bin = BinEdit.new
      len = th.fifo_wait()
      printf("    th1: len -> %d\n", len)
      str = th.fifo_pop(bin)
      printf("    th1: str -> %d, bin.dump -> %s\n", str.length, bin.dump);
    end
    th2 = WorkerThread.run(1) do |th|
      print "  Thread & fifo test 2: th2: Core.fifo_push(bin)\n"
      bin = BinEdit.new('1234')
      th1.fifo_push(bin)
      print "  Thread & fifo test 2: th2: Core.fifo_push(bin) end\n"
    end
    th1.join
    th2.join
  end
  begin
    th1 = WorkerThread.run(1) do |th|
      bin = BinEdit.new
      len = th.fifo_wait()
      printf("    th1: len -> %d\n", len)
      str = th.fifo_pop(bin)
      printf("    th1: str(%d), bin(%s)\n", str.length, bin.dump);
    end
    th2 = WorkerThread.run(1) do |th|
      print "  Thread & fifo test 3: th2: Core.fifo_push(bin)\n"
      bin = BinEdit.new('1234')
      th1.fifo_push(bin)
      print "  Thread & fifo test 3: th2: Core.fifo_push(bin) end\n"
    end
    th1.join
    th2.join
  end
  begin
    th1 = WorkerThread.run(1) do |th|
      bin = BinEdit.new
      len = th.fifo_wait()
      printf("    th1: len -> %d\n", len)
      str = th.fifo_pop(bin)
      printf("    th1: str(%d:%s), bin(%s)\n", str.length, str, bin.dump);
    end
    th2 = WorkerThread.run(1) do |th|
      print "  Thread & fifo test 4: th2: Core.fifo_push('test1', bin)\n"
      th1.fifo_push('test1', BinEdit.new('1234'))
    end
    th1.join
    th2.join
  end
  print "thead test end\n"
end

opt = Core.args()
if 0 < opt.length() then
  (opt.length()).times { |idx| printf("  arg: %s\n", opt[idx]); }
else
  print "mruby test script 1\n"
  test_core(); print "\n"
  test_bin_edit(); print "\n"
  test_cpp_regexp(); print "\n"
  test_thead(); print "\n"
  print "mruby test script 1 end\n"
  print "\n"
end
