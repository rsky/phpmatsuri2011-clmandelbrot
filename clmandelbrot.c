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

/* $ Id: $ */ 

#include "php_clmandelbrot.h"

#if HAVE_CLMANDELBROT

/* {{{ clmandelbrot_functions[] */
function_entry clmandelbrot_functions[] = {
	PHP_FE(clmandelblot        , clmandelblot_arg_info)
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
#if ZEND_EXTENSION_API_NO >= 220050617
		STANDARD_MODULE_HEADER_EX, NULL,
		clmandelbrot_deps,
#else
		STANDARD_MODULE_HEADER,
#endif

	"clmandelbrot",
	clmandelbrot_functions,
	PHP_MINIT(clmandelbrot),     /* Replace with NULL if there is nothing to do at php startup   */ 
	PHP_MSHUTDOWN(clmandelbrot), /* Replace with NULL if there is nothing to do at php shutdown  */
	PHP_RINIT(clmandelbrot),     /* Replace with NULL if there is nothing to do at request start */
	PHP_RSHUTDOWN(clmandelbrot), /* Replace with NULL if there is nothing to do at request end   */
	PHP_MINFO(clmandelbrot),
	PHP_CLMANDELBROT_VERSION, 
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CLMANDELBROT
ZEND_GET_MODULE(clmandelbrot)
#endif


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(clmandelbrot)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(clmandelbrot)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(clmandelbrot)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(clmandelbrot)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(clmandelbrot)
{
	php_printf("PHP Matsuri 2011\n");
	php_info_print_table_start();
	php_info_print_table_row(2, "Version",PHP_CLMANDELBROT_VERSION " (alpha)");
	php_info_print_table_row(2, "Released", "2011-10-16");
	php_info_print_table_row(2, "CVS Revision", "$Id: $");
	php_info_print_table_row(2, "Authors", "Ryusuke Sekiyama 'rsky0711@gmail.com' (lead)\n");
	php_info_print_table_end();
	/* add your stuff here */

}
/* }}} */


/* {{{ proto resource clmandelblot(int width, int height[, float unit])
   */
PHP_FUNCTION(clmandelblot)
{
	void * return_res;
	long return_res_id = -1;

	long width = 0;
	long height = 0;
	double unit = 0.0;



	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|d", &width, &height, &unit) == FAILURE) {
		return;
	}

	php_error(E_WARNING, "clmandelblot: not yet implemented"); RETURN_FALSE;

	/* RETURN_RESOURCE(...); */
}
/* }}} clmandelblot */

#endif /* HAVE_CLMANDELBROT */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
