AC_OUTPUT

if test "$enable_experimental_features" = "yes"; then
enable_experimental_text="
  Experimental support for future BSON features    : yes"
else
enable_experimental_text=""
fi

if test -n "$BSON_PRERELEASE_VERSION"; then
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
libbson $BSON_VERSION was configured with the following options:

Build configuration:
  Enable debugging (slow)                          : ${enable_debug}
  Enable extra alignment (required for 1.0 ABI)    : ${enable_extra_align}
  Compile with debug symbols (slow)                : ${enable_debug_symbols}
  Enable GCC build optimization                    : ${enable_optimizations}
  Enable automatic binary hardening                : ${enable_hardening}
  Code coverage support                            : ${enable_coverage}
  Cross Compiling                                  : ${enable_crosscompile}
  Big endian                                       : ${enable_bigendian}
  Compile with native _Decimal128 (BID) support    : ${enable_decimal}
  Link Time Optimization (experimental)            : ${enable_lto}${enable_experimental_text}

Documentation:
  man                                              : ${enable_man_pages}
  HTML                                             : ${enable_html_docs}
"
