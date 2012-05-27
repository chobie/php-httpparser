# Joyent's http parser binding for php

````
$parser = http_parser_init();
$result = array();
if(http_parser_execute($parser,"GET /img/http-parser.png?key=value#frag HTTP/1.1
Host: chobie.net
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.7; rv:12.0) Gecko/20100101 Firefox/12.0
Accept: image/png,image/*;q=0.8,*/*;q=0.5
Accept-Language: en-us,en;q=0.5
Accept-Encoding: gzip, deflate
Connection: keep-alive
Referer: http://chobie.net/
Cookie: key=value
Cache-Control: max-age=0

",$result)) {
    var_dump($result);
//        array(5) {
//          ["headers"]=>
//          array(9) {
//            ["Host"]=>
//            string(10) "chobie.net"
//            ["User-Agent"]=>
//            string(81) "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.7; rv:12.0) Gecko/20100101 Firefox/12.0"
//            ["Accept"]=>
//            string(33) "image/png,image/*;q=0.8,*/*;q=0.5"
//            ["Accept-Language"]=>
//            string(14) "en-us,en;q=0.5"
//            ["Accept-Encoding"]=>
//            string(13) "gzip, deflate"
//            ["Connection"]=>
//            string(10) "keep-alive"
//            ["Referer"]=>
//            string(18) "http://chobie.net/"
//            ["Cookie"]=>
//            string(9) "key=value"
//            ["Cache-Control"]=>
//            string(9) "max-age=0"
//          }
//          ["QUERY_STRING"]=>
//          string(35) "/img/http-parser.png?key=value#frag"
//          ["path"]=>
//          string(20) "/img/http-parser.png"
//          ["query"]=>
//          string(9) "key=value"
//          ["fragment"]=>
//          string(4) "frag"
//        }
}
````

## API

### resource http_parser_init()


### bool http_parser_execute(resource $resource, string $request, array &$result)


# Author

Shuhei Tanuma

# License

PHP License