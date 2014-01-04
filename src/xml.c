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
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include "config.h"
#include "http.h"
#include "string.h"
#include "xml.h"

int flattenxml(xmlDocPtr doc, char **xml, int pretty);
void xml_prepend_element(xmlDocPtr docxml, char *name, char *value);

static void xmlSchemaValidityErrorFunc_impl(void __attribute__((unused)) *ctx,
        const char *msg, ...);
static void xmlSchemaValidityWarningFunc_impl(void __attribute__((unused))
        *ctx, const char *msg, ...);

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
        int isconn = 0;
        int rowc = 0;
        row_t *rows = NULL;
        row_t *r = NULL;
        xmlNodePtr n = NULL;
        xmlNodePtr nrow = NULL;
        xmlNodePtr nfld = NULL;
        xmlNodePtr nval = NULL;
        xmlDocPtr doc;
        field_t *f;
        char *fname;
        char *fvalue;
        char *newsql;

        /* connect if we aren't already */
        if (db->conn == NULL) {
                if (db_connect(db) != 0) {
                        syslog(LOG_ERR, "Failed to connect to db on %s",
                                db->host);
                        return -1;
                }
                isconn = 1;
        }

        /* do variable substitution */
        newsql = strdup(sql);
        sqlvars(&newsql, request->res);
        syslog(LOG_DEBUG, "SQL: %s\n", newsql);

        if (db_fetch_all(db, newsql, filter, &rows, &rowc) < 0) {
                syslog(LOG_ERR, "Error in db_fetch_all()");
        }
        free(newsql);

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

        /* leave the connection how we found it */
        if (isconn == 1)
                db_disconnect(db);

        return 0;
}

