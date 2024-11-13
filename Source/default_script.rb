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
              if 0 < rcv_msg.length then
                printf("%d:%s: %s\n", idx, arg, rcv_msg)
              end
              printf("%d:%s: GAP\n", idx, arg)
            end
          when Smon::TO1 then
            th_prn.synchronize do
              printf("%d:%s: TO%d\n", idx, arg, state)
            end
          when Smon::TO2 then
            th_prn.synchronize do
              printf("%d:%s: TO%d\n", idx, arg, state)
            end
          when Smon::TO3 then
            th_prn.synchronize do
              printf("%d:%s: TO%d\n", idx, arg, state)
            end
          else
            loop = false
          end
        end
      end
    end
  end
  while true do
    str = Core.gets()
    if nil == str     then; break; end
    if 'quit' == str  then; break; end
    if 0 < str.length then
      idx = 0
      if Core.reg_match(str, ':') then
        idx = (Core.reg_replace(str, ':.*$', '')).to_i
      end
      msg = Core.reg_replace(str, '^.*:', '')
      ( smon, th_ctrl, idx_, arg ) = objs[idx]
      smon.send(msg, 0)
      printf("%d:%s: Send: %s\n", idx, arg, msg)
    else
      break
    end
  end
  objs.each do |items|
    ( smon, th_ctrl, idx, arg ) = items
    smon.close()
  end
  objs.each do |items|
    ( smon, th_ctrl, idx, arg ) = items
    th_ctrl.join()
  end
else
  print "smon [options] comXX comXX ...", "\n"
  print "see of help)\n"
  print "  smon --help\n"
  print "ex)\n"
  print "  smon -b 9600E1 -g 50 -t 100 --timer2 300 --timer3 500 com10 com11\n"
  print "\n"
end
