// id $Id: gguis.c 1.10.1.16 Thu, 30 Dec 2004 14:14:05 +0100 basile $
//  Copyright © 2003,2004 Basile STARYNKEVITCH
// $ProjectHeader: Guis 1.62 Thu, 30 Dec 2004 14:14:05 +0100 basile $

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

#include <glib-object.h>
#include <glib.h>
#include <glib/gmarkup.h>

#include <gtk/gtk.h>

#include <pango/pango.h>


#include "gguis.h"


void free_gstring (GString * s);

int guis_rawinfd = -1;               /* raw input file descriptor */
int guis_rawoutfd = -1;              /* raw output file descriptor */
GIOChannel *guis_inchan = 0;         /* input channel */
char *guis_inencoding = 0;           /* input encoding */
GIOChannel *guis_outchan = 0;        /* output channel */
char *guis_outencoding = 0;          /* output encoding */

volatile pid_t guis_pipepid = 0;     /* process id of piped
                                   command, cleared by
                                   child handler */
char *guis_pipecmd = 0;              /* piped command */

char *guis_initialscript = 0;        /*  initial script */
char *guis_logfilename = 0;          /* logfilename */
FILE *guis_logfile = 0;              /* logfile for processed requests */
GString *guis_readstr = 0;           /* read buffer string  */
int guis_readoff = 0;                /* offet of last handled byte in read buffer */


int guis_dbgflag = 0;                /* debug flag */
int guis_traceflag = 0;              /* trace flag */
guint guis_inid, guis_outid, guis_idleid;      /* id for input, output and idle callbacks */

GQueue *guis_request_queue;          /* request queue */
GQueue *guis_reply_queue;            /* reply queue */

int guis_nbreq = 0;                  /* request number - seen by script */
int guis_nbsend = 0;                 /* sent reply number - seen by script */
int guis_endtimeout = 0;             /* timeout (in millisecond) - seen & writable by script */

int guis_pipecheckperiod = 1000;     /* period in milliseconds for checking the pipepid */
int guis_pipecheckid;                /*  id for pipecheck perioding callback */

char *guis_progarg0 = "?gguis?";

struct {                        /* trace widgets */
  GtkWidget *topwin;            /* toplevel window */
  GtkWidget *vbox;              /* top vertical box */
  GtkWidget *menubar;           /* the menubar */
  GtkWidget *tracemenu;         /* the trace menu */
  GtkWidget *tracemenuitem;     /* the trace item */
  GtkWidget *traceonitem;       /* the trace on item */
  GtkWidget *traceoffitem;      /* the trace off item */
  GtkWidget *traceclearitem;    /* the trace clear item */
#ifndef NDEBUG
  GtkWidget *debugmenu;         /* the debug menu */
  GtkWidget *debugmenuitem;     /* the debug item */
  GtkWidget *debugonitem;       /* debug on item */
  GtkWidget *debugoffitem;      /* debug off item */
  GtkWidget *debugxtraitem;     /* debug extra item */
#endif
  GtkWidget *label;             /* label */
  GtkTextTagTable *tagtbl;      /* tag table for trace */
  GtkTextTag *tg_tim;           /* tag for time display */
  GtkTextTag *tg_title;         /* tag for title display */
  GtkTextTag *tg_imp;           /* tag for important display */
  GtkTextTag *tg_in;            /* tag for input display */
  GtkTextTag *tg_out;           /* tag for output display */
  GtkTextBuffer *txtbuf;        /* the textbuffer for trace */
  GtkWidget *txtscroll;         /* the scrollbox for trace */
  GtkWidget *txtview;           /* the textview for trace */
} guis_trw = {
0};

const char guis_release[] =
// for PRCS format
// $Format: " \"guis-$ReleaseVersion$\"; "$
 "guis-1.6"; 

static gboolean parse_requests (void *data);
static void parse_request_string(char* str);
static void insert_trace_time (int dated);
static gboolean end_of_input (void *data);



static void
notify_end (void)
{
  static int endid;
  dbgprintf ("notify_end endid=%d", endid);
  if (!endid) {
    endid = gtk_idle_add_priority (GTK_PRIORITY_LOW, end_of_input, (void *) 0);
    dbgprintf ("added low priority end_of_input endid=%d", endid);
  };
}

static int
pipe_periodical_checker_cb (void *data)
{
  dbgprintf ("pipe checker pipepid=%d", (int) guis_pipepid);
  if (guis_pipepid <= 0) {
    dbgprintf ("pipe checker found dead pid");
    notify_end ();
    return FALSE;
  }
  return TRUE;
}


static GString *pendreqstr;     /* pending (incomplete) request string */

