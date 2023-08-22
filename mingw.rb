MRuby::CrossBuild.new("cross-mingw-winetest") do |conf|
  conf.toolchain :gcc

#  conf.gem :mgem => 'mruby-posix-regexp'

  # Ubuntu 20
  conf.cc.command = "gcc"
  conf.cc.defines << %w(MRB_USE_THREAD_API MRB_USE_MUTEX_API MRB_USE_GVL_API MRB_USE_ATOMIC_API)

  conf.linker.command = conf.cc.command
  conf.archiver.command = "ar"
  conf.exts.executable = ".exe"

  # By default, we compile as static as possible to remove runtime
  # MinGW dependencies; they are probably fixable but it gets
  # complicated.
  conf.cc.flags = ['-static']
  conf.linker.flags += ['-static']

  conf.test_runner do |t|
    thisdir = File.absolute_path( File.dirname(__FILE__) )
    t.command = File.join(thisdir, * %w{ helpers wine_runner.rb})
  end

  conf.gembox "full-core"

  conf.enable_bintest
  conf.enable_test
end
