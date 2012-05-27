/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2012 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Shuhei Tanuma <chobieeee@php.net>                          |
   +----------------------------------------------------------------------+
 */


#include "php_http_parser.h"

extern zend_class_entry *httpparser_class_entry;

static int httpparser_resource_handle;

//void php_httpparser_init(TSRMLS_D);

void static destruct_httpparser(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	http_parser *obj = (http_parser *)rsrc->ptr;
	
	efree(obj);
}

// ry's http parser callbacks

int on_message_begin(http_parser *p)
{
	return 0;
}

int on_headers_complete(http_parser *p)
{
	return 0;
}

int on_message_complete(http_parser *p)
{
	php_http_parser_context *result = p->data;
	result->finished = 1;

	return 0;
}

int on_url_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	zval *data = result->data;
	
	http_parser_parse_url(at, len, false, &result->handle);
	
	if (result->handle.field_set & (1<<UF_SCHEMA)) {
		const char *schema = at+result->handle.field_data[UF_SCHEMA].off;
		int length = result->handle.field_data[UF_SCHEMA].len;
		add_assoc_stringl(data, "schema", schema, length, 1);
	}
	if (result->handle.field_set & (1<<UF_HOST)) {
		const char *host = at+result->handle.field_data[UF_HOST].off;
		int length = result->handle.field_data[UF_HOST].len;
		add_assoc_stringl(data, "host", host, length, 1);
	}
	if (result->handle.field_set & (1<<UF_PORT)) {
		const char *port = at+result->handle.field_data[UF_PORT].off;
		int length = result->handle.field_data[UF_PORT].len;
		add_assoc_stringl(data, "port", port, length, 1);
	}
	if (result->handle.field_set & (1<<UF_PATH)) {
		const char *path = at+result->handle.field_data[UF_PATH].off;
		int length = result->handle.field_data[UF_PATH].len;
		add_assoc_stringl(data, "path", path, length, 1);
	}
	if (result->handle.field_set & (1<<UF_QUERY)) {
		const char *query = at+result->handle.field_data[UF_QUERY].off;
		int length = result->handle.field_data[UF_QUERY].len;
		add_assoc_stringl(data, "query", query, length, 1);
	}
	if (result->handle.field_set & (1<<UF_FRAGMENT)) {
		const char *fragment = at+result->handle.field_data[UF_FRAGMENT].off;
		int length = result->handle.field_data[UF_FRAGMENT].len;
		add_assoc_stringl(data, "fragment", fragment, length, 1);
	}
	
	add_assoc_stringl(data, "url", at, len, 1);
	return 0;
}

int header_field_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	result->tmp = estrndup(at, len);
	
	return 0;
}

int header_value_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	zval *data = result->data;
	
	add_assoc_stringl(data, result->tmp, at, len, 1);
	efree(result->tmp);
	result->tmp = NULL;
	return 0;
}

int on_body_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	zval *data = result->data;
	
	add_assoc_stringl(data, "body", at, len,  1);

	return 0;
}
// end of callback


PHP_MINIT_FUNCTION(httpparser) {
	//php_httpparser_init(TSRMLS_C);
	httpparser_resource_handle = zend_register_list_destructors_ex(destruct_httpparser, NULL, PHP_HTTPPARSER_RESOURCE_NAME, module_number);

	return SUCCESS;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_http_parser_init, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_http_parser_execute, 0, 0, 3)
	ZEND_ARG_INFO(0, resource)
	ZEND_ARG_INFO(0, buffer)
	ZEND_ARG_INFO(0, setting)
ZEND_END_ARG_INFO()



PHP_FUNCTION(http_parser_init)
{
	php_http_parser_context *ctx;
	ctx = emalloc(sizeof(php_http_parser_context));

	http_parser_init(&ctx->parser, HTTP_REQUEST);
	
	memset(&ctx->handle, 0, sizeof(struct http_parser_url));

		/* setup callback */
	ctx->settings.on_message_begin = on_message_begin;
	ctx->settings.on_header_field = header_field_cb;
	ctx->settings.on_header_value = header_value_cb;
	ctx->settings.on_url = on_url_cb;
	ctx->settings.on_body = on_body_cb;
	ctx->settings.on_headers_complete = on_headers_complete;
	ctx->settings.on_message_complete = on_message_complete;

	ZEND_REGISTER_RESOURCE(return_value, ctx, httpparser_resource_handle);
}

PHP_FUNCTION(http_parser_execute)
{
	zval *z_parser,*result;
	php_http_parser_context *context;
	char *body;
	int body_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"rsa",&z_parser, &body, &body_len, &result) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(context, php_http_parser_context*, &z_parser, -1, PHP_HTTPPARSER_RESOURCE_NAME, httpparser_resource_handle);

	context->data = result;
	context->parser.data = context;
	
	http_parser_execute(&context->parser, &context->settings, body, body_len);
	
	Z_ISREF_P(result);
	
	if (context->finished == 1) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

static zend_function_entry httpparser_functions[] = {
	PHP_FE(http_parser_init, arginfo_http_parser_init)
	PHP_FE(http_parser_execute, arginfo_http_parser_init)
	{NULL, NULL, NULL}
};


PHP_MINFO_FUNCTION(httpparser)
{
	php_printf("PHP httpparser Extension\n");
}

zend_module_entry httpparser_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"httpparser",
	httpparser_functions,					/* Functions */
	PHP_MINIT(httpparser),	/* MINIT */
	NULL,					/* MSHUTDOWN */
	NULL,					/* RINIT */
	NULL,					/* RSHUTDOWN */
	PHP_MINFO(httpparser),	/* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_HTTPPARSER_EXTVER,
#endif
	STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_HTTPPARSER
ZEND_GET_MODULE(httpparser)
#endif
