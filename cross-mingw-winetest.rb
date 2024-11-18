MRuby::CrossBuild.new("cross-mingw") do |conf|
  conf.toolchain :gcc

  conf.host_target = "x86_64-w64-mingw32.static"
  conf.exts.executable = ".exe"

  conf.cc.command = "#{conf.host_target}-gcc"
  conf.cc.flags = [ '-static', '-Os', '-pipe' ]
  conf.cc.defines << %w(MRB_USE_THREAD_API MRB_USE_MUTEX_API MRB_USE_GVL_API MRB_USE_ATOMIC_API)

  conf.archiver.command = "#{conf.host_target}-gcc-ar"

  conf.linker.command   = conf.cc.command
  conf.linker.flags     += [ '-static', '-Os', '-pipe' ]
  #conf.linker.libraries += [ 'setupapi', 'ksguid', 'ole32', 'winmm', 'dsound', 'ws2_32', 'readline', 'tinfo' ]
  conf.linker.libraries += [ 'setupapi', 'ksguid', 'ole32', 'winmm', 'dsound', 'ws2_32', 'readline', 'ncurses' ]

  conf.test_runner do |t|
    thisdir = File.absolute_path( File.dirname(__FILE__) )
    t.command = File.join(thisdir, * %w{ helpers wine_runner.rb})
  end

#  conf.gem :mgem => 'mruby-onig-regexp'
#  conf.gem :mgem => 'mruby-posix-regexp'
#  conf.gem :github => 'mattn/mruby-thread'
#  conf.gembox 'default'
#  conf.gem :github => 'mattn/mruby-onig-regexp'
#  conf.gem :github => 'udzura/mruby-posix-regexp'
#  conf.gem :github => 'iij/mruby-regexp-pcre'
  conf.gembox "full-core"

  conf.enable_bintest
  conf.enable_test
end
