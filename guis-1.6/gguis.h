#ifndef GGUIS_INCLUDED_
#define GGUIS_INCLUDED_
//  Copyright © 2003 Basile STARYNKEVITCH

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

#include <glib.h>

// put_cstring implemented in common.c
void guis_put_cstring (FILE * f, const char *msg, const char *s, int len);

//// ++++++++++++ the interpreter specific file should define these:::

// initialize the interpreter 
void guis_initialize_interpreter (void);

// load_initial_script implemented in client stuff - return 0 iff ok or an error string
char *guis_load_initial_script (const char *script);

// interpret a request - return 0 iff ok or a static char* error string
char *guis_interpret_request (const char *req);

// run the end of input hook (timeout in milliseconds) return 0 iff ok or a static char* error string
char *guis_end_of_input_hook (int timeout);

extern const char guis_doc[];   // documentation string

// return a static string describing the language version
char *guis_language_version (void);

///// ------------- end of interpreter specific routines

#ifndef NDEBUG
int guis_dbgflag;

void guis_debug_extra (void);

#define dbgprintf(Fmt,Args...) do { if(guis_dbgflag) {			\
  fprintf(stderr, "%s:%d!! " Fmt "\n", __FILE__, __LINE__, ##Args);	\
  fflush(stderr);							\
} } while (0)

#define dbgputs(Msg,Str) do{if (guis_dbgflag) {		\
  fprintf(stderr, "%s:%d!! ", __FILE__, __LINE__);	\
  guis_put_cstring(stderr,(Msg),(Str),-1);     		\
  fflush(stderr);					\
}} while(0)

#define dbgputslen(Msg,Str,Len) do{if (guis_dbgflag) {	\
  fprintf(stderr, "%s:%d!! ", __FILE__, __LINE__);	\
  guis_put_cstring(stderr,(Msg),(Str),(Len));       	\
  fflush(stderr);					\
}} while(0)

#else
#define dbgprintf(Fmt,Args...) do {} while(0)
#define dbgputs(Msg,Str) do {} while(0)
#define dbgputslen(Msg,Str) do {} while(0)
#endif


extern void guis_panic_at (int err, const char *fil, int lin, const char *fct,
                           const char *fmt, ...)
  __attribute__ ((noreturn, format (printf, 5, 6)));

#define guis_panic(Fmt,Args...) \
  guis_panic_at(0, __FILE__, __LINE__, __FUNCTION__, Fmt, ##Args)

#define guis_epanic(Fmt,Args...) \
  guis_panic_at(errno, __FILE__, __LINE__, __FUNCTION__, Fmt, ##Args)

void guis_send_reply_string (const char *buf);
void guis_send_reply_printf (const char *fmt, ...)
  __attribute__ ((format (printf, 1, 2)));

// utility to append to a GString the XML representation of a
// unicode character
void guis_append_gstring_xml_unichar (GString * gs, int c);

// utility to append to a GString a C like representation of an
// unicode character, with the exception that the doublequote is \Q,
// characters below 255 are in hex like \xe4 and chars below 65536 are
// in hex like \u3ef5
void guis_append_gstring_cguis_unichar (GString * gs, int c);

char *guis_initialscript;            /* our initial script */

char *guis_progarg0;                 /* program arg0 */
int guis_nbreq;                      /* request number */
int guis_nbsend;                     /* sent reply number */
int guis_endtimeout;                 /* timeout (in millisecond) */
int guis_pipecheckperiod;            /* period in milliseconds for checking the pipepid */
int guis_pipecheckid;                /*  id for pipecheck perioding callback */

// change the pipecheckperiod
void guis_set_pipe_check_period (int newper);

extern const char guis_release[];

#endif /*GGUIS_INCLUDED_ */
