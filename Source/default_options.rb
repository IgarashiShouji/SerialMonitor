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
    opts = Core.new()
    prn = WorkerThread.new
    if(0 < opts.length()) then
      port = opts[0]
      smon = Smon.new(port)
      if(1 < opts.length()) then
        (opts.length() - 1).times do |idx|
          data = opts[1+idx]
          data = CppRegexp.reg_replace(data, '[ _\-/@#(){}<>,\.]', '')
          smon.send(data, 0)
          tick += Core.tick() % 100000000000
          printf("%10d[ms]: Send: %s -- ", tick, data)
          rdata = ''
          interval = 0
          loop = true
          bin = BinEdit.new
          while loop do
            state = smon.read_wait(bin)
            now = Core.tick()
            tick += now % 100000000000
            case state
            when Smon::CACHE_FULL then
            when Smon::GAP then
              prn.synchronize do
                if 0 < bin.length then
                  rdata += bin.dump()
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
          prn.synchronize do
            if 0 < rdata.length then
              printf(": (%d[ms]) --> %s\n", interval, rdata)
            else
              print " ... No Recived\n"
            end
          end
        end
      else
        loop = true
        reg = CppRegexp.new(['^[0-9a-fA-F _\-/@#(){}<>,\.]*$', '^tx:', '^quit'])
        th_rcv = WorkerThread.new
        th_rcv.run(1) do
          bin = BinEdit.new
          while loop do
            state = smon.read_wait(bin)
            tick += Core.tick() % 100000000000
            case state
            when Smon::CACHE_FULL then
            when Smon::GAP then
              prn.synchronize do
                if 0 < bin.length then
                  printf("%s: %10d[ms]: %s\n", port, tick, bin.dump())
                  bin.resize(0)
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
              th_rcv.stop()
            end
          end
        end
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
  def self.bin_cmd_editor()
    bin = BinEdit.new()
    loop = true
    offset = 0
    list_cmd = Array.new
    list_reg = Array.new

    list_reg.push('^[ \t]*quit\b')
    cmd = Proc.new do |str|
      # quit command
      loop = false;
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*echo\b')
    cmd = Proc.new do |str|
      # echo command
      str = CppRegexp.reg_replace(str, '^[ \t]*echo[ \t]*', '')
      print str, "\n"
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*sum\b')
    cmd = Proc.new do |str|
      # sum command
      str = CppRegexp.reg_replace(str, '^[ \t]*sum[ \t]*', '')
      case str
      when 'dump'
        printf("%s%s\n", bin.dump, bin.sum)
      when 'make'
        bin = BinEdit.new([bin, bin.sum])
      else
        printf("%s: SUM(~+1)\n", bin.sum)
      end
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*xsum\b')
    cmd = Proc.new do |str|
      # xsum command
      str = CppRegexp.reg_replace(str, '^[ \t]*xsum[ \t]*', '')
      case str
      when 'dump'
        printf("%s%s\n", bin.dump, bin.xsum)
      when 'make'
        bin = BinEdit.new([bin, bin.xsum])
      else
        printf("%s: SUM(xor)\n", bin.xsum)
      end
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*[Cc][Rr][Cc]8\b')
    cmd = Proc.new do |str|
      # crc8 command
      str = CppRegexp.reg_replace(str, '^[ \t]*[Cc][Rr][Cc]8[ \t]*', '')
      case str
      when 'dump'
        printf("%s%s\n", bin.dump, bin.crc8)
      when 'make'
        bin = BinEdit.new([bin, bin.crc8])
      else
        printf("%s: CRC8\n", bin.crc8)
      end
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*[Cc][Rr][Cc]16\b')
    cmd = Proc.new do |str|
      # crc16 command
      str = CppRegexp.reg_replace(str, '^[ \t]*[Cc][Rr][Cc]16[ \t]*', '')
      case str
      when 'dump'
        printf("%s%s\n", bin.dump, bin.crc16)
      when 'make'
        bin = BinEdit.new([bin, bin.crc16])
      else
        printf("%s: CRC16\n", bin.crc16)
      end
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*[Cc][Rr][Cc]32\b')
    cmd = Proc.new do |str|
      # crc32 command
      str = CppRegexp.reg_replace(str, '^[ \t]*[Cc][Rr][Cc]32[ \t]*', '')
      case str
      when 'dump'
        printf("%s%s\n", bin.dump, bin.crc32)
      when 'make'
        bin = BinEdit.new([bin, bin.crc32])
      else
        printf("%s: CRC32\n", bin.crc32)
      end
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*dump\b')
    cmd = Proc.new do |str|
      # dump command
      def_dump = true;
      str = CppRegexp.reg_replace(str, '^[ \t]*dump[ \t]*', '')
      arg = CppRegexp.reg_split(str, '[ \t]+')
      if 0 < arg.length then
        crc = 'none'
        crc = (1 < arg.length ? arg[1] : crc)
        if CppRegexp.reg_match(arg[0], '[0-9,]+') then
          nums = CppRegexp.reg_split(arg[0], ',')
          num  = (1 < nums.length ? nums[1] : nums[0] ).to_i
          addr = (1 < nums.length ? nums[0].to_i(16) : 0)
          len  = bin.length
          sz = len - addr
          sz = (num < sz ? num : sz)
          while 0 < sz
            sub = BinEdit.new(bin, addr, sz)
            fmt = '%08X: %-' +  sprintf("%ds", num*2) + ' %s' + "\n"
            case crc
            when 'crc8'
              printf(fmt, offset + addr, sub.dump, sub.crc8)
            when 'crc16'
              printf(fmt, offset + addr, sub.dump, sub.crc16)
            when 'crc32'
              printf(fmt, offset + addr, sub.dump, sub.crc32)
            when 'sum'
              printf(fmt, offset + addr, sub.dump, sub.sum)
            when 'xsum'
              printf(fmt, offset + addr, sub.dump, sub.xsum)
            else
              printf("%08X: %s\n", offset + addr, sub.dump)
            end
            addr += sz
            sz = len - addr
            sz = (num < sz ? num : sz)
          end
          def_dump = false;
        end
      end
      if def_dump then
        printf("dump: %s\n", bin.dump)
      end
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*offset\b')
    cmd = Proc.new do |str|
      # set offset command
      str = CppRegexp.reg_replace(str, '^[ \t]*offset[ \t]*', '')
      hex = CppRegexp.reg_replace(str, '[ \t]', '')
      offset = (CppRegexp.reg_match(hex, '[0-9a-fA-F]+') ? hex.to_i(16) : offset)
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*len\b')
    cmd = Proc.new do |str|
      # len command
      printf("length: %d\n", bin.length)
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*clear\b')
    cmd = Proc.new do |str|
      # clear command
      bin = BinEdit.new()
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*save\b')
    cmd = Proc.new do |str|
      # save command
      fname = CppRegexp.reg_replace(str, '^[ \t]*save[ \t]*', '')
      bin.save(fname)
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*load\b')
    cmd = Proc.new do |str|
      # load command
      fname = CppRegexp.reg_replace(str, '^[ \t]*load[ \t]*', '')
      bin = BinEdit.new(sprintf("file:%s", fname))
    end
    list_cmd.push(cmd)

    list_reg.push('^[ \t]*#')
    cmd = Proc.new do |str|
      # Comment line
    end
    list_cmd.push(cmd)

    reg = CppRegexp.new(list_reg)
    while loop
      str = Core.gets()
      if nil == str then
        break
      end
      idx = reg.select(str)
      if idx < list_cmd.length then
        cmd = list_cmd[idx]
        cmd.call(str)
      else
        if 0 < str.length then
          temp = ''
          if CppRegexp.reg_match(str, '^fmt:[^:]+:') then
            list = CppRegexp.reg_split(str, ':')
            fmt = list[1]
            data = CppRegexp.reg_split(list[2], ',')
            arg = Array.new
            data.each_with_index do |val, idx|
              ch = fmt[idx]
              case ch
              when 'f', 'F' then
                arg.push(val.to_f)
              when 'c', 'b', 's', 'w', 'i', 'd', 'S', 'W', 'I', 'D' then
                arg.push(val.to_i)
              when 'a', 'A' then
              when 'h', 'H' then
              else
              end
            end
            temp = BinEdit.hexFromArray(fmt, arg)
          else
            str = CppRegexp.reg_replace(str, '^[0-9a-fA-f]+:', '')
            str = CppRegexp.reg_replace(str, '0x([0-9a-fA-F])', '$1')
            temp = BinEdit.new(str)
          end
          #printf("in: %s\n", temp.dump)
          bin = BinEdit.new( [ bin, temp ] )
        end
      end
    end
  end
  def self.checkOptions()
    Core.tick()
    opts = Core.new
    if nil == opts['mruby-script'] then
      if nil != opts['comlist'] then
        regs = Array.new
        (opts.length()).times do |idx|
          regs.push(opts[idx])
        end
        (Smon.comlist(regs)).each do |name|
          print name, "\n"
        end
        exit 0
      end
      if nil != opts['pipelist'] then
        regs = Array.new
        (opts.length()).times do |idx|
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
      if nil != opts['bin-edit'] then
        Core.bin_cmd_editor()
        exit 0
      end
      if nil != opts['crc'] then
        bin = BinEdit.new(opts['crc'])
        printf("%s: %s\n", bin.crc16(), opts['crc']);
        exit 0
      end
      if nil != opts['crc8'] then
        bin = BinEdit.new(opts['crc8'])
        printf("%s: %s\n", bin.crc8(), opts['crc8']);
        exit 0
      end
      if nil != opts['sum'] then
        str = opts['sum']
        if CppRegexp.reg_match(str, '^~:') then
          str = CppRegexp.reg_replace(str, '^~:', '')
          bin = BinEdit.new(str)
          printf("%s: %s\n", bin.sum, str);
        else
          bin = BinEdit.new(str)
          printf("%s: %s\n", bin.xsum, str);
        end
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
        opts = Core.new()
        if 2 == opts.length() then
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
    Core.tick()
  end
