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
#include <OpenCL/opencl.h>
#include "mandelbrot_cl.h"

#define MAX_NUM_DEVICES 10

typedef struct {
	union {
		cl_device_id id;
		cl_device_id list[MAX_NUM_DEVICES];
	} device;
	cl_uint          deviceCount;
	cl_context       context;
	cl_command_queue queue;
	cl_program       program;
	cl_kernel        kernel;
	cl_mem           output;
	int width;
	int height;
	float centerX;
	float centerY;
	float unit;
	unsigned char *bitmap;
} clmandelbrot_t;

/* {{{ function prototypes */

static PHP_MINFO_FUNCTION(clmandelbrot);
static PHP_FUNCTION(clmandelbrot);

static int clm_process(gdImagePtr im, clmandelbrot_t *ctx TSRMLS_DC);
static void clm_release(clmandelbrot_t *ctx TSRMLS_DC);
static int clm_setup_device(clmandelbrot_t *ctx TSRMLS_DC);
static int clm_setup_kernel(clmandelbrot_t *ctx TSRMLS_DC);
static int clm_setup_queue(clmandelbrot_t *ctx TSRMLS_DC);
static int clm_execute(clmandelbrot_t *ctx TSRMLS_DC);
static void clm_draw(gdImagePtr im, clmandelbrot_t *ctx TSRMLS_DC);

/* }}} */

/* {{{ argument informations */

ZEND_BEGIN_ARG_INFO_EX(clmandelbrot_arg_info, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 2)
  ZEND_ARG_INFO(0, width)
  ZEND_ARG_INFO(0, height)
  ZEND_ARG_INFO(0, unit)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ clmandelbrot_functions[] */
static zend_function_entry clmandelbrot_functions[] = {
	PHP_FE(clmandelbrot, clmandelbrot_arg_info)
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
}
/* }}} */


/* {{{ proto resource clmandelbrot(int width, int height[, float unit])
   */
static PHP_FUNCTION(clmandelbrot)
{
	long width = 0;
	long height = 0;
	double unit = 0.0;
	size_t len;

	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval *zim = NULL, *callable, *args, *zwidth, *zheight;
	int err;
	gdImagePtr im;

	RETVAL_FALSE;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
			"ll|d", &width, &height, &unit) == FAILURE) {
		return;
	}

	MAKE_STD_ZVAL(callable);
	ZVAL_STRING(callable, "imagecreatetruecolor", 1);
	err = zend_fcall_info_init(callable, 0, &fci, &fcc,
	                           NULL, NULL TSRMLS_CC);

	if (err != SUCCESS) {
		zval_ptr_dtor(&callable);
		return;
	}

	MAKE_STD_ZVAL(args);
	MAKE_STD_ZVAL(zwidth);
	MAKE_STD_ZVAL(zheight);
	ZVAL_LONG(zwidth, width);
	ZVAL_LONG(zheight, height);
	array_init(args);
	add_next_index_zval(args, zwidth);
	add_next_index_zval(args, zheight);

	zend_fcall_info_call(&fci, &fcc, &zim, args TSRMLS_CC);
	zval_ptr_dtor(&callable);
	zval_ptr_dtor(&args);

	if (!zim) {
		return;
	}

	ZEND_FETCH_RESOURCE_NO_RETURN(im, gdImagePtr, &zim, -1,
	                              "Image", phpi_get_le_gd());
	if (im) {
		size_t len;
		clmandelbrot_t ctx = { 0 };
		ctx.width = gdImageSX(im);
		ctx.height = gdImageSY(im);
		if (unit > 0.0) {
			ctx.unit = (float)unit;
		} else {
			ctx.unit = 10.0f / (float)(ctx.width + ctx.height);
		}
		len = ctx.width * ctx.height;
		ctx.bitmap = ecalloc(len, sizeof(unsigned char));
		if (!ctx.bitmap) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING,
			                 "cannot allocate memory");
		} else {
			if (clm_process(im, &ctx) == SUCCESS) {
				RETVAL_ZVAL(zim, 1, 0);
			}
		}
		clm_release(&ctx);
	}
	zval_ptr_dtor(&zim);
}
/* }}} clmandelbrot */

