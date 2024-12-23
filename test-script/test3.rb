class Tester
  def initialize(_name)
    @idx = 0
    @name = _name
    @com = Smon.new(@name)
    @th = WorkerThread.new
    @reg = CppRegexp.new('^01', '^11', '^22')
    @data = ['11223344', '220011', '44556677']
  end
  def run(to_com)
    @th.run do
      bin = BinEdit.new
      state = @com.read_wait(bin)
      case state
      when Smon::CACHE_FULL then
      when Smon::GAP then
        if(0 < bin.length) then
          rcv_msg = bin.dump()
          idx = @reg.select(rcv_msg)
          printf("%-20s: %d/%d: %s\n", @name, idx, @data.length, rcv_msg)
          if idx < @data.length then
            send_data = @data[idx]
            send_data = sprintf("%s %s", @data[idx], rcv_msg)
            printf("%-20s: %d: send: %s\n", @name, idx, send_data)
            to_com.send(send_data )
          end
        end
      when Smon::TO1 then
        printf("%-20s: TO1\n", @name);
      when Smon::TO2 then
        printf("%-20s: TO2\n", @name);
      when Smon::TO3 then
        printf("%-20s: TO3\n", @name);
        @th.stop()
      when Smon::CLOSE  then
      when Smon::NONE then
        th.stop()
      else
        th.stop()
      end
    end
  end
  def send(data)
    @com.send(data, 0)
  end
  def close()
    @th.join()
  end
end

def test_smon()
  arg = Core.new()
  (arg.size()).times do |idx|
    printf("arg[%d] = %s\n", idx, arg[idx])
  end
  if 2 <= arg.size() then
    t0 = Tester.new(arg[0])
    t1 = Tester.new(arg[1])
    t0.run(t1)
    t1.run(t0)
    t0.send('01020304')
    t1.send('010203040506')
    t1.close()
    t0.close()
  else
    print "smon.exe -m test3.rb com1 com2"
  end
end

print "mruby test script 3\n"
test_smon()
print "mruby test script 3 end\n"
print "\n"
