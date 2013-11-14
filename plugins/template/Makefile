# Makefile for gladd plugin
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

PROGRAM=plugin
INCLUDES=-I/usr/include/libxml2
OBJECTS=plugin.o
LIBS=-lxml2

${PROGRAM}: main.c ${OBJECTS}
	gcc -Wall -Werror -g ${INCLUDES} -o ${PROGRAM} main.c ${OBJECTS} ${LIBS}

plugin.o: plugin.c plugin.h
	gcc -Wall -Werror -g ${INCLUDES} -c plugin.c ${LIBS}

tests.o: tests.c tests.h
	gcc -Wall -Werror -g ${INCLUDES} -c tests.c ${LIBS}

.PHONY: clean test

clean:
	rm *.o ${PROGRAM} ${PROGRAM}_test || true

test: clean ${PROGRAM} ${PROGRAM}_test.c tests.o ${OBJECTS}
	gcc -Wall -Werror -g ${INCLUDES} -o ${PROGRAM}_test ${PROGRAM}_test.c tests.o ${OBJECTS} ${LIBS}
	./${PROGRAM}_test || true
