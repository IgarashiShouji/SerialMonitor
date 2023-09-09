def test1(fname)
  print "test: file compress\n"
  bin = BinEdit.new(sprintf("file:%s", fname))
  printf("  length: %d\n", bin.length)
  print "  ", bin.dump(0, 16), "\n"
  bin.compress()
  bin.save(sprintf("%s.compress", fname))
  print "test: file compress end\n"
end
def test2(fname)
  print "test: file uncompress\n"
  bin = BinEdit.new(sprintf("compress:%s.compress", fname))
  bin.uncompress()
  printf("  length: %d\n", bin.length)
  print "  ", bin.dump(0, 16), "\n"
  bin.save(sprintf("s.txt", fname))
  printf("  length: %d\n", bin.length)
  print "test: file uncompress end\n"
end
def test3()
  print "test 3\n"
  bin = BinEdit.new('00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ')
  bin.set(0, 'dfwbcsifAH',  [ 0x11223344, 1.0, 0xaabb, 0x99, -10, -3, -2, 0.1, '01234', 'AABBCC' ])
  print "  ", bin.dump(), "\n"
  print "  ", bin.get(0, 'dfwbcsifA5H3'), "\n"

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

  print "test 3: end\n"
end
def test4()
  print "test 4\n"
  bin1 = BinEdit.new('01020304')
  bin2 = BinEdit.new(bin1)
  bin3 = BinEdit.new(bin1, 3)
  bin4 = BinEdit.new(bin1, 1, 2)
  print "  bin1: ", bin1.dump(), "\n"
  print "  bin2: ", bin2.dump(), "\n"
  print "  bin3: ", bin3.dump(), "\n"
  print "  bin4: ", bin4.dump(), "\n"
  print "test 4: end\n"
end
class String
  def toItems(fmt)
    bin = bin.BinEdit.new(self)
    return gin.get(fmt)
  end
end
def test5()
  print "test 5\n"
  print "  ", (BinEdit.new("00000001")).get('D'), "\n"
  print "  ", (BinEdit.new("000000010200")).get('Dw'), "\n"
  print "  ", (BinEdit.new("000000010200")).get(' '), "\n"
  GC.start()
  print "test 5: end\n"
end

print "test\n"

bin = BinEdit.new('0102030405')
printf("%d: %s\n", bin.length, bin.dump)
printf("%d: %s\n", bin.length, bin.dump(3))
printf("%d: %s\n", bin.length, bin.dump(1, 2))
printf("%d: %s\n", bin.length, bin.dump(0,10))
print BinEdit.hexToArray('bb sS wW iI dD fF h2 A3', '0102 FEFF FFFE 0100 0100 FCFFFFFF FFFFFFFC 01000000 00000001 12345678 12345678 5566 303132'), "\n"

bin = BinEdit.new(3)
printf("wsize: %s\n", bin.write('11 22'))
printf("wsize: %s\n", bin.write(2, '03'))
printf("%d: %s\n", bin.length, bin.dump)

bin = BinEdit.new('010203040506070809112233445566778899AABBCCDDEEFF010203040506070809')
printf("compress: \n")
printf("%d: %s\n", bin.length, bin.dump)
bin.compress();   printf("%d: %s\n", bin.length, bin.dump)
bin.uncompress(); printf("%d: %s\n", bin.length, bin.dump)

bin = BinEdit.new('010203040506070809FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF010203040506070809')
printf("compress: \n")
printf("%d: %s\n", bin.length, bin.dump)
bin.compress();   printf("%d: %s\n", bin.length, bin.dump)
bin.uncompress(); printf("%d: %s\n", bin.length, bin.dump)

opt = Args.new()
test1(opt['mruby-script'])
test2(opt['mruby-script'])
test3()
test4()
test5()

print "test end\n"
