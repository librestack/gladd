#include <stdlib.h>
#include <string.h>
#include "mime.h"

void get_mime_type(char *mimetype, char *filename)
{
        char *fileext;

        /* default for unknown mime type */
        strcpy(mimetype, MIME_DEFAULT);

        /* TODO: use apache's mime.types file */
        if (strrchr(filename, '.')) {
                fileext = strdup(strrchr(filename, '.')+1);
                if (strncmp(fileext, "html", 4) == 0) {
                        strcpy(mimetype, "text/html");
                }
                else if (strncmp(fileext, "xsl", 3) == 0) {
                        strcpy(mimetype, "application/xml");
                }
                else if (strncmp(fileext, "css", 3) == 0) {
                        strcpy(mimetype, "text/css");
                }
                free(fileext);
        }
}
