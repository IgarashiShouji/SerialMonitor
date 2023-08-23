
def test_options()
  opt = Args.new()
  printf("--mruby-script: %s\n", opt['mruby-script'])
  printf("opt.size: %d\n", opt.size())
  (opt.size()).times do |idx|
    printf("arg: %s\n", opt[idx]);
  end
end

def test_thead_1
  print "thread test 1\n"
  list = Array.new
  th = WorkerThread.new
  th.run do
    print "  run thrad\n"
    th.notify do
      print "  nofit \n"
      list.push(1) 
      print "  nofity end\n"
    end
    print "  run thrad end\n"
  end

  th.wait do
    print "  wait\n"
    if( 0 < list.length ) then
      print "  wait true\n"
      return true;
    end
    print "  wait false\n"
    return false;
  end
  print "  thread join\n"
  th.join
  print "thead test 1 end\n"
end

def test_calc()
  print "Core test\n"
  str = '010203040506070809'
  printf("  modbus crc: %s -> %s\n",  str, Core.crc16(str)  );
  printf("  crc8 crc  : %s -> %s\n",    str, Core.crc8(str)   );
  printf("  checksum  : %s -> %s\n",  str, Core.sum(str)    );
  str = '01020304'
  printf("  convert float big   : %s -> %e\n",str, Core.float(str)  );
  printf("  convert float little: %s -> %e\n",str, Core.float_l(str));
  print "Calc test end\n"
end

print "mruby test script 1\n"
test_options()
test_thead_1()
test_calc()
print "mruby test script 1 end\n"
print "\n"
