/*
 * args.c - handle commandline arguments
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

#include "config.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>

int g_signal = 0;

int argue(int argc, char *arg)
{

        if (argc == 2) {
                if (strcmp(arg, "reload") == 0) {
                        g_signal = SIGHUP;
                        return 0;
                }
                if (strcmp(arg, "shutdown") == 0) {
                        g_signal = SIGTERM;
                        return 0;
                }
        }
        else {
                fprintf(stderr, "%s must be a solo argument\n", arg);
        }
        return -1;
}

int process_args(int argc, char **argv)
{
        int i;

        for (i = 1; i < argc; i++) {
                if (argue(argc, argv[i]) != 0) {
                        return -1;
                }
        }

        return 0;
}
