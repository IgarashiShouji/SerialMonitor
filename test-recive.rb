#print "mruby test script\n"
mon = Smon.new('/dev/ttyUSB0')
#msg='0102030405'
#print msg, "\n";
#msg += mon.crc(msg);
#print msg, "\n";
#File.open("test.txt", "r") do |str|
#  print str
#end

while( 1 )
  msg = mon.wait()
  case msg
  when "exit" then
    exit 0;
#  when "TO1:300ms" then
#    print msg, "\n"
#  when "TO2:500ms" then
#    print msg, "\n"
#  when "TO3:1000ms" then
#    print msg, "\n"
  else
    if mon.reg_matches('GAP', msg) then
      print "Gap Time Out: ", msg, "\n"
      next
    end
    if mon.reg_matches('^TO[0-9]', msg) then
      print "Time Out: ", msg, "\n"
      next
    end
    if 0 < msg.length then
      print "Recive: ", msg, "\n"
    end
  end
  STDOUT.flush
end
