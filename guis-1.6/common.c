// file Knobas/common.c
// emacs Time-stamp: <2004 Dec 30 11h37 CET {common.c} Basile STARYNKEVITCH>
// cvsid $Id: common.c 1.2 Thu, 30 Dec 2004 14:14:05 +0100 basile $

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

void guis_put_cstring (FILE * f, const char *msg, const char *s, int len);



void
guis_put_cstring (FILE * f, const char *msg, const char *s, int len)
{
  int i = 0;
  char c = 0;
  if (msg)
    fputs (msg, f);
  if (len < 0 && s)
    len = strlen (s);
  if (!s) {
    fputs (" nil\n", f);
    return;
  };
  fprintf (f, " [%d]%c \"", len, (len > 30) ? '\n' : ' ');
  for (i = 0; i < len; i++) {
    c = s[i];
    switch (c) {
    case '\n':
      fputs ("\\n", f);
      break;
    case '\r':
      fputs ("\\r", f);
      break;
    case '\t':
      fputs ("\\t", f);
      break;
    case '\v':
      fputs ("\\v", f);
      break;
    case '\f':
      fputs ("\\f", f);
      break;
    case '\'':
      fputs ("\\'", f);
      break;
    case '\\':
      fputs ("\\\\", f);
      break;
    case '\"':
      fputs ("\\\"", f);
      break;
    default:
      if (isprint (c))
	putc (c, f);
      else {
	fprintf (f, "\\x%02x", c);
      };
      break;
    } /*end switch */ ;
    if ((i + 1) % 64 == 0 && i < len - 2)
      fprintf (f, "\"\n @%03d \"", i);
  };
  fputs ("\"\n", f);
  if (len > 250)
    putc ('\n', f);
}				/* end of put_cstring */

#ifdef TEST_COMMON_MAIN
int
main (int argc, char **argv)
{
  int i = 0;
  guis_put_cstring (stdout, "prelude test", "abc\nde\tfg\f\rhij\t'AB\"CD\\", -1);
  for (i = 0; i < argc; i++) {
    printf (" #%d=", i);
    guis_put_cstring (stdout, "arg", argv[i], -1);
  };
  return 0;
}				/*end of main */
#endif
/* eof $Id: common.c 1.2 Thu, 30 Dec 2004 14:14:05 +0100 basile $ */
