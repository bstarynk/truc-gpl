// file Guis/ru_gguis.c
//  Copyright © 2003,2004 Basile STARYNKEVITCH
/* $Id: ru_gguis.c 1.5.1.12 Thu, 30 Dec 2004 14:14:05 +0100 basile $ */
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

#include <ruby.h>
#include <version.h>            //from ruby to have RUBY_VERSION

#include "gguis.h"


const char ru_gguis_ident[] = 
"$Id: ru_gguis.c 1.5.1.12 Thu, 30 Dec 2004 14:14:05 +0100 basile $" 
" on " __DATE__ " at " __TIME__;

#ifndef NDEBUG
int dbgflag;


#define dbgruby(Msg,Ob) do{if (dbgflag) {			\
  VALUE _ob=(Ob);						\
  fprintf(stderr, "%s:%d!! %s: %#lx/%s= ", __FILE__, __LINE__, 	\
	  (Msg), _ob, rb_obj_classname(_ob));			\
  rb_io_write(rb_stderr,					\
	      rb_obj_as_string(rb_inspect(_ob)));		\
  rb_io_write(rb_stderr, rb_default_rs);			\
}} while(0)

#else /*NDEBUG*/
#define dbgruby(Msg,Ob)  do{}while(0)
#endif

char bigbuf[1024];
#define bigbufprintf(Fmt,Args...) snprintf(bigbuf,sizeof(bigbuf)-1,Fmt,##Args)

const char guis_doc[] = "Ruby GUIS \n" "This is release "
// format for PRCS
// $Format:" \"$ReleaseVersion$\""$
 "1.6"
  " built on " __DATE__ "@" __TIME__ "\n"
  "Author: Basile STARYNKEVITCH; see http://freshmeat.net/projects/guis/\n"
  "or http://starynkevitch.net/Basile/guisintro.html\n"
  "** NO WARRANTY - free software under GNU General Public License **\n";

// for help: http://www.rubygarden.org/ruby?EmbedRuby

static VALUE ru_guis_module;
static VALUE ru_eoi_hook_block;
static VALUE ru_main_loop_in_script;

char *
guis_language_version (void)
{
  static char buf[150];
  // don't work because Ruby not initialized VALUE verval = rb_eval_string("RUBY_VERSION");
  snprintf (buf, sizeof (buf) - 1, "Ruby %s", RUBY_VERSION);
  return buf;
}

static VALUE my_guis_send (VALUE self, VALUE str);

struct global_entry;
static VALUE my_nbreq_getter (ID id, void *data, struct global_entry *entry);
static void my_nbreq_setter (VALUE val, ID id, void *data,
                             struct global_entry *entry);
static VALUE my_nbsend_getter (ID id, void *data, struct global_entry *entry);
static void my_nbsend_setter (VALUE val, ID id, void *data,
                              struct global_entry *entry);
static VALUE my_pipecheckperiod_getter (ID id, void *data,
                                        struct global_entry *entry);
static void my_pipecheckperiod_setter (VALUE val, ID id, void *data,
                                       struct global_entry *entry);

static VALUE my_string_to_xml (VALUE self);

static VALUE my_string_to_guis (VALUE self);
static VALUE my_integer_to_guis (VALUE self);
static VALUE my_float_to_guis (VALUE self);
static VALUE my_symbol_to_guis (VALUE self);
static VALUE my_array_to_guis (VALUE self);
static VALUE my_on_eoi (VALUE self);