static gboolean
ioreader (GIOChannel * chan, GIOCondition cond, gpointer data)
{
  gchar buf[1024];
  gchar *cur = 0, *eol2 = 0, *eop = 0, *end = 0;
  gsize blen = 0;
  gsize eolpos = 0;
  GError *err = 0;
  GIOStatus stat = 0;
  g_assert (chan == guis_inchan);
  do {
    eolpos = 0;
    err = 0;
    memset (buf, 0, sizeof (buf));
    blen = 0;
    stat = g_io_channel_read_chars (chan, buf, sizeof (buf) - 1, &blen, &err);
    dbgprintf ("after read_chars stat=%d blen=%d err=%p:%s",
               stat, blen, err, err ? (err->message) : "_");
    switch (stat) {
    case G_IO_STATUS_ERROR:
      dbgprintf ("ioreader got error  %s", err ? (err->message) : "*noerr*");
      dbgputslen ("ioreader error buf", buf, blen);
      break;
    case G_IO_STATUS_NORMAL:
      dbgputslen ("ioreader normal buf", buf, blen);
      break;
    case G_IO_STATUS_EOF:
      dbgputslen ("ioreader eof buf", buf, blen);
      break;
    case G_IO_STATUS_AGAIN:
      dbgputslen ("ioreader again buf", buf, blen);
      break;
    };
    cur = buf;
    end = eol2 = eop = 0;
    do {
      if (!cur || !cur[0])
        break;
      end = 0;
      if (guis_inencoding) {
        gchar *cp = 0;
        eop = g_utf8_strchr (cur, buf + blen - cur, '\f');
        cp = cur;
        while (cp && cp < buf + blen) {
          eol2 = g_utf8_strchr (cp, buf + blen - cp, '\n');
          if (cp[1] == '\n')
            break;
          cp = eol2 + 1;
          eol2 = 0;
        };
      } else {
        eol2 = strstr (cur, "\n\n");
        eop = strchr (cur, '\f');
      };
      if (eol2) {
        if (eop && eop < eol2) {
          end = eop;
          *eop = '\n';
        } else
          end = eol2 + 1;
      } else if (eop) {
        *eop = '\n';
        end = eop;
      } else
        end = 0;
      if (end) {
        int siz = 0;
        siz = end - cur + 1;
        dbgprintf ("with end @%p siz%d", end, siz);
        dbgputslen ("new requeststring", cur, siz);
        if (!pendreqstr)
          pendreqstr = g_string_new_len (cur, siz);
        else
          pendreqstr = g_string_append_len (pendreqstr, cur, siz);
        g_queue_push_head (guis_request_queue, pendreqstr);
        if (!guis_idleid)
          guis_idleid = gtk_idle_add (parse_requests, (void *) 0);
        pendreqstr = 0;
        cur = end + 1;
      };
    } while (end);
    if (cur && cur < buf + blen) {
      if (!pendreqstr)
        pendreqstr = g_string_sized_new ((buf + blen - cur + 50) | 63);
      dbgputslen ("appending to pendreqstr linstr", cur, buf + blen - cur);
      g_string_append_len (pendreqstr, cur, buf + blen - cur);
    };
    /* if we got EOF */
    if (pendreqstr && stat == G_IO_STATUS_EOF) {
      /* last request */
      dbgputslen ("pushing pendreqstr  last request", pendreqstr->str,
                  pendreqstr->len);
      g_queue_push_head (guis_request_queue, pendreqstr);
      pendreqstr = 0;
      if (!guis_idleid)
        guis_idleid = gtk_idle_add (parse_requests, (void *) 0);
    }
  } while (stat == G_IO_STATUS_NORMAL);
  dbgprintf ("ending ioreader return %d", stat != G_IO_STATUS_EOF);
  // should return false if reader should be removed
  return (gboolean) (stat != G_IO_STATUS_EOF);
}                               //end ioreader


static gboolean
iowriter (GIOChannel * chan, GIOCondition cond, gpointer data)
{
  gsize cnt = 0;
  GError *err = 0;
  GString *str = 0;
  GIOStatus stat = 0;
  assert (chan == guis_outchan);
  do {
    if (g_queue_is_empty (guis_reply_queue)) {
      //g_source_remove(outid);
      guis_outid = 0;
      return FALSE;
    };
    str = g_queue_peek_head (guis_reply_queue);
    if (!str)
      return FALSE;
    errno = 0;
    dbgputslen ("writing str", str->str, str->len);
    cnt = 0;
    err = 0;
    if (str->len > 0) {
      stat = g_io_channel_write_chars (chan, (const gchar *) str->str,
                                       (gssize) str->len, &cnt, &err);
      dbgprintf ("after write cnt=%d err %s", cnt, err ? err->message : "*ok*");
      if (err)
        g_error_free (err);
      err = 0;
      g_io_channel_flush (chan, &err);
      dbgprintf ("after flush err %s", err ? err->message : "*ok*");
    };
    if (cnt > 0) {
      if (cnt >= (int) str->len) {
        g_queue_pop_head (guis_reply_queue);
        free_gstring (str);
        str = 0;
      } else
        (void) g_string_erase (str, 0, cnt);
    };
    if (stat == G_IO_STATUS_EOF) {
      g_source_remove (guis_outid);
      guis_outid = 0;
    };
  } while (cnt > 0);
  // should return false if writer should be removed
  return (gboolean) (stat != G_IO_STATUS_EOF);
}

static void
trace_reply (GString * str)
{
  char bufn[48];
  if (!guis_traceflag)
    return;
  GtkTextIter itend;
  insert_trace_time (0);
  snprintf (bufn, sizeof (bufn), " ?%d ", guis_nbsend);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, bufn, -1);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, str->str, str->len,
                                    guis_trw.tg_out, 0);
  if (str->len > 0 && str->str[str->len - 1] != '\n')
    gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
}

void
guis_send_reply_string (const char *buf)
{
  GString *str = 0;
  dbgputs ("send reply", buf);
  if (!guis_outid) {
    if (guis_outchan) {
      guis_outid = g_io_add_watch (guis_outchan, G_IO_OUT, iowriter, NULL);
      dbgprintf ("added iowriter outid=%d fd%d", guis_outid,
                 g_io_channel_unix_get_fd (guis_outchan));
    };
  };
  str = g_string_new (buf);
  if (str->len <= 0 || str->str[str->len - 1] != '\n')
    str = g_string_append_c (str, '\n');
  g_queue_push_tail (guis_reply_queue, str);
  guis_nbsend++;
  if (guis_traceflag)
    trace_reply (str);
}                               // end of send_reply_string

