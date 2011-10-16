/*
   +----------------------------------------------------------------------+
   | All rights reserved                                                  |
   |                                                                      |
   | Redistribution and use in source and binary forms, with or without   |
   | modification, are permitted provided that the following conditions   |
   | are met:                                                             |
   |                                                                      |
   | 1. Redistributions of source code must retain the above copyright    |
   |    notice, this list of conditions and the following disclaimer.     |
   | 2. Redistributions in binary form must reproduce the above copyright |
   |    notice, this list of conditions and the following disclaimer in   |
   |    the documentation and/or other materials provided with the        |
   |    distribution.                                                     |
   | 3. The names of the authors may not be used to endorse or promote    |
   |    products derived from this software without specific prior        |
   |    written permission.                                               |
   |                                                                      |
   | THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS  |
   | "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT    |
   | LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    |
   | FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE       |
   | COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  |
   | INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, |
   | BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     |
   | LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER     |
   | CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT   |
   | LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN    |
   | ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE      |
   | POSSIBILITY OF SUCH DAMAGE.                                          |
   +----------------------------------------------------------------------+
   | Authors: Ryusuke Sekiyama <rsky0711@gmail.com>                       |
   +----------------------------------------------------------------------+
*/

#include "php_clmandelbrot.h"
#ifdef HAVE_GD_BUNDLED
#include <ext/gd/libgd/gd.h>
#else
#include <gd.h>
#endif

/* {{{ function prototypes */

static PHP_MINFO_FUNCTION(clmandelbrot);
static PHP_FUNCTION(clmandelblot);

/* }}} */

/* {{{ argument informations */

ZEND_BEGIN_ARG_INFO_EX(clmandelblot_arg_info, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 2)
  ZEND_ARG_INFO(0, width)
  ZEND_ARG_INFO(0, height)
  ZEND_ARG_INFO(0, unit)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ clmandelbrot_functions[] */
static zend_function_entry clmandelbrot_functions[] = {
	PHP_FE(clmandelblot, clmandelblot_arg_info)
	{ NULL, NULL, NULL }
};
/* }}} */

/* {{{ cross-extension dependencies */

#if ZEND_EXTENSION_API_NO >= 220050617
static zend_module_dep clmandelbrot_deps[] = {
	ZEND_MOD_REQUIRED("gd")
	{NULL, NULL, NULL, 0}
};
#endif
/* }}} */

/* {{{ clmandelbrot_module_entry
 */
zend_module_entry clmandelbrot_module_entry = {
	STANDARD_MODULE_HEADER_EX, NULL,
	clmandelbrot_deps,
	"clmandelbrot",
	clmandelbrot_functions,
	NULL,
	NULL,
	NULL,
	NULL,
	PHP_MINFO(clmandelbrot),
	PHP_CLMANDELBROT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CLMANDELBROT
ZEND_GET_MODULE(clmandelbrot)
#endif


/* {{{ PHP_MINFO_FUNCTION */
static PHP_MINFO_FUNCTION(clmandelbrot)
{
	php_printf("PHP Matsuri 2011\n");
	php_info_print_table_start();
	php_info_print_table_row(2, "Version",PHP_CLMANDELBROT_VERSION " (alpha)");
	php_info_print_table_row(2, "Released", "2011-10-16");
	php_info_print_table_row(2, "Authors", "Ryusuke Sekiyama 'rsky0711@gmail.com' (lead)\n");
	php_info_print_table_end();
	/* add your stuff here */

}
/* }}} */


/* {{{ proto resource clmandelblot(int width, int height[, float unit])
   */
static PHP_FUNCTION(clmandelblot)
{
	long width = 0;
	long height = 0;
	double unit = 0.0;

	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval *retval = NULL, *callable, *args, *zwidth, *zheight;
	int err;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|d", &width, &height, &unit) == FAILURE) {
		return;
	}

	MAKE_STD_ZVAL(callable);
	ZVAL_STRING(callable, "imagecreatetruecolor", 1);
	err = zend_fcall_info_init(callable, 0, &fci, &fcc
#if ZEND_EXTENSION_API_NO >= 220090626
		, NULL, NULL
#endif
		TSRMLS_CC);
	if (err != SUCCESS) {
		zval_ptr_dtor(&callable);
		RETURN_FALSE;
	}

	MAKE_STD_ZVAL(args);
	MAKE_STD_ZVAL(zwidth);
	MAKE_STD_ZVAL(zheight);
	ZVAL_LONG(zwidth, width);
	ZVAL_LONG(zheight, height);
	array_init(args);
	add_next_index_zval(args, zwidth);
	add_next_index_zval(args, zheight);

	zend_fcall_info_call(&fci, &fcc, &retval, args TSRMLS_CC);
	if (retval) {
		RETVAL_ZVAL(retval, 1, 1);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&callable);
	zval_ptr_dtor(&args);
}
/* }}} clmandelblot */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