end
Core.checkOptions()

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
    smon.send(send_msg)
    msg = ''
    loop_enable = true
    bin = BinEdit.new()
    while loop_enable
      state = smon.read_wait(bin)
      if(0 < bin.length) then
        msg += bin.dump()
        bin.resize(0)
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
    return reply
  end
  def self.wait_send(smon, list, def_cmd=nil)
    if nil == def_cmd then
      def_cmd = WaitReplyEventer.new
    end
    rcv_enable = true
    msg = ''
    bin = BinEdit.new()
    while rcv_enable
      state = smon.read_wait(bin)
      if(0 < bin.length) then
        msg += bin.dump()
        bin.resize(0)
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

class SvgSequence
  def initialize()
    @title = ''
    @subtitle = ''
    @cnt  = 0
    @len  = 0
    @page = 0
    @x    = 280
    @y    = 110;
    @log = Array.new
    @height = 125
    @actor = Array.new
  end
  def setTitle(main, sub='')
    @title    = main
    @subtitle = sub
  end
  def setPage(p)
    @page = p
  end
  def length()
    return @cnt
  end
  def add_actor(name)
    @actor.push(name)
  end
  def act_life(height)
    act = ''
    @actor.each_with_index do |name, idx|
      act += '   <rect fill="none"  x ="' + sprintf("%d",  5 + idx*190) + '"           y = "5"   rx="15" ry="15" width="140" height="45" stroke="black" stroke-width="3"/>' + "\n"
      act += '   <text fill="black" x ="' + sprintf("%d", 75 + idx*190) + '"           y ="35"          stroke="black" stroke-width="0" font-family="serif" font-size="24"  text-anchor="middle" xml:space="preserve">' + name + "</text>\n"
      act += '   <line fill="none"  x1="' + sprintf("%d", 75 + idx*190) + '"  x2="' + sprintf("%d", 75 + idx*190) + '"  y1="55"  y2="' +  sprintf("%d", height) +  '" stroke="black" stroke-width="1" stroke-dasharray="5"/>' + "\n"
      @x = (idx * 190) + 75 + 15
    end
    return act
  end
  def log_data(tick='', data='', st=0, ed=1, msg='', fmt='', arg1='', arg2='', arg3='')
    @log.push(["send", [tick, data, st, ed, msg, fmt, arg1, arg2, arg3]])
    @cnt += 1
    if @len < data.length then
      @len = data.length
    end
  end
  def inteval(val)
    @log.push(['interval', val])
  end
  def hedder()
    height = 125 + @cnt * 50
    svg_width  = (@actor.length * 280) + @len * 6
    head = sprintf("<!-- len(%d), cnt(%d) -->\n", @len, @cnt)
    head += '<svg width="' + sprintf("%d", svg_width) + '" height="' + sprintf("%d", height+5) + '" xmlns="http://www.w3.org/2000/svg" xmlns:svg="http://www.w3.org/2000/svg">' +  "\n"
    head += '  <defs>' + "\n"
    head += '   <marker id="arrow-black" markerHeight="5" markerUnits="strokeWidth" markerWidth="5" orient="auto" refX="50" refY="50" se_type="rightarrow" viewBox="0 0 100 100"> <path d="m100,50l-100,40l30,-40l-30,-40z" fill="black" stroke="black" stroke-width="10"/> </marker>' + "\n"
    head += '   <marker id="arrow-limegreen" markerHeight="5" markerUnits="strokeWidth" markerWidth="5" orient="auto" refX="50" refY="50" se_type="rightarrow" viewBox="0 0 100 100"> <path d="m100,50l-100,40l30,-40l-30,-40z" fill="limegreen" stroke="limegreen " stroke-width="10"/> </marker>' + "\n"
    head += '   <marker id="arrow-red"   markerHeight="5" markerUnits="strokeWidth" markerWidth="5" orient="auto" refX="50" refY="50" se_type="rightarrow" viewBox="0 0 100 100"> <path d="m100,50l-100,40l30,-40l-30,-40z" fill="red" stroke="red" stroke-width="10"/> </marker>' + "\n"
    head += '  </defs>' + "\n"
    head += '  <g class="layer">' + "\n"
    head += '   <title>Communication Log</title>' + "\n"
    head += '   <text fill="black" x ="' + sprintf("%d", (200+@actor.length*140)) + '" y ="30" stroke="black" stroke-width="0" font-family="serif" font-size="24"  text-anchor="start" xml:space="preserve">' + @title + "</text>\n"
    head += '   <text fill="black" x ="' + sprintf("%d", (200+@actor.length*140)) + '" y ="50" stroke="black" stroke-width="0" font-family="serif" font-size="18"  text-anchor="start" xml:space="preserve">' + @subtitle + "</text>\n"
    head += '   <!-- actor and life line -->' + "\n"
    head += act_life(height)
    head += '   <!--  <rect fill="white"   x="5"             y ="65" width="270" height="12" stroke="white" stroke-width="0"/> -->' + "\n"
    if 0 < @page then
      head += '   <text fill="blue"    x="170"           y ="73"                         stroke="blue"  stroke-width="0" font-family="Monospace" font-size="12" text-anchor="middle" xml:space="preserve"> -------- Page ' +  sprintf("%04d", @page) + ' -------- </text>' + "\n"
    end
    head += "\n"
    return head
  end
  def prn_data(tick, data, send, recv, msg, fmt, arg1, arg2, arg3)
    svg = sprintf("   <!-- log_data: %8d[ms]: %s -->\n", tick, data)
    if send < recv then
      svg += '   <line fill="none"      x1="' + sprintf("%d", (75+send*190)) + '"   y1="' + sprintf("%d", @y)   + '"  x2="' + sprintf("%d", ((65+recv*190))) + '" y2="' + sprintf("%d", @y) + '" stroke="black" stroke-width="3" marker-end="url(#arrow-black)"/>' + "\n"
      svg += '   <text fill="black"     x ="' + sprintf("%d", (75+send*190)+((recv-send)*190)/2) + '"  y ="' + sprintf("%d", @y-5) +  '"   stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="middle" xml:space="preserve">' + sprintf("%s", msg) + '</text>' + "\n"
    else
      if 0 < data.length then
        svg += '   <line fill="none"      x1="' + sprintf("%d", (75+send*190 - 10)) + '"  y1="' + sprintf("%d", @y) +   '"  x2="' + sprintf("%d", (75+recv*190 + 10)) + '" y2="' + sprintf("%d", @y) + '" stroke="limegreen" stroke-width="2" stroke-dasharray="10" marker-end="url(#arrow-limegreen)"/>' + "\n"
      else
        svg += '   <line fill="none"      x1="' + sprintf("%d", (75+send*190 - 10)) + '"  y1="' + sprintf("%d", @y) +   '"  x2="' + sprintf("%d", (75+recv*190 + 10)) + '" y2="' + sprintf("%d", @y) + '" stroke="red" stroke-width="2" stroke-dasharray="10" marker-end="url(#arrow-red)"/>' + "\n"
      end
      svg += '   <text fill="black"     x ="' + sprintf("%d", ((75+recv*190)+((send-recv)*190)/2)) + '"  y ="' + sprintf("%d", @y-5) +  '"   stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="middle" xml:space="preserve">' + sprintf("%s", msg) + '</text>' + "\n"
    end
    if 0 < tick then
      svg += '   <text fill="black"     x ="5"    y ="' + sprintf("%d", @y+6) + '"  stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="left"   xml:space="preserve"> ' + sprintf("%6d", tick) + '[ms]</text> ' + "\n"
    end
    x = @x
    fg_b = true; fg_w = true; fg_d = true;
    reg_b = CppRegexp.new('[cbCB]')
    reg_w = CppRegexp.new('[wsWS]')
    reg_d = CppRegexp.new('[idfIDF]')
    (fmt.chars()).each do |c|
      if    reg_b.match(c) then
        if fg_b then
          svg += '   <rect fill="magenta"   x ="' + sprintf("%d", x) + '"  y ="' + sprintf("%d", @y-5) + '"   width="12" height="13" stroke="none" stroke-width="0"/>' + "\n"; x = x + 12; fg_b = false;
        else
          svg += '   <rect fill="yellow"    x ="' + sprintf("%d", x) + '"  y ="' + sprintf("%d", @y-5) + '"   width="12" height="13" stroke="none" stroke-width="0"/>' + "\n"; x = x + 12; fg_b = true;
        end
      elsif reg_w.match(c) then
        if fg_w then
          svg += '   <rect fill="orange"    x ="' + sprintf("%d", x) + '"  y ="' + sprintf("%d", @y-5) + '"   width="24" height="13" stroke="none" stroke-width="0"/>' + "\n"; x = x + 24; fg_w = false;
        else
          svg += '   <rect fill="lightgray" x ="' + sprintf("%d", x) + '"  y ="' + sprintf("%d", @y-5) + '"   width="24" height="13" stroke="none" stroke-width="0"/>' + "\n"; x = x + 24; fg_w = true;
        end
      elsif reg_d.match(c) then
        if fg_d then
          svg += '   <rect fill="lime"      x ="' + sprintf("%d", x) + '"  y ="' + sprintf("%d", @y-5) + '"   width="48" height="13" stroke="none" stroke-width="0"/>' + "\n"; x = x + 48; fg_d = false;
        else
          svg += '   <rect fill="cyan"      x ="' + sprintf("%d", x) + '"  y ="' + sprintf("%d", @y-5) + '"   width="48" height="13" stroke="none" stroke-width="0"/>' + "\n"; x = x + 48; fg_d = true;
        end
      else
      end
    end
    svg += '   <text fill="black"     x ="' + sprintf("%d", @x) + '"  y ="' + sprintf("%d", @y+6) +    '"  stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="left"   xml:space="preserve">' + sprintf("%s", data) + '</text>' + "\n"
    svg += '   <text fill="black"     x ="' + sprintf("%d", @x) + '"  y ="' + sprintf("%d", @y+6+12) + '"  stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="left"   xml:space="preserve">' + sprintf("%s", arg1) + '</text>' + "\n"
    svg += '   <text fill="black"     x ="' + sprintf("%d", @x) + '"  y ="' + sprintf("%d", @y+6+24) + '"  stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="left"   xml:space="preserve">' + sprintf("%s", arg2) + '</text>' + "\n"
    svg += '   <text fill="black"     x ="' + sprintf("%d", @x) + '"  y ="' + sprintf("%d", @y+6+36) + '"  stroke="black" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="left"   xml:space="preserve">' + sprintf("%s", arg3) + '</text>' + "\n"
    @y = @y + 50
    return svg
  end
  def fotter()
    tail = '   <!-- Page Lavel -->' + "\n"
    if 0 < @page then
      tail += '   <text fill="blue"    x="170"           y="' +  sprintf("%d", @y+12) + '"                         stroke="blue"  stroke-width="0" font-family="Monospace" font-size="12" text-anchor="middle" xml:space="preserve"> -------- Next ' + sprintf("%04d", @page+1) + ' -------- </text>' + "\n"
    end
    tail += '  </g>' + "\n"
    tail += '</svg>' + "\n"
    return tail
  end
  def body()
    body = ''
    @log.each do |item|
      (cmd, arg) = item
      case cmd
      when 'send'
        (tick, data, send, recv, msg, fmt, arg1, arg2, arg3) = arg
        body += prn_data(tick, data, send, recv, msg, fmt, arg1, arg2, arg3)
      when 'interval'
        val = arg
        body += '   <!-- interval: +' + sprintf("%d[ms]", val) + ' -->' + "\n"
        body += '   <rect fill="white"    x="5"             y ="' + sprintf("%d",(@y+20-50)) + '" width="100" height="12" stroke="white"    stroke-width="0"/>' + "\n"
        body += '   <text fill="DeepPink" x="75"            y ="' + sprintf("%d",(@y+32-50)) + '"                         stroke="DeepPink" stroke-width="0" font-family="Monospace" font-size="12" text-anchor="middle" xml:space="preserve">+' + sprintf("%d[ms]", val) + '</text>' + "\n"
      else
      end
    end
    return body
  end
end
