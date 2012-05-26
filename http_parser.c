/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2011 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Shuhei Tanuma <chobieee@gmail.com>                          |
   +----------------------------------------------------------------------+
 */


#include "php_http_parser.h"

extern zend_class_entry *httpparser_class_entry;

static int httpparser_resource_handle;

typedef struct
{
	int finished;
	zval *data;
	char *tmp;
} php_httpparser_t;

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
	php_httpparser_t *result = p->data;
	result->finished = 1;

	return 0;
}

int on_url_cb(http_parser *p, const char *at, size_t len)
{
	php_httpparser_t *result = p->data;
	zval *data = result->data;
	
	add_assoc_stringl(data, "url", at, len, 1);
	return 0;
}

int header_field_cb(http_parser *p, const char *at, size_t len)
{
	php_httpparser_t *result = p->data;
	result->tmp = estrndup(at, len);
	
	return 0;
}

int header_value_cb(http_parser *p, const char *at, size_t len)
{
	php_httpparser_t *result = p->data;
	zval *data = result->data;
	
	add_assoc_stringl(data, result->tmp, at, len, 1);
	efree(result->tmp);
	result->tmp = NULL;
	return 0;
}

int on_body_cb(http_parser *p, const char *at, size_t len)
{
	php_httpparser_t *result = p->data;
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
	http_parser *parser;
	parser = emalloc(sizeof(http_parser));

	http_parser_init(parser, HTTP_REQUEST);
	
	ZEND_REGISTER_RESOURCE(return_value, parser, httpparser_resource_handle);
}

PHP_FUNCTION(http_parser_execute)
{
	zval *z_parser,*result;
	php_httpparser_t rsrt;
	http_parser *parser;
	http_parser_settings *settings;
	char *body;
	int body_len, r = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"rsa",&z_parser, &body, &body_len, &result) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(parser, http_parser*, &z_parser, -1, PHP_HTTPPARSER_RESOURCE_NAME, httpparser_resource_handle);

	settings = malloc(sizeof(http_parser_settings));

	settings->on_message_begin = on_message_begin;
	settings->on_header_field = header_field_cb;
	settings->on_header_value = header_value_cb;
	settings->on_url = on_url_cb;
	settings->on_body = on_body_cb;
	settings->on_headers_complete = on_headers_complete;
	settings->on_message_complete = on_message_complete;
	
	rsrt.data = result;
	parser->data = &rsrt;
	
	http_parser_execute(parser, settings, body, body_len);
	
	Z_ISREF_P(result);
	
	if (rsrt.finished == 1) {
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