void
guis_send_reply_printf (const char *fmt, ...)
{
  static char buf[128];
  va_list args;
  int len = 0;
  char *dbuf = 0;
  GString *str = 0;
  memset (buf, 0, sizeof (buf));
  if (!guis_outid) {
    if (guis_outchan) {
      guis_outid = g_io_add_watch (guis_outchan, G_IO_OUT, iowriter, NULL);
      dbgprintf ("added iowriter outid=%d fd%d", guis_outid,
                 g_io_channel_unix_get_fd (guis_outchan));
    };
  };
  va_start (args, fmt);
  len = vsnprintf (buf, sizeof (buf) - 1, fmt, args);
  va_end (args);
  buf[sizeof (buf) - 1] = 0;
  if (len < (int) sizeof (buf) - 2) {
    str = g_string_new (buf);
  } else {
    dbuf = g_malloc0 (len + 2);
    va_start (args, fmt);
    vsnprintf (dbuf, len + 1, fmt, args);
    str = g_string_new (dbuf);
    va_end (args);
    g_free (dbuf);
  };
  if (str->len <= 0 || str->str[str->len - 1] != '\n')
    str = g_string_append_c (str, '\n');
  g_queue_push_tail (guis_reply_queue, str);
  guis_nbsend++;
  if (guis_traceflag)
    trace_reply (str);
}                               // end of guis_send_reply_printf

static gboolean
parse_requests (void *ignored)
{
  GString *str = 0;
  int nb = 0;
  dbgprintf ("begin parse_requests queue=%p %s empty", guis_request_queue,
             g_queue_is_empty (guis_request_queue) ? "is" : "not");
  while (!g_queue_is_empty (guis_request_queue)) {
    nb++;
    dbgprintf ("parse_requests loop %d", nb);
    str = g_queue_peek_tail (guis_request_queue);
    if (str) {
      dbgputs ("parse_requests req", str->str);
      parse_request_string (str->str);
      free_gstring (str);
    } else
      dbgprintf ("request str nil");
    g_queue_pop_tail (guis_request_queue);
    if (nb > 10) {
      dbgprintf ("parse_requests did %d loop and return", nb);
      return TRUE;
    }
  };
  dbgprintf ("end parse_requests %d", guis_nbreq);
  guis_idleid = 0;
  return FALSE;
}                               // end of parse_requests

static int endinputid = 0;

static gboolean
end_of_input (void *data)
{
  char *stat = 0;
  dbgprintf ("end of input endtimeout=%d endinputid=%d", guis_endtimeout,
             endinputid);
  if (guis_endtimeout >= 0) {
    dbgprintf ("running end_input_hook");
    if ((stat = guis_end_of_input_hook (guis_endtimeout)) != 0) {
      fprintf (stderr, "%s - failed to run end_input_hook - %s\n", guis_progarg0,
               stat);
      fflush (stderr);
    };
    if (!endinputid) {
      dbgprintf ("adding main_quit endtimeout %d", guis_endtimeout);
      endinputid = gtk_timeout_add (guis_endtimeout, (GtkFunction) gtk_main_quit, 0);
    };
  } else {
    gtk_main_quit ();
  };
  return FALSE;
}                               // end of end_of_input

static void
usage (char *argv0, int exitcode)
{
  fprintf (stderr,
           "usage: %s [-i inputchan] [-o outputchan]\n"
           "\t [-s initialscript]\n"
           "\t [-I inputencoding]\n"
           "\t [-O inputencoding]\n"
           "\t [-L logfile] #log of input requests\n"
           "\t [-v] #show version number\n"
           "\t [-T] #show protocol trace window (or GGUIS_TRACE env.var =1)\n"
           "\t [-p pipecommand]\n" "\t [...gtkargs]\n"
#ifndef NDEBUG
           "\t [-D] #for debug output (or GGUIS_DEBUG env.var =1)\n"
#endif
           "\t [-h] #for this help\n"
           "where a channel is either a file descriptor number or a file path\n"
           "eg -i 0 or -i - reads from stdin and -i /tmp/input_pipe reads from a namedpipe \n"
           "and the pipecommand is a command whose stdin is the inputchan\n"
           "and whose stdout is the outputchan\n"
           "if command name ends with '-scripter' then the next argument is the initial script\n"
           "[this release is %s]\n\n%s\n", argv0, guis_release, guis_doc);
  if (exitcode >= 0)
    exit (exitcode);
}



// utility to append the XML repr of a unicode char
void
guis_append_gstring_xml_unichar (GString * gs, int c)
{
  switch (c) {
  case 'a'...'z':
  case 'A'...'Z':
  case '0'...'9':
  case ' ':
  case '+':
  case '-':
  case '*':
  case '(':
  case ')':
  case '[':
  case ']':
    g_string_append_c (gs, c);
    break;
  case '&':
    g_string_append (gs, "&amp;");
    break;
  case '\'':
    g_string_append (gs, "&apos;");
    break;
  case '<':
    g_string_append (gs, "&lt;");
    break;
  case '"':
    g_string_append (gs, "&quot;");
    break;
  case '>':
    g_string_append (gs, "&gt;");
    break;
  default:
    if (c > ' ' && (int) c < 127 && isprint (c))
      g_string_append_c (gs, c);
    else
      g_string_append_printf (gs, "&#%d;", (int) c & 0xffffff);
    break;
  }
}

