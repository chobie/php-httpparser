--TEST--
Check for httpparser presence
--SKIPIF--
<?php if (!extension_loaded("httpparser")) print "skip"; ?>
--FILE--
<?php
echo "httpparser extension is available";
--EXPECT--
httpparser extension is available
