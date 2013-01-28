/*
 * string.h - handy string functions
 *
 * this file is part of GLADD
 *
 * Copyright (c) 2012, 2013 Brett Sheffield <brett@gladserv.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING in the distribution).
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BACS_STRING_H__
#define __BACS_STRING_H__ 1

char *lstrip(char *str);
char *replace(char *str, char *find, char *repl);
char *replaceall(char *str, char *find, char *repl);
char *rstrip(char *str);
char *strip(char *str);
char *strstrip(char *s);

#endif /* __BACS_STRING_H__ */
