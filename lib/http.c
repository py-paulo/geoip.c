#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "http.h"

/*
 * Routines to manage HTTP connections
 */
typedef enum { ST_REQ, ST_HDRS, ST_BODY, ST_DONE } HTTP_STATE;

struct http {
    FILE *file;
    HTTP_STATE state;	/* State of the connection */
    int code;			/* Response code */
    char version[4];	/* HTTP version from the response */
    char *response;		/* Response string with message */
    int sock;           /* Socket connection */
};

/*
 * Open an HTTP connection for a specified IP address and port number
 */
HTTP *
http_open(URL *uri, IPADDR *addr, int port)
{
	HTTP *http;
	struct sockaddr_in sa;

	if ( addr == NULL )
		return NULL;
	if ( ( http = malloc(sizeof(*http)) ) == NULL )
		return NULL;

	bzero(http, sizeof(*http));

	if ( ( http->sock = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
		free(http);
		return NULL;
	}

	bzero(&sa, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	bcopy(addr, &sa.sin_addr.s_addr, sizeof(struct in_addr));

	if ( connect(http->sock, (struct sockaddr *)(&sa), sizeof(sa) ) < 0 ) {
		free(http);
		close(http->sock);
		return NULL;
	}

	http->state = ST_REQ;

    return http;
}

/*
 * Close an HTTP connection that was previously opened.
 */
int
http_close(HTTP *http)
{
    int err;

    shutdown(http->sock, SHUT_RDWR);
    err = close(http->sock);
    free(http->response);
    // http_free_headers(http->headers);
    free(http);

    return err;
}

/*
 * Obtain the underlying FILE in an HTTP connection.
 * This can be used to issue additional headers after the request.
 */
FILE *
http_file(HTTP *http)
{
    return(http->file);
}

/*
 * Issue an HTTP GET request for a URL on a connection.
 * After calling this function, the caller may send additional
 * headers, if desired, then must call http_response() before
 * attempting to read any document returned on the connection.
 */
int http_request(HTTP *http, URL *uri)
{
    void *prev;
    char header_request[200];

    if ( http->state != ST_REQ )
        return 1;

    /* Ignore SIGPIPE so we don't die while doing this */
    prev = signal(SIGPIPE, SIG_IGN);
    if ( sprintf(header_request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
            url_path(uri), url_hostname(uri) ) == -1) {
        signal(SIGPIPE, prev);
        return 1;
    }

    write(http->sock, header_request, strlen (header_request));
    http->state = ST_HDRS;
    signal(SIGPIPE, prev);

    return 0;
}

/*
 * Finish outputting an HTTP request and read the reply
 * headers from the response.  After calling this, http_getc()
 * may be used to collect any document returned as part of the
 * response.
 */
int
http_response(HTTP *http)
{
    char buffer[1024];

    /*
    if ( sprintf(http->sock, "\r\n" ) == -1 ) {
        return 1;
    }
    */
    bzero(buffer, 1024);
    
    while ( read(http->sock, buffer, 1024 - 1) != 0 ) {
        fprintf(stderr, "%s", buffer);
        bzero(buffer, 1024);
    }
    return 0;
}

/*
 * Retrieve the HTTP status line and code returned as the
 * first line of the response from the server
 */
char *
http_status(HTTP *http, int *code)
{
    if ( http->state != ST_BODY )
        return NULL;
    if ( code != NULL )
        *code = http->code;
    return http->response;
}

/*
 * Read the next character of a document from an HTTP connection
 */
int
http_getc(HTTP *http)
{
    if ( http->state != ST_BODY )
        return EOF;
    return fgetc(http->file);
}


/*
 * Routines to interpret and manage URL's
 */
struct url {
    char *stuff;			/* Working storage containing all parts */
    char *method;			/* The access method (http, ftp, etc.) */
    char *hostname;		    /* The server host name */
    int port;			    /* The TCP port to contact */
    char *path;			    /* The path of the document on the server */
    int dnsdone;			/* Have we done DNS lookup yet? */
    struct in_addr addr;	/* IP address of the server */
};

/*
 * Parse a URL given as a string into its constituent parts,
 * and return it as a URL object.
 */
URL *
url_parse(char *url)
{
    URL *up;
    char *cp, c;
    char *slash, *colon;

    if ( ( up = malloc(sizeof(*up)) ) == NULL )
        return NULL;
    /*
    * Make a copy of the argument that we can fiddle with
    */
    if ( ( up->stuff = strdup(url) ) == NULL ) {
        free(up);
        return(NULL);
    }
    up->dnsdone = 0;
    bzero(&up->addr, sizeof(struct in_addr));
    /*
    * Now ready to parse the URL
    */
    cp = up->stuff;
    slash = strchr(cp, '/');
    colon = strchr(cp, ':');
    if ( colon != NULL ) {
        /*
        * If a colon occurs before any slashes, then we assume the portion
        * of the URL before the colon is the access method.
        */
        if ( slash == NULL || colon < slash ) {
            *colon = '\0';
            free(up->method);
            up->method = strdup(cp);
            cp = colon+1;

            if ( !strcasecmp(up->method, "http") )
                up->port = 80;
        }

        if ( slash != NULL && *(slash+1) == '/' ) {
            /*
            * If there are two slashes, then we have a full, absolute URL,
            * and the following string, up to the next slash, colon or the end
            * of the URL, is the host name.
            */
            for ( cp = slash+2; *cp != '\0' && *cp != ':' && *cp != '/'; cp++ );
            c = *cp;

            *cp = '\0';
            free(up->hostname);
            up->hostname = strdup(slash+2);
            *cp = c;
            /*
            * If we found a ':', then we have to collect the port number
            */
            if ( *cp == ':' ) {
                char *cp1;
                cp1 = ++cp;
                while ( isdigit(*cp) )
                    cp++;
                c = *cp;
                *cp = '\0';
                up->port = atoi(cp1);
                *cp = c;
            }
        }
        if ( *cp == '\0' )
            up->path = "/";
        else
            up->path = cp;
    } else {
        /*
        * No colon: a relative URL with no method or hostname
        */
        up->path = cp;
    }
    return up;
}

/*
 * Free a URL object that was previously created by url_parse()
 */
void
url_free(URL *up)
{
    free(up->stuff);
    if(up->method != NULL) free(up->method);
    if(up->hostname != NULL) free(up->hostname);
    free(up);
}

/*
 * Extract the "access method" portion of a URL
 */
char *
url_method(URL *up)
{
    return up->method ;
}

/*
 * Extract the hostname portion of a URL
 */
char *
url_hostname(URL *up)
{
    return up->hostname;
}

/*
 * Obtain the TCP port number portion of a URL
 */
int
url_port(URL *up)
{
    return up->port;
}

/*
 * Obtain the path portion of a URL
 */
char *
url_path(URL *up)
{
    return up->path;
}

/*
 * Obtain the network (IP) address of the host specified in a URL.
 * This will cause a DNS lookup if the address is not already cached
 * in the URL object.
 */
IPADDR *
url_address(URL *up)
{
    struct hostent *he;

    if ( !up->dnsdone ) {
        if ( up->hostname != NULL && *up->hostname != '\0' ) {
            if ( ( he = gethostbyname(up->hostname)) == NULL )
                return NULL;
            bcopy(he->h_addr, &up->addr, sizeof(struct in_addr));
        }
        up->dnsdone = 1;
    }
    return &up->addr;
}

/*
 * Print out the components of a parsed URL, for debugging purposes
 */
void
url_print(FILE *f, URL *uri)
{
    fprintf(f, "URL [method: '%s', host: '%s (%s)', "
	    "port: %d, path: '%s']\n",
	    uri->method != NULL ? uri->method : "?",
	    uri->hostname != NULL ? uri->hostname : "?",
	    uri->dnsdone ? inet_ntoa(uri->addr) : "?.?.?.?",
	    uri->port, uri->path);
}