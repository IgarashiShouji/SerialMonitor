class Tester
  def initialize(_name)
    @idx = 0
    @name = _name
    @com = Smon.new(@name)
    @th = WorkerThread.new
  end
  def run(to_com)
    reg = CppRegexp.new('^01', '^11', '^22')
    data = ['11223344', '220011', '44556677']
    @th.run do
      @com.wait do |state, rcv_msg|
        case state
        when Smon::CACHE_FULL then
        when Smon::GAP then
          if(0 < rcv_msg.length) then
            printf("%-20s: %s\n", @name, rcv_msg)
            idx = reg.select(rcv_msg)
            if idx < data.length then
              to_com.send(data[idx]+rcv_msg)
            end
          end
        when Smon::TO1 then
          printf("%-20s: TO1\n", @name);
        when Smon::TO2 then
          printf("%-20s: TO2\n", @name);
        when Smon::TO3 then
          printf("%-20s: TO3\n", @name);
          #if @idx < 3 then
            #to_com.send('01020304')
            #@idx += 1
          #else
            @th.stop()
          #end
        when Smon::CLOSE  then
        when Smon::NONE then
          th.stop()
        else
          th.stop()
        end
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
  opts = Args.new()
  (opts.size()).times do |idx|
    printf("opts[%d] = %s\n", idx, opts[idx])
  end
  if 2 <= opts.size() then
    t0 = Tester.new(opts[0])
    t1 = Tester.new(opts[1])
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
