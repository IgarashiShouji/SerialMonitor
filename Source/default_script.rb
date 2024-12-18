opts = Args.new()
if 0 < opts.size() then
  th_prn = WorkerThread.new
  objs = Array.new
  (opts.size()).times do |idx|
    th_ctrl = WorkerThread.new
    smon    = Smon.new( opts[idx] )
    objs.push( [ smon, th_ctrl, idx, opts[idx] ] )
    arg = opts[idx]
    th_ctrl.run() do
      msg = ''
      loop = true
      while loop do
        smon.wait do |state, rcv_msg|
          case state
          when Smon::CACHE_FULL then
          when Smon::GAP then
            th_prn.synchronize do
              if 0 < rcv_msg.length then
                printf("%d:%s: %s\n", idx, arg, rcv_msg)
              end
              printf("%d:%s: GAP\n", idx, arg)
            end
          when Smon::TO1 then
            th_prn.synchronize do
              printf("%d:%s: TO%d\n", idx, arg, state)
            end
          when Smon::TO2 then
            th_prn.synchronize do
              printf("%d:%s: TO%d\n", idx, arg, state)
            end
          when Smon::TO3 then
            th_prn.synchronize do
              printf("%d:%s: TO%d\n", idx, arg, state)
            end
          else
            loop = false
          end
        end
      end
    end
  end
  while true do
    str = Core.gets()
    if nil == str     then; break; end
    if 'quit' == str  then; break; end
    if 0 < str.length then
      idx = 0
      if Core.reg_match(str, ':') then
        idx = (Core.reg_replace(str, ':.*$', '')).to_i
      end
      msg = Core.reg_replace(str, '^.*:', '')
      ( smon, th_ctrl, idx_, arg ) = objs[idx]
      smon.send(msg, 0)
      printf("%d:%s: Send: %s\n", idx, arg, msg)
    else
      break
    end
  end
  objs.each do |items|
    ( smon, th_ctrl, idx, arg ) = items
    smon.close()
  end
  objs.each do |items|
    ( smon, th_ctrl, idx, arg ) = items
    th_ctrl.join()
  end
else
  print "smon [options] comXX comXX ...", "\n"
  print "see of help)\n"
  print "  smon --help\n"
  print "ex)\n"
  print "  smon -b 9600E1 -g 50 -t 100 --timer2 300 --timer3 500 com10 com11\n"
  print "\n"
end


class CppRegexp
  @memo = {}

  # ISO 15.2.15.6.1
  def self.compile(*args)
    as = args.to_s
    unless @memo.key? as
      @memo[as] = self.new(*args)
    end
    @memo[as]
  end

  # ISO 15.2.15.6.3
  def self.last_match
    @last_match
  end

  def self.last_match=(match)
    @last_match = match
  end

  # ISO 15.2.15.7.2
  def initialize_copy(other)
    initialize(other.source, other.options)
  end

  # ISO 15.2.15.7.4
  def ===(str)
    not self.match(str).nil?
  rescue TypeError
    false
  end

  # ISO 15.2.15.7.5
  def =~(str)
    m = self.match(str)
    m ? m.begin(0) : nil
  end

  # ISO 15.2.15.7.8
  attr_reader :source
end

#class string
#  # iso 15.2.10.5.5
#  def =~(a)
#    begin
##      (a.class.to_s == 'String' ?  Regexp.new(a.to_s) : a) =~ self
##      (a.class.to_s == 'String' ?  Core.reg_match(self, a) : false)
#      if a.class.to_s == 'String' then
#        if Core.reg_match(self, a) then
#          true
#        else
#          false
#        end
#      else
#        false
#      end
#    rescue
#      false
#    end
#  end
#
#  # ISO 15.2.10.5.27
#  def match(re, pos=0, &block)
#    re.match(self, pos, &block)
#  end
#
#
#  # redefine methods with oniguruma regexp version
#  %i[sub gsub split scan].each do |v|
#    alias_method :"string_#{v}", v if method_defined?(v)
#    alias_method v, :"onig_regexp_#{v}"
#  end
#
#  alias_method :match?, :onig_regexp_match?
#
#  alias_method :old_slice, :slice
#  alias_method :old_square_brancket, :[]
#  alias_method :old_square_brancket_equal, :[]=
#
#  def [](*args)
#    return old_square_brancket(*args) unless args[0].class == Regexp
#
#    if args.size == 2
#      match = args[0].match(self)
#      if match
#        if args[1] == 0
#          str = match[0]
#        else
#          str = match.captures[args[1] - 1]
#        end
#        return str
#      end
#    end
#
#    match_data = args[0].match(self)
#    if match_data
#      result = match_data.to_s
#      return result
#    end
#  end
#
#  alias_method :slice, :[]
#
#  def []=(*args)
#    return old_square_brancket_equal(*args) unless args[0].class == Regexp
#
#    n_args = args.size
#    case n_args
#    when 2
#      match = args[0].match(self)
#      beg = match.begin(0)
#      self[beg, match.end(0) - beg] = args[1]
#    when 3
#      match = args[0].match(self)
#      n = args[1]
#      beg = match.begin(n)
#      self[beg, match.end(n) - beg] = args[2]
#    else
#      raise ArgumentError, "wrong number of arguments (#{n_args} for 2..3)"
#    end
#
#    self
#  end
#
#  def slice!(*args)
#    if args.size < 2
#      result = slice(*args)
#      nth = args[0]
#
#      if nth.class == Regexp
#        lm = Regexp.last_match
#        self[nth] = '' if result
#        Regexp.last_match = lm
#      else
#        self[nth] = '' if result
#      end
#    else
#      result = slice(*args)
#
#      nth = args[0]
#      len = args[1]
#
#      if nth.class == Regexp
#        lm = Regexp.last_match
#        self[nth, len] = '' if result
#        Regexp.last_match = lm
#      else
#        self[nth, len] = '' if result && nth != self.size
#      end
#    end
#
#    result
#  end
#
#  alias_method :old_index, :index
#
#  def index(pattern, pos=0)
#    if pattern.class == Regexp
#      str = self[pos..-1]
#      if str
#        if num = (pattern =~ str)
#          if pos < 0
#            num += self.size
#          end
#          return num + pos
#        end
#      end
#      nil
#    else
#      self.old_index(pattern, pos)
#    end
#  end
#end
#
#module Kernel
#  def =~(_)
#    nil
#  end
#end
#
#Regexp = CppRegexp unless Object.const_defined?(:Regexp)
#MatchData = OnigMatchData unless Object.const_defined? :MatchData
#
## This is based on https://github.com/masamitsu-murase/mruby-hs-regexp
