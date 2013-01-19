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
#include <string.h>
#include <syslog.h>
#include "config.h"
#include "string.h"
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
        xmlCleanupParser();

        return 0;
}

int sqltoxml(db_t *db, char *sql, char **xml)
{
        int rowc;
        row_t *rows;
        row_t *r;
        xmlNodePtr n = NULL;
        xmlNodePtr nfld = NULL;
        xmlNodePtr nval = NULL;
        xmlDocPtr doc;
        field_t *f;

        if (db_connect(db) != 0) {
                syslog(LOG_ERR, "Failed to connect to db on %s", db->host);
                return -1;
        }
        syslog(LOG_DEBUG, "sqltoxml()"); /* FIXME: temp */

        db_fetch_all(db, sql, &rows, &rowc);

        doc = xmlNewDoc(BAD_CAST "1.0");
        n = xmlNewNode(NULL, BAD_CAST "resources");
        xmlDocSetRootElement(doc, n);

        /* insert query results */
        syslog(LOG_DEBUG, "Rows returned: %i\n", rowc);
        if (rowc > 0) {
                r = rows;
                while (r != NULL) {
                        f = rows->fields;
                        while (f != NULL) {
                                syslog(LOG_DEBUG, "F: %s", f->fname);
                                syslog(LOG_DEBUG, "V: %s", f->fvalue);

                                char *fname;
                                char *fvalue;

                                asprintf(&fname, "%s", f->fname);
                                asprintf(&fvalue, "%s", f->fvalue);

                                /* TODO: ensure strings are UTF-8 encoded */

                                nfld = xmlNewNode(NULL, BAD_CAST fname);
                                nval = xmlNewText(BAD_CAST fvalue);
                                xmlAddChild(nfld, nval);
                                xmlAddChild(n, nfld);

                                free(fname);
                                free(fvalue);

                                /* TODO: test nodes are correct here */

                                f = f->next;
                        }
                        r = r->next;
                }
        }
        free_rows(rows);

        flattenxml(doc, xml);

        xmlFreeDoc(doc);
        xmlCleanupParser();

        db_disconnect(db);

        return 0;
}

int flattenxml(xmlDocPtr doc, char **xml)
{
        xmlChar *xmlbuff;
        int buffersize;

        xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, 
                                                        config->encoding, 1);
        *xml = malloc(snprintf(NULL, 0, "%s", (char *) xmlbuff) + 1);
        sprintf(*xml, "%s", (char *) xmlbuff);

        xmlFree(xmlbuff);
        
        return 0;
}

/* convert latin1 (ISO-8859-1) to UTF-8 */
char *toutf8(char *str)
{
        int inlen, outlen;
        inlen = outlen = strlen(str);
        isolat1ToUTF8((unsigned char *) str, &outlen, 
                      (unsigned char *) str, &inlen);
        return str;
}

