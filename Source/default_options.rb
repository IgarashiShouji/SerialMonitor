# Defalult Option Script for smon.exe

class BinEdit
  def self.hexToArray(format, hex_string)
    bin = BinEdit.new(hex_string)
    return bin.get(format)
  end
  def self.hexFromArray(format, item)
    size = 0
    (format.length).times do |idx|
      m = format[idx, 1]
      size += (m == 'c' ? 1 : 0)
      size += (m == 'b' ? 1 : 0)
      size += (m == 's' ? 2 : 0)
      size += (m == 'w' ? 2 : 0)
      size += (m == 'i' ? 4 : 0)
      size += (m == 'd' ? 4 : 0)
      size += (m == 'f' ? 4 : 0)
      size += (m == 'S' ? 2 : 0)
      size += (m == 'W' ? 2 : 0)
      size += (m == 'I' ? 4 : 0)
      size += (m == 'D' ? 4 : 0)
      size += (m == 'F' ? 4 : 0)
    end
    bin = BinEdit.new(size)
    bin.set(format, item)
    return bin.dump
  end
  def self.readBinToXlsx(bin_file, xls_file)
    xls = OpenXLSX.new
    bin = BinEdit.new(sprintf("file:%s", bin_file))
    xls.open(xls_file) do
      list = Array.new
      xls.sheet_names do |sheet_name|
        printf("sheet name: %s\n", sheet_name)
        xls.sheet(sheet_name) do
          blank_cnt = 0
          list_type = Array.new
          address = nil; format = ''; row_top = nil;
          (4..0xffff).each do |row|
            addr = xls.cell(sprintf('B%d', row))
            name = xls.cell(sprintf('C%d', row))
            type = xls.cell(sprintf('D%d', row))
            if nil == type then
              if nil != address then
                list.push( [ row_top, address, bin.get(address, format), list_type, bin.pos ] )
              end
              address = nil; format = ''; row_top = nil
              list_type = Array.new
              blank_cnt += 1
              if 3 < blank_cnt then
                break;
              end
            else
              if nil != addr then
                if nil != address then
                  list.push( [ row_top, address, bin.get(address, format), list_type, bin.pos ] )
                  address = nil; format = ''; row_top = nil
                  list_type = Array.new
                end
                if addr =~ /0x/ then
                  address = (addr.gsub(/0x/, '')).to_i(16)
                else
                  address = addr.to_i;
                end
                row_top = row
              end
              if nil != address then
                add = ' '
                case type
                when 'uint8'  then; add = 'b';
                when  'int8'  then; add = 'c';
                when 'uint16' then; add = 'w';
                when  'int16' then; add = 's';
                when 'uint32' then; add = 'd';
                when  'int32' then; add = 'i';
                when 'UINT16' then; add = 'W';
                when  'INT16' then; add = 'S';
                when 'UINT32' then; add = 'D';
                when  'INT32' then; add = 'I';
                when  'float' then; add = 'f';
                when  'FLOAT' then; add = 'F';
                else
                  if    type =~ /^ASCII/ then
                    num = type.gsub(/^ASCII/, '')
                    num.gsub!(/[^0-9]/, '')
                    add = 'A' + num
                  elsif type =~ /^ascii/ then
                    num = type.gsub(/^ascii/, '')
                    num.gsub!(/[^0-9]/, '')
                    add = 'a' + num
                  elsif type =~ /^HEX/ then
                    num = type.gsub(/^HEX/, '')
                    num.gsub!(/[^0-9]/, '')
                    add = 'H' + num
                  elsif type =~ /^hex/ then
                    num = type.gsub(/^hex/, '')
                    num.gsub!(/[^0-9]/, '')
                    add = 'h' + num
                  else
                    list_type.push('-')
                    next;
                  end
                end
                format += add
                list_type.push(add)
              end
              blank_cnt = 0
            end
          end
          list.each do |row_top, address, data, types, next_addr|
            printf("%04X - %04X : ", address, next_addr); print data, "\n"
            idx = 0;
            types.each_with_index do |type, tidx|
              if 1 < types.length then
                item = data[idx]
              else
                item = data
              end
              cel = sprintf("E%d", row_top + tidx)
              if    type =~ /[bc]/ then
                xls.set_value(cel, item.to_i)
              elsif type =~ /[wsWS]/ then
                xls.set_value(cel, item.to_i)
              elsif type =~ /[diDI]/ then
                xls.set_value(cel, item.to_i)
              elsif type =~ /[fF]/ then
                xls.set_value(cel, item.to_f)
              elsif type =~ /[AHah]/ then
                xls.set_value(cel, sprintf("%s", item))
              else
                next;
              end
              idx += 1
            end
          end
        end
        list.clear()
        print "\n"
        true;
      end
      xls.save()
    end
  end
end