void
guis_initialize_interpreter (void)
{
  dbgprintf ("initialize_interpreter start (ruby)");
  ruby_init ();
  ruby_init_loadpath ();        /* to find the libraries */
  ru_guis_module = rb_define_module ("Guis");
  dbgruby ("ru_guis_module", ru_guis_module);
  rb_define_module_function (ru_guis_module, "guis_send", my_guis_send, 1);
  rb_define_module_function (ru_guis_module, "on_end_of_input", my_on_eoi, 0);
  rb_global_variable (&ru_eoi_hook_block);
  rb_define_variable ("guis_main_loop_in_script", &ru_main_loop_in_script);
  ru_main_loop_in_script = Qfalse;
  rb_define_virtual_variable ("guis_nbreq", my_nbreq_getter, my_nbreq_setter);
  rb_define_virtual_variable ("guis_nbsend", my_nbsend_getter,
                              my_nbsend_setter);
  rb_define_virtual_variable ("guis_pipecheckperiod", my_pipecheckperiod_getter,
                              my_pipecheckperiod_setter);
  rb_define_method (rb_cString, "to_xml", my_string_to_xml, 0);
  rb_define_method (rb_cString, "to_guis", my_string_to_guis, 0);
  rb_define_method (rb_cFloat, "to_guis", my_float_to_guis, 0);
  rb_define_method (rb_cInteger, "to_guis", my_integer_to_guis, 0);
  rb_define_method (rb_cSymbol, "to_guis", my_symbol_to_guis, 0);
  rb_define_method (rb_cArray, "to_guis", my_array_to_guis, 0);
  dbgprintf ("initialize_interpreter end (ruby)");
  // since ruby dump cores on SIGINT, set to default
  signal (SIGINT, SIG_DFL);
}


/**** NOTE
it is possible to add (in Ruby) a to_guis method into an existing class thru
the following Ruby code
   klass.class_eval { def to_guis; return "result"; end }
*****/

static VALUE
my_guis_send (VALUE self, VALUE arg)
{
  char *str = 0;
  dbgruby ("ruby guis_send self", self);
  dbgruby ("ruby guis_send arg", arg);
  str = StringValuePtr (arg);
  dbgputs ("ruby guis_send", str);
  guis_send_reply_string (str);
  return Qnil;
}

static VALUE
my_on_eoi (VALUE self)
{
  dbgruby ("ruby on_eoi self", self);
  dbgprintf ("rb_block_given_p=%d", rb_block_given_p ());
  if (rb_block_given_p ()) {
    ru_eoi_hook_block = rb_block_proc ();
    dbgruby ("ruby on_eoi block", ru_eoi_hook_block);
  } else {
    dbgprintf ("no block given to on_eoi");
    ru_eoi_hook_block = Qnil;
  }
  return Qnil;
}

static VALUE
my_nbreq_getter (ID id, void *data, struct global_entry *entry)
{
  dbgprintf ("my_nbreq_getter %d", guis_nbreq);
  return INT2NUM (guis_nbreq);
}

static void
my_nbreq_setter (VALUE val, ID id, void *data, struct global_entry *entry)
{
  dbgruby ("my_nbreq_setter val", val);
  guis_nbreq = NUM2INT (val);
  dbgprintf ("my_nbreq_setter %d", guis_nbreq);
}

static VALUE
my_nbsend_getter (ID id, void *data, struct global_entry *entry)
{
  dbgprintf ("my_nbsend_getter %d", guis_nbsend);
  return INT2NUM (guis_nbsend);
}

static void
my_nbsend_setter (VALUE val, ID id, void *data, struct global_entry *entry)
{
  dbgruby ("my_nbsend_setter val", val);
  guis_nbsend = NUM2INT (val);
  dbgprintf ("my_nbsend_setter %d", guis_nbsend);
}

static VALUE
my_pipecheckperiod_getter (ID id, void *data, struct global_entry *entry)
{
  dbgprintf ("my_pipecheckperiod_getter %d", guis_pipecheckperiod);
  return INT2NUM (guis_pipecheckperiod);
}

static void
my_pipecheckperiod_setter (VALUE val, ID id, void *data,
                           struct global_entry *entry)
{
  guis_set_pipe_check_period (NUM2INT (val));
  dbgprintf ("my_pipecheckperiod_setter %d", guis_pipecheckperiod);
}




