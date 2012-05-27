PHP_ARG_ENABLE(httpparser, Whether to enable the "httpparser" extension,
    [ --enable-httpparser     Enable "httpparser" extension support])

if test $PHP_HTTPPARSER != "no"; then
    PHP_NEW_EXTENSION(httpparser, http_parser.c http-parser/http_parser.c, $ext_shared)
    PHP_ADD_INCLUDE([$ext_srcdir/http-parser])

    PHP_SUBST(HTTPPARSER_SHARED_LIBADD)
    CFLAGS=" -g -O0 -Wunused-variable -Wpointer-sign -Wimplicit-function-declaration"
    PHP_SUBST([CFLAGS])
fi
