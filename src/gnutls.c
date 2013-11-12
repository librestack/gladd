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

#include "tls.h"
#include "config.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

#define GNUTLS_POINTER_TO_INT_CAST (long)
#define GNUTLS_POINTER_TO_INT(_) ((int) GNUTLS_POINTER_TO_INT_CAST (_))

gnutls_dh_params_t dh_params;
gnutls_session_t session;
gnutls_certificate_credentials_t x509_cred;
gnutls_priority_t priority_cache;

int generate_dh_params(void)
{
          unsigned int bits = 
            gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_NORMAL);

          syslog(LOG_INFO, "Generating Diffie-Hellman params...");

          /* Generate Diffie-Hellman parameters - for use with DHE
           * kx algorithms. When short bit length is used, it might
           * be wise to regenerate parameters often.  */
          gnutls_dh_params_init(&dh_params);
          gnutls_dh_params_generate2(dh_params, bits);

          syslog(LOG_INFO, "Diffie-Hellman generated.");

          return 0;
}

ssize_t ssl_push(gnutls_transport_ptr_t ptr, const void *data, size_t len)
{
        int sock = GNUTLS_POINTER_TO_INT(ptr);
        return send(sock, data, len, 0);
}

ssize_t ssl_pull(gnutls_transport_ptr_t ptr, void *data, size_t len)
{
        int sock = GNUTLS_POINTER_TO_INT(ptr);
        struct timeval tv;
        tv.tv_sec = 1; tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                (char *)&tv, sizeof(struct timeval));
        int ret = recv(sock, data, len, MSG_WAITALL);
	if (ret == -1 && config->debug) {
		syslog(LOG_DEBUG, "%s", strerror(errno));
	}
	return ret;
}

void do_tls_handshake(int fd)
{
        int ret;

#if GNUTLS_VERSION_NUMBER >= 0x030109
        gnutls_transport_set_int(session, fd); /* 3.1.9+ */
#else
	gnutls_transport_set_ptr(session, (gnutls_transport_ptr_t)(long)fd);
#endif
        gnutls_transport_set_pull_function(session, ssl_pull);
        gnutls_transport_set_push_function(session, ssl_push);
        do
        {       
                ret = gnutls_handshake(session);
        }      
        while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

        if (ret < 0) {
                close(fd);
                gnutls_deinit(session);
                syslog(LOG_ERR, "SSL handshake failed (%s)", 
                        gnutls_strerror(ret));
                _exit(EXIT_FAILURE);
        }
#if GNUTLS_VERSION_NUMBER >= 0x030110
        else {
                char* desc;
                desc = gnutls_session_get_desc(session); /* 3.1.10+ */
                syslog(LOG_DEBUG, "- Session info: %s", desc);
                gnutls_free(desc);
        }
#endif
        syslog(LOG_DEBUG, "SSL Handshake completed");
}

/* set a TCP cork the gnutls way */
void setcork_ssl(int state)
{
#if GNUTLS_VERSION_NUMBER >= 0x030109
        if (state == 0) {
                gnutls_record_cork(session);
        }
        else {
                gnutls_record_uncork(session, 0);
        }
#endif
}

int sendfile_ssl(int sock, int fd, size_t size)
{
        char buf[4096];
        size_t len;
        size_t sent = 0;
        int ret;
        int offset = 0;

        syslog(LOG_DEBUG, "Sending file...");

        /* read from file descriptor and send to ssl socket */
        do {
                if (offset == 0) len = read(fd, buf, sizeof buf);
                ret = gnutls_record_send(session, buf+offset, len-offset);
                while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                        ret = gnutls_record_send(session, 0, 0);
                }
                if (ret >= 0) {
                        sent += ret;
                        offset = len - ret;
                }
                else if (gnutls_error_is_fatal(ret) == 1) {
                        break;
                }

        } while (sent < size);
        syslog(LOG_DEBUG, "%i/%i bytes sent", (int)sent, (int)size);
        
        return sent;
}

size_t ssl_send(char *msg, size_t len)
{
        int ret;
        size_t sent = 0;
        int offset = 0;

        /* "Begin at the beginning," the King said gravely, */
        do {
                ret = gnutls_record_send(session, msg+offset, len-offset);
                while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                        ret = gnutls_record_send(session, 0, 0);
                }
                if (ret >= 0) {
                        sent += ret;
                        offset = len - sent;
                }
                else if (gnutls_error_is_fatal(ret) == 1) {
                        break;
                }
        } while (sent < len);
        /* "and go on till you come to the end: then stop." */

        if (ret < 0) {
                syslog(LOG_ERR, "gnutls send error: %s", gnutls_strerror(ret));
        }

        return sent;
}

size_t ssl_recv(char *b, int len)
{
        int ret;
        ret = gnutls_record_recv(session, b, len);
        while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                ret = gnutls_record_recv(session, b, len);
        }
        if (ret < 0) {
                syslog(LOG_ERR, "gnutls recv error: %s", gnutls_strerror(ret));
        }
        return  (ret >= 0) ? ret : -1;
}

void ssl_log_debug(int level, const char *msg)
{
	syslog(LOG_DEBUG, "%s", msg);
}

void ssl_setup()
{
        int ret;

	if (TLS_DEBUG_LEVEL > 0) {
		gnutls_global_set_log_level(TLS_DEBUG_LEVEL);
		gnutls_global_set_log_function(ssl_log_debug);
	}
        gnutls_global_init();
        gnutls_certificate_allocate_credentials (&x509_cred);
        gnutls_certificate_set_x509_trust_file (x509_cred, config->sslca,
                GNUTLS_X509_FMT_PEM);
        gnutls_certificate_set_x509_crl_file (x509_cred, config->sslcrl,
                GNUTLS_X509_FMT_PEM);
        ret = gnutls_certificate_set_x509_key_file (x509_cred, config->sslcert,
                config->sslkey, GNUTLS_X509_FMT_PEM);
        if (ret < 0) {
                syslog(LOG_ERR, "No certificate or key were found");
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

        syslog(LOG_DEBUG, "SSL setup complete");
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