void
guis_append_gstring_cguis_unichar (GString * gs, int c)
{
  switch (c) {
  case 'a'...'z':
  case 'A'...'Z':
  case '0'...'9':
  case ' ':
  case '_':
  case '+':
  case '-':
  case '*':
  case '/':
  case '&':
  case '<':
  case '>':
    g_string_append_c (gs, c);
    break;
  case '\n':
    g_string_append (gs, "\\n");
    break;
  case '\t':
    g_string_append (gs, "\\t");
    break;
  case '\f':
    g_string_append (gs, "\\f");
    break;
  case '\v':
    g_string_append (gs, "\\v");
    break;
  case '\r':
    g_string_append (gs, "\\r");
    break;
  case '\'':
    g_string_append (gs, "\\'");
    break;
  case '\"':
    g_string_append (gs, "\\Q");
    break;
  default:
    if (c > 32 && c < 127 && isprint (c))
      g_string_append_c (gs, c);
    else if (c >= 0 && c < 256)
      g_string_append_printf (gs, "\\x%02x", ((int) c) & 0xff);
    else
      g_string_append_printf (gs, "\\u%04x", c);
    break;
  };
}

#if GLIB_CHECK_VERSION(2,4,0)
static void child_handler (GPid pid, gint status, gpointer data);
#else
static void sigchld_handler (int signum);
#endif

static void
parseargs (int argc, char **argv)
{
  int i = 0;
  int inpipefds[2] = {
    -1, -1
  };
  int outpipefds[2] = {
    -1, -1
  };
  char *inarg = 0;
  char *outarg = 0;
  char *pipearg = 0;
  for (i = 1; i < argc; i++) {
    char *curarg = argv[i];
    if (curarg[0] == '-') {
#define ISLONGARG(Arg) (curarg[1]=='-' && !strcmp(curarg,Arg))
      if (curarg[1] == 'i' || ISLONGARG ("--input")) {
        if (curarg[2])
          inarg = curarg + 2;
        else if (i < argc - 1)
          inarg = argv[++i];
      } else if (curarg[1] == 'I' || ISLONGARG ("--input-encoding")) {
        if (curarg[2])
          guis_inencoding = curarg + 2;
        else if (i < argc - 1)
          guis_inencoding = argv[++i];
      } else if (curarg[1] == 'o' || ISLONGARG ("--output")) {
        if (curarg[2])
          outarg = curarg + 2;
        else if (i < argc - 1)
          outarg = argv[++i];
      } else if (curarg[1] == 'O' || ISLONGARG ("--output-encoding")) {
        if (curarg[2])
          guis_outencoding = curarg + 2;
        else if (i < argc - 1)
          guis_outencoding = argv[++i];
      } else if (curarg[1] == 'p' || ISLONGARG ("--pipe")) {
        if (curarg[2])
          pipearg = curarg + 2;
        else if (i < argc - 1)
          pipearg = argv[++i];
      } else if (curarg[1] == 's' || ISLONGARG ("--script")) {
        if (curarg[2])
          guis_initialscript = curarg + 2;
        else if (i < argc - 1)
          guis_initialscript = argv[++i];
      } else if (curarg[1] == 'L' || ISLONGARG ("--logfile")) {
        if (curarg[2])
          guis_logfilename = curarg + 2;
        else if (i < argc - 1)
          guis_logfilename = argv[++i];
      } else if (curarg[1] == 'h' || ISLONGARG ("--help"))
        usage (argv[0], 0);
      else if (curarg[1] == 'T' || ISLONGARG ("--trace"))
        guis_traceflag = 1;
#ifndef NDEBUG
      else if (curarg[1] == 'D' || ISLONGARG ("--debug"))
        guis_dbgflag = 1;
#endif
      else if (curarg[1] == 'v' || ISLONGARG ("--version"))
        fprintf (stderr,
                 "%s:\n release %s\n compiled on " __DATE__ " at " __TIME__
                 "\n", argv[0], guis_release);
      else
        goto badarg;
    } else {
    badarg:
      fprintf (stderr, "%s bad argument %s\n", argv[0], curarg);
      usage (argv[0], 1);
    }
  };
  if (inarg) {
    if (isdigit (inarg[0]))
      guis_rawinfd = atoi (inarg);
    else if (!strcmp (inarg, "-"))
      guis_rawinfd = STDIN_FILENO;
    else {
      guis_rawinfd = open (inarg, O_RDONLY);
      if (guis_rawinfd < 0) {
        fprintf (stderr, "%s bad input %s : %s\n",
                 argv[0], inarg, strerror (errno));
        exit (1);
      }
    }
  }
  if (outarg) {
    if (isdigit (outarg[0]))
      guis_rawoutfd = atoi (outarg);
    else if (!strcmp (outarg, "-"))
      guis_rawoutfd = STDOUT_FILENO;
    else {
      guis_rawoutfd = open (outarg, O_WRONLY);
      if (guis_rawoutfd < 0) {
        fprintf (stderr, "%s bad output %s : %s\n",
                 argv[0], outarg, strerror (errno));
        exit (1);
      }
    }
  }
  if (pipearg) {
    guis_pipecmd = pipearg;
    if (guis_rawinfd >= 0 || guis_rawoutfd >= 0) {
      fprintf (stderr,
               "%s pipe command -p %s incompatible with -i or -o\n",
               argv[0], pipearg);
      exit (1);
    };
    if (pipe (inpipefds)) {
      perror ("pipe in");
      exit (1);
    };
    if (pipe (outpipefds)) {
      perror ("pipe out");
      exit (1);
    };
#if !GLIB_CHECK_VERSION(2,4,0)
    signal (SIGCHLD, sigchld_handler);
#endif

    guis_pipepid = fork ();
    /* maybe we should use g_spawn_async_with_pipes - this is only
       useful on Windows which I don't know at all; and I am not sure
       that Windows have shells which accept the -c convention; so I
       leave the fork exec sequence */
    if (guis_pipepid == 0) {
      /* child process */
      int i = 0;
      char *shell = 0;
      nice (2);
      usleep (50000);
      if (dup2 (outpipefds[0], 0) < 0 || dup2 (inpipefds[1], 1) < 0) {
        perror ("dup2");
        exit (1);
      };
      close (outpipefds[1]);
      close (inpipefds[0]);
      for (i = 32; i > 2; i--)
        close (i);
      usleep (50000);
      dbgprintf ("before exec %s\n", pipearg);
      if (!strchr (pipearg, ' '))
        /* optimize case when no space, so no shell needed... */
        execlp (pipearg, pipearg, 0);
      else {
        shell = getenv ("SHELL");
        if (!shell)
          shell = "/bin/sh";
        execl (shell, shell, "-c", pipearg, 0);
      }
      fprintf (stderr, "exec %s failed - %s\n", pipearg, strerror (errno));
      exit (1);
    } else if (guis_pipepid < 0) {
      perror ("fork");
      exit (1);
    } else {
      /* parent process */
#if GLIB_CHECK_VERSION(2,4,0)
      g_child_watch_add (guis_pipepid, child_handler, NULL);
#endif
      usleep (5000);            /* sleep a bit to let child run */
      guis_rawinfd = inpipefds[0];
      close (inpipefds[1]);
      guis_rawoutfd = outpipefds[1];
      close (outpipefds[0]);
      dbgprintf ("after fork pipepid=%d rawinfd=%d rawoutfd=%d", guis_pipepid,
                 guis_rawinfd, guis_rawoutfd);
    }
  }
  if (guis_rawinfd >= 0) {
    fcntl (guis_rawinfd, F_SETFL, O_NONBLOCK);
    dbgprintf ("nonblocking rawinfd%d", guis_rawinfd);
  };
  if (guis_rawoutfd >= 0) {
    fcntl (guis_rawoutfd, F_SETFL, O_NONBLOCK);
    dbgprintf ("nonblocking rawoutfd%d", guis_rawoutfd);
  };
}                               // end of parseargs


