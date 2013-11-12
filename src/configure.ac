# configure.ac for gladd
# 
# this file is part of GLADD
# 
# Copyright (c) 2012, 2013 Brett Sheffield <brett@gladserv.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program (see the file COPYING in the distribution).
# If not, see <http://www.gnu.org/licenses/>.

# To build configure script:
#   autoconf configure.ac > configure

AC_INIT([gladd], [0.1.1], [brett@gladserv.com])
AC_CONFIG_SRCDIR([xml.c])
AC_PROG_CC
AC_LANG(C)
AC_PREFIX_DEFAULT([/usr/local/bin])
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# with arguments
AC_ARG_WITH(gnutls, [ --with-gnutls enable gnutls])
AC_ARG_WITH(openssl, [ --with-openssl enable openssl])
AC_ARG_WITH(postgres, [ --without-postgres disable postgres])
AC_ARG_WITH(mysql, [ --without-mysql disable mysql])
AC_ARG_WITH(tds, [ --without-tds disable tds support])
AC_ARG_WITH(openldap, [ --without-openldap disable openldap])
AC_ARG_WITH(xml, [ --without-xml disable xml support])

# choose SSL library
if test "$with_gnutls" == "yes"
then
        AC_MSG_NOTICE([GNUTTLS enabled.])
        AC_SUBST(cflagssl, -D_GNUTLS)
        AC_SUBST(ssl, gnutls)
        AC_SUBST(libssl,["`pkg-config gnutls --cflags --libs`"])
elif test "$with_openssl" == "yes"
then
        AC_MSG_NOTICE([OPENSSL enabled.])
        AC_SUBST(cflagssl, -D_OPENSSL)
        AC_SUBST(ssl, openssl)
        AC_SUBST(libssl, -lssl)
else
        AC_MSG_NOTICE([No SSL library selected.  Disabling SSL.])
        AC_SUBST(ssl, tls)
fi

# backend support
if test "$with_mysql" == "no"
then
	AC_MSG_NOTICE([disabling mysql support])
	AC_SUBST(mysql, -D_NMY)
else
	AC_SUBST(libmysql, ["`mysql_config --libs`"])
	AC_SUBST(cflagmysql, ["`mysql_config --cflags`"])
fi
if test "$with_openldap" == "no"
then
	AC_MSG_NOTICE([disabling openldap support])
	AC_SUBST(openldap, -D_NLDAP)
	AC_SUBST(libopenldap, -lldap)
fi
if test "$with_postgres" == "no"
then
	AC_MSG_NOTICE([disabling postgres support])
	AC_SUBST(postgres, -D_NPG)
else
	AC_SUBST(incpostgres, -I/usr/include/postgresql)
	AC_SUBST(libpg, -lpq)
fi
if test "$with_tds" == "no"
then
	AC_MSG_NOTICE([disabling tds support])
	AC_SUBST(tds, -D_NTDS)
else
	AC_SUBST(libtds, -lsybdb)
fi

# xml support
if test "$with_xml" == "no"
then
	AC_MSG_NOTICE([disabling xml support])
	AC_SUBST(xml, -D_NXML)
else
	AC_SUBST(incxml, -I/usr/include/libxml2)
	AC_SUBST(libxml, -lxml2)
fi

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lb64':
#AC_CHECK_LIB([b64], [main])
# FIXME: Replace `main' with a function in `-lcurl':
#AC_CHECK_LIB([curl], [main])
# FIXME: Replace `main' with a function in `-lgladdb':
#AC_CHECK_LIB([gladdb], [main])
# FIXME: Replace `main' with a function in `-lldap':
#AC_CHECK_LIB([ldap], [main])
# FIXME: Replace `main' with a function in `-lpam':
#AC_CHECK_LIB([pam], [main])
# FIXME: Replace `main' with a function in `-lpam_misc':
#AC_CHECK_LIB([pam_misc], [main])
# FIXME: Replace `main' with a function in `-lpq':
#AC_CHECK_LIB([pq], [main])
# FIXME: Replace `main' with a function in `-lrt':
#AC_CHECK_LIB([rt], [main])
# FIXME: Replace `main' with a function in `-lssl':
#AC_CHECK_LIB([ssl], [main])
# FIXME: Replace `main' with a function in `-lsybdb':
#AC_CHECK_LIB([sybdb], [main])
# FIXME: Replace `main' with a function in `-lxml2':
#AC_CHECK_LIB([xml2], [main])
# FIXME: Replace `main' with a function in `-lxslt':
#AC_CHECK_LIB([xslt], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h stdlib.h string.h sys/file.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memchr memmove memset mkdir socket strdup strerror strndup strrchr strstr strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
