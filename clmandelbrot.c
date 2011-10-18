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

#define MAX_NUM_DEVICES 10

/* {{{ type definitions */

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

typedef enum {
	PARAM_TYPE_BITFIELD = 0,
	PARAM_TYPE_BOOL,
	PARAM_TYPE_SIZE,
	PARAM_TYPE_UINT,
	PARAM_TYPE_ULONG,
	PARAM_TYPE_STRING,
	PARAM_TYPE_PLATFORM,
	PARAM_TYPE_MAX_WORK_ITEM_SIZES,
} param_type_t;

typedef struct {
	const char *key;
	cl_device_info name;
	param_type_t type;
} device_info_param_t;

typedef struct {
	const char *key;
	cl_platform_info name;
} platform_info_param_t;

/* }}} */

/* {{{ globals */

#include "mandelbrot_cl.h"

static const device_info_param_t device_info_list[] = {
	{ "type",                          CL_DEVICE_TYPE,                          PARAM_TYPE_BITFIELD  },
	{ "vendor_id",                     CL_DEVICE_VENDOR_ID,                     PARAM_TYPE_UINT      },
	{ "max_compute_units",             CL_DEVICE_MAX_COMPUTE_UNITS,             PARAM_TYPE_UINT      },
	{ "max_work_item_dimensions",      CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,      PARAM_TYPE_UINT      },
	{ "max_work_group_size",           CL_DEVICE_MAX_WORK_GROUP_SIZE,           PARAM_TYPE_SIZE      },
	{ "max_work_item_sizes",           CL_DEVICE_MAX_WORK_ITEM_SIZES, PARAM_TYPE_MAX_WORK_ITEM_SIZES },
	{ "preferred_vector_width_char",   CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,   PARAM_TYPE_SIZE      },
	{ "preferred_vector_width_short",  CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,  PARAM_TYPE_SIZE      },
	{ "preferred_vector_width_int",    CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,    PARAM_TYPE_SIZE      },
	{ "preferred_vector_width_long",   CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,   PARAM_TYPE_SIZE      },
	{ "preferred_vector_width_float",  CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,  PARAM_TYPE_SIZE      },
	{ "preferred_vector_width_double", CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, PARAM_TYPE_SIZE      },
#ifdef CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF /* OpenCL 1.1 */
	{ "preferred_vector_width_half",   CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF,   PARAM_TYPE_UINT      },
	{ "native_vector_width_char",      CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR,      PARAM_TYPE_UINT      },
	{ "native_vector_width_short",     CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT,     PARAM_TYPE_UINT      },
	{ "native_vector_width_int",       CL_DEVICE_NATIVE_VECTOR_WIDTH_INT,       PARAM_TYPE_UINT      },
	{ "native_vector_width_long",      CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG,      PARAM_TYPE_UINT      },
	{ "native_vector_width_float",     CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT,     PARAM_TYPE_UINT      },
	{ "native_vector_width_double",    CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE,    PARAM_TYPE_UINT      },
	{ "native_vector_width_half",      CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF,      PARAM_TYPE_UINT      },
#endif
	{ "max_clock_frequency",           CL_DEVICE_MAX_CLOCK_FREQUENCY,           PARAM_TYPE_UINT      },
	{ "address_bits",                  CL_DEVICE_ADDRESS_BITS,                  PARAM_TYPE_UINT      },
	{ "max_read_image_args",           CL_DEVICE_MAX_READ_IMAGE_ARGS,           PARAM_TYPE_UINT      },
	{ "max_write_image_args",          CL_DEVICE_MAX_WRITE_IMAGE_ARGS,          PARAM_TYPE_UINT      },
	{ "max_mem_alloc_size",            CL_DEVICE_MAX_MEM_ALLOC_SIZE,            PARAM_TYPE_ULONG     },
	{ "image2d_max_width",             CL_DEVICE_IMAGE2D_MAX_WIDTH,             PARAM_TYPE_SIZE      },
	{ "image2d_max_height",            CL_DEVICE_IMAGE2D_MAX_HEIGHT,            PARAM_TYPE_SIZE      },
	{ "image3d_max_width",             CL_DEVICE_IMAGE3D_MAX_WIDTH,             PARAM_TYPE_SIZE      },
	{ "image3d_max_height",            CL_DEVICE_IMAGE3D_MAX_HEIGHT,            PARAM_TYPE_SIZE      },
	{ "image3d_max_depth",             CL_DEVICE_IMAGE3D_MAX_DEPTH,             PARAM_TYPE_SIZE      },
	{ "image_support",                 CL_DEVICE_IMAGE_SUPPORT,                 PARAM_TYPE_BOOL      },
	{ "max_parameter_size",            CL_DEVICE_MAX_PARAMETER_SIZE,            PARAM_TYPE_SIZE      },
	{ "max_samplers",                  CL_DEVICE_MAX_SAMPLERS,                  PARAM_TYPE_UINT      },
	{ "mem_base_addr_align",           CL_DEVICE_MEM_BASE_ADDR_ALIGN,           PARAM_TYPE_UINT      },
	{ "min_data_type_align_size",      CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,      PARAM_TYPE_UINT      },
	{ "single_fp_config",              CL_DEVICE_SINGLE_FP_CONFIG,              PARAM_TYPE_BITFIELD  },
	{ "global_mem_cache_type",         CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,         PARAM_TYPE_UINT      },
	{ "global_mem_cacheline_size",     CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,     PARAM_TYPE_UINT      },
	{ "global_mem_cache_size",         CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,         PARAM_TYPE_ULONG     },
	{ "global_mem_size",               CL_DEVICE_GLOBAL_MEM_SIZE,               PARAM_TYPE_ULONG     },
	{ "max_constant_buffer_size",      CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,      PARAM_TYPE_ULONG     },
	{ "max_constant_args",             CL_DEVICE_MAX_CONSTANT_ARGS,             PARAM_TYPE_UINT      },
	{ "local_mem_type",                CL_DEVICE_LOCAL_MEM_TYPE,                PARAM_TYPE_UINT      },
	{ "local_mem_size",                CL_DEVICE_LOCAL_MEM_SIZE,                PARAM_TYPE_ULONG     },
	{ "error_correction_support",      CL_DEVICE_ERROR_CORRECTION_SUPPORT,      PARAM_TYPE_BOOL      },
	{ "profiling_timer_resolution",    CL_DEVICE_PROFILING_TIMER_RESOLUTION,    PARAM_TYPE_SIZE      },
	{ "endian_little",                 CL_DEVICE_ENDIAN_LITTLE,                 PARAM_TYPE_BOOL      },
	{ "available",                     CL_DEVICE_AVAILABLE,                     PARAM_TYPE_BOOL      },
	{ "compiler_available",            CL_DEVICE_COMPILER_AVAILABLE,            PARAM_TYPE_BOOL      },
	{ "execution_capabilities",        CL_DEVICE_EXECUTION_CAPABILITIES,        PARAM_TYPE_BITFIELD  },
	{ "queue_properties",              CL_DEVICE_QUEUE_PROPERTIES,              PARAM_TYPE_BITFIELD  },
	{ "name",                          CL_DEVICE_NAME,                          PARAM_TYPE_STRING    },
	{ "vendor",                        CL_DEVICE_VENDOR,                        PARAM_TYPE_STRING    },
	{ "driver_version",                CL_DRIVER_VERSION,                       PARAM_TYPE_STRING    },
	{ "profile",                       CL_DEVICE_PROFILE,                       PARAM_TYPE_STRING    },
	{ "version",                       CL_DEVICE_VERSION,                       PARAM_TYPE_STRING    },
	{ "extensions",                    CL_DEVICE_EXTENSIONS,                    PARAM_TYPE_STRING    },
	{ "platform",                      CL_DEVICE_PLATFORM,                      PARAM_TYPE_PLATFORM  },
 	{ "double_fp_config",              CL_DEVICE_DOUBLE_FP_CONFIG,              PARAM_TYPE_BITFIELD  },
 	{ "half_fp_config",                CL_DEVICE_HALF_FP_CONFIG,                PARAM_TYPE_BITFIELD  },
#ifdef CL_DEVICE_HOST_UNIFIED_MEMORY /* OpenCL 1.1 */
	{ "host_unified_memory",           CL_DEVICE_HOST_UNIFIED_MEMORY,           PARAM_TYPE_BOOL      },
	{ "opencl_c_version",              CL_DEVICE_OPENCL_C_VERSION,              PARAM_TYPE_STRING    },
#endif
	{ NULL, 0, 0 }
};

