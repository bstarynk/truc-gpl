// file Guis/sl_gguis.c
// Copyright © 2004 Basile STARYNKEVITCH
/* $Id: sl_gguis.c 1.2 Thu, 30 Dec 2004 14:14:05 +0100 basile $ */

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

/* see http://www.s-lang.org/ and http://space.mit.edu/home/mnoble/slgtk/ */

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

///slang includes
#include <slang.h>

#include "gguis.h"


const char sl_gguis_ident[] = 
"$Id: sl_gguis.c 1.2 Thu, 30 Dec 2004 14:14:05 +0100 basile $" 
" on " __DATE__ " at " __TIME__;

const char guis_doc[] = "Slang GUIS\n" "This is release "
// format for PRCS
// $Format:" \"$ReleaseVersion$\""$
 "1.6"
  " built on " __DATE__ "@" __TIME__ "\n"
  "Author: Basile STARYNKEVITCH; see http://freshmeat.net/projects/guis/\n"
  "or http://starynkevitch.net/Basile/guisintro.html\n"
  "** NO WARRANTY - free software under GNU General Public License **\n";


static int withmainloop;	/* seen & writable by slang  */

static char bigbuf[1024];
#define bigbufprintf(Fmt,Args...) snprintf(bigbuf,sizeof(bigbuf)-1,Fmt,##Args)

static const SLang_Intrin_Fun_Type guis_slang_funs[] = {
  MAKE_INTRINSIC_1("send", guis_send_reply_string, 
		   SLANG_VOID_TYPE, SLANG_STRING_TYPE),
  SLANG_END_INTRIN_FUN_TABLE
};

static const SLang_Intrin_Var_Type guis_slang_vars [] =
  {
    MAKE_VARIABLE("nbreq", &guis_nbreq, SLANG_INT_TYPE, 1),
    MAKE_VARIABLE("nbsend", &guis_nbsend, SLANG_INT_TYPE, 1),
    MAKE_VARIABLE("endtimeout", &guis_endtimeout, SLANG_INT_TYPE, 0),
    MAKE_VARIABLE("withmainloop", &withmainloop, SLANG_INT_TYPE, 0),
    SLANG_END_TABLE
  };


void
guis_initialize_interpreter (void)
{    SLang_NameSpace_Type *ns =0;

  if (-1 == SLang_init_slang () || -1 == SLang_init_all ()
      || (ns = SLns_create_namespace("guis")) == 0
      || -1 == SLns_add_intrin_fun_table(ns, guis_slang_funs, "__GUISFUN__")
      || -1 == SLns_add_intrin_var_table(ns, guis_slang_vars, "__GUISVAR__")) {
    fprintf(stderr, "GUIS failed to init slang\n");
    exit(1);
  };
  
}

int
guis_script_without_main_loop (void)
{
  dbgprintf("in script_without_main_loop withmainloop=%d", withmainloop);
  return !withmainloop;
}

char *
guis_end_of_input_hook (int timeout)
{
  dbgprintf("unimplemented end_of_input_hook timeout=%d", timeout);
  return 0;
}

// interpret a request - return 0 iff ok or a static char* error string
char *
guis_interpret_request (const char *req) 
{
  if (SLang_load_string(req)) {
    bigbufprintf("failed to interpret slang request %.60s", req);
    return bigbuf;
  }
  return 0;
}

char *
guis_load_initial_script (const char *script){ 
  if (script && access (script, R_OK)) {
    bigbufprintf ("cannot read script %s - %s", script, strerror (errno));
    return bigbuf;
  };
  if (SLang_load_file(script))  {
    bigbufprintf("failed to load slang script file %s", script);
    return bigbuf;
  };
  return 0;
}

char *
guis_language_version (void)
{
  static char buf[150];
  snprintf(buf, sizeof(buf)-1, "Slang %s", SLang_Version_String);
  return buf;
}

void
guis_debug_extra(void) {
  dbgprintf("unimplemented debug_extra");
}

/* eof sl_gguis.c */
