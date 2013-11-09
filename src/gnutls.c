/* 
 * gnutls.c
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

#include "gnutls.h"
#include "config.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

gnutls_certificate_credentials_t x509_cred;
gnutls_priority_t priority_cache;

int generate_dh_params(void)
{
          unsigned int bits = 
            gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_LOW);

          fprintf(stderr, "Generating Diffie-Hellman params...");

          /* Generate Diffie-Hellman parameters - for use with DHE
           * kx algorithms. When short bit length is used, it might
           * be wise to regenerate parameters often.
           */
          gnutls_dh_params_init(&dh_params);
          gnutls_dh_params_generate2(dh_params, bits);

          fprintf(stderr, "done\n");

          return 0;
}

void do_tls_handshake(int fd)
{
        int ret;
        gnutls_transport_set_int(session, fd);
        do
        {       
                ret = gnutls_handshake(session);
        }      
        while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

        if (ret < 0) {
                close(fd);
                gnutls_deinit(session);
                fprintf (stderr, "*** Handshake has failed (%s)\n\n",
                        gnutls_strerror(ret));
                _exit(EXIT_FAILURE);
        }
        else {
                char* desc;
                desc = gnutls_session_get_desc(session);
                printf ("- Session info: %s\n", desc);
                gnutls_free(desc);
        }
        printf ("- Handshake was completed\n");
}

/* set a TCP cork the gnutls way */
void setcork_ssl(int state)
{
        if (state == 0) {
                gnutls_record_cork(session);
        }
        else {
                gnutls_record_uncork(session, 0);
        }
}

int sendfile_ssl(int sock, int fd, size_t size)
{
        char buf[4096];
        size_t bytes;
        size_t sent = 0;
        int ret;
        int offset = 0;

        fprintf(stderr, "Sending file...");

        /* read from file descriptor and send to ssl socket */
        do {
                if (offset == 0) bytes = read(fd, buf, sizeof buf);
                ret = gnutls_record_send(session, buf+offset, bytes-offset);
                while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                        ret = gnutls_record_send(session, 0, 0);
                }
                if (ret >= 0) {
                        sent += ret;
                        offset = bytes - ret;
                }
                else if (gnutls_error_is_fatal(ret) == 1) {
                        break;
                }

        } while (sent < size);
        fprintf(stderr, "done.\n");
        fprintf(stderr, "%i/%i bytes sent\n", (int)sent, (int)size);
        
        return sent;
}

size_t ssl_send(char *msg, ...)
{
        char s[4096];
        va_list args;
        int ret;

        /* build message */
        va_start(args, msg);
        int size = vsnprintf(s, LINE_MAX, msg, args);
        va_end(args);
        if (size < 0) return -1;

        size_t sent = 0;
        int offset = 0;
        /* "Begin at the beginning," the King said gravely, */
        do {
                ret = gnutls_record_send(session, s+offset, size-offset);
                while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                        ret = gnutls_record_send(session, 0, 0);
                }
                if (ret >= 0) {
                        sent += ret;
                        offset = size - sent;
                }
                else if (gnutls_error_is_fatal(ret) == 1) {
                        break;
                }
        } while (sent < size);
        /* "and go on till you come to the end: then stop." */

        if (ret < 0) {
                fprintf(stderr, "%s\n", gnutls_strerror(ret));
        }

        return sent;
}

size_t ssl_recv(char *b, int bytes)
{
        int ret;
        /* TODO: loop until done */
        /* TODO: MSG_WAITALL */
        ret = gnutls_record_recv(session, b, bytes);
        while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                ret = gnutls_record_recv(session, 0, 0);
        }
        b[ret] = '\0';
        return ret;
}

void ssl_setup()
{
        int ret;

        gnutls_global_init();
        gnutls_certificate_allocate_credentials (&x509_cred);
        gnutls_certificate_set_x509_trust_file (x509_cred, config->sslca,
                GNUTLS_X509_FMT_PEM);
        gnutls_certificate_set_x509_crl_file (x509_cred, config->sslcrl,
                GNUTLS_X509_FMT_PEM);
        ret = gnutls_certificate_set_x509_key_file (x509_cred, config->sslcert,
                config->sslkey, GNUTLS_X509_FMT_PEM);
        if (ret < 0) {
                fprintf(stderr, "No certificate or key were found\n");
                _exit(EXIT_FAILURE);
        }
        generate_dh_params();
        gnutls_priority_init (&priority_cache,
                "PERFORMANCE:%SERVER_PRECEDENCE", NULL);
        gnutls_certificate_set_dh_params (x509_cred, dh_params);
        gnutls_init(&session, GNUTLS_SERVER);
        gnutls_priority_set(session, priority_cache);
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);
        gnutls_certificate_server_set_request(session, GNUTLS_CERT_IGNORE);

        fprintf(stderr, "SSL setup complete\n");
}

void ssl_cleanup(int fd)
{
        gnutls_bye(session, GNUTLS_SHUT_WR);
        close(fd);
        gnutls_deinit(session);
        gnutls_certificate_free_credentials(x509_cred);
        gnutls_priority_deinit(priority_cache);
        gnutls_global_deinit();
}
