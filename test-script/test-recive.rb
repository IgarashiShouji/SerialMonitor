class Test1 < WaitReplyEventer
  def exec(smon, msg, reply)
    print msg, ' -> ', reply, "\n"
    smon.send(reply, 0)
  end
end
class Test2 < WaitReplyEventer
  def exec(smon, msg, reply)
    print msg, ' -> ', reply, "\n"
    smon.send(reply, 0)
  end
end

opts = Args.new()
if 0 < opts.size() then
  list = Array.new
  list.push( [ '010203040506070809', [ '^0101', '^0301' ], Test1.new ] )
  list.push( [ '112233445566778899', [ '^0102', '^0302' ], Test2.new ] )
  list.push( [ '999999999999999999', [ '^0103', '^0303' ] ] )
  th = WorkerThread.new
  smon = Smon.new(opts[0])
  run_enable = true
  th.run do
    while run_enable
      Smon.wait_send(smon, list)
    end
  end
  str = Core.gets()
  run_enable = false
  smon.close
  th.join
  #Smon.wait_send(opts[0], list)
end