static VALUE
my_string_to_xml (VALUE self)
{
  char *str = 0, *pc = 0;
  GString *gs = 0;
  VALUE res = 0;
  dbgruby ("string_to_xml self", self);
  str = StringValuePtr (self);
  dbgputs ("string_to_xml str", str);
  assert (str != 0);
  gs = g_string_sized_new (5 * strlen (str) / 4 + 5);
  for (pc = str; *pc; pc++)
    guis_append_gstring_xml_unichar (gs, *pc);
  res = rb_str_new (gs->str, gs->len);
  g_string_free (gs, TRUE);
  dbgruby ("string_to_xml res", res);
  return res;
}

////////////////////////////////////////////////////////////

// id of "to_guis" name
static ID to_guis_id;

static VALUE
my_value_to_guis (VALUE self)
{
  VALUE res = 0;
  if (!to_guis_id)
    to_guis_id = rb_intern ("to_guis");
  dbgruby ("value_to_guis self", self);
  res = rb_funcall2 (self, to_guis_id, 0, (VALUE *) 0);
  if (rb_type (res) != T_STRING)
    res = rb_str_to_str (res);
  dbgruby ("value_to_guis res", res);
  return res;
}

static VALUE
my_string_to_guis (VALUE self)
{
  char *str = 0, *pc = 0;
  GString *gs = 0;
  VALUE res = 0;
  dbgruby ("string_to_guis self", self);
  str = StringValuePtr (self);
  dbgputs ("string_to_guis str", str);
  assert (str != 0);
  gs = g_string_sized_new (5 * strlen (str) / 4 + 5);
  for (pc = str; *pc; pc++)
    guis_append_gstring_cguis_unichar (gs, *pc);
  res = rb_str_new (gs->str, gs->len);
  g_string_free (gs, TRUE);
  dbgruby ("string_to_guis res", res);
  return res;
}

static VALUE
my_integer_to_guis (VALUE self)
{
  char buf[12 * sizeof (long)];
  VALUE res = 0;
  dbgruby ("integer_to_guis self", self);
  memset (buf, 0, sizeof (buf));
  if (FIXNUM_P (self)) {
    snprintf (buf, sizeof (buf) - 1, "%d", FIX2INT (self));
  } else {
#if HAVE_LONG_LONG
    snprintf (buf, sizeof (buf) - 1, "%lld", NUM2LL (self));
#else
#warning are you sure you have no long long
    snprintf (buf, sizeof (buf) - 1, "%ld", NUM2LONG (self));
#endif
  }
  res = rb_str_new2 (buf);
  dbgruby ("integer_to_guis res", res);
  return res;
}


static VALUE
my_float_to_guis (VALUE self)
{
  char buf[24 * sizeof (double)];
  VALUE res = 0;
  double x = 0.0;
  dbgruby ("float_to_guis self", self);
  x = NUM2DBL (self);
  memset (buf, 0, sizeof (buf));
  snprintf (buf, sizeof (buf) - 1, "#%g", x);
  if (atof (buf + 1) == x && strchr (buf, '.'))
    goto ok;
  snprintf (buf, sizeof (buf) - 1, "#%.2g", x);
  if (atof (buf + 1) == x && strchr (buf, '.'))
    goto ok;
  snprintf (buf, sizeof (buf) - 1, "#%.4f", x);
  if (atof (buf + 1) == x && strlen (buf) < 10 && strchr (buf, '.'))
    goto ok;
  snprintf (buf, sizeof (buf) - 1, "#%.7e", x);
  if (atof (buf + 1) == x && strchr (buf, '.'))
    goto ok;
  snprintf (buf, sizeof (buf) - 1, "#%.*e", DBL_DIG + 1, x);
ok:
  res = rb_str_new2 (buf);
  dbgruby ("float_to_guis res", res);
  return res;
}

