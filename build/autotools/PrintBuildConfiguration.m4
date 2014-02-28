AC_OUTPUT

echo "
libbson was configured with the following options:

Build configuration:
  Enable debugging (slow)                          : ${enable_debug}
  Compile with debug symbols (slow)                : ${enable_debug_symbols}
  Developer mode                                   : ${enable_developer_mode}
  Enable GCC build optimization                    : ${enable_optimizations}
  Code coverage support                            : ${enable_coverage}
  Optimized memory allocator                       : ${enable_fast_malloc}
  Big endian                                       : ${enable_bigendian}
  Cross Compiling                                  : ${enable_crosscompile}

Documentation:
  Generate man pages                               : ${enable_manpages}

Bindings:
  Python (experimental)                            : ${enable_python}
"
