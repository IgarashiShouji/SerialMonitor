def test1(fname)
  print "test: file compress\n"
  bin = BinEdit.new(sprintf("file:%s", fname))
  printf("length: %d\n", bin.length)
  print bin.dump(0, 16), "\n"
  bin.compress()
  bin.save(sprintf("%s.compress", fname))
  print "test: file compress end\n"
end
def test2(fname)
  print "test: file uncompress\n"
  bin = BinEdit.new(sprintf("compress:%s.compress", fname))
  bin.uncompress()
  printf("length: %d\n", bin.length)
  print bin.dump(0, 16), "\n"
  bin.save(sprintf("s.txt", fname))
  printf("length: %d\n", bin.length)
  print "test: file uncompress end\n"
end

print "test\n"

bin = BinEdit.new('0102030405')
printf("%d: %s\n", bin.length, bin.dump)
printf("%d: %s\n", bin.length, bin.dump(1, 5))
printf("%d: %s\n", bin.length, bin.dump(3))
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

print "test end\n"