static void
handle_pipepid (void)
{
  GError *err = 0;
  if (guis_inchan) {
    dbgprintf ("closing inchan");
    g_io_channel_shutdown (guis_inchan, FALSE, &err);
    if (guis_inid) {
      dbgprintf ("removing inid");
      g_source_remove (guis_inid);
      guis_inid = 0;
    };
    g_io_channel_unref (guis_inchan);
    guis_inchan = 0;
  }
  if (guis_outchan) {
    dbgprintf ("closing outchan");
    g_io_channel_shutdown (guis_outchan, TRUE, &err);
    if (guis_outid) {
      dbgprintf ("removing outid");
      g_source_remove (guis_outid);
      guis_outid = 0;
    };
    g_io_channel_unref (guis_outchan);
    guis_outchan = 0;
  }
#if GLIB_CHECK_VERSION(2,4,0)
  g_spawn_close_pid (guis_pipepid);
#endif
  guis_pipepid = 0;
}

#if GLIB_CHECK_VERSION(2,4,0)
static void
child_handler (GPid pid, gint status, gpointer data)
{
  dbgprintf ("child_handler pid=%d status=%#x", (int) pid, (int) status);
  if (pid == guis_pipepid) {
    handle_pipepid ();
  }
}
#else
static void
sigchld_handler (int signum)
{
  int status = 0;
  if (pipepid > 0 && waitpid (pipepid, &status, WNOHANG) == pipepid) {
    handle_pipepid ();
  };
}
#endif


static void
insert_trace_time (int dated)
{
  struct {
    char buf[200];
    GTimeVal tv;
    struct tm tm;
    char sec[10];
  } t;
  time_t tim;
  GtkTextIter itend;
  memset (&t, 0, sizeof (t));
  g_get_current_time (&t.tv);
  tim = t.tv.tv_sec;
  localtime_r (&tim, &t.tm);
  strftime (t.buf, sizeof (t.buf) - 10, dated ? "%G %b %d @ %T" : "%T", &t.tm);
  sprintf (t.sec, ".%03d", (int) t.tv.tv_usec / 1000);
  strcat (t.buf, t.sec);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, t.buf, -1,
                                    guis_trw.tg_tim, 0);
}

static void
traceon_cb (void *unused)
{
  GtkTextIter itend;
  char buf[48];
  dbgprintf ("traceon");
  insert_trace_time (1);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  snprintf (buf, sizeof (buf), "TRACE ON #%d", guis_nbreq);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, buf, -1,
                                    guis_trw.tg_title, 0);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
  guis_traceflag = 1;
}

static void
traceoff_cb (void *unused)
{
  char buf[48];
  GtkTextIter itend;
  dbgprintf ("traceoff");
  insert_trace_time (1);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  snprintf (buf, sizeof (buf), "TRACE OFF #%d", guis_nbreq);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, buf, -1,
                                    guis_trw.tg_title, 0);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
  guis_traceflag = 0;
}

static void
traceclear_cb (void *unused)
{
  GtkTextIter itend;
  char buf[48];
  dbgprintf ("traceclear");
  gtk_text_buffer_set_text (guis_trw.txtbuf, "", 0);
  insert_trace_time (1);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  snprintf (buf, sizeof (buf), "TRACE CLEARED #%d", guis_nbreq);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, buf, -1,
                                    guis_trw.tg_title, 0);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
  guis_traceflag = 0;
}

#ifndef NDEBUG
static void
debugon_cb (void *unused)
{
  GtkTextIter itend;
  char buf[48];
  guis_dbgflag = 1;
  fprintf (stderr, "\ngguis: **** debug on ***\n");
  insert_trace_time (1);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  snprintf (buf, sizeof (buf), "DEBUG ON #%d", guis_nbreq);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, buf, -1,
                                    guis_trw.tg_title, 0);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
}

