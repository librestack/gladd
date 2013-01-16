/*
 * xml.c - produce xml output
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

#define _GNU_SOURCE
#include <libxml/parser.h>
#include "config.h"
#include "xml.h"

int flattenxml(xmlDocPtr doc, char **xml);

int buildxml(char **xml)
{
        xmlNodePtr n;
        xmlDocPtr doc;

        doc = xmlNewDoc(BAD_CAST "1.0");
        n = xmlNewNode(NULL, BAD_CAST "resources");
        xmlDocSetRootElement(doc, n);

        flattenxml(doc, xml);
        xmlFreeDoc(doc);

        fprintf(stderr, "%s", *xml); /* FIXME: temp */

        return 0;
}

int sqltoxml(db_t *db, char *sql, char **xml)
{
        int rowc;
        row_t *rows;
        char *cursorsql;
        xmlNodePtr n;
        xmlDocPtr doc;
        field_t *f;

        asprintf(&cursorsql, "DECLARE sqltoxml CURSOR FOR %s", sql);

        db_connect(db);
        db_exec_sql(db, "BEGIN;");
        db_exec_sql(db, cursorsql);
        db_fetch_all(db, "sqltoxml", &rows, &rowc);
        db_exec_sql(db, "END;");
        free(cursorsql);

        doc = xmlNewDoc(BAD_CAST "1.0");
        n = xmlNewNode(NULL, BAD_CAST "resources");
        xmlDocSetRootElement(doc, n);

        /* insert query results */
        fprintf(stderr, "Rows returned: %i\n", rowc);
        if (rowc > 0) {
                f = rows->fields;
                while (f != NULL) {
                        xmlNewTextChild(n, NULL,
                                (xmlChar*) f->fname,
                                (xmlChar*) f->fvalue);
                        f = f->next;
                }
        }
        free_rows(rows);

        flattenxml(doc, xml);
        xmlFreeDoc(doc);

        db_disconnect(db);

        return 0;
}

int flattenxml(xmlDocPtr doc, char **xml)
{
        xmlChar *xmlbuff;
        int buffersize;

        xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
        *xml = malloc(snprintf(NULL, 0, "%s", (char *) xmlbuff) + 1);
        sprintf(*xml, "%s", (char *) xmlbuff);

        xmlFree(xmlbuff);
        
        return 0;
}
