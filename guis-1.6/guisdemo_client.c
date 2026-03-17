// file guisdemo_client.c
// $Id: guisdemo_client.c 1.5 Thu, 06 May 2004 22:49:21 +0200 basile $

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


// this file is a simple application understanding the protocol
// defined in the guisdemo_script
// first, send a start(welcomemessage) request
// wait for replies:
////
/// when END - send a stopdemo() request and exit
////
/// when ADD "x" "y" - try to add x & y and send a
/// displaysum(x,y,x+y) request
/// or send 
/// displayerror(errormessage)

int
main (int argc, char **argv)
{
  char linbuf[1024];
  FILE *toguis = stdout;
  FILE *fromguis = stdin;
  int nbr = 0;
#ifndef NDEBUG
  fprintf (stderr, "guisdemo_client argc=%d\n", argc);
#endif
  memset (linbuf, 0, sizeof (linbuf));
  if (argc > 1) {
    fromguis = fopen (argv[1], "r");
    if (!fromguis) {
      perror (argv[1]);
      exit (1);
    }
#ifndef NDEBUG
    fprintf (stderr, "guisdemo_client read from %s fd %d\n",
             argv[1], fileno (fromguis));
#endif
  };
  if (argc > 2) {
    toguis = fopen (argv[2], "w");
    if (!toguis) {
      perror (argv[1]);
      exit (1);
    }
#ifndef NDEBUG
    fprintf (stderr, "guisdemo_client write to %s fd %d\n",
             argv[2], fileno (toguis));
#endif
  }
#ifndef NDEBUG
  fprintf (stderr, "guisdemo_client start pid %d\n", (int) getpid ());
#endif
  fprintf (toguis, "#initial start\n"
           "start(\"welcome from pid %d\")\n\n",
           (int) getpid ());
  fflush (toguis);
  while (!feof (fromguis)) {
#ifndef NDEBUG
    fprintf (stderr, "guisdemo_client %d", nbr);
    fflush (stderr);
#endif
    fgets (linbuf, sizeof (linbuf) - 1, fromguis);
    nbr++;
#ifndef NDEBUG
    fprintf (stderr, ": %s\n", linbuf);
    fflush (stderr);
#endif
    if (!strncmp (linbuf, "ADD", 3)) {
      int x=0,  y=0,  pos=0;
      if (sscanf (linbuf, "ADD \"%d\" \"%d\" %n", &x, &y, &pos) > 0 && pos > 0) {
#ifndef NDEBUG
        fprintf (stderr, "guisdemo_client good sum %d+%d=%d\n", x, y, x + y);
#endif
        fprintf (toguis, "#good sum\n"
                 "displaysum(%d,%d,%d)\n\n",
                 x, y, x + y);
      } else {
#ifndef NDEBUG
        fprintf (stderr, "guisdemo_client bad add\n");
#endif
        fprintf (toguis, "#bad input\n"
                 "displayerror(\"invalid input!\")\n\n");
      }
    } else if (!strncmp (linbuf, "END", 3)) {
      fprintf (toguis, "#stop\n" "stopdemo()\n\n");
      fflush (toguis);
      sleep (1);
      exit (0);
    } else {
      fprintf (stderr, "invalid input: %s\n", linbuf);
      exit (1);
    }
    fflush (toguis);
  }; // end of while feof
  // we get here only on EOF - this should not happen that guis close
  // the reply (event) channel
  fprintf(stderr, "guisdemo_client got eof\n");
  return 1;
}

// eof $Id: guisdemo_client.c 1.5 Thu, 06 May 2004 22:49:21 +0200 basile $
