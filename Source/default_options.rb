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
class SendWaitEventer
  def initilize
  end
  def exec(smon, send_msg, rcv_msg)
    printf("%s -> %s\n", send_msg, rcv_msg)
  end
  def gaptimer(smon, send_msg)
    printf("%s -> GAP\n", send_msg)
    true
  end
  def timeout1(smon, send_msg)
    printf("%s -> TO1\n", send_msg)
    true
  end
  def timeout2(smon, send_msg)
    printf("%s -> TO2\n", send_msg)
    true
  end
  def timeout3(smon, send_msg)
    printf("%s -> TO3\n", send_msg)
    false
  end
end
class WaitReplyEventer
  def initilize
  end
  def exec(smon, msg, reply)
    printf("%s -> %s\n", msg, reply)
    smon.send(reply, 0)
  end
  def timeout1(smon, msg)
    printf("TO1\n", msg)
    true
  end
  def timeout2(smon, msg)
    printf("TO2\n", msg)
    true
  end
  def timeout3(smon, msg)
    printf("TO3\n", msg)
    false
  end
end
class Smon
  def self.send_wait(smon, send_msg, regs, cmd=nil)
    reply=''
    if nil == cmd then
      cmd = SendWaitEventer.new
    end
    smon.send(send_msg, 0)
    msg = ''
    loop_enable = true
    while loop_enable
      smon.wait do |state, rcv_msg|
        if(0 < rcv_msg.length) then
          msg += rcv_msg
        end
        case state
        when Smon::CACHE_FULL then
        when Smon::GAP then
          regs.each do |reg|
            if Core.reg_match(msg, reg) then
              reply=msg
              loop_enable = false
              cmd.exec(smon, send_msg, msg)
              break
            end
          end
          if loop_enable then
            cmd.gaptimer(smon, send_msg)
          end
          msg = ''
        when Smon::TO1 then
          reply='TO1'
          loop_enable = cmd.timeout1(smon, send_msg)
        when Smon::TO2 then
          reply='TO2'
          loop_enable = cmd.timeout2(smon, send_msg)
        when Smon::TO3 then
          reply='TO3'
          cmd.timeout3(smon, send_msg)
          loop_enable = false
        end
      end
    end
    return reply
  end
  def self.wait_send(com, list, def_cmd=nil)
    if nil == def_cmd then
      def_cmd = WaitReplyEventer.new
    end
    th = WorkerThread.new
    smon = Smon.new(com)
    th.run do
      rcv_enable = true
      msg = ''
      while rcv_enable
        smon.wait do |state, rcv_msg|
          if(0 < rcv_msg.length) then
            msg += rcv_msg
          end
          case state
          when Smon::CACHE_FULL then
          when Smon::GAP then
            list_break = false
            list.each do |arg|
              ( reply, regs, cmd ) = arg
              if nil == cmd then
                cmd = def_cmd
              end
              regs.each do |reg|
                if Core.reg_match(msg, reg) then
                  cmd.exec(smon, msg, reply)
                  break
                end
              end
              if list_break then
                break;
              end
            end
            msg = ''
          when Smon::TO1 then
            def_cmd.timeout1(smon, msg)
          when Smon::TO2 then
            def_cmd.timeout2(smon, msg)
          when Smon::TO3 then
            def_cmd.timeout3(smon, msg)
          else
            rcv_enable = false
          end
        end
        STDOUT.flush
      end
    end
    str = Core.gets()
    smon.close
    th.join
  end
end
