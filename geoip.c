#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib/http.h"


int
main(int argc, char *argv[])
{
    URL *uri;
    HTTP *http;
    IPADDR *addr;

	int port, code;
    char *status, *method;

    if ( argc != 2 ) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        exit(1);
    }
    if ( ( uri = url_parse(argv[1]) ) == NULL ) {
        fprintf(stderr, "Illegal URL: '%s'\n", argv[1]);
        exit(1);
    }

    method = url_method(uri);
    addr = url_address(uri);
    port = url_port(uri);

    url_print(stderr, uri);
    if ( method == NULL || strcasecmp(method, "http") ) {
        fprintf(stderr, "Only HTTP access method is supported\n");
        exit(1);
    }

    if ( ( http = http_open(uri, addr, port)) == NULL ) {
        fprintf(stderr, "Unable to contact host '%s', port %d\n",
            url_hostname(uri) != NULL ? url_hostname(uri) : "(NULL)", port);
        exit(1);
    }
    http_request(http, uri);

    http_response(http);

    http_close(http);
    url_free(uri);

    return 0;
}