static void
debugoff_cb (void *unused)
{
  char buf[48];
  GtkTextIter itend;
  fprintf (stderr, "\ngguis: **** debug off ***\n");
  insert_trace_time (1);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  snprintf (buf, sizeof (buf), "DEBUG OFF #%d", guis_nbreq);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, buf, -1,
                                    guis_trw.tg_title, 0);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
  guis_dbgflag = 0;
}

static void
debugxtra_cb (void *unused)
{
  char buf[48];
  GtkTextIter itend;
  fprintf (stderr, "\ngguis: **** debug xtra ***\n");
  insert_trace_time (1);
  gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
  snprintf (buf, sizeof (buf), "DEBUG XTRA #%d", guis_nbreq);
  gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, buf, -1,
                                    guis_trw.tg_title, 0);
  gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
  guis_debug_extra ();
}
#endif


static void
initialize (void)
{
  static char buf[300];
  char hnam[100];
  dbgprintf ("start of initialize compiled %s @%s", __DATE__, __TIME__);
  if (guis_traceflag) {
    guis_trw.topwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (guis_trw.topwin), "guis trace");
    gtk_window_set_default_size (GTK_WINDOW (guis_trw.topwin), 650, 400);
    guis_trw.vbox = gtk_vbox_new (FALSE /*heterogenous */ , 5);
    gtk_container_add (GTK_CONTAINER (guis_trw.topwin), guis_trw.vbox);
    guis_trw.menubar = gtk_menu_bar_new ();
    ////
    guis_trw.tracemenu = gtk_menu_new ();
    guis_trw.tracemenuitem = gtk_menu_item_new_with_label ("Trace");
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.menubar), guis_trw.tracemenuitem);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (guis_trw.tracemenuitem),
                               guis_trw.tracemenu);
    guis_trw.traceonitem = gtk_menu_item_new_with_mnemonic ("trace _on");
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.tracemenu), guis_trw.traceonitem);
    g_signal_connect_swapped (G_OBJECT (guis_trw.traceonitem), "activate",
                              G_CALLBACK (traceon_cb), (gpointer) 0);
    guis_trw.traceoffitem = gtk_menu_item_new_with_mnemonic ("trace o_ff");
    g_signal_connect_swapped (G_OBJECT (guis_trw.traceoffitem), "activate",
                              G_CALLBACK (traceoff_cb), (gpointer) 0);
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.tracemenu), guis_trw.traceoffitem);
    guis_trw.traceclearitem = gtk_menu_item_new_with_mnemonic ("trace _clear");
    g_signal_connect_swapped (G_OBJECT (guis_trw.traceclearitem), "activate",
                              G_CALLBACK (traceclear_cb), (gpointer) 0);
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.tracemenu), guis_trw.traceclearitem);
    ////
#ifndef NDEBUG
    guis_trw.debugmenu = gtk_menu_new ();
    guis_trw.debugmenuitem = gtk_menu_item_new_with_label ("Debug");
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.menubar), guis_trw.debugmenuitem);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (guis_trw.debugmenuitem),
                               guis_trw.debugmenu);
    guis_trw.debugonitem = gtk_menu_item_new_with_mnemonic ("debug _on");
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.debugmenu), guis_trw.debugonitem);
    g_signal_connect_swapped (G_OBJECT (guis_trw.debugonitem), "activate",
                              G_CALLBACK (debugon_cb), (gpointer) 0);
    guis_trw.debugoffitem = gtk_menu_item_new_with_mnemonic ("debug o_ff");
    g_signal_connect_swapped (G_OBJECT (guis_trw.debugoffitem), "activate",
                              G_CALLBACK (debugoff_cb), (gpointer) 0);
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.debugmenu), guis_trw.debugoffitem);
    guis_trw.debugxtraitem = gtk_menu_item_new_with_mnemonic ("debug e_xtra");
    g_signal_connect_swapped (G_OBJECT (guis_trw.debugxtraitem), "activate",
                              G_CALLBACK (debugxtra_cb), (gpointer) 0);
    gtk_menu_shell_append (GTK_MENU_SHELL (guis_trw.debugmenu), guis_trw.debugxtraitem);
