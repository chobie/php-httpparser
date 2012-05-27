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

static int httpparser_resource_handle;

void static destruct_httpparser(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	http_parser *obj = (http_parser *)rsrc->ptr;
	
	efree(obj);
}

/*  http parser callbacks */
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

#define PHP_HTTP_PARSER_PARSE_URL(flag, name) \
	if (result->handle.field_set & (1 << flag)) { \
		const char *name = at+result->handle.field_data[flag].off; \
		int length = result->handle.field_data[flag].len; \
		add_assoc_stringl(data, #name, (char*)name, length, 1); \
	} 

int on_url_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	zval *data = result->data;
	
	http_parser_parse_url(at, len, 0, &result->handle);
	
	add_assoc_stringl(data, "QUERY_STRING", (char*)at, len, 1);

	PHP_HTTP_PARSER_PARSE_URL(UF_SCHEMA, scheme);
	PHP_HTTP_PARSER_PARSE_URL(UF_HOST, host);
	PHP_HTTP_PARSER_PARSE_URL(UF_PORT, port);
	PHP_HTTP_PARSER_PARSE_URL(UF_PATH, path);
	PHP_HTTP_PARSER_PARSE_URL(UF_QUERY, query);
	PHP_HTTP_PARSER_PARSE_URL(UF_FRAGMENT, fragment);
	
	return 0;
}

int header_field_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	/* TODO: */
	result->tmp = estrndup(at, len);
	
	return 0;
}

int header_value_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	zval *data = result->headers;
	
	add_assoc_stringl(data, result->tmp, (char*)at, len, 1);
	/* TODO: */
	efree(result->tmp);
	result->tmp = NULL;
	return 0;
}

int on_body_cb(http_parser *p, const char *at, size_t len)
{
	php_http_parser_context *result = p->data;
	zval *data = result->headers;
	
	add_assoc_stringl(data, "body", (char*)at, len,  1);

	return 0;
}
/* end of callback */


PHP_MINIT_FUNCTION(httpparser) {
	httpparser_resource_handle = zend_register_list_destructors_ex(destruct_httpparser, NULL, PHP_HTTPPARSER_RESOURCE_NAME, module_number);

	zend_register_long_constant("HTTP_BOTH", sizeof("HTTP_BOTH"), HTTP_BOTH, CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant("HTTP_REQUEST", sizeof("HTTP_REQUEST"), HTTP_REQUEST, CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant("HTTP_RESPONSE", sizeof("HTTP_RESPONSE"), HTTP_RESPONSE, CONST_PERSISTENT, module_number TSRMLS_CC);

	return SUCCESS;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_http_parser_init, 0, 0, 1)
	ZEND_ARG_INFO(0, target)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_http_parser_execute, 0, 0, 3)
	ZEND_ARG_INFO(0, resource)
	ZEND_ARG_INFO(0, buffer)
	ZEND_ARG_INFO(0, setting)
ZEND_END_ARG_INFO()

PHP_FUNCTION(http_parser_init)
{
	long target = HTTP_REQUEST;
	php_http_parser_context *ctx;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"|l",&target) == FAILURE) {
		return;
	}

	ctx = emalloc(sizeof(php_http_parser_context));
	http_parser_init(&ctx->parser, target);
	
	if (target == HTTP_RESPONSE) {
		ctx->is_response = 1;
	} else {
		ctx->is_response = 0;
	}
	
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
	zval *z_parser,*result, *headers;
	php_http_parser_context *context;
	char *body;
	int body_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"rs/a",&z_parser, &body, &body_len, &result) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(context, php_http_parser_context*, &z_parser, -1, PHP_HTTPPARSER_RESOURCE_NAME, httpparser_resource_handle);

	MAKE_STD_ZVAL(headers);
	array_init(headers);
	add_assoc_zval(result, "headers", headers);

	context->headers = headers;
	context->data = result;
	context->parser.data = context;
	
	http_parser_execute(&context->parser, &context->settings, body, body_len);
	
	if (context->is_response == 0) {
		add_assoc_string(result, "REQUEST_METHOD", (char*)http_method_str(context->parser.method), 1);
	} else {
		add_assoc_long(result, "status_code", (long)context->parser.status_code);
	}

	if (context->finished == 1) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

static zend_function_entry httpparser_functions[] = {
	PHP_FE(http_parser_init,    arginfo_http_parser_init)
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
