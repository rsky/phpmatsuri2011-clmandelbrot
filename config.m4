PHP_ARG_ENABLE(clmandelbrot, whether to enable clmandelbrot functions,
[  --enable-clmandelbrot         Enable clmandelbrot support])

if test "$PHP_CLMANDELBROT" != "no"; then
  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_CLMANDELBROT"

  AC_MSG_CHECKING(PHP version)
  AC_TRY_COMPILE([#include <php_version.h>], [
#if !defined(PHP_VERSION_ID) || PHP_VERSION_ID < 50300
#error  this extension requires at least PHP version 5.3.0
#endif
],
[AC_MSG_RESULT(ok)],
[AC_MSG_ERROR([need at least PHP 5.3.0])])

  export CPPFLAGS="$OLD_CPPFLAGS"

  PHP_EVAL_LIBLINE([-L. -lOpenCL], CLMANDELBROT_SHARED_LIBADD)
  PHP_SUBST(CLMANDELBROT_SHARED_LIBADD)
  AC_DEFINE(HAVE_CLMANDELBROT, 1, [ ])

  PHP_NEW_EXTENSION(clmandelbrot, clmandelbrot.c , $ext_shared)
fi
