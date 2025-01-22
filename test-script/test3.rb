class Tester
  def initialize(port)
    @idx = 0
    @name = port
    @com = Smon.new(@name)
    @th = WorkerThread.new
    @reg = CppRegexp.new('^01', '^11', '^22')
    @res = ['11223344', '220011', '44556677']
  end
  def run(to_com)
    @th.start(1) do
      bin = BinEdit.new
      loop = true
      while loop do
        state = @com.read_wait(bin)
        case state
        when Smon::CACHE_FULL then
        when Smon::GAP then
          if(0 < bin.length) then
            data = bin.dump()
            idx = @reg.select(data)
            printf("%-20s: %d/%d: %s\n", @name, idx, @res.length, data)
            if idx < @res.length then
              send_data = @res[idx]
              send_data = sprintf("%s %s", @res[idx], data)
              printf("%-20s: %d: send: %s\n", @name, idx, send_data)
              to_com.send(send_data)
            end
          else
          end
        when Smon::TO1, Smon::TO2 then
          printf("%-20s: TO%d\n", @name, state);
        when Smon::TO3 then
          printf("%-20s: TO%d\n", @name, state);
          loop = false;
          @th.stop()
        else
          loop = false;
        end
      end
    end
  end
  def send(data)
    @com.send(data)
  end
  def close()
    @th.join()
  end
end

def test_smon()
  arg = Core.args()
  (arg.length()).times do |idx|
    printf("arg[%d] = %s\n", idx, arg[idx])
  end
  if 2 <= arg.length() then
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
