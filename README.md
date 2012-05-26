# Joyent http parser binding for php

````
$parser = http_parser_init();

$result = array();
if (http_parser_execute($parser, $request_string, $result)) {
    var_dump($result);
}
````

# Author

Shuhei Tanuma

# License

PHP License