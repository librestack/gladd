/*
 * xml.h - produce xml output
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

#ifndef __GLADD_XML_H__
#define __GLADD_XML_H__ 1

#include "db.h"

int buildxml(char **xmldoc);
int sqltoxml(db_t *db, char *sql, field_t *filter, char **xml, int pretty);
int xmltransform(const char *xslt_filename, const char *xml, char **output);
int xml_validate(const char *schema_filename, const char *xml);

#endif /* __GLADD_XML_H__ */
