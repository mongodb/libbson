AC_OUTPUT

if test $(( ${BSON_MINOR_VERSION} % 2 )) -eq 1; then
cat << EOF
 *** IMPORTANT *** 

 This is an unstable version of libbson.
 It is for test purposes only.

 Please, DO NOT use it in a production environment.
 It will probably crash and you will lose your data.

 Additionally, the API/ABI may change during the course
 of development.

 Thanks,

   The libbson team.

 *** END OF WARNING ***

EOF
fi

echo "
libbson was configured with the following options:

Build configuration:
  Enable debugging (slow)                          : ${enable_debug}
  Compile with debug symbols (slow)                : ${enable_debug_symbols}
  Developer mode                                   : ${enable_developer_mode}
  Enable GCC build optimization                    : ${enable_optimizations}
  Use -Bsymbolic                                   : ${enable_Bsymbolic}
  Code coverage support                            : ${enable_coverage}
  Optimized memory allocator                       : ${enable_fast_malloc}
  Big endian                                       : ${enable_bigendian}
  Cross Compiling                                  : ${enable_crosscompile}

Documentation:
  Generate man pages                               : ${enable_manpages}

Bindings:
  Python (experimental)                            : ${enable_python}
"
