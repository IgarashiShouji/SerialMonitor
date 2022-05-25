#print "mruby test script\n"
mon = Smon.new('/dev/ttyUSB0')

while( 1 )
  msg = mon.wait()
  case msg
  when "exit" then
    exit 0;
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
