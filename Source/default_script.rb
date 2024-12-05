# default script

tick = Core.tick()
opts = Args.new()
if 0 < opts.size() then
  th_prn = WorkerThread.new
  objs = Array.new
  (opts.size()).times do |idx|
    th_ctrl = WorkerThread.new
    smon    = Smon.new( opts[idx] )
    objs.push( [ smon, th_ctrl, idx, opts[idx] ] )
    arg = opts[idx]
    th_ctrl.run() do
      msg = ''
      loop = true
      while loop do
        smon.wait do |state, rcv_msg|
          case state
          when Smon::CACHE_FULL then
          when Smon::GAP then
            th_prn.synchronize do
              tick += Core.tick() % 100000000000
              if 0 < rcv_msg.length then
                printf("%d:%s: %10d[ms]: %s\n", idx, arg, tick, rcv_msg)
              end
              printf("%d:%s: %10d[ms]: GAP\n", idx, arg, tick)
            end
          when Smon::TO1 then
            th_prn.synchronize do
              tick += Core.tick() % 100000000000
              printf("%d:%s: %10d[ms]: TO%d\n", idx, arg, tick, state)
            end
          when Smon::TO2 then
            th_prn.synchronize do
              tick += Core.tick() % 100000000000
              printf("%d:%s: %10d[ms]: TO%d\n", idx, arg, tick, state)
            end
          when Smon::TO3 then
            th_prn.synchronize do
              tick += Core.tick() % 100000000000
              printf("%d:%s: %10d[ms]: TO%d\n", idx, arg, tick, state)
            end
          else
            loop = false
          end
        end
      end
      th_ctrl.stop()
    end
  end
  while true do
    str = Core.gets()
    if nil == str     then; break; end
    if 'quit' == str  then; break; end
    if 0 < str.length then
      idx = 0
      if CppRegexp.reg_match(str, ':') then
        idx = (CppRegexp.reg_replace(str, ':.*$', '')).to_i
      end
      msg = CppRegexp.reg_replace(str, '^.*:', '')
      ( smon, th_ctrl, idx_, arg ) = objs[idx]
      smon.send(msg, 0)
      th_prn.synchronize do
        tick += Core.tick() % 100000000000
        printf("%d:%s: %10d[ms]: Send: %s\n", idx, arg, tick, msg)
      end
    else
      break
    end
  end
  objs.each do |items|
    ( smon, th_ctrl, idx, arg ) = items
    smon.close()
  end
else
  print "smon [options] comXX comXX ...", "\n"
  print "see of help)\n"
  print "  smon --help\n"
  print "ex)\n"
  print "  smon -b 9600E1 -g 50 -t 100 --timer2 300 --timer3 500 com10 com11\n"
  print "\n"
end
