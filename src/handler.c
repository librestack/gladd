/*
 * handler.c
 * some code to handle incoming connections
 */

#include "main.h"
#include "handler.h"
#include <libxml/parser.h>
#include <sys/sendfile.h>

/*
 * get sockaddr, IPv4 or IPv6:
 */
void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 * child process where we handle incoming connections
 */
void handle_connection(int sock, struct sockaddr_storage their_addr)
{
        /* FIXME: some very lazy memory allocation here */
        char *basefile;
        char *guess;
        char *r;
        char *xml;
        char basedir[BUFSIZE];
        char buf[BUFSIZE];
        char filename[BUFSIZE];
        char httpv[BUFSIZE];
        char method[BUFSIZE];
        char resource[BUFSIZE];
        char s[INET6_ADDRSTRLEN];
        int byte_count;
        int state;

        inet_ntop(their_addr.ss_family,
                        get_in_addr((struct sockaddr *)&their_addr),
                        s, sizeof s);

        syslog(LOG_INFO, "server: got connection from %s", s);

        /* What are we being asked to do? */
        byte_count = recv(sock, buf, sizeof buf, 0);
        syslog(LOG_DEBUG, "recv()'d %d bytes of data in buf\n", byte_count);

        if (sscanf(buf, "%s %s HTTP/%s", method, resource, httpv) != 3) {
                syslog(LOG_INFO, "Invalid Request");
                exit(EXIT_FAILURE);
        }

        syslog(LOG_DEBUG, "Method: %s", method);
        syslog(LOG_DEBUG, "Resource: %s", resource);
        syslog(LOG_DEBUG, "HTTP Version: %s", httpv);

        /* Return HTTP response */

        /* put a cork in it */
        state = 1;
        setsockopt(sockme, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));

        /* TODO: refactor to remove duplication */
        if (strncmp(method, "GET", 3) == 0) {
                if (strncmp(resource, "/static/", 8) == 0) {
                        basefile = strndup(resource+8, sizeof(resource));
                        sprintf(basedir,
                                       "/home/bacs/dev/gladd/static/");
                        sprintf(filename, "%s%s", basedir, basefile);
                        send_file(sock, filename);
                        free(basefile);
                }
                else if (strncmp(resource, "/guess/", 7) == 0){
                        guess = strndup(resource+7, sizeof(resource));
                        syslog(LOG_DEBUG, "guess: %s", guess);
                        prepare_response(resource, &xml, guess);
                        if (asprintf(&r, RESPONSE_200, MIME_XML, xml) == -1) {
                                syslog(LOG_ERR, "Malloc failed");
                                exit(EXIT_FAILURE);
                        }
                        respond(sock, r);
                        free(guess);
                        free(xml);
                        free(r);
                }
                else {
                        send_file(sock, 
                                "/home/bacs/dev/gladd/static/index.html");
                }
        }
        else {
                respond(sock, RESPONSE_405);
        }

        /* close client connection */
        close(sock);

        /* child process can exit */
        exit (EXIT_SUCCESS);
}


/*
 *  prepare an XML document response
 *
 *  free() xml after use
 */
int prepare_response(char resource[], char **xml, char *query)
{
        xmlNodePtr n;
        xmlDocPtr doc;
        xmlChar *xmlbuff;
        int buffersize;

        doc = xmlNewDoc(BAD_CAST "1.0");
        n = xmlNewNode(NULL, BAD_CAST "resources");

        xmlDocSetRootElement(doc, n);

        /* flatten xml */
        xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
        *xml = malloc(snprintf(NULL, 0, "%s", (char *) xmlbuff));
        sprintf(*xml, "%s", (char *) xmlbuff);

        xmlFree(xmlbuff);
        xmlFreeDoc(doc);

        return 0;
}

int send_file(int sock, char *path)
{
        char *r;
        char *mimetype;
        int f;
        int rc;
        off_t offset;
        int state;
        struct stat stat_buf;

        /* TODO: first, check file exists, or return 404 */

        f = open(path, O_RDONLY);
        if (f == -1) {
                syslog(LOG_ERR, "unable to open '%s': %s\n", path,
                                strerror(errno));
                return 1;
        }

        /* get size of file */
        fstat(f, &stat_buf);
        syslog(LOG_DEBUG, "Sending %i bytes", (int)stat_buf.st_size);

        /* send headers */
        mimetype = malloc(strlen(MIME_DEFAULT)+1);
        get_mime_type(mimetype, path);
        syslog(LOG_DEBUG, "Content-Type: %s", mimetype);
        if (asprintf(&r, RESPONSE_200, mimetype, "") == -1) {
                syslog(LOG_ERR, "Malloc failed");
                exit(EXIT_FAILURE);
        }
        free(mimetype);
        respond(sock, r);

        /* send the file */
        errno = 0;
        offset = 0;
        rc = sendfile(sock, f, &offset, stat_buf.st_size);
        if (rc == -1) {
                syslog(LOG_ERR, "error from sendfile: %s\n", strerror(errno));
        }
        syslog(LOG_DEBUG, "Sent %i bytes", rc);

        /* everything sent ? */
        if (rc != stat_buf.st_size) {
                syslog(LOG_ERR,
                        "incomplete transfer from sendfile: %d of %d bytes\n",
                        rc, (int)stat_buf.st_size);
        }

        /* pop my cork */
        state = 0;
        setsockopt(sockme, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));

        close(f);

        return 0;
}
