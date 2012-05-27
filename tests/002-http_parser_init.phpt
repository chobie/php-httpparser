--TEST--
Check for http_parser_init presence
--SKIPIF--
<?php if (!extension_loaded("httpparser")) print "skip"; ?>
--FILE--
<?php
$resource = http_parser_init();
var_dump($resource);
--EXPECT--
resource(4) of type (httpparser)