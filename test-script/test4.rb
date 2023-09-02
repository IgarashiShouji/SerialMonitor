print "test\n"

bin = BinEdit.new('0102030405')
printf("%d: %s\n", bin.length, bin.dump)
printf("%d: %s\n", bin.length, bin.dump(1, 5))
printf("%d: %s\n", bin.length, bin.dump(3))

print BinEdit.hexToArray('bb sS wW iI dD fF h2 A3', '0102 FEFF FFFE 0100 0100 FCFFFFFF FFFFFFFC 01000000 00000001 12345678 12345678 5566 303132'), "\n"

bin = BinEdit.new(3)
printf("wsize: %s\n", bin.write('11 22'))
printf("wsize: %s\n", bin.write(2, '03'))
printf("%d: %s\n", bin.length, bin.dump)

bin = BinEdit.new('010203040506070809112233445566778899AABBCCDDEEFF010203040506070809')
printf("compress: \n")
printf("%d: %s\n", bin.length, bin.dump)
bin.compress();   printf("%d: %s\n", bin.length, bin.dump)
bin.decompress(); printf("%d: %s\n", bin.length, bin.dump)

bin = BinEdit.new('010203040506070809FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF010203040506070809')
printf("compress: \n")
printf("%d: %s\n", bin.length, bin.dump)
bin.compress();   printf("%d: %s\n", bin.length, bin.dump)
bin.decompress(); printf("%d: %s\n", bin.length, bin.dump)

print "test end\n"
