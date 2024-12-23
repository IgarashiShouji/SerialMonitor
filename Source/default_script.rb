# default script

tick = Core.tick()
arg = Core.new()
if 0 < arg.size() then
  objs = Array.new
  fmt = "%s:"; arg_sz = 0
  (arg.size()).times do |idx|
    port = String.new(arg[idx])
    smon = Smon.new(port)
    if arg_sz < port.length then
      arg_sz = port.length
      fmt = '%-' + sprintf("%d", port.length) + 's:'
    end
    objs.push( [ smon, port ] )
  end
  loop = true
  th = WorkerThread.new
  (objs.length).times do |idx|
    (smon, port) = objs[idx]
    WorkerThread.new(1) do
      while loop do
        bin = BinEdit.new
        state = smon.read_wait(bin)
        case state
        when Smon::CACHE_FULL then
        when Smon::GAP then
          th.synchronize do
            tick += Core.tick() % 100000000000
            if 0 < bin.length then
              rcv_msg = bin.dump()
              printf("%d:%s %10d[ms]: %s\n", idx, sprintf(fmt, port), tick, rcv_msg)
            else
              printf("%d:%s %10d[ms]: GAP\n", idx, sprintf(fmt, port), tick)
            end
          end
        when Smon::TO1 then
          th.synchronize do
            tick += Core.tick() % 100000000000
            printf("%d:%s %10d[ms]: TO%d\n", idx, sprintf(fmt, port), tick, state)
          end
        when Smon::TO2 then
          th.synchronize do
            tick += Core.tick() % 100000000000
            printf("%d:%s %10d[ms]: TO%d\n", idx, sprintf(fmt, port), tick, state)
          end
        when Smon::TO3 then
          th.synchronize do
            tick += Core.tick() % 100000000000
            printf("%d:%s %10d[ms]: TO%d\n", idx, sprintf(fmt, port), tick, state)
          end
        else
        end
      end
    end
  end
  WorkerThread.new(1) do
    cnt = 0
    while loop do
      if 0 == cnt then
        th.synchronize do
          print "%d:Date: ", cnt, Core.date(), "\n"
        end
      end
      WorkerThread.ms_sleep(100)
      cnt += 1
      cnt %= 500
    end
  end
  while true do
    str = Core.gets()
    if nil == str     then; break; end
    if 'quit' == str  then; break; end
    data = str
    if 0 < data.length then
      idx = 0
      if CppRegexp.reg_match(data, '^[0-9]+:') then
        idx  = (CppRegexp.reg_replace(data, ':.*$', '')).to_i
        data = CppRegexp.reg_replace(data, '^[0-9]+:', '')
      end
      if idx < objs.length then
        ( smon, port ) = objs[idx]
        th.synchronize do
          smon.send(data, 0)
          tick += Core.tick() % 100000000000
          printf("%d:%s %10d[ms]: Send: %s\n", idx, sprintf(fmt, port), tick, data)
        end
      end
    else
      break
    end
  end
  loop = false
  (objs.length).times do |idx|
    ( smon, port ) = objs[idx]
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