static const platform_info_param_t platform_info_list[] = {
	{ "profile",    CL_PLATFORM_PROFILE    },
	{ "version",    CL_PLATFORM_VERSION    },
	{ "name",       CL_PLATFORM_NAME       },
	{ "vendor",     CL_PLATFORM_VENDOR     },
	{ "extensions", CL_PLATFORM_EXTENSIONS },
	{ NULL, 0 }
};

/* }}} */

/* {{{ function prototypes */

static PHP_MINFO_FUNCTION(clmandelbrot);

static PHP_FUNCTION(clmandelbrot);
static PHP_FUNCTION(cl_get_devices);

static zval *clm_get_device_info(cl_device_id device TSRMLS_DC);
static zval *clm_get_platform_info(cl_platform_id device TSRMLS_DC);

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
	PHP_FE(cl_get_devices, NULL)
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
			if (clm_process(im, &ctx TSRMLS_CC) == SUCCESS) {
				RETVAL_ZVAL(zim, 1, 0);
			}
		}
		clm_release(&ctx);
	}
	zval_ptr_dtor(&zim);
}
/* }}} clmandelbrot */

/* {{{ proto array cl_get_devices(void)
   */
static PHP_FUNCTION(cl_get_devices)
{
	clmandelbrot_t ctx = { 0 };
	cl_uint i = 0;

	RETVAL_FALSE;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (clm_setup_device(&ctx TSRMLS_CC) == FAILURE) {
		return;
	}

	array_init(return_value);

	for (i = 0; i < ctx.deviceCount; i++) {
		zval *zinfo = clm_get_device_info(ctx.device.list[i] TSRMLS_CC);
		add_next_index_zval(return_value, zinfo);
	}
}
/* }}} cl_get_devices */

