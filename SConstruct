env = Environment()
env.Append(CPPFLAGS=['-std=c++0x'])
env.Append(LIBS=['Irrlicht', 'GL', 'freenect'])

build_dir = 'build'
SConscript('src/SConscript', variant_dir=build_dir, duplicate=0, exports=['env'])
Clean('.', build_dir)