#endif
    ////
    gtk_box_pack_start (GTK_BOX (guis_trw.vbox), guis_trw.menubar, /*expand= */ FALSE,
                        /*fill= */ FALSE, /*space= */ 3);
    guis_trw.label = gtk_label_new (0);
    gtk_box_pack_start (GTK_BOX (guis_trw.vbox), guis_trw.label, /*expand= */ FALSE,
                        /*fill= */ FALSE, /*space= */ 3);
    memset (hnam, 0, sizeof (hnam));
    gethostname (hnam, sizeof (hnam) - 1);
    snprintf (buf, sizeof (buf) - 1,
              "<b>%s</b> <tt>%s</tt> pid%d on <tt>%s</tt>\n<small>compiled on "
              __DATE__ "@" __TIME__ "</small>",
              guis_progarg0, guis_initialscript ? : "", (int) getpid (), hnam);
    gtk_label_set_markup (GTK_LABEL (guis_trw.label), buf);
    guis_trw.txtscroll = gtk_scrolled_window_new (0, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (guis_trw.txtscroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (guis_trw.vbox), guis_trw.txtscroll, /*expand= */ TRUE,
                        /*fill= */ TRUE, /*space= */ 3);
    guis_trw.tagtbl = gtk_text_tag_table_new ();
    guis_trw.txtbuf = gtk_text_buffer_new (guis_trw.tagtbl);
    guis_trw.tg_tim = gtk_text_buffer_create_tag (guis_trw.txtbuf, "tim",
                                             "weight", PANGO_WEIGHT_BOLD,
                                             "scale", PANGO_SCALE_SMALL,
                                             "foreground", "DarkGoldenrod4", 0);
    guis_trw.tg_title = gtk_text_buffer_create_tag (guis_trw.txtbuf, "title",
                                               "scale", PANGO_SCALE_X_LARGE,
                                               "foreground", "red", 0);
    guis_trw.tg_imp = gtk_text_buffer_create_tag (guis_trw.txtbuf, "imp",
                                             "scale", PANGO_SCALE_LARGE,
                                             "weight", PANGO_WEIGHT_BOLD,
                                             "foreground", "red", 0);
    guis_trw.tg_in = gtk_text_buffer_create_tag (guis_trw.txtbuf, "in",
                                            "weight", PANGO_WEIGHT_BOLD,
                                            "foreground", "blue", 0);
    guis_trw.tg_out = gtk_text_buffer_create_tag (guis_trw.txtbuf, "out",
                                             "style", PANGO_STYLE_ITALIC,
                                             "foreground", "darkgreen", 0);
    guis_trw.txtview = gtk_text_view_new_with_buffer (guis_trw.txtbuf);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (guis_trw.txtview), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (guis_trw.txtview), GTK_WRAP_CHAR);
    gtk_container_add (GTK_CONTAINER (guis_trw.txtscroll), guis_trw.txtview);
    {
      GtkTextIter itend;
      insert_trace_time (1);
      memset (buf, 0, sizeof (buf));
      gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
      gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend,
                                        "TRACE WINDOW\n", -1, guis_trw.tg_title, 0);
      gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend,
                                        guis_release, -1, guis_trw.tg_tim, 0);
      snprintf (buf, sizeof (buf) - 1, " for %s\n", guis_language_version ());
      gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend,
                                        buf, -1, guis_trw.tg_tim, 0);
      gtk_text_buffer_insert (guis_trw.txtbuf, &itend,
                              "see http://starynkevitch.net/Basile/guisdoc.html"
                              " or freshmeat.net/projects/guis\n"
                              "input requests shown ", -1);
      gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend,
                                        "like this", -1, guis_trw.tg_in, 0);
      gtk_text_buffer_insert (guis_trw.txtbuf, &itend,
                              " & output replies shown ", -1);
      gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend,
                                        "like that\n\n", -1, guis_trw.tg_out, 0);
    }
    gtk_widget_show_all (guis_trw.topwin);
    //g_signal_connect (G_OBJECT (guis_trw.topwin), "delete_event",
    //                  G_CALLBACK (delete_event), NULL);
  };
  dbgprintf ("end of initialize");
}                               // end of initialize



int
main (int argc, char **argv)
{
  const char *status = 0;
  char *envar = 0;
  if (!GLIB_CHECK_VERSION (2, 2, 0))
    g_error ("GLib version 2.2.0 or above is needed for GUIS");
  if (!GTK_CHECK_VERSION (2, 2, 0))
    g_error ("Gtk version 2.2.0 or above is needed for GUIS");
  // horrible trick to be able to accept script starting with
  // #! /usr/bin/env *-scripter
  guis_progarg0 = argv[0];
  {
    int argv0len = 0;
    if (argv[0] && (argv0len = strlen (argv[0])) >= 10
        && !strcmp (argv[0] + argv0len - 9, "-scripter")) {
      if (argc > 1 && !access (argv[1], R_OK)) {
        guis_initialscript = argv[1];
        argv += 1;
        argc -= 1;
      }
    }
  }
#ifndef NDEBUG
  envar = getenv ("GGUIS_DEBUG");
  if (envar) {
    guis_dbgflag = atoi (envar);
    fprintf (stderr, "GGUIS_DEBUG sets guis_dbgflag to %d\n", guis_dbgflag);
  }
#endif
  envar = getenv ("GGUIS_TRACE");
  if (envar)
    guis_traceflag = atoi (envar);
  dbgprintf ("before gtk_init argc=%d", argc);
  gtk_init (&argc, &argv);
  dbgprintf ("after gtk_init argc=%d", argc);
  parseargs (argc, argv);
  if (guis_logfilename) {
    guis_logfile = fopen (guis_logfilename, "w");
    dbgprintf ("logfilename %s logfile %p #%d", guis_logfilename, guis_logfile,
               guis_logfile ? fileno (guis_logfile) : (-1));
    if (guis_logfile)
      fprintf (stderr, "%s logging input requests on %s\n", guis_progarg0,
               guis_logfilename);
    else
      fprintf (stderr, "%s cannot open logfile %s - %s\n",
               guis_progarg0, guis_logfilename, strerror (errno));
  };
  dbgprintf ("gguis pid %d compiled " __DATE__ "@" __TIME__ "\n",
             (int) getpid ());
  initialize ();
  guis_initialize_interpreter ();
  dbgputs ("loading script", guis_initialscript);
  status = guis_load_initial_script (guis_initialscript);
  dbgputs ("initial script status", status);
  if (status) {
    fprintf (stderr, "gguis failed to initialize %s - %s\n",
             guis_initialscript ? : "-", status);
    exit (EXIT_FAILURE);
  }
  guis_request_queue = g_queue_new ();
  guis_reply_queue = g_queue_new ();
  if (guis_rawinfd >= 0) {
    guis_inchan = g_io_channel_unix_new (guis_rawinfd);
    dbgprintf ("made inchan %p from rawinfd %d",  guis_inchan,  guis_rawinfd);
    if (guis_inencoding) {
      GError *err = 0;
      dbgprintf ("setting input encoding to %s", guis_inencoding);
      if (g_io_channel_set_encoding (guis_inchan, guis_inencoding, &err) !=
          G_IO_STATUS_NORMAL) {
        fprintf (stderr, "GUIS cannot set input encoding to %s - %s\n",
                 guis_inencoding, err ? "??" : (err->message));
        if (err)
          g_error_free (err);
      };
    };
    guis_inid = g_io_add_watch (guis_inchan, G_IO_IN, ioreader, NULL);
    dbgprintf ("added ioreader inid=%d fd%d", guis_inid,
               g_io_channel_unix_get_fd (guis_inchan));
  };
  if (guis_rawoutfd >= 0) {
    guis_outchan = g_io_channel_unix_new (guis_rawoutfd);
    dbgprintf ("made outchan %p from rawoutfd %d", guis_outchan, guis_rawoutfd);
    if (guis_outencoding) {
      GError *err = 0;
      dbgprintf ("setting output encoding to %s", guis_outencoding);
      if (g_io_channel_set_encoding (guis_outchan, guis_outencoding, &err) !=
          G_IO_STATUS_NORMAL) {
        fprintf (stderr, "GUIS cannot set output encoding to %s - %s\n",
                 guis_outencoding, err ? "??" : (err->message));
        if (err)
          g_error_free (err);
      };
    };
  };
#ifndef NDEBUG
  if (guis_dbgflag && guis_pipecheckperiod > 0)
    guis_pipecheckperiod += guis_pipecheckperiod * 2 + 3000;
#endif
  if (guis_pipepid > 0 && guis_pipecheckperiod > 0) {
    guis_pipecheckid = gtk_timeout_add (guis_pipecheckperiod,
                                   pipe_periodical_checker_cb, (void *) 0);
    dbgprintf ("pipecheckid %d pipepid %d period %d",
               guis_pipecheckid, (int) guis_pipepid, guis_pipecheckperiod);
  }
  if (guis_script_without_main_loop ()) {
    dbgprintf ("guis pid %d don't call gtk_main", (int) getpid ());
  } else {
    dbgprintf ("guis before gtk_main pid%d", (int) getpid ());
    gtk_main ();
  }
  if (guis_logfile) {
    fclose (guis_logfile);
    guis_logfile = 0;
  };
  dbgprintf ("guis ended pid%d", (int) getpid ());
  return 0;
}                               // end of main