/* {{{ clm_get_device_info() */
static zval *clm_get_device_info(cl_device_id device TSRMLS_DC)
{
	const device_info_param_t *param = device_info_list;
	cl_int err = CL_SUCCESS;
	char buf[1024] = { 0 };
	size_t len = 0;
	cl_uint max_work_item_dimensions = 0;
	zval *zinfo;

	err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
	                      sizeof(max_work_item_dimensions),
	                      &max_work_item_dimensions, NULL);
	if (err != CL_SUCCESS) {
		max_work_item_dimensions = 0;
	}

	MAKE_STD_ZVAL(zinfo);
	array_init_size(zinfo, 64);

	while (param->key != NULL) {
		switch (param->type) {
			case PARAM_TYPE_BITFIELD: {
				cl_bitfield val = 0;
				err = clGetDeviceInfo(device, param->name, sizeof(val), &val, NULL);
				if (err == CL_SUCCESS) {
					long lval = (long)val;
					add_assoc_long(zinfo, param->key, lval);
				}
			}
			break;

			case PARAM_TYPE_BOOL: {
				cl_bool val = 0;
				err = clGetDeviceInfo(device, param->name, sizeof(val), &val, NULL);
				if (err == CL_SUCCESS) {
					zend_bool bval = (zend_bool)val;
					add_assoc_bool(zinfo, param->key, bval);
				}
			}
			break;

			case PARAM_TYPE_SIZE: {
				size_t val = 0;
				err = clGetDeviceInfo(device, param->name, sizeof(val), &val, NULL);
				if (err == CL_SUCCESS) {
					long lval = (long)val;
					add_assoc_long(zinfo, param->key, lval);
				}
			}
			break;

			case PARAM_TYPE_UINT: {
				cl_uint val = 0;
				err = clGetDeviceInfo(device, param->name, sizeof(val), &val, NULL);
				if (err == CL_SUCCESS) {
					long lval = (long)val;
					add_assoc_long(zinfo, param->key, lval);
				}
			}
			break;

			case PARAM_TYPE_ULONG: {
				cl_ulong val = 0;
				err = clGetDeviceInfo(device, param->name, sizeof(val), &val, NULL);
				if (err == CL_SUCCESS) {
					long lval = (long)val;
					add_assoc_long(zinfo, param->key, lval);
				}
			}
			break;

			case PARAM_TYPE_STRING: {
				err = clGetDeviceInfo(device, param->name, sizeof(buf), buf, &len);
				if (err == CL_SUCCESS) {
					add_assoc_stringl(zinfo, param->key, buf, len, 1);
				}
			}
			break;

			case PARAM_TYPE_PLATFORM: {
				cl_platform_id platform;
				err = clGetDeviceInfo(device, param->name, sizeof(platform), &platform, NULL);
				if (err == CL_SUCCESS) {
					zval *pinfo = clm_get_platform_info(platform TSRMLS_CC);
					add_assoc_zval(zinfo, param->key, pinfo);
				}
			}
			break;

			case PARAM_TYPE_MAX_WORK_ITEM_SIZES: {
				size_t siz = sizeof(size_t) * max_work_item_dimensions;
				size_t *sizes = ecalloc(max_work_item_dimensions, sizeof(size_t));
				if (!sizes) {
					break;
				}

				err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, siz, sizes, NULL);
				if (err == CL_SUCCESS) {
					cl_uint i;
					zval *zsizes;
					MAKE_STD_ZVAL(zsizes);
					array_init_size(zsizes, max_work_item_dimensions);
					for (i = 0; i < max_work_item_dimensions; i++) {
						long lval = (long)sizes[i];
						add_next_index_long(zsizes, lval);
					}
					add_assoc_zval(zinfo, param->key, zsizes);
				}

				efree(sizes);
			}
			break;
		}

		if (err != CL_SUCCESS) {
			add_assoc_null(zinfo, param->key);
		}

		param++;
	}

	return zinfo;
}
/* }}} */

/* {{{ clm_get_platform_info() */
static zval *clm_get_platform_info(cl_platform_id platform TSRMLS_DC)
{
	const platform_info_param_t *param = platform_info_list;
	cl_int err = CL_SUCCESS;
	char buf[1024] = { 0 };
	size_t len = 0;
	zval *zinfo;

	MAKE_STD_ZVAL(zinfo);
	array_init_size(zinfo, 8);

	while (param->key != NULL) {
		err = clGetPlatformInfo(platform, param->name, sizeof(buf), buf, &len);
		if (err == CL_SUCCESS) {
			add_assoc_stringl(zinfo, param->key, buf, len, 1);
		} else {
			add_assoc_null(zinfo, param->key);
		}
		param++;
	}

	return zinfo;
}
/* }}} */

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
