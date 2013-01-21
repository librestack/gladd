# Makefile for gladd
# 
# this file is part of GLADD
# 
# Copyright (c) 2012, 2013 Brett Sheffield <brett@gladserv.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program (see the file COPYING in the distribution).
# If not, see <http://www.gnu.org/licenses/>.

CC=gcc

# the following are valid in CFLAGS:
# -D_NPG   => compile without postgres support
# -D_NMY   => compile without mysql support
# -D_NLDAP => compile without ldap support
# -D_NXML  => compile without xml support
CFLAGS=-g -Wall -Werror

INCLUDES=-I/usr/include/libxml2 -I/usr/include/postgresql
MYSQLFLAGS=`mysql_config --cflags --libs`
LIBS=-L/usr/lib/x86_64-linux-gnu -lxml2 -lpq ${MYSQLFLAGS} -lmysqlclient -lldap
PROGRAM=gladd
INSTALLDIR=/usr/local/bin/
OBJECTFILES=args.o auth.o db.o config.o handler.o http.o mime.o signals.o string.o xml.o
TESTFILES=test.o args_test.o auth_test.o db_test.o config_test.o string_test.o xml_test.o

${PROGRAM}: ${PROGRAM}_test main.o
	${CC} ${CFLAGS} ${INCLUDES} -o ${PROGRAM} ${OBJECTFILES} main.o ${LIBS}

${PROGRAM}_test: ${OBJECTFILES} ${TESTFILES}
	${CC} ${CFLAGS}  ${INCLUDES} -o ${PROGRAM}_test ${TESTFILES} ${OBJECTFILES} ${LIBS}

args.o: args.c args.h
	${CC} ${CFLAGS} -c args.c

args_test.o: args_test.c args_test.h
	${CC} ${CFLAGS} -c args_test.c

auth.o: auth.c auth.h
	${CC} ${CFLAGS} -c auth.c

auth_test.o: auth_test.c auth_test.h
	${CC} ${CFLAGS} -c auth_test.c

config.o: config.c config.h
	${CC} ${CFLAGS} -c config.c

config_test.o: minunit.h config_test.c config_test.h
	${CC} ${CFLAGS} -c config_test.c

db.o: db.c db.h
	${CC} ${CFLAGS} ${INCLUDES} -c db.c ${LIBS}

db_test.o: db_test.c db_test.h
	${CC} ${CFLAGS} -c db_test.c

handler.o: handler.c handler.h http.h main.h mime.h config.h
	${CC} ${CFLAGS} ${INCLUDES} -c handler.c ${LIBS}

http.o: http.c http.h main.h
	${CC} ${CFLAGS} -c http.c

main.o: main.c main.h handler.h config.h
	${CC} ${CFLAGS} -c main.c

mime.o: mime.c mime.h
	${CC} ${CFLAGS} -c mime.c

signals.o: signals.c signals.h
	${CC} ${CFLAGS} -c signals.c

string.o: string.c string.h
	${CC} ${CFLAGS} -c string.c

string_test.o: string_test.c string_test.h
	${CC} ${CFLAGS} -c string_test.c

test.o: minunit.h test.c test.h db_test.h config_test.h args_test.h auth_test.h
	${CC} ${CFLAGS} -c test.c

xml.o: xml.h xml.c
	${CC} ${CFLAGS} ${INCLUDES} -c xml.c ${LIBS}

xml_test.o: minunit.h xml_test.c xml_test.h
	${CC} ${CFLAGS} -c xml_test.c


.PHONY: clean test leakcheck install

clean:
	rm ${PROGRAM} ${PROGRAM}_test main.o ${OBJECTFILES} ${TESTFILES} core

test:
	make clean || true
	make && ./${PROGRAM}_test || true

leakcheck:
	valgrind -v  --leak-check=full --show-reachable=yes --suppressions=.valgrind.suppressions ./gladd_test

install:
	cp ${PROGRAM} ${INSTALLDIR}
