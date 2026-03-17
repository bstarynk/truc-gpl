// file Guis/pl_gguis.c
// Copyright © 2004 Basile STARYNKEVITCH
/* $Id: pl_gguis.c 1.1 Sun, 09 May 2004 00:29:36 +0200 basile $ */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#include <glib-object.h>
#include <glib.h>

///perl includes
#include <EXTERN.h>
#include <perl.h>

const char pl_gguis_ident[] = 
"$Id: pl_gguis.c 1.1 Sun, 09 May 2004 00:29:36 +0200 basile $" 
" on " __DATE__ " at " __TIME__;

const char guis_doc[] = "Perl GUIS\n" "This is release "
// format for PRCS
// $Format:" \"$ReleaseVersion$\""$
 "1.6"
  " built on " __DATE__ "@" __TIME__ "\n"
  "Author: Basile STARYNKEVITCH; see http://freshmeat.net/projects/guis/\n"
  "or http://starynkevitch.net/Basile/guisintro.html\n"
  "** NO WARRANTY - free software under GNU General Public License **\n";

extern int nbreq;               /* request number - seen by perl */
extern int nbsend;              /* sent reply number - seen by perl */
extern int endtimeout;          /* timeout (in millisecond) - seen & writable by perl */

static char bigbuf[1024];
#define bigbufprintf(Fmt,Args...) snprintf(bigbuf,sizeof(bigbuf)-1,Fmt,##Args)

static PerlInterpreter *my_perl;  

char *
guis_language_version (void)
{
  static char buf[150];
  snprintf (buf, sizeof (buf) - 1, "Perl %s api %d.%d",PERL_XS_APIVERSION, 
	    PERL_API_VERSION, PERL_API_SUBVERSION);
  return buf;
}

void
initialize_interpreter (void)
{
  dbgprintf ("initialize_interpreter start (perl)");    
  my_perl = perl_alloc();   
  perl_construct(my_perl);                  
  PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
}

char *
load_initial_script (const char *script) {
  char my_perlargs[4]={"guisperl", "-Mgtk2", 0, 0};
  if (script && access (script, R_OK)) {
    bigbufprintf ("cannot read perl script %s - %s", script, strerror (errno));
    return bigbuf;
  };
  my_perlargs[2] = script;
  perl_parse(my_perl, NULL, 3, my_perlargs, (char**)NULL);
  perl_run(myperl);
}



// interpret a request - return 0 iff ok or a static char* error string
char *
interpret_request (const char *req)
{
  dbgputs("perl req", req);
  eval_pv(req, FALSE);
  if (SvTRUE(ERRSV)) {
    bigbufprintf ("perl failed to eval %.60s", req);
    return bigbuf;
  }
  return 0;
}

#error primitives to send back to GUIS Client are missing

/* eof $Id: pl_gguis.c 1.1 Sun, 09 May 2004 00:29:36 +0200 basile $ */
