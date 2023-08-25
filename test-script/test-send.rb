class Eventer < SendWaitEventer
  def initilize()
  end
  def set(obj)
    @smon = obj
  end
  def gaptimer(send_msg)
    true
  end
  def timeout1(send_msg)
    printf("%s: Recive Timeout\n", send_msg)
    false
  end
end
opts = Args.new()
if 0 < opts.size() then
  com  = opts[0]
  smon = Smon.new(com)
  evt = Eventer.new()
  evt.set(smon)
  Smon.send_wait(smon, '01012233445566',   [ '^0102' ], evt)
  Smon.send_wait(smon, '0102030405060709', [ '^1122' ], evt)
  Smon.send_wait(smon, '0303030405060709', [ '^0102', '9999' ], evt)
#  Smon.send_wait(smon, '0000', [ '^1122' ], evt)
  smon.close
end