static VALUE
my_array_to_guis (VALUE self)
{
  VALUE res = 0, comp = 0, subres = 0;
  int sz = 0, i = 0;
  dbgruby ("array_to_guis self", self);
  Check_Type (self, T_ARRAY);
  sz = RARRAY (self)->len;
  if (sz <= 0) {
    res = rb_str_new2 ("");
    return res;
  };
  // handle first component
  comp = RARRAY (self)->ptr[0];
  res = my_value_to_guis (comp);
  // handle remaining components
  for (i = 1; i < sz; i++) {
    res = rb_str_cat (res, " ", 1);
    comp = RARRAY (self)->ptr[i];
    subres = my_value_to_guis (comp);
    res = rb_str_concat (res, subres);
  }
  dbgruby ("array_to_guis res", res);
  return res;
}

static VALUE
my_symbol_to_guis (VALUE self)
{
  VALUE res = 0;
  ID id = 0;
  dbgruby ("symbol_to_guis self", self);
  id = rb_to_id (self);
  res = rb_str_new2 (rb_id2name (id));
  dbgruby ("symbol_to_guis res", res);
  return res;
}

char *
guis_load_initial_script (const char *script)
{
  extern int ruby_nerrs;
  //  VALUE initval = 0;
  VALUE gtk2val = 0, gtkmod = 0, appres = 0, ar0 = 0;
  ID initid = 0;
  int state = 0;
  dbgprintf ("load_initial_script start (ruby) script=%s", script ? : "*none*");
  if (script && access (script, R_OK)) {
    bigbufprintf ("cannot read script %s - %s", script, strerror (errno));
    return bigbuf;
  };
  dbgprintf ("script=%s before ruby_script", script);
  ruby_script (((char *) script) ? : "(no initial ruguis script)");
  dbgprintf ("script=%s after ruby_script", script);
  gtk2val = rb_require ("gtk2");
  dbgruby ("init.interp. gtk2val", gtk2val);
  if (gtk2val == Qfalse) {
    fprintf (stderr, "%s failed to require gtk2 ruby module\n", guis_progarg0);
    if (ruby_errinfo && ruby_errinfo != Qnil) {
      rb_p (ruby_errinfo);
    }
    exit (EXIT_FAILURE);
  }
  gtkmod = rb_eval_string ("Gtk");
  dbgruby ("gtkmod", gtkmod);
  ar0 = rb_ary_new2 (0);
  dbgruby ("ar0", ar0);
  initid = rb_intern ("init");
  dbgprintf ("initid=%d", (int) initid);
  appres = rb_apply (gtkmod, initid, ar0);
  dbgruby ("after Gtk.init appres", appres);
  if (script) {
    // thanks to Guy Decoux for suggesting this
    rb_load_protect (rb_str_new2 (script), Qfalse, &state);
  }
  if (ruby_errinfo && ruby_errinfo != Qnil) {
    VALUE errstr = rb_obj_as_string (ruby_errinfo);
    bigbufprintf ("in script %s :: %s ", script,
                  rb_string_value_cstr (&errstr));
    rb_p (ruby_errinfo);
    return bigbuf;
  } else if (script && ruby_nerrs > 0) {
    bigbufprintf ("got %d errors in script %s", ruby_nerrs, script);
    return bigbuf;
  }
  signal (SIGSEGV, SIG_DFL);
  dbgprintf ("load_initial_script end (ruby)");
  return 0;
}

