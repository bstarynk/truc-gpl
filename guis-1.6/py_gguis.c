// id $Id: py_gguis.c 1.10.1.10 Thu, 30 Dec 2004 14:14:05 +0100 basile $
//  Copyright © 2003,2004 Basile STARYNKEVITCH

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

#include <Python.h>

#include <glib-object.h>
#include <glib.h>

#include "gguis.h"

/******
   This seems to work with Python 2.2.2 from http://www.python.org/ 
   which is compiled *WITHOUT THREAD SUPPORT*
   and with Pygtk 1.99.18 from http://www.daa.com.au/~james/software/pygtk/
   using Gtk-2.2.2 (and related libraries Glib, Pango, Atk...)

   see the THREAD file (plain/text) from pygtk 1.99.18 for more explantions
*****/

// put_cstring implemented in common.c
void guis_put_cstring (FILE * f, const char *msg, const char *s, int len);

// initialize the interpreter 
void guis_initialize_interpreter (void);

// load_initial_script implemented in client stuff - return 0 iff ok or an error string
char *guis_load_initial_script (const char *script);

// interpret a request - return 0 iff ok or a static char* error string
char *guis_interpret_request (const char *req);

// run the end of input hook (timeout in milliseconds) return 0 iff ok or a static char* error string
char *guis_end_of_input_hook (int timeout);

void guis_send_reply_string (const char *buf);



extern void guis_panic_at (int err, const char *fil, int lin, const char *fct,
                           const char *fmt, ...)
  __attribute__ ((noreturn, format (printf, 5, 6)));

