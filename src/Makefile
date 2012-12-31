CC=gcc
CFLAGS=-g -Wall -Werror
INCLUDES=-I/usr/include/libxml2
LIBS=-L/usr/lib/x86_64-linux-gnu -lxml2 -lpq
PROGRAM=gladd
OBJECTFILES=handler.o http.o main.o mime.o

gladd: main.c ${OBJECTFILES}
	${CC} ${CFLAGS} ${INCLUDES} -o ${PROGRAM} ${OBJECTFILES} ${LIBS}

handler.o: handler.c main.h
	${CC} ${CFLAGS} ${INCLUDES} -c handler.c ${LIBS}

http.o: http.c http.h
	${CC} ${CFLAGS} ${INCLUDES} -c http.c

main.o: main.c main.h
	${CC} ${CFLAGS} ${INCLUDES} -c main.c

mime.o: mime.c main.h
	${CC} ${CFLAGS} ${INCLUDES} -c mime.c

.PHONY: clean

clean:
	rm ${PROGRAM} ${OBJECTFILES}
