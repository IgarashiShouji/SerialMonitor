# MODBUS slave mruby script on smon

class ModbusSlave
  def initialize(port, fname)
    @port  = port
    @fname = fname
    @th_check = WorkerThread.new
    @th_smon  = WorkerThread.new
    @smon     = Smon.new(port)
    @timestamp = Core.timestamp(@fname)
    @enable_check_file = true
    @regs = loadFile(@fname)
    printRegisters(@regs)
  end
  def loadFile(fname)
    regs = Hash.new
    items   = Array.new
    addr_start = nil;
    reg_cnt    = 0;
    xls = OpenXLSX.new
    xls.open(@fname) do
      [ 'Input Registers', 'Hold Registers' ].each do |sheet_name|
        xls.sheet(sheet_name) do
          blank_cnt = 0
          (4..(0xffff+4)).each do |idx|
            if 3 < blank_cnt then
              break;
            end
            cel_name = sprintf("B%d", idx); addr = xls.cell(cel_name)
            cel_name = sprintf("C%d", idx); name = xls.cell(cel_name)
            cel_name = sprintf("D%d", idx); type = xls.cell(cel_name)
            cel_name = sprintf("E%d", idx); num  = xls.cell(cel_name)
            case type
            when 'uint16' then reg_cnt += 1
            when 'int16'  then reg_cnt += 1
            when 'uint32' then reg_cnt += 2
            when 'int32'  then reg_cnt += 2
            when 'float'  then reg_cnt += 2
            else
              blank_cnt += 1
              if nil != addr_start then
                key = sprintf("%s: %04d(%d) - %04d", sheet_name, addr_start, reg_cnt, addr_start + reg_cnt)
                regs[key] = items
              end
              items      = Array.new
              addr_start = nil;
              reg_cnt    = 0;
              next;
            end
            if nil == addr then next; end
            if nil == name then next; end
            if nil == num  then next; end
            if nil == addr_start then
              addr_start = addr
            end
            items.push( [ addr, name, type, num ] )
            blank_cnt = 0
          end
        end
      end
    end
    return regs
  end
  def printRegisters(regs)
    (regs.keys()).each do |key|
      print key, "\n"
      (regs[key]).each do |addr, name, type, num|
        if "float" != type then
          printf("  %04d: %-25s: %-8s: %d\n", addr, name, type, num)
        else
          printf("  %04d: %-25s: %-8s: %f\n", addr, name, type, num)
        end
      end
    end
  end
  def watchReload()
    reload_cnt = 0
    @th_check.run do
      WorkerThread.ms_sleep( 1000 )
      while @enable_check_file do
        WorkerThread.ms_sleep( 1000 )
        timestamp_temp = Core.timestamp(@fname)
        if timestamp_temp != @timestamp then
          printf("reload file(%d)\n", reload_cnt)
          temp = loadFile(@fname)
          @th_check.synchronize do
            printRegisters(temp)
            print "\n"
            @regs = temp
          end
          reload_cnt += 1
          @timestamp = timestamp_temp
        end
      end
    end
  end
  def wachSerialPort()
    @th_smon.run() do
      enable = true;
      while enable do
        @smon.wait do |state, rcv_msg|
          if(0 < rcv_msg.length) then
            fc = Core.reg_replace(rcv_msg, '^..(..).*$', '$1')
            if nil != fc then
              @th_check.synchronize do
                 case fc
                 when '03' then
                   printf("%s: %s : ", @port, rcv_msg)
                   addr = Core.reg_replace(rcv_msg, '^.{4}(.{4}).*$', '$1');     addr = addr.to_i(16)
                   cnt  = Core.reg_replace(rcv_msg, '^.{4}.{4}(.{4}).*$', '$1'); cnt  = cnt.to_i(16)
                   (@ir_hr.keys).each do |key|
                     search = sprintf("Hold Registers: .*%04d", addr);
                     if Core.reg_match(key, search) then
                       replay = Core.reg_replace(rcv_msg, '^(.{4}).*$', '$1')
                       replay += sprintf("%02X", (cnt*2))
                       reg_cnt = 0
                       (@ir_hr[key]).each do |addr, name, type, num|
                         if reg_cnt < cnt then
                           replay += Core.to_hex(type, num)
                         end
                         case type
                         when 'uint16' then reg_cnt += 1
                         when 'int16'  then reg_cnt += 1
                         when 'uint32' then reg_cnt += 2
                         when 'int32'  then reg_cnt += 2
                         when 'float'  then reg_cnt += 2
                         else
                         end
                       end
                       replay = replay + Core.crc16(replay)
                       printf("Read HR:%04d(%d) -> %s\n", addr, cnt, replay)
                       @smon.send(replay, 0)
                     end
                   end
                 when '04' then
                   printf("%s: %s : ", @port, rcv_msg)
                   addr = Core.reg_replace(rcv_msg, '^.{4}(.{4}).*$', '$1');     addr = addr.to_i(16)
                   cnt  = Core.reg_replace(rcv_msg, '^.{4}.{4}(.{4}).*$', '$1'); cnt  = cnt.to_i(16)
                   (@ir_hr.keys).each do |key|
                     search = sprintf("Input Registers: .*%04d", addr);
                     if Core.reg_match(key, search) then
                       replay = Core.reg_replace(rcv_msg, '^(.{4}).*$', '$1')
                       replay += sprintf("%02X", (cnt*2))
                       reg_cnt = 0
                       (@ir_hr[key]).each do |addr, name, type, num|
                         if reg_cnt < cnt then
                           replay += Core.to_hex(type, num)
                         end
                         case type
                         when 'uint16' then reg_cnt += 1
                         when 'int16'  then reg_cnt += 1
                         when 'uint32' then reg_cnt += 2
                         when 'int32'  then reg_cnt += 2
                         when 'float'  then reg_cnt += 2
                         else
                         end
                       end
                       replay = replay + Core.crc16(replay)
                       printf("Read IR:%04d(%d) -> %s\n", addr, cnt, replay)
                       @smon.send(replay, 0)
                     end
                   end
                 else
                   printf("%s: %s\n", @port, rcv_msg)
                 end
              end
            end
          end
          case state
          when Smon::CACHE_FULL then
          when Smon::GAP        then
          when Smon::TO1        then
          when Smon::TO2        then
          when Smon::TO3        then
          when Smon::CLOSE      then enable = false
          when Smon::NONE       then enable = false
          else                       enable = false
          end
        end
      end
    end
  end
  def main
    str = Core.gets()
    @smon.close()
    @enable_check_file = false
    @th_smon.join()
    @th_check.join
  end
