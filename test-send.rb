def msg_wait(mon, reg)
  while true
    msg = mon.wait()
    case msg
    when 'exit' then
      exit 0;
    else
      print msg, "\n"
      STDOUT.flush
      #if msg == reg then
      if mon.reg_matches(reg, msg) then
        break;
      end
    end
  end
end

mon = Smon.new('/dev/ttyUSB0')
msg_wait(mon, 'GAP:')
mon.send('010203040506')
msg_wait(mon, 'TO1:')
mon.send('1122334455')
msg_wait(mon, 'TO2:')
mon.send('99887766')
msg_wait(mon, '^..3:')    # regex
msg = 'ffaabb1234'
msg = msg + mon.crc(msg)
print "send: ", msg, "\n"
mon.send(msg)
msg_wait(mon, 'TO3:1000ms')
sleep(10);
mon.send('aabbcc9977')

#while true
#  msg_wait(mon, 'TO3:1000ms')
#end