class Core
  def self.opt_send()
    print 'Date: ', Core.date(), "\n"
    tick = Core.tick()
    opts = Args.new()
    prn = WorkerThread.new
    if(0 < opts.size()) then
      port = opts[0]
      smon = Smon.new(port)
      if(1 < opts.size()) then
        (opts.size() - 1).times do |idx|
          data = opts[1+idx]
          data = CppRegexp.reg_replace(data, '[ _\-/@#(){}<>,\.]', '')
          smon.send(data, 0)
          tick += Core.tick() % 100000000000
          printf("%10d[ms]: Send: %s -- ", tick, data)
          rdata = ''
          interval = 0
          loop = true
          while loop do
            smon.wait do |state, rcv|
              now = Core.tick()
              tick += now % 100000000000
              case state
              when Smon::CACHE_FULL then
              when Smon::GAP then
                prn.synchronize do
                  if 0 < rcv.length then
                    rdata += rcv
                    interval += now
                    printf(" -- RCV", tick)
                  else
                    printf("GAP", tick)
                  end
                end
              when Smon::TO1 then
                prn.synchronize do
                  printf(", TO%d", state)
                end
              when Smon::TO2 then
                prn.synchronize do
                  printf(", TO%d", state)
                end
              when Smon::TO3 then
                prn.synchronize do
                  printf(", TO%d", state)
                end
                loop = false
              else
                loop = false
              end
            end
          end
          prn.synchronize do
            if 0 < rdata.length then
              printf(": (%d[ms]) --> %s\n", interval, rdata)
            else
              print " ... No Recived\n"
            end
          end
        end
      else
        reg = CppRegexp.new(['^[0-9a-fA-F _\-/@#(){}<>,\.]*$', '^tx:', '^quit'])
        th_rcv = WorkerThread.new
        th_rcv.run() do
          loop = true
          while loop do
            smon.wait do |state, rcv|
              tick += Core.tick() % 100000000000
              case state
              when Smon::CACHE_FULL then
              when Smon::GAP then
                prn.synchronize do
                  if 0 < rcv.length then
                    printf("%s: %10d[ms]: %s\n", port, tick, rcv)
                  else
                    printf("%s: %10d[ms]: GAP\n", port, tick)
                  end
                end
              when Smon::TO1 then
                prn.synchronize do
                  printf("%s: %10d[ms]: TO%d\n", port, tick, state)
                end
              when Smon::TO2 then
                prn.synchronize do
                  printf("%s: %10d[ms]: TO%d\n", port, tick, state)
                end
              when Smon::TO3 then
                prn.synchronize do
                  printf("%s: %10d[ms]: TO%d\n", port, tick, state)
                end
              else
                loop = false
                th_rcv.stop()
              end
            end
          end
        end
        loop = true
        while loop do
          str = Core.gets()
          cmd_idx = reg.select(str)
          case cmd_idx
          when 0
            data = CppRegexp.reg_replace(str, '[ _\-/@#(){}<>,\.]', '')
            if 0 < data.length then
              smon.send(data, 0)
              tick += Core.tick() % 100000000000
              prn.synchronize do
                printf("%s: %10d[ms]: Send: %s\n", port, tick, data)
              end
            end
          when 1
          when 2
            loop = false;
            break;
          else
          end
        end
      end
      smon.close()
    else
      print "smon [options] comxx data1 data2 ...", "\n"
      print "ex)\n"
      print "  ./smon.exe -1 com11,9600,odd,one,GAP=80,TO1=100,TO2=200,TO3=500 009912 9988 001123 00889987 9988 6787 9944 2211 7889\n"
      print "  ./smon -1 /dev/pts/6 '01 02_030405...060-70809' 00112233445566778899\n"
      print "\n"
    end
  end
  def self.__CheckOptions()
    opts = Args.new
    if nil == opts['mruby-script'] then
      if nil != opts['comlist'] then
        regs = Array.new
        (opts.size()).times do |idx|
          regs.push(opts[idx])
        end
        (Smon.comlist(regs)).each do |name|
          print name, "\n"
        end
        exit 0
      end
      if nil != opts['pipelist'] then
        regs = Array.new
        (opts.size()).times do |idx|
          regs.push(opts[idx])
        end
        (Smon.pipelist(regs)).each do |name|
          print name, "\n"
        end
        exit 0
      end
      if nil != opts['oneline'] then
        Core.opt_send()
        exit 0
      end
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
      if nil != opts['FLOAT'] then
        hex = opts['FLOAT']
        if 8 == hex.length then
          bin = BinEdit.new(hex)
          printf("%f: %s\n", bin.get(0, 'F'), hex);
        else
          print "not 8 charactor", "\n"
        end
        exit 0
      end
      if nil != opts['float'] then
        hex = opts['float']
        if 8 == hex.length then
          bin = BinEdit.new(hex)
          printf("%f: %s\n", bin.get(0, 'f'), hex);
        else
          print "not 8 charactor", "\n"
        end
        exit 0
      end
      if nil != opts['makeQR'] then
        printf("%s", Core.makeQR(opts['makeQR']));
        exit 0
      end
      if nil != opts['read-bin-to-xlsx'] then
        opts = Args.new()
        if 2 == opts.size() then
          if (File.exist?(opts[0]) && File.exist?(opts[1])) then
            if opts[1] =~ /xls/ then
              BinEdit.readBinToXlsx(opts[0], opts[1])
              print "end\n"
              exit 0
            end
          end
        end
        print "smon --read-bin-to-xlsx [binary file] [xlsx file]\n"
        print "\n"
        exit 0
      end
    end
  end
end
Core.__CheckOptions()

class SendWaitEventer
  def initialize
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
  def initialize
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
            if CppRegexp.reg_match(msg, reg) then
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
              if CppRegexp.reg_match(msg, reg) then
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

class CppRegexp
  def self.compile(arg)
    if arg.class.to_s == 'String' then
      return CppRegexp.new(arg)
    end
    return nil
  end
  def === (arg)
    begin
      if arg.class.to_s == 'String' then
        return self.match(str)
      end
    rescue
    end
    return false
  end
  def =~ (arg)
    begin
      if arg.class.to_s == 'String' then
        return self.match(str)
      end
    rescue
    end
    return false
  end
end
