dnl
dnl $ Id: $
dnl

PHP_ARG_ENABLE(clmandelbrot, whether to enable clmandelbrot functions,
[  --enable-clmandelbrot         Enable clmandelbrot support])

if test "$PHP_CLMANDELBROT" != "no"; then
  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_CLMANDELBROT"

  AC_MSG_CHECKING(PHP version)
  AC_TRY_COMPILE([#include <php_version.h>], [
#if PHP_VERSION_ID < 50100
#error  this extension requires at least PHP version 5.1.0rc1
#endif
],
[AC_MSG_RESULT(ok)],
[AC_MSG_ERROR([need at least PHP 5.1.0rc1])])

  export CPPFLAGS="$OLD_CPPFLAGS"


  PHP_SUBST(CLMANDELBROT_SHARED_LIBADD)
  AC_DEFINE(HAVE_CLMANDELBROT, 1, [ ])

  PHP_NEW_EXTENSION(clmandelbrot, clmandelbrot.c , $ext_shared)

fi

