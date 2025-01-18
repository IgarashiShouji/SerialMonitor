# default script

list_port = Core.args()
if 0 < args.length then
  tick = Core.tick(); arg_sz = 0
  list_port.each do |port|
    arg_sz = (arg_sz < port.length ? arg_sz : port.length)
  end
  fmt = '%-' + sprintf("%d", arg_sz) + 's:'
  Smon.open(list_port) do |smon|
    loop = true
    bin = BinEdit.new(1024)
    th = WorkerThread.run(1) do
      cnt = 500
      while loop do
        (500).times do |idx|
          workerthread.ms_sleep(100)
          if !loop then; break; end
        end
        th.synchronize do
          printf("%s\n", core.date())
        end
      end
    end
    while loop do
      str = Core.gets(smon) do |state, idx|
        port = list_port[idx]
        case state
        when Smon::CACHE_FULL then
        when Smon::GAP then
          th.synchronize do
            tick += Core.tick() % 100000000000
            smon.read(bin)
            if 0 < bin.length then
              printf("%d:%s %10d[ms]: %s\n", idx, sprintf(fmt, port), tick, bin.dump())
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
      if nil == str     then; loop = false; break; end
      if 'quit' == str  then; loop = false; break; end
      data = str
      if 0 < data.length then
        idx = 0
        if CppRegexp.reg_match(data, '^[0-9]+:') then
          idx  = (CppRegexp.reg_replace(data, ':.*$', '')).to_i
          data = CppRegexp.reg_replace(data, '^[0-9]+:', '')
        end
        if idx < list_port.length then
          th.synchronize do
            smon.send(data, idx)
            tick += Core.tick() % 100000000000
            printf("%d:%s %10d[ms]: Send: %s\n", idx, sprintf(fmt, port), tick, data)
          end
        end
      else
        break
      end
    end
    loop = false
  end
else
  Core.bin_cmd_editor()
end
