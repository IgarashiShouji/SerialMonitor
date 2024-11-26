
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
  th1 = WorkerThread.new
  th1.run do
    print "  run thread\n"
    th1.wait()
    th1.synchronize do
      print "  list.pop: ", list.pop(), "\n"
    end
    th1.stop()
    print "  run thread end\n"
  end
  print "  notify \n"
  th1.notify do
    list.push(1)
  end
  print "  thread join\n"
  th1.join
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

class CppRegexp
  def compile(args)
print "igarashi\n"
    reg_arg = Array.new
    args.each do |arg|
      if arg.class.name == 'String' then
        reg_arg.push(arg)
      elsif arg.class.name == 'Regexp' then
        reg_arg.push(arg.source)
      end
    end
    return CppRegexp.new(reg_arg)
  end
end

def test_cpp_regexp
  print "CppRegexp test\n"
  reg = CppRegexp.new('[abc]')
  print '  test: ',    reg.match('test')    ? "ok" : "ng" , "\n"
  print '  abcdecg: ', reg.match('abcdecg') ? "ok" : "ng" , "\n"
  reg = CppRegexp.new( [ '[abc]', '[def]', '[xyz]' ] )
  printf("  reg.length(): %d\n", reg.length())
  printf("  select:a -> %d\n", reg.select('a'))
  printf("  select:e -> %d\n", reg.select('e'))
  reg = CppRegexp.new( '[abc]', '[def]', '[xyz]' )
  printf("  select:z -> %d\n", reg.select('z'))
  printf("  select:  -> %d\n", reg.select(' '))
  reg = CppRegexp.new('1', '[bc]')
  print "  grep:  ", reg.grep(  [ '10:a', '11:b', '00:ig', '12:c', '13:d' ] ), "\n"
  print "  match: ", reg.match( [ '10:a', '11:b', '00:ig', '12:c', '13:d' ] ), "\n"
  reg = CppRegexp.new('1', 'x')
  print "  grep: ", reg.grep('0', 's1as2bs3cs4ds5es6fs7xs8xs9zs10 s11', 's1as2bs3cs4ds5es6fs7    s11'), "\n"
  reg = CppRegexp.new(' ', 'x')
  print "  replace: ", reg.replace('--', 's1as2bs3cs4ds5es6fs7xs8xs9zs10 s11', 's1as2bs3cs4ds5es6fs7    s11'), "\n"
  print "CppRegexp test end\n"
end

print "mruby test script 1\n"
test_options()
test_thead_1()
test_calc()
test_cpp_regexp()
print "mruby test script 1 end\n"
print "\n"
