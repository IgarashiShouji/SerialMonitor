def __CheckOptions()
  opts = Args.new
  if nil == opts['mruby-script'] then
    if nil != opts['crc'] then
      printf("%s: %s\n", Core.crc16(opts['crc']), opts['crc']);
      exit 0
    end
    if nil != opts['crc8'] then
      printf("%s: %s\n", Core.crc8(opts['crc8']), opts['crc8']);
      exit 0
    end
    if nil != opts['sum'] then
      printf("%s: %s\n", Core.sum(opts['sum']), opts['sum']);
      exit 0
    end
    if nil != opts['float'] then
      printf("%s: %s\n", Core.float(opts['float']), opts['float']);
      exit 0
    end
    if nil != opts['floatl'] then
      printf("%s: %s\n", Core.float_l(opts['floatl']), opts['floatl']);
      exit 0
    end
  end
end
__CheckOptions()
