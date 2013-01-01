CC=gcc
CFLAGS=-g -Wall -Werror
INCLUDES=-I/usr/include/libxml2
LIBS=-L/usr/lib/x86_64-linux-gnu -lxml2 -lpq
PROGRAM=gladd
OBJECTFILES=handler.o http.o main.o mime.o
TESTFILES=test.o config.o config_test.o

gladd: ${OBJECTFILES}
	${CC} ${CFLAGS} ${INCLUDES} -o ${PROGRAM} ${OBJECTFILES} ${LIBS}

handler.o: handler.c handler.h http.h main.h mime.h
	${CC} ${CFLAGS} ${INCLUDES} -c handler.c ${LIBS}

http.o: http.c http.h main.h
	${CC} ${CFLAGS} ${INCLUDES} -c http.c

main.o: main.c main.h handler.h
	${CC} ${CFLAGS} ${INCLUDES} -c main.c

mime.o: mime.c mime.h
	${CC} ${CFLAGS} ${INCLUDES} -c mime.c

config.o: config.c config.h
	${CC} ${CFLAGS} -c config.c

config_test.o: config_test.c config_test.h
	${CC} ${CFLAGS} -c config_test.c

test.o: test.c test.h config_test.h
	${CC} ${CFLAGS} ${INCLUDES} -c test.c


.PHONY: clean test testclean

clean:
	rm ${PROGRAM} ${OBJECTFILES} test test.o

test: ${TESTFILES}
	${CC} ${CFLAGS} ${INCLUDES} -o test ${TESTFILES}
	./test