/* subsitute variables in sql for their values */
void sqlvars(char **sql, char *url)
{
        char **tokens;
        int toknum;
        char *sqltmp;
        char *var;
        int i;

        tokens = tokenize(&toknum, &url, "/");

        for (i=1; i <= toknum; i++) {
                asprintf(&var, "$%i", i-1);
                sqltmp = replaceall(*sql, var, tokens[i]);
                free(var);
                free(*sql);
                *sql = strdup(sqltmp);
                free(sqltmp);
        }

        /* replace occurances of $user with username */
        if (request) {
                if (request->authuser) {
                        char *auser = strdup(request->authuser);
                        sqltmp = replaceall(*sql, "$user", auser);
                        *sql = strdup(sqltmp);
                        free(sqltmp);
                        free(auser);
                }
        }

        free(tokens);
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

/* perform xslt transformation on xml
 * remember to free() output in calling function when done */
int xmltransform(const char *xslt_filename, const char *xml, char **output)
{
        xsltStylesheetPtr docxslt;
        xmlDocPtr docxml;
        xmlDocPtr docsql;
        xmlChar *sqlout;
        int doclen;

        /* check stylesheet file exists */
        if (access(xslt_filename, R_OK) != 0) {
                syslog(LOG_ERR, "read access denied to '%s'", xslt_filename);
                return -1;
        }

        /* tell libxml2 parser to substitute entities */
        xmlSubstituteEntitiesDefault(1);

        /* tell libxml2 to load external subsets */
        xmlLoadExtDtdDefaultValue = 1;

        /* read the stylesheet */
        docxslt = xsltParseStylesheetFile((const xmlChar *)xslt_filename);

        /* read the xml doc in from memory */
        docxml = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
        if (docxml == NULL) {
                xsltFreeStylesheet(docxslt);
                return -1;
        }

        /* add some server variables to xml before transformation */
        if ((config->xforward == 1) && (request->xforwardip)) {
                /* behind proxy, so use X-Forwarded-For header */
                xml_prepend_element(docxml, "clientip", request->xforwardip);
        }
        else {
                xml_prepend_element(docxml, "clientip", request->clientip);
        }
        xml_prepend_element(docxml, "authuser", request->authuser);

        /* perform the transformation */
        docsql = xsltApplyStylesheet(docxslt, docxml, NULL);
        assert(docsql != NULL);

        /* flatten output */
        xsltSaveResultToString(&sqlout, &doclen, docsql, docxslt);
        asprintf(output, "%s", (char *)sqlout);

        /* cleanup */
        free(sqlout);
        xsltFreeStylesheet(docxslt);
        xmlFreeDoc(docxml);
        xmlFreeDoc(docsql);
        xsltCleanupGlobals();
        xmlCleanupParser();

        return 0;
}

void xml_prepend_element(xmlDocPtr docxml, char *name, char *value)
{
        xmlNodePtr r;
        xmlNodePtr n;
        xmlNodePtr nfld = NULL;
        xmlNodePtr nval = NULL;

        nfld = xmlNewNode(NULL, BAD_CAST name);
        nval = xmlNewText(BAD_CAST value);
        xmlAddChild(nfld, nval);
        r = xmlDocGetRootElement(docxml);
        n = xmlFirstElementChild(r);
        xmlAddPrevSibling(n, nfld);
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
                syslog(LOG_DEBUG, "xmlReadMemory() failed");
                return -1;
        }

        syslog(LOG_DEBUG, "loading schema '%s'", schema_filename);
        docschema = xmlReadFile(schema_filename, NULL, XML_PARSE_NONET);
        if (docschema == NULL) {
                /* the schema cannot be loaded or is not well-formed */
                syslog(LOG_DEBUG,
                        "the schema cannot be loaded or is not well-formed");
                return -2;
        }

        parser_ctxt = xmlSchemaNewDocParserCtxt(docschema);
        if (parser_ctxt == NULL) {
                /* unable to create a parser context for the schema */
                syslog(LOG_DEBUG,
                        "unable to create a parser context for the schema");
                xmlFreeDoc(docschema);
                return -3;
        }

        schema = xmlSchemaParse(parser_ctxt);
        if (schema == NULL) {
                /* the schema itself is not valid */
                syslog(LOG_DEBUG, "invalid schema");
                xmlSchemaFreeParserCtxt(parser_ctxt);
                xmlFreeDoc(docschema);
                return -4;
        }

        xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
        if (valid_ctxt == NULL) {
                /* unable to create a validation context for the schema */
                syslog(LOG_DEBUG, 
                        "unable to create a validation context for the schema");
                xmlSchemaFree(schema);
                xmlSchemaFreeParserCtxt(parser_ctxt);
                xmlFreeDoc(docschema);
                return -5;
        }

        /* register error callbacks */
        xmlSchemaSetValidErrors(valid_ctxt, &xmlSchemaValidityErrorFunc_impl,
               &xmlSchemaValidityWarningFunc_impl, NULL);

        syslog(LOG_DEBUG, "validating xml document");

        is_valid = xmlSchemaValidateDoc(valid_ctxt, docxml);
        if (is_valid == 0) {
                syslog(LOG_DEBUG, "xml validates");
        }
        else if (is_valid < 0) {
                syslog(LOG_DEBUG, "libxml2 internal or API error");
        }
        else if (is_valid > 0) {
                syslog(LOG_DEBUG, "xmlSchemaValidateDoc() error code %i",
                        is_valid);
        }

        /* clean up */
        xmlSchemaFreeValidCtxt(valid_ctxt);
        xmlSchemaFree(schema);
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(docschema);
        xmlFreeDoc(docxml);
        xmlCleanupParser();

        /* 0 = success, -1 = failure */
        return is_valid == 0 ? 0 : -1 ;
}

static void xmlSchemaValidityErrorFunc_impl(void __attribute__((unused)) *ctx,
const char *msg, ...) {
        static char buffer[LINE_MAX];
        va_list argp;
        va_start(argp, msg);
        vsprintf(buffer, msg, argp);
        va_end(argp);
        syslog(LOG_DEBUG, "%s", buffer);
}

static void xmlSchemaValidityWarningFunc_impl(void __attribute__((unused))
*ctx, const char *msg, ...) {
        static char buffer[LINE_MAX];
        va_list argp;
        va_start(argp, msg);
        vsprintf(buffer, msg, argp);
        va_end(argp);
        syslog(LOG_DEBUG, "%s", buffer);
}