void
guis_set_pipe_check_period (int newper)
{
  int oldper = guis_pipecheckperiod;
  if (newper > 0 && newper < 100)
    newper = 100;
  else if (newper > 10000)
    newper = 10000;
  if (oldper != newper) {
    if (guis_pipecheckid > 0)
      gtk_timeout_remove (guis_pipecheckid);
    if (guis_pipepid > 0 && newper > 0)
      guis_pipecheckid = gtk_timeout_add (guis_pipecheckperiod,
                                     pipe_periodical_checker_cb, (void *) 0);
    guis_pipecheckperiod = newper;
  }
}

//////////////////////////////////////////////////////////////////
// usual panic function
void
guis_panic_at (int err, const char *fil, int lin, const char *fct,
               const char *fmt, ...)
{
  va_list ap;
  fflush (stderr);
  fflush (stdout);
  fprintf (stderr, "\n**GUIS PANIC [%s:%d", fil, lin);
  if (fct && fct[0])
    fprintf (stderr, "@%s]", fct);
  else
    putc (']', stderr);
  putc (' ', stderr);
  fflush (stderr);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  if (err > 0)
    fprintf (stderr, ":%d - %s", err, strerror (err));
  putc ('\n', stderr);
  fflush (stderr);
  abort ();
   /*NOTREACHED*/ exit (2);
}                               // end of guis_panic_at

void
free_gstring (GString * str)
{
  if (!str)
    return;
  memset (str->str, 0, str->len);
  str->len = 0;
  g_string_free (str, TRUE);
}                               // end of free_gstring 




static void
parse_request_string (char *str)
{
  char bufn[48];
  char bufe[256];
  char *status = 0;
  GtkTextIter itend;
  guis_nbreq++;
  dbgprintf ("request#%d", guis_nbreq);
  dbgputs ("process request", str);
  if (guis_traceflag) {
    int slen = strlen (str);
    insert_trace_time (0);
    gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
    memset (bufn, 0, sizeof (bufn));
    snprintf (bufn, sizeof (bufn) - 1, " #%d ", guis_nbreq);
    gtk_text_buffer_insert (guis_trw.txtbuf, &itend, bufn, -1);
    gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, str, -1,
                                      guis_trw.tg_in, 0);
    if (slen > 0 && str[slen - 1] != '\n')
      gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", 1);
  };
  if (guis_logfile) {
    dbgprintf ("logging request");
    fputs (str, guis_logfile);
    putc ('\n', guis_logfile);
    putc ('\n', guis_logfile);
    fflush (guis_logfile);
  }
  status = guis_interpret_request (str);
  if (status) {
    fprintf (stderr,
             "gguis - failed to execute request %.120s : %s\n", str, status);
    if (guis_traceflag) {
      memset (bufe, 0, sizeof (bufe));
      gtk_text_buffer_get_end_iter (guis_trw.txtbuf, &itend);
      snprintf (bufe, sizeof (bufe) - 1, "req.#%d failed: ", guis_nbreq);
      gtk_text_buffer_insert_with_tags (guis_trw.txtbuf, &itend, bufe, -1,
                                        guis_trw.tg_imp, 0);
      gtk_text_buffer_insert (guis_trw.txtbuf, &itend, status, -1);
      gtk_text_buffer_insert (guis_trw.txtbuf, &itend, "\n", -1);
    }
  }
}                               /* end of parse_request_string */

/* eof $Id: gguis.c 1.10.1.16 Thu, 30 Dec 2004 14:14:05 +0100 basile $ */
