class Core
  def self.__CheckOptions()
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
end
Core.__CheckOptions()
class BinEdit
  def self.hexToArray(format, hex_string)
    bin = BinEdit.new(hex_string)
    return bin.toItems(format)
  end
end
class SendWaitEventer
  def initilize
  end
  def exec(send_msg, rcv_msg)
    printf("%s -> %s\n", send_msg, rcv_msg)
  end
  def gaptimer(send_msg)
    printf("%s -> GAP\n", send_msg)
    true
  end
  def timeout1(send_msg)
    printf("TO1\n", send_msg)
    true
  end
  def timeout2(send_msg)
    printf("TO2\n", send_msg)
    true
  end
  def timeout3(send_msg)
    printf("TO3\n", send_msg)
    false
  end
end
class WaitReplyEventer
  def initilize
  end
  def exec(smon, msg, reply)
    printf("%s -> %s\n", msg, reply)
    #smon.send(reply, 0)
    smon.send(reply)
  end
  def gaptimer(msg)
  end
  def timeout1(msg)
    printf("TO1\n", msg)
    true
  end
  def timeout2(msg)
    printf("TO2\n", msg)
    true
  end
  def timeout3(msg)
    printf("TO3\n", msg)
  end
end
class Smon
  def self.send_wait(smon, send_msg, regs, cmd=nil)
    reply=''
    if nil == cmd then
      cmd = SendWaitEventer.new
    end
    #smon.send(send_msg, 0)
    smon.send(send_msg)
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
              cmd.exec(send_msg, msg)
              break
            end
          end
          if loop_enable then
            cmd.gaptimer(send_msg)
          end
          msg = ''
        when Smon::TO1 then
          reply='TO1'
          loop_enable = cmd.timeout1(send_msg)
        when Smon::TO2 then
          reply='TO2'
          loop_enable = cmd.timeout2(send_msg)
        when Smon::TO3 then
          reply='TO3'
          cmd.timeout3(send_msg)
          loop_enable = false
        else
          loop_enable = false
        end
      end
    end
    return reply
  end
  #def self.wait_send(smon, list, def_cmd=nil)
  def self.wait_send(smon, list, def_cmd=nil)
    if nil == def_cmd then
      def_cmd = WaitReplyEventer.new
    end
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
          list.each do |arg|
            ( reply, regs, cmd ) = arg
            if nil == cmd then
              cmd = def_cmd
            end
            regs.each do |reg|
              if Core.reg_match(msg, reg) then
                cmd.exec(smon, msg, reply)
                rcv_enable = false
                break
              end
            end
            if !rcv_enable then
              break;
            end
          end
          def_cmd.gaptimer(msg)
          msg = ''
        when Smon::TO1 then
          def_cmd.timeout1(msg)
        when Smon::TO2 then
          def_cmd.timeout2(msg)
        when Smon::TO3 then
          def_cmd.timeout3(msg)
          rcv_enable = false
        else
          rcv_enable = false
        end
      end
    end
  end
end