// interpret a request - return 0 iff ok or a static char* error string
char *
guis_interpret_request (const char *req)
{
#ifdef BASILE_HACK
  // I suggested to add the following function and provided for it a
  // patch on the comp.lang.ruby newsgroup. see
  // http://blade.nagaokaut.ac.jp/ruby/ruby-talk/80801-81000.shtml
  extern VALUE
    rb_eval_string_protect_location (const char *str, const char *loc,
                                     int *state);
#endif //BASILE_HACK
  char locbuf[96];
  char *pc = 0;
  const char *cur = 0;
  VALUE res = 0;
  int state = 0;
  dbgputs ("interpret request ruby ", req);
  memset (locbuf, 0, sizeof (locbuf));
  snprintf (locbuf, sizeof (locbuf) / 2, "req#%d:", guis_nbreq);
  pc = locbuf + strlen (locbuf);
  for (cur = req;
       *cur && *cur != '\n' && *cur != '\f'
       && pc < locbuf + sizeof (locbuf) - 1; *(pc++) = *(cur++));
  dbgprintf ("locbuf:%s", locbuf);
#ifdef BASILE_HACK
  res = rb_eval_string_protect_location (req, locbuf, &state);
#else
  res = rb_eval_string_protect (req, &state);
#endif
  dbgruby ("result of request", res);
  if (state) {
    VALUE errstr = rb_obj_as_string (ruby_errinfo);
    bigbufprintf ("in %s:: %s ", locbuf, rb_string_value_cstr (&errstr));
    rb_p (ruby_errinfo);
    return bigbuf;
  }
  return 0;
}                               // end of interpret_request

void
guis_debug_extra (void)
{
  extern VALUE ruby_top_self;
  dbgprintf ("debug_extra ruby ruby_to_self=%#x/%s", (int) ruby_top_self,
             rb_obj_classname (ruby_top_self));
  dbgruby ("debug_extra ruby_top_self", ruby_top_self);
}

static int eoi_ok = 0;
static VALUE
eoi_handler (VALUE a1)
{
  VALUE res = 0;
  VALUE tabarg[1] = { 0 };
  int call_id = rb_intern ("call");
  dbgprintf ("call_id %d", call_id);
  dbgruby ("eoi_handler a1", a1);
  dbgruby ("eoi_handler ru_guis_module", ru_guis_module);
  dbgruby ("ru_eoi_hook_block", ru_eoi_hook_block);
  tabarg[0] = a1;
  res = rb_funcall2 (ru_eoi_hook_block, call_id, 1, tabarg);
  dbgruby ("eoi_handler res", res);
  eoi_ok = 1;
  return res;
}

static VALUE
eoi_rescuer (VALUE a1)
{
  eoi_ok = 0;
  dbgruby ("eoi_rescuer a1", a1);
  return Qnil;
}

char *
guis_end_of_input_hook (int timeout)
{
  VALUE res = 0;
  eoi_ok = 0;
  dbgprintf ("end_of_input_hook ruby timeout=%d", timeout);
  dbgruby ("ru_eoi_hook_block", ru_eoi_hook_block);
  if (!rb_obj_is_kind_of (ru_eoi_hook_block, rb_cProc)) {
    dbgprintf ("ru_eoi_hook_block is not a Proc");
    return 0;
  };
  dbgprintf (" end_of_input_hook with rb_rescue");
  res = rb_rescue (eoi_handler, INT2NUM (timeout), eoi_rescuer, Qfalse);
  dbgruby ("end_of_input_hook after rb_rescue res", res);
  dbgruby ("end_of_input_hook ruby res", res);
  if (res == Qfalse || eoi_ok == 0) {
    VALUE errstr = 0;
    if (ruby_errinfo)
      errstr = rb_obj_as_string (ruby_errinfo);
    if (errstr) {
      bigbufprintf ("end_of_input_hook failed - [%s] ",
                    rb_string_value_cstr (&errstr));
      rb_p (ruby_errinfo);
    } else {
      bigbufprintf ("failure of end_of_input_hook");
    }
    return bigbuf;
  }
  return 0;
}


int
guis_script_without_main_loop (void)
{
  dbgruby ("in script_without_main_loop main_loop_in_script",
           ru_main_loop_in_script);
  return ru_main_loop_in_script != Qfalse && ru_main_loop_in_script != Qnil;
}

/* eof $Id: ru_gguis.c 1.5.1.12 Thu, 30 Dec 2004 14:14:05 +0100 basile $ */
