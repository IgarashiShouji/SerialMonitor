def test_smon()
  th_prn = CppThread.new
  th_s = Array.new
  opts = Args.new()
  (opts.size()).times do |idx|
    arg  = opts[idx]
    timer  = 30
    smon = Smon.new( arg )
    th   = CppThread.new
    th_s.push( [ th, smon ] )
    th.run() do
      smon.send('0102030405060708090a0b0C0D0E0F', 0)
      (10).times do |idx|
        enable = true;
        while enable do
          str = nil
          smon.wait do |state, rcv_msg|
            if(0 < rcv_msg.length) then
              str = sprintf("%-20s: %s", arg, rcv_msg)
            end
            case state
            when Smon::CACHE_FULL then
            when Smon::GAP then
              if(0 < rcv_msg.length) then
                str += "\n"
                msg = sprintf("0102030405060708090a0b0C0D0E0F%02x", idx)
                msg += Core.crc16(msg)
                smon.send(msg, timer)
                enable = false
              end
            when Smon::TO1 then
              str = sprintf("%-20s: TO1\n", arg);
            when Smon::TO2 then
              str = sprintf("%-20s: TO2\n", arg);
            when Smon::TO3 then
              str = sprintf("%-20s: TO3\n", arg);
#           when Smon::CLOSE  then
#           when Smon::NONE then
#              enable = false
            else
              enable = false
            end
          end
          if nil != str then
            th_prn.synchronize do
              print idx, ": ", str;
            end
          end
          timer += 30
        end
      end
      print "finish: ", arg, "\n"
    end
  end

  str = Core.gets()
  rep = Core.reg_replace(str, '01020304', '11121314')
  printf("test: %s -> %s\n", str, rep)
  th_s.each do |array|
    ( th, smon ) = array;
    th.join
  end
end

print "mruby test script 3\n"
test_smon()
print "mruby test script 3 end\n"
print "\n"
