#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

typedef struct http HTTP;		/* An "HTTP connection" object */
typedef struct url URL;			/* A "parsed URL" object */
typedef struct in_addr IPADDR;		/* Structure to hold IP address */


/*
 * Functions for managing an HTTP connection
 *
 * Usage:
 *  (1) Use http_open() to create an open HTTP connection to
 *	the specified IP address and port.
 *
 *  (2) Use http_request() to issue an HTTP GET request for a particular
 *	URL to the server at the other end of an HTTP connection.
 *
 *  (3) Use http_response() to finish the request and collect the response
 *	code and response headers.
 *
 *  (4) Close the HTTP connection using http_close();
 *
 * Functions that return int return zero if successful, nonzero if
 *	an error occurs.
 * Functions that return pointers return NULL if unsuccessful.
 */
HTTP *http_open(URL *uri, IPADDR *addr, int port);
int http_close(HTTP *http);
int http_request(HTTP *http, URL *uri);
int http_response(HTTP *http);
int http_getc(HTTP *http);
char *http_status(HTTP *http, int *code);

/*
 * Functions for managing URL's (Web addresses)
 *
 * Usage:
 *  (1) Parse a URL string into a URL object using url_parse();
 *
 *  (2) Extract IP address and port number for http_open()
 *	using url_address() and url_port().
 *	Note: the first time url_address() is used on a URL
 *	object, a DNS lookup will occur.
 *	
 *  (3) If desired, extract the "access method"
 *	("http", "ftp", "mailto", etc.) using url_method(),
 *	the hostname using url_hostname(), and the path on
 *	the remote server using url_path().
 *
 *  (4) When finished with the URL, free it with url_free().
 *
 * Functions that return pointers return NULL if unsuccessful.
 *
 * Do not attempt to free() any pointers returned by url_method()
 *	or url_hostname(), and do not use these pointers or the
 *	URL object itself again once url_free() has been called.
 */

URL *url_parse(char *url);
IPADDR *url_address(URL *url);
char *url_method(URL *url);
char *url_hostname(URL *url);
char *url_path(URL *url);
int url_port(URL *url);
void url_print(FILE *f, URL *url);
void url_free(URL *url);