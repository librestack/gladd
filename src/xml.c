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
#include <libxml/xmlschemas.h>
#include <string.h>
#include <syslog.h>
#include "config.h"
#include "string.h"
#include "xml.h"

int flattenxml(xmlDocPtr doc, char **xml, int pretty);

int buildxml(char **xml)
{
        xmlNodePtr n;
        xmlDocPtr doc;

        doc = xmlNewDoc(BAD_CAST "1.0");
        n = xmlNewNode(NULL, BAD_CAST "resources");
        xmlDocSetRootElement(doc, n);

        flattenxml(doc, xml, 1);
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return 0;
}

int sqltoxml(db_t *db, char *sql, field_t *filter, char **xml, int pretty)
{
        int rowc;
        row_t *rows;
        row_t *r;
        xmlNodePtr n = NULL;
        xmlNodePtr nrow = NULL;
        xmlNodePtr nfld = NULL;
        xmlNodePtr nval = NULL;
        xmlDocPtr doc;
        field_t *f;
        char *fname;
        char *fvalue;

        if (db_connect(db) != 0) {
                syslog(LOG_ERR, "Failed to connect to db on %s", db->host);
                return -1;
        }

        if (db_fetch_all(db, sql, filter, &rows, &rowc) < 0) {
                syslog(LOG_ERR, "Error in db_fetch_all()");
        }

        doc = xmlNewDoc(BAD_CAST "1.0");
        n = xmlNewNode(NULL, BAD_CAST "resources");
        xmlDocSetRootElement(doc, n);

        /* insert query results */
        syslog(LOG_DEBUG, "Rows returned: %i\n", rowc);
        if (rowc > 0) {
                r = rows;
                while (r != NULL) {
                        f = r->fields;
                        nrow = xmlNewNode(NULL, BAD_CAST "row");
                        while (f != NULL) {
                                asprintf(&fname, "%s", f->fname);
                                asprintf(&fvalue, "%s", strip(f->fvalue));

                                /* TODO: ensure strings are UTF-8 encoded */

                                nfld = xmlNewNode(NULL, BAD_CAST fname);
                                nval = xmlNewText(BAD_CAST fvalue);
                                xmlAddChild(nfld, nval);
                                xmlAddChild(nrow, nfld);

                                free(fname);
                                free(fvalue);

                                f = f->next;
                        }
                        xmlAddChild(n, nrow);
                        r = r->next;
                }
        }
        liberate_rows(rows);

        flattenxml(doc, xml, pretty);

        xmlFreeDoc(doc);
        xmlCleanupParser();

        db_disconnect(db);

        return 0;
}

int flattenxml(xmlDocPtr doc, char **xml, int pretty)
{
        xmlChar *xmlbuff;
        int buffersize;

        xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, 
                                        config->encoding, pretty);
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

/* 
 * Based on an example by Volker Grabsch at:
 * http://wiki.njh.eu/XML-Schema_validation_with_libxml2
 * site is licenced under GNU Free Documentation License 1.2.
 */
int xml_validate(const char *schema_filename, const char *xml)
{
        xmlDocPtr docschema;
        xmlDocPtr docxml;
        xmlSchemaParserCtxtPtr parser_ctxt;
        xmlSchemaPtr schema;
        int is_valid;

        /* parse xml from memory */
        docxml = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
        if (docxml == NULL) {
                return -1;
        }

        docschema = xmlReadFile(schema_filename, NULL, XML_PARSE_NONET);
        if (docschema == NULL) {
                /* the schema cannot be loaded or is not well-formed */
                return -2;
        }

        parser_ctxt = xmlSchemaNewDocParserCtxt(docschema);
        if (parser_ctxt == NULL) {
                /* unable to create a parser context for the schema */
                xmlFreeDoc(docschema);
                return -3;
        }

        schema = xmlSchemaParse(parser_ctxt);
        if (schema == NULL) {
                /* the schema itself is not valid */
                xmlSchemaFreeParserCtxt(parser_ctxt);
                xmlFreeDoc(docschema);
                return -4;
        }

        xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
        if (valid_ctxt == NULL) {
                /* unable to create a validation context for the schema */
                xmlSchemaFree(schema);
                xmlSchemaFreeParserCtxt(parser_ctxt);
                xmlFreeDoc(docschema);
                return -5;
        }
        is_valid = (xmlSchemaValidateDoc(valid_ctxt, docxml) == 0);

        /* clean up */
        xmlSchemaFreeValidCtxt(valid_ctxt);
        xmlSchemaFree(schema);
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(docschema);
        xmlFreeDoc(docxml);

        /* force the return value to be non-negative on success */
        return is_valid ? 0 : 1;
}
