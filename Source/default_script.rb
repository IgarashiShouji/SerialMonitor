# default script

list_port = Core.args()
if 0 == list_port.length then
  Core.bin_cmd_editor()
  return
end

tick = Core.tick(); arg_sz = 0
list_port.each do |port|
  arg_sz = (arg_sz < port.length ? port.length : arg_sz)
end
fmt = '%-' + sprintf("%d", arg_sz) + 's:'
printf("Date: %s\n", Core.date())
Smon.open(Array.new(list_port)) do |smon|
  loop = true
  smon.timer(Smon::CTIMER, (5*1000))
  bin = BinEdit.new()
  while loop do
    str = Core.gets(smon, bin) do |idx, state|
      port = list_port[idx]
      case state
      when Smon::OTIMER then
      when Smon::CTIMER then
        printf("Date: %s\n", Core.date())
      when Smon::CACHE_FULL then
      when Smon::GAP then
        tick += Core.tick() % 100000000000
        if 0 < bin.length then
          printf("%d:%s %10d[ms]: %s\n", idx, sprintf(fmt, port), tick, bin.dump())
        else
          printf("%d:%s %10d[ms]: GAP\n", idx, sprintf(fmt, port), tick)
        end
      when Smon::TO1, Smon::TO2, Smon::TO3 then
        tick += Core.tick() % 100000000000
        printf("%d:%s %10d[ms]: TO%d\n", idx, sprintf(fmt, port), tick, state)
      else
      end
    end
    if nil != str then
      case str
      when 'quit' then
        loop = false;
      else
        data = str
        if 0 < data.length then
          idx = 0
          if CppRegexp.reg_match(data, '^[0-9]+:') then
            idx  = (CppRegexp.reg_replace(data, ':.*$', '')).to_i
            data = CppRegexp.reg_replace(data, '^[0-9]+:', '')
          end
          if idx < list_port.length then
            port = list_port[idx]
            smon.send(idx, data)
            tick += Core.tick() % 100000000000
            printf("%d:%s %10d[ms]: Send: %s\n", idx, sprintf(fmt, port), tick, data)
          end
        end
      end
    else
      loop = false;
    end
  end
end