/* {{{ clm_process() */
static int clm_process(gdImagePtr im, clmandelbrot_t *ctx TSRMLS_DC)
{
	if (clm_setup_device(ctx TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}
	if (clm_setup_kernel(ctx TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}
	if (clm_setup_queue(ctx TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}
	if (clm_execute(ctx TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}
	clm_draw(im, ctx TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ clm_release() */
static void clm_release(clmandelbrot_t *ctx TSRMLS_DC)
{
	if (ctx->context) {
		clReleaseContext(ctx->context);
	}
	if (ctx->queue) {
		clReleaseCommandQueue(ctx->queue);
	}
	if (ctx->program) {
		clReleaseProgram(ctx->program);
	}
	if (ctx->kernel) {
		clReleaseKernel(ctx->kernel);
	}
	if (ctx->output) {
		clReleaseMemObject(ctx->output);
	}
	if (ctx->bitmap) {
		efree(ctx->bitmap);
	}
}
/* }}} */

/* {{{ clm_setup_device() */
static int clm_setup_device(clmandelbrot_t *ctx TSRMLS_DC)
{
	cl_int err = CL_SUCCESS;

	err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, MAX_NUM_DEVICES,
	                     ctx->device.list, &ctx->deviceCount);
	if (err != CL_SUCCESS || ctx->deviceCount == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot get device IDs");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ clm_setup_kernel() */
static int clm_setup_kernel(clmandelbrot_t *ctx TSRMLS_DC)
{
	cl_int err = CL_SUCCESS;

	ctx->context = clCreateContext(0, 1, &ctx->device.id, NULL, NULL, &err);
	if (!ctx->context) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot create context");
		return FAILURE;
	}

	ctx->program = clCreateProgramWithSource(ctx->context, 1, &Mandelbrot_cl, NULL, &err);
	if (err != CL_SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot create program with source");
		return FAILURE;
	}

	// compile
	err = clBuildProgram(ctx->program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char info[2048];

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error: Failed to build program executable");
		clGetProgramBuildInfo(ctx->program, ctx->device.id,
		                      CL_PROGRAM_BUILD_LOG, sizeof(info), info, &len);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", info);
		return FAILURE;
	}

	ctx->kernel = clCreateKernel(ctx->program, "Mandelbrot", &err);
	if (!ctx->kernel || err != CL_SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot create kernel");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ clm_setup_queue() */
static int clm_setup_queue(clmandelbrot_t *ctx TSRMLS_DC)
{
	cl_int err = CL_SUCCESS;

	ctx->queue = clCreateCommandQueue(ctx->context, ctx->device.id, 0, &err);
	if (!ctx->queue) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot create command queue");
		return FAILURE;
	}

	size_t len = sizeof(unsigned char) * ctx->width * ctx->height;
	ctx->output = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY, len, NULL, NULL);
	if (!ctx->output) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot create buffer");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ clm_execute() */
static int clm_execute(clmandelbrot_t *ctx TSRMLS_DC)
{
	cl_int err = CL_SUCCESS;

	err |= clSetKernelArg(ctx->kernel, 0, sizeof(cl_mem), &ctx->output);
	err |= clSetKernelArg(ctx->kernel, 1, sizeof(ctx->width), &ctx->width);
	err |= clSetKernelArg(ctx->kernel, 2, sizeof(ctx->height), &ctx->height);
	err |= clSetKernelArg(ctx->kernel, 3, sizeof(ctx->centerX), &ctx->centerX);
	err |= clSetKernelArg(ctx->kernel, 4, sizeof(ctx->centerY), &ctx->centerY);
	err |= clSetKernelArg(ctx->kernel, 5, sizeof(ctx->unit), &ctx->unit);
	if (err != CL_SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot set kernel argument(s)");
		return FAILURE;
	}

	size_t local;
	err = clGetKernelWorkGroupInfo(ctx->kernel, ctx->device.id,
	                               CL_KERNEL_WORK_GROUP_SIZE,
	                               sizeof(local), &local, NULL);
	if (err != CL_SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot get kernel work group info");
		return FAILURE;
	}

	size_t global = ctx->width * ctx->height;
	err = clEnqueueNDRangeKernel(ctx->queue, ctx->kernel, 1, NULL,
	                             &global, &local, 0, NULL, NULL);
	if (err) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot enqueue ND range kernel");
		return FAILURE;
	}

	clFinish(ctx->queue);

	size_t len = sizeof(unsigned char) * ctx->width * ctx->height;
	err = clEnqueueReadBuffer(ctx->queue, ctx->output, CL_TRUE, 0,
	                          len, ctx->bitmap, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot enqueue read buffer");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ clm_draw() */
static void clm_draw(gdImagePtr im, clmandelbrot_t *ctx TSRMLS_DC)
{
	int x, y;
	for (y = 0; y < ctx->height; y++) {
		for (x = 0; x < ctx->width; x++) {
			int c = (int)ctx->bitmap[x + y * ctx->height];
			im->tpixels[y][x] = gdTrueColor(c, c, c);
		}
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