#define guis_panic(Fmt,Args...) \
  guis_panic_at(0, __FILE__, __LINE__, __FUNCTION__, Fmt, ##Args)


char bigbuf[1024];
#define bigbufprintf(Fmt,Args...) snprintf(bigbuf,sizeof(bigbuf)-1,Fmt,##Args)


static int main_loop_in_script = 0;

static PyObject *my_end_of_input_hook (PyObject * my, PyObject * args);
static PyObject *my_guis_send (PyObject * my, PyObject * args);
static PyObject *my_to_guis (PyObject * my, PyObject * args);
static PyObject *my_pipe_check_period (PyObject * my, PyObject * args);
static PyObject *my_end_timeout (PyObject * my, PyObject * args);
static PyObject *my_nb_replies (PyObject * my, PyObject * args);
static PyObject *my_nb_requests (PyObject * my, PyObject * args);
static PyObject *my_xml_coded (PyObject * my, PyObject * args);
static PyObject *my_main_loop_in_script (PyObject * my, PyObject * args);

const char guis_doc[] =
  "GUIS is a graphic user interface \"server\" which interpret\n"
  "requests (terminated by a double-newline or a formfeed) in Python\n"
  "and send replies to an applications.  the guis Python module\n"
  "provides the primitives to exchange with the client application,\n"
  "eg send replies, etc..; an initial Python script may be used to\n"
  "initialise GUIS (eg define functions, make widgets, ...).\n"
  "GUIS requires the pygtk (>= 1.99.18) Python binding to GTK2\n"
  "This is release "
// format for PRCS
// $Format:" \"$ReleaseVersion$\""$
 "1.6"
  " built on " __DATE__ "@" __TIME__ "\n"
  "Author: Basile STARYNKEVITCH; see http://freshmeat.net/projects/guis/\n"
  "or http://starynkevitch.net/Basile/guisintro.html\n"
  "** NO WARRANTY - free software under GNU General Public License **\n";

const static PyMethodDef GuisMethods[] = {
  {"end_of_input_hook", my_end_of_input_hook, METH_VARARGS,
   "Set (if given) the end of input hook, returning the old one"},
  {"end_timeout", my_end_timeout, METH_VARARGS,
   "Set (if given) the end timeout (in milliseconds), returning the old one"},
  {"pipe_check_period", my_pipe_check_period, METH_VARARGS,
   "Set (if given) the pipe_check_period (in milliseconds), returning the old one"},
  {"nb_replies", my_nb_replies, METH_VARARGS,
   "get the number of sent replies"},
  {"nb_requests", my_nb_requests, METH_VARARGS,
   "get (or set) the number of processed requests"},
  {"main_loop_in_script", my_main_loop_in_script, METH_VARARGS,
   "get (or set) the flag telling if the gtk main loop is called inside the script"},
  {"guis_send", my_guis_send, METH_VARARGS,
   "Send a raw string to client application"},
  {"to_guis", my_to_guis, METH_VARARGS,
   "Convert argument[s] to sendable string;\n"
   " for strings, escapes characters like in C;\n"
   " for integers, convert (in base 10);\n"
   " for floats, convert with a # prefix;\n"
   " for tuples, convert component with space separation;\n"
   " for objects:\n"
   "  if it has a to_guis attribute,\n"
   "   get it and return it (if scalar) or apply it (if callable)\n"
   "  otherwise call its to_guis method\n"},
  {"xml_coded", my_xml_coded, METH_VARARGS,
   "convert a string or unicode to XML notation (using &lt; for < etc...)"},
  {NULL, NULL, 0, NULL}         /* Sentinel */
};


char *
guis_language_version (void)
{
  static char buf[100];
  snprintf (buf, sizeof (buf) - 1, "Python %s", Py_GetVersion ());
  return buf;
}

void
guis_initialize_interpreter (void)
{
  PyObject *mod = 0;
  dbgprintf ("initialise python version %s", Py_GetVersion ());
  Py_Initialize ();
  dbgprintf ("python prefix is %s", Py_GetPrefix ());
  mod = Py_InitModule3 ("guis", (PyMethodDef *) GuisMethods, (char *) guis_doc);
  PyModule_AddStringConstant (mod, "version",
                              // a format for PRCS
                              // $Format: "\t\t\t\t\"$ReleaseVersion$\""$
				"1.6"
                              ///
    );
  PyModule_AddStringConstant (mod, "build_info",
                              __DATE__ "@" __TIME__
                              "; $ProjectHeader: Guis 1.62 Thu, 30 Dec 2004 14:14:05 +0100 basile $"
                              "; $Date: Thu, 30 Dec 2004 14:14:05 +0100 $ "
                              "; $ReleaseVersion: 1.6 $");
}                               // end of initialize_interpreter

char *
guis_load_initial_script (const char *script)
{
  PyObject *pName = 0, *pModuleGtk = 0, *pModuleGuis = 0;
  dbgputs ("load initial script ", script);
  dbgputs ("python path", Py_GetPath ());
  /// import pygtk
  pName = PyString_FromString ("pygtk");
  assert (pName);
  pModuleGtk = PyImport_Import (pName);
  dbgprintf ("pModuleGtk=%p", pModuleGtk);
  if (!pModuleGtk) {
    bigbufprintf ("cannot import builtin pygtk module");
    return bigbuf;
  };
  Py_DECREF (pName);
  pName = 0;
  /// import guis
  pName = PyString_FromString ("guis");
  assert (pName);
  pModuleGuis = PyImport_Import (pName);
  dbgprintf ("pModuleGuis=%p", pModuleGuis);
  Py_DECREF (pName);
  pName = 0;
  /// load the script
  if (script) {
    FILE *fp = fopen (script, "r");
    dbgprintf ("script %s fp%p", script, fp);
    if (!fp) {
      bigbufprintf ("cannot open script %s - %s", script, strerror (errno));
      return bigbuf;
    };
    if (PyRun_SimpleFile (fp, (char *) script)) {
      fclose (fp);
      bigbufprintf ("failed to load script %s", script);
      return bigbuf;
    };
    fclose (fp);
  };
  return 0;
}                               // end of load_initial_script


static PyObject *mod;

// interpret a request - return 0 iff ok or a static char* error string
char *
guis_interpret_request (const char *req)
{
  PyObject *dict = 0, *res = 0, *errtyp = 0, *errval = 0, *errtrace = 0;
  dbgputs ("interpret request ", req);
  if (!mod)
    mod = PyImport_AddModule ("__main__");
  if (!mod)
    return "no __main__ in python";
  dict = PyModule_GetDict (mod);
  res = PyRun_String ((char *) req, Py_file_input, /*global */ dict,    /*local */
                      dict);
  if (!res) {
    char linebuf[100];
    int linlen = 0;
    char *eol = strchr (req, '\n');
    PyObject *strob = 0, *strval = 0;
    errtyp = errval = errtrace = 0;
    PyErr_Fetch (&errtyp, &errval, &errtrace);
    dbgprintf ("errtyp=%p errval=%p errtrace=%p", errtyp, errval, errtrace);
    memset (linebuf, 0, sizeof (linebuf));
    if (eol && eol > req + 1)
      linlen = eol - req;
    else
      linlen = strlen (req);
    if (linlen >= sizeof (linebuf) - 3)
      linlen = sizeof (linebuf) - 3;
    strncpy (linebuf, req, linlen);
    if (errtyp)
      strob = PyObject_Str (errtyp);
    if (errval)
      strval = PyObject_Str (errval);
    dbgprintf ("strob %p", strob);
    bigbufprintf ("in %.120s:: [%s] %s.",
                  linebuf, strob ? PyString_AsString (strob) : "...",
                  strval ? PyString_AsString (strval) : "...");
    PyErr_Restore (errtyp, errval, errtrace);
    PyErr_PrintEx (0);
    Py_XDECREF (strob);
    Py_XDECREF (strval);
    return bigbuf;
  };
  Py_XDECREF (res);
  return 0;
}                               // end of interpret_request


void
guis_debug_extra (void)
{
  PyObject *dict = 0, *key = 0, *val = 0;
  int pos = 0;
  int nbv = 0;
  if (!mod)
    mod = PyImport_AddModule ("__main__");
  dbgprintf ("debug_extra mod%p", mod);
  dict = PyModule_GetDict (mod);
  fprintf (stderr, "\n*** global dictionnary (%p) **\n", dict);
  while (PyDict_Next (dict, &pos, &key, &val)) {
    fputs (" + ", stderr);
    PyObject_Print (key, stderr, 0);
    fputs (" :: ", stderr);
    PyObject_Print (val, stderr, 0);
    putc ('\n', stderr);
    nbv++;
  }
  fprintf (stderr, "*** end of %d bindings\n\n", nbv);
}

static PyObject *py_end_of_input_hook = NULL;


char *
guis_end_of_input_hook (int timeout)
{
  char *err = 0;
  PyObject *arglist = 0, *result = 0;
  if (PyCallable_Check (py_end_of_input_hook)) {
    /* Time to call the callback */
    arglist = Py_BuildValue ("(i)", timeout);
    result = PyEval_CallObject (py_end_of_input_hook, arglist);
    if (!result) {
      err = "failed to call end_of_input_hook";
    };
    Py_DECREF (result);
    Py_DECREF (arglist);
  }
  return err;
}                               // end of end_of_input_hook

static PyObject *
my_end_of_input_hook (PyObject * my, PyObject * args)
{
  PyObject *result = NULL;
  PyObject *temp = 0, *old = 0;
  old = py_end_of_input_hook;
  if (!old)
    old = Py_None;
  if (PyArg_ParseTuple (args, "|O:end_of_input_hook", &temp)) {
    if (temp) {
      if (!PyCallable_Check (temp)) {
        PyErr_SetString (PyExc_TypeError, "parameter must be callable");
        return NULL;
      }
      Py_INCREF (temp);         /* Add a reference to new callback */
      py_end_of_input_hook = temp;      /* Remember new callback */
    };
    /* Boilerplate to return "None" */
    Py_INCREF (old);
    result = old;
  }
  return result;
}

static PyObject *
my_guis_send (PyObject * my, PyObject * args)
{
  PyObject *result = NULL;
  char *str = 0;
  if (!PyArg_ParseTuple (args, "s", &str))
    return NULL;
  dbgputs ("raw send", str);
  guis_send_reply_string (str);
  /* Boilerplate to return "None" */
  Py_INCREF (Py_None);
  result = Py_None;
  return result;
}

static PyObject *
my_to_guis (PyObject * my, PyObject * args)
{
  char *str = 0, *pc = 0;
  int slen = 0, n = 0;
  double dx = 0.0;
  PyObject *result = NULL, *ob = 0, *at = 0;
  GString *gs = 0;
  char buf[96];
#ifndef NDEBUG
  if (guis_dbgflag) {
    dbgprintf ("to_guis my@%p=", my);
    fputs ("  ", stderr);
    PyObject_Print (my, stderr, 0);
    putc ('\n', stderr);
    dbgprintf ("to_guis args@%p=", args);
    fputs ("  ", stderr);
    PyObject_Print (args, stderr, 0);
    putc ('\n', stderr);
  }
#endif
  if (PyTuple_Check (args) && PyTuple_Size (args) != 1) {
    ob = args;
    goto handle_tuple;
  } else
    ob = PyTuple_GetItem (args, 0);
  if (PyString_Check (ob)) {
    PyString_AsStringAndSize (ob, &str, &slen);
    dbgputslen ("to_guis source", str, slen);
    gs = g_string_sized_new (5 * slen / 4 + 10);
    g_string_append_c (gs, '"');
    for (pc = str; pc < str + slen; pc++)
      guis_append_gstring_cguis_unichar (gs, *pc);
    g_string_append_c (gs, '"');
    result = PyString_FromStringAndSize (gs->str, gs->len);
    dbgputslen ("to_guis string", gs->str, gs->len);
    g_string_free (gs, TRUE);
    gs = 0;
  } else if (PyFloat_Check (ob)) {
    dx = PyFloat_AsDouble (ob);
    memset (buf, 0, sizeof (buf));
    snprintf (buf, sizeof (buf), "#%.9g", dx);
    dbgprintf ("to_guis double %s", buf);
    result = PyString_FromString (buf);
  } else if (PyInt_Check (ob)) {
    n = PyInt_AsLong (ob);
    memset (buf, 0, sizeof (buf));
    snprintf (buf, sizeof (buf), "%d", n);
    dbgprintf ("to_guis int %s", buf);
    result = PyString_FromString (buf);
  } else
  handle_tuple:
  if (PyTuple_Check (ob)) {
    int i = 0;
    int size = PyTuple_Size (ob);
    gs = g_string_sized_new (7 * size + 40);
    for (i = 0; i < size; i++) {
      PyObject *comp = 0, *subres = 0, *subargs = 0;
      if (gs->len > 2000000) {
        PyErr_Format (PyExc_ValueError,
                      "guis.to_guis: huge partial string (for tuple @%d)", i);
        g_string_free (gs, TRUE);
        gs = 0;
        result = 0;
        goto end;
      }
      comp = PyTuple_GetItem (ob, i);
      if (comp) {
        subargs = PyTuple_New (1);
        PyTuple_SET_ITEM (subargs, 0, comp);
        Py_XINCREF (comp);
        subres = my_to_guis (my, subargs);
        Py_XDECREF (comp);
        if (!subres)
          goto failed_tuple;
        else if (PyString_Check (subres)) {
          char *substr = PyString_AS_STRING (subres);
          int sublen = PyString_GET_SIZE (subres);
          if (i > 0)
            g_string_append_c (gs, ' ');
          g_string_append_len (gs, substr, sublen);
        } else
          goto failed_tuple;
      }
    };
    result = PyString_FromStringAndSize (gs->str, gs->len);
    dbgputslen ("to_guis tuple", gs->str, gs->len);
    g_string_free (gs, TRUE);
    gs = 0;
    goto end;
  failed_tuple:
    PyErr_Format (PyExc_ValueError, "guis.to_guis: bad tuple (@%d)", i);
    if (gs)
      g_string_free (gs, TRUE);
    gs = 0;
    result = 0;
    goto end;
  } else if (PyObject_HasAttrString (ob, "to_guis")) {
    at = PyObject_GetAttrString (ob, "to_guis");
    if (PyCallable_Check (at)) {
      result = PyObject_CallFunctionObjArgs (at, ob, NULL);
    } else {
      result = at;
    };
    dbgprintf ("to_guis attr %p", result);
  } else if ((result = PyObject_CallMethod (ob, "to_guis", NULL))
             != 0) {
    dbgprintf ("to_guis meth %p", result);
  } else {
    result = PyObject_Str (ob);
    dbgprintf ("to_guis as str %p", result);
  }
end:
  return result;
}                               // end of my_to_guis

static PyObject *
my_pipe_check_period (PyObject * my, PyObject * args)
{
  PyObject *result = 0;
  int per = guis_pipecheckperiod;
  if (PyArg_ParseTuple (args, "|i:pipe_check_period", &per)) {
    result = PyInt_FromLong (guis_pipecheckperiod);
    guis_set_pipe_check_period (per);
  };
  return result;
}

static PyObject *
my_end_timeout (PyObject * my, PyObject * args)
{
  PyObject *result = 0;
  int i = guis_endtimeout;
  if (PyArg_ParseTuple (args, "|i:end_timeout", &i)) {
    result = PyInt_FromLong (guis_endtimeout);
    guis_endtimeout = i;
  };
  return result;
}

static PyObject *
my_nb_requests (PyObject * my, PyObject * args)
{
  PyObject *result = 0;
  int i = guis_nbreq;
  if (PyArg_ParseTuple (args, "|i:nb_requests", &i)) {
    result = PyInt_FromLong (guis_nbreq);
    guis_nbreq = i;
  };
  return result;
}

static PyObject *
my_nb_replies (PyObject * my, PyObject * args)
{
  PyObject *result = 0;
  int i = guis_nbsend;
  if (PyArg_ParseTuple (args, "|i:nb_replies", &i)) {
    result = PyInt_FromLong (guis_nbsend);
    guis_nbsend = i;
  };
  return result;
}

static PyObject *
my_main_loop_in_script (PyObject * my, PyObject * args)
{
  PyObject *result = 0;
  PyObject *arg = 0;
  int i = main_loop_in_script;
  if (PyArg_ParseTuple (args, "|O:main_loop_in_script", &arg)) {
    if (arg) {
      main_loop_in_script = PyObject_IsTrue (arg) ? 1 : 0;
    };
  };
  // only Python 2.3 has explicit bool type
#if PY_VERSION_HEX >= 0x02030000
  result = PyBool_FromLong (i);
#else
  result = PyInt_FromLong (i);
#endif
  return result;
}

static PyObject *
my_xml_coded (PyObject * my, PyObject * args)
{
  PyObject *result = 0;
  PyObject *comp = 0;
  GString *gs = 0;
  int i = 0;
  if (PyTuple_Check (args) && PyTuple_Size (args) == 1
      && (comp = PyTuple_GetItem (args, 0)) != 0) {
    if (PyString_Check (comp)) {
      int slen = PyString_GET_SIZE (comp);
      char *str = PyString_AS_STRING (comp);
      gs = g_string_sized_new (5 * slen / 4 + 10);
      for (i = 0; i < slen; i++) {
        char c = str[i];
        guis_append_gstring_xml_unichar (gs, c);
      }
      result = PyString_FromStringAndSize (gs->str, gs->len);
      dbgputslen ("xml_coded(string) result", gs->str, gs->len);
      g_string_free (gs, TRUE);
      gs = 0;
    }
  } else if (PyUnicode_Check (comp)) {
    Py_UNICODE *unstr = PyUnicode_AsUnicode (comp);
    int slen = PyUnicode_GetSize (comp);
    for (i = 0; i < slen; i++) {
      Py_UNICODE uc = unstr[i];
      guis_append_gstring_xml_unichar (gs, uc);
    }
    result = PyString_FromStringAndSize (gs->str, gs->len);
    dbgputslen ("xml_coded(unicode) result", gs->str, gs->len);
    g_string_free (gs, TRUE);
    gs = 0;
  } else {
    if (gs)
      g_string_free (gs, TRUE);
    gs = 0;
    PyErr_Format (PyExc_ValueError,
                  "guis.xml_coded expects a single string or unicode argument");
  }
  return result;
}

// should the program call gtk_main or is it called by the initial script
/// return 1 if initial script does not call gtk_main
int
guis_script_without_main_loop (void)
{
  dbgprintf ("script_without_main_loop main_loop_in_script=%d",
             main_loop_in_script);
  return main_loop_in_script;
}

/* eof $Id: py_gguis.c 1.10.1.10 Thu, 30 Dec 2004 14:14:05 +0100 basile $ */
