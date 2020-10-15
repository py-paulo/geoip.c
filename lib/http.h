#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

typedef struct http HTTP;		/* An "HTTP connection" object */
typedef struct url URL;			/* A "parsed URL" object */
typedef struct in_addr IPADDR;		/* Structure to hold IP address */

HTTP *http_open(URL *uri, IPADDR *addr, int port);

URL *url_parse(char *url);

IPADDR *url_address(URL *url);

char *url_method(URL *url);
char *url_hostname(URL *url);
char *url_path(URL *url);

int url_port(URL *url);

void url_print(FILE *f, URL *url);
void url_free(URL *url);
