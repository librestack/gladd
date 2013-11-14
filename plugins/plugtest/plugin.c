/* 
 * Example plugin code
 *
 * plugin.c
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
#include <libxml/xpath.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "plugin.h"

char buf[4096] = "";
xmlDocPtr doc;

void xml_cleanup()
{
        if (doc) xmlFreeDoc(doc);
        xmlCleanupParser();
}

char *xml_element(char *element)
{
        char *value;
        xmlXPathContextPtr context;
        xmlXPathObjectPtr result;
        xmlChar *xpath = (xmlChar*) element;

        context = xmlXPathNewContext(doc);
        result = xmlXPathEvalExpression(xpath, context);
        xmlXPathFreeContext(context);
        if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
                xmlXPathFreeObject(result);
                return NULL;
        }
        value = (char *)xmlNodeListGetString(doc,
                result->nodesetval->nodeTab[0]->xmlChildrenNode, 1);

        xmlXPathFreeObject(result);
        return value;
}

void xml_free(void *ptr)
{
        xmlFree(ptr);
}

int xml_parse(char *xml)
{
        int ret = XML_STATUS_OK;
        doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 
                XML_PARSE_NOERROR + XML_PARSE_NOWARNING);
        if (doc == NULL) {
                ret = XML_STATUS_INVALID;
        }
        return ret;
}

size_t read_input(FILE *f)
{
        return fread(buf, sizeof buf, 1, f);
}

ssize_t write_output(FILE *f, int result)
{
        char *response;
        int len;
        switch (result) {
        case XML_STATUS_OK:
                asprintf(&response, RESPONSE, 200, "Success");
                break;
        case XML_STATUS_INVALID:
                asprintf(&response, RESPONSE, 400, "Invalid XML");
                break;
        default:
                asprintf(&response, RESPONSE, 500, "Unknown Error");
                break;
        }
        len = strlen(response);
        size_t size = 0;
        size = fwrite(response, len, 1, f);
        free(response);
        xml_cleanup();
        return (size == len) ? size : -1;
}
