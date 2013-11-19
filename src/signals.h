/*
 * signals.h - handle process signals
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

#ifndef __GLADD_SIGNALS_H__
#define __GLADD_SIGNALS_H__ 1

extern int handler_procs;

void killhandlerprocs();
int sighandlers();
void sigchld_handler (int signo);
void sigint_handler (int signo);
void sigterm_handler (int signo);
void sighup_handler (int signo);
void sigusr1_handler (int signo, siginfo_t *si, void *ucontext);
void sigusr2_handler (int signo, siginfo_t *si, void *ucontext);
int signal_gladd (int lockfd);
void signal_wait();

#endif /* __GLADD_SIGNALS_H__ */
