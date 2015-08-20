# link_directories() treats paths relative to the source dir.
cmake_policy(SET CMP0015 NEW)

# let cmake autolink dependencies on windows
cmake_policy(SET CMP0020 NEW)

# CMake 2.8.12 and lower allowed the use of targets and files with double
# colons in target_link_libraries,
cmake_policy(SET CMP0028 OLD)
    
# fix MACOSX_RPATH
cmake_policy(SET CMP0042 OLD)

# ignore COMPILE_DEFINITIONS_<Config> properties
cmake_policy(SET CMP0043 OLD)

# allow VERSION argument in project()
cmake_policy(SET CMP0048 NEW)
