CFLAGS=-Wall -g

geoip: geoip.o http.o
		cc ${CFLAGS} -o geoip geoip.o http.o

geoip.o: geoip.c lib/http.h
		cc -c ${CFLAGS} geoip.c

http.o: lib/http.c lib/http.h
		cc -c ${CFLAGS} lib/http.c