end

def createTestFile(fname)
  xls = OpenXLSX.new
  xls.create(fname) do
    xls.sheet("Sheet1") do
      xls.setSheetName('Input Registers')
      list = [ [ [ 'B3',  'C3',  'D3',  'E3' ], ['Address', 'Item Name',             'Type',   'Data'] ],
               [ [ 'B4',  'C4',  'D4',  'E4' ], [        0, 'int data',              'uint16',     9 ] ],
               [ [ 'B5',  'C5',  'D5',  'E5' ], [        1, 'float data',            'float',    1.2 ] ],
               [ [ 'B6',  'C6',  'D6',  'E6' ], [        3, '32 bit data(unsigned)', 'uint32',   100 ] ],
               [ [ 'B7',  'C7',  'D7',  'E7' ], [        5, 'float data',            'float',  0.005 ] ],
               [ [ 'B9',  'C9',  'D9',  'E9' ], [      100, 'int 16 data',           'int16',     20 ] ],
               [ ['B10', 'C10', 'D10', 'E10' ], [      101, 'float data',            'float',  100.3 ] ],
               [ ['B11', 'C11', 'D11', 'E11' ], [      103, 'int 32 data',           'int32',  12345 ] ] ]
      list.each do |item|
        ( cels, values ) = item;
        cels.each_with_index do |cel_name,idx|
          xls.set_value(cel_name, values[idx])
        end
      end
    end
    xls.sheet('Hold Registers') do
      list = [ [ [ 'B3',  'C3',  'D3',  'E3' ], ['Address', 'Item Name',            'Type',   'Data'] ],
               [ [ 'B4',  'C4',  'D4',  'E4' ], [       0, 'unsigned int data',     'uint16',    10 ] ],
               [ [ 'B5',  'C5',  'D5',  'E5' ], [       1, 'float data',            'float',  0.005 ] ],
               [ [ 'B6',  'C6',  'D6',  'E6' ], [       3, '32 bit data(unsigned)', 'uint32',     4 ] ],
               [ [ 'B7',  'C7',  'D7',  'E7' ], [       5, 'float data',            'float',    5.3 ] ],
               [ [ 'B9',  'C9',  'D9',  'E9' ], [     100, 'int 16 data',           'int16',      4 ] ],
               [ ['B10', 'C10', 'D10', 'E10' ], [     101, 'float data',            'float',  50.06 ] ],
               [ ['B11', 'C11', 'D11', 'E11' ], [     103, 'int 32 data',           'int32',   1000 ] ] ]
      list.each do |item|
        ( cels, values ) = item;
        cels.each_with_index do |cel_name,idx|
          xls.set_value(cel_name, values[idx])
        end
      end
    end
  end
end

opts = Args.new()
printf("opts.size: %d\n", opts.size())
if 0 < opts.size() then
  port  = opts[0]
  fname = ""
  if 1 < opts.size() then
    fname = opts[1]
  else
    fname = 'test-modbus.xlsx'
    if !Core.exists(fname) then
      createTestFile(fname)
      exit 0
    end
  end
  mslave = ModbusSlave.new(port, fname)
  mslave.watchReload()
  mslave.wachSerialPort()
  mslave.main()
end
