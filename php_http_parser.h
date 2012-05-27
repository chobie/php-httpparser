#ifndef PHP_HTTPPARSER_H

#define PHP_HTTPPARSER_H

#define PHP_HTTPPARSER_EXTNAME "httpparser"
#define PHP_HTTPPARSER_EXTVER "0.1"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "http_parser.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_interfaces.h"

/* Define the entry point symbol
 * Zend will use when loading this module
 */
extern zend_module_entry httpparser_module_entry;
#define phpext_httpparser_ptr &httpparser_module_entry;

extern zend_class_entry *httpparser_class_entry;

typedef struct {
	struct http_parser parser;
	struct http_parser_url handle;
	struct http_parser_settings settings;
	int was_header_value;
	int finished;
	zval *data;
	zval *headers;
	char *tmp;
} php_http_parser_context;

#define PHP_HTTPPARSER_RESOURCE_NAME "httpparser"

#endif /* PHP_HTTPPARSER_H */
