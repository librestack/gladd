CC=gcc
CFLAGS=-g -Wall -Werror
INCLUDES=-I/usr/include/libxml2
LIBS=-L/usr/lib/x86_64-linux-gnu -lxml2 -lpq
PROGRAM=gladd
OBJECTFILES=config.o handler.o http.o main.o mime.o
TESTFILES=test.o config_test.o
TESTOBJECTS=config.o

${PROGRAM}: ${OBJECTFILES}
	${CC} ${CFLAGS} ${INCLUDES} -o ${PROGRAM} ${OBJECTFILES} ${LIBS}

${PROGRAM}_test: ${TESTOBJECTS} ${TESTFILES}
	${CC} ${CFLAGS} -o ${PROGRAM}_test ${TESTFILES} ${TESTOBJECTS}

handler.o: handler.c handler.h http.h main.h mime.h config.h
	${CC} ${CFLAGS} ${INCLUDES} -c handler.c ${LIBS}

http.o: http.c http.h main.h
	${CC} ${CFLAGS} ${INCLUDES} -c http.c

main.o: main.c main.h handler.h config.h
	${CC} ${CFLAGS} ${INCLUDES} -c main.c

mime.o: mime.c mime.h
	${CC} ${CFLAGS} ${INCLUDES} -c mime.c

config.o: config.c config.h
	${CC} ${CFLAGS} -c config.c

config_test.o: minunit.h config_test.c config_test.h
	${CC} ${CFLAGS} -c config_test.c

test.o: minunit.h test.c test.h config_test.h
	${CC} ${CFLAGS} ${INCLUDES} -c test.c


.PHONY: clean test

clean:
	rm ${PROGRAM} ${PROGRAM}_test ${OBJECTFILES} ${TESTFILES}

test:
	make clean || true
	make ${PROGRAM}_test
	./${PROGRAM}_test 2> /dev/null || true
