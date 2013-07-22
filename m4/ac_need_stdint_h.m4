dnl @synopsis AC_NEED_STDINT_H [( HEADER-TO-GENERATE [, HEDERS-TO-CHECK])]
dnl
dnl obsoleted -  superseded by AC_CREATE_STDINT_H
dnl
dnl the "ISO C9X: 7.18 Integer types <stdint.h>" section requires the
dnl existence of an include file <stdint.h> that defines a set of 
dnl typedefs, especially uint8_t,int32_t,uintptr_t.
dnl Many older installations will not provide this file, but some will
dnl have the very same definitions in <inttypes.h>. In other enviroments
dnl we can use the inet-types in <sys/types.h> which would define the
dnl typedefs int8_t and u_int8_t respectivly.
dnl
dnl This macros will create a local "stdint.h" if it cannot find the
dnl global <stdint.h> (or it will create the headerfile given as an argument).
dnl In many cases that file will just have a singular "#include <inttypes.h>"
dnl statement, while in other environments it will provide the set of basic
dnl stdint's defined: 
dnl int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t,intptr_t,uintptr_t
dnl int_least32_t.. int_fast32_t.. intmax_t
dnl which may or may not rely on the definitions of other files,
dnl or using the AC_COMPILE_CHECK_SIZEOF macro to determine the actual
dnl sizeof each type.
dnl
dnl if your header files require the stdint-types you will want to create an
dnl installable file package-stdint.h that all your other installable header
dnl may include. So if you have a library package named "mylib", just use
dnl      AC_NEED_STDINT(zziplib-stdint.h) 
dnl in configure.in and go to install that very header file in Makefile.am
dnl along with the other headers (mylib.h) - and the mylib-specific headers
dnl can simply use "#include <mylib-stdint.h>" to obtain the stdint-types.
dnl
dnl Remember, if the system already had a valid <stdint.h>, the generated
dnl file will include it directly. No need for fuzzy HAVE_STDINT_H things...
dnl
dnl @version $Id: ac_need_stdint_h.m4,v 1.4 2003/02/02 19:32:26 guidod Exp $
dnl @author  Guido Draheim <guidod@gmx.de>

AC_DEFUN([AC_NEED_STDINT_H],
[AC_MSG_CHECKING([for stdint-types])
 ac_cv_header_stdint="no-file"
 ac_cv_header_stdint_u="no-file"
 for i in $1 inttypes.h sys/inttypes.h sys/int_types.h stdint.h ; do
   AC_CHECK_TYPEDEF_(uint32_t, $i, [ac_cv_header_stdint=$i])
 done
 for i in $1 sys/types.h inttypes.h sys/inttypes.h sys/int_types.h ; do
   AC_CHECK_TYPEDEF_(u_int32_t, $i, [ac_cv_header_stdint_u=$i])
 done
 dnl debugging: __AC_MSG( !$ac_cv_header_stdint!$ac_cv_header_stdint_u! ...)

 ac_stdint_h=`echo ifelse($1, , stdint.h, $1)`
 if test "$ac_cv_header_stdint" != "no-file" ; then
   if test "$ac_cv_header_stdint" != "$ac_stdint_h" ; then
     AC_MSG_RESULT(found in $ac_cv_header_stdint)
     echo "#include <$ac_cv_header_stdint>" >$ac_stdint_h
     AC_MSG_RESULT(creating $ac_stdint_h - (just to include  $ac_cv_header_stdint) )
   else
     AC_MSG_RESULT(found in $ac_stdint_h)
   fi
   ac_cv_header_stdint_generated=false
 elif test "$ac_cv_header_stdint_u" != "no-file" ; then
   AC_MSG_RESULT(found u_types in $ac_cv_header_stdint_u)
   if test $ac_cv_header_stdint = "$ac_stdint_h" ; then
     AC_MSG_RESULT(creating $ac_stdint_h - includes $ac_cv_header_stdint, expect problems!)
   else
     AC_MSG_RESULT(creating $ac_stdint_h - (include inet-types in $ac_cv_header_stdint_u and re-typedef))
   fi
echo "#ifndef __AC_STDINT_H" >$ac_stdint_h
echo "#define __AC_STDINT_H" '"'$ac_stdint_h'"' >>$ac_stdint_h
echo "#ifndef _GENERATED_STDINT_H" >>$ac_stdint_h
echo "#define _GENERATED_STDINT_H" '"'$PACKAGE $VERSION'"' >>$ac_stdint_h
   cat >>$ac_stdint_h <<EOF

#include <stddef.h>
#include <$ac_cv_header_stdint_u>

/* int8_t int16_t int32_t defined by inet code */
typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;

/* it's a networkable system, but without any stdint.h */
/* hence it's an older 32-bit system... (a wild guess that seems to work) */
typedef u_int32_t uintptr_t;
typedef   int32_t  intptr_t;
EOF
   ac_cv_header_stdint_generated=true
 else
   AC_MSG_RESULT(not found, need to guess the types now... )
   AC_COMPILE_CHECK_SIZEOF(long, 32)
   AC_COMPILE_CHECK_SIZEOF(void*, 32)
   AC_MSG_RESULT( creating $ac_stdint_h - using detected values for sizeof long and sizeof void* )
echo "#ifndef __AC_STDINT_H" >$ac_stdint_h
echo "#define __AC_STDINT_H" '"'$ac_stdint_h'"' >>$ac_stdint_h
echo "#ifndef _GENERATED_STDINT_H" >>$ac_stdint_h
echo "#define _GENERATED_STDINT_H" '"'$PACKAGE $VERSION'"' >>$ac_stdint_h
   cat >>$ac_stdint_h <<EOF

/* ISO C 9X: 7.18 Integer types <stdint.h> */

#define __int8_t_defined  
typedef   signed char    int8_t;
typedef unsigned char   uint8_t;
typedef   signed short  int16_t;
typedef unsigned short uint16_t;
EOF

   if test "$ac_cv_sizeof_long" = "64" ; then
     cat >>$ac_stdint_h <<EOF

typedef   signed int    int32_t;
typedef unsigned int   uint32_t;
typedef   signed long   int64_t;
typedef unsigned long  uint64_t;
#define  int64_t  int64_t
#define uint64_t uint64_t
EOF

   else
    cat >>$ac_stdint_h <<EOF

typedef   signed long   int32_t;
typedef unsigned long  uint32_t;
EOF

   fi
   if test "$ac_cv_sizeof_long" != "$ac_cv_sizeof_voidp" ; then
     cat >>$ac_stdint_h <<EOF

typedef   signed int   intptr_t;
typedef unsigned int  uintptr_t;
EOF
   else
     cat >>$ac_stdint_h <<EOF

typedef   signed long   intptr_t;
typedef unsigned long  uintptr_t;
EOF
     ac_cv_header_stdint_generated=true
   fi
 fi   

 if "$ac_cv_header_stdint_generated" ; then
     cat >>$ac_stdint_h <<EOF

typedef  int8_t    int_least8_t;
typedef  int16_t   int_least16_t;
typedef  int32_t   int_least32_t;

typedef uint8_t   uint_least8_t;
typedef uint16_t  uint_least16_t;
typedef uint32_t  uint_least32_t;

typedef  int8_t    int_fast8_t;	
typedef  int32_t   int_fast16_t;
typedef  int32_t   int_fast32_t;

typedef uint8_t   uint_fast8_t;	
typedef uint32_t  uint_fast16_t;
typedef uint32_t  uint_fast32_t;

typedef long int       intmax_t;
typedef unsigned long uintmax_t;

 /* once */
#endif
#endif
EOF
  fi dnl
])


