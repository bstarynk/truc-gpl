// SPDX-License-Identifier: GPL-3.0-or-later
// file github.com/bstarynk/truc-gpl/NanoEngine/ne_main.c
// Author:
//      Basile STARYNKEVITCH, 92340 Bourg-la-Reine, France,
//                    <basile@starynkevitch.net>
//                 or <b.starynkevitch@gmail.com>

// License:
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>
//
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
bool ne_debug;
char *ne_progname;
void *ne_selfhandle;

void ne_fatal_at (const char *fil, int lin);
#define NE_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {	\
  fprintf(stderr, "%s:%d:", (Fil), (Lin));	\
  fprintf(stderr,Fmt,##__VA_ARGS__);		\
  fflush(NULL);					\
  ne_fatal_at((Fil),(Lin));			\
  abort();					\
} while (0)
#define NE_FATAL_AT(Fmt,...) NE_FATAL_AT_BIS(__FILE__,__LINE__,Fmt,\
					     ##__VA_ARGS__);
#define NE_FATAL(Fmt,...) NE_FATAL_AT(Fmt,##__VA_ARGS__);
void
ne_process_program_arguments (int argc, char **argv)
{
  /// Debugging GTK can also be provided by the G_DEBUG environment
  /// variable.  See https://docs.gtk.org/glib/running.html
  if (argc > 1 && !strcmp (argv[1], "--version"))
    {
      extern const char _ne_shortgit[];
      //extern const char _ne_fullgit[];
      extern const char _ne_timestamp[];
      //extern const long _ne_timelong;
      printf ("%s version git %s built %s;\n",
	      ne_progname, _ne_shortgit, _ne_timestamp);
      printf ("see NanoEngine under github.com/bstarynk/truc-gpl\n");
      exit (0);
    };
  for (int ix = 1; ix < argc; ix++)
    if (!strcmp (argv[ix], "-D") || !strcmp (argv[ix], "--debug"))
      {
	ne_debug = true;
	printf ("%s enables debugging\n", ne_progname);
      }
}				/* end ne_process_program_arguments */

void
ne_fatal_at (const char *fil, int lin)
{
  assert (fil != NULL);
  assert (lin > 0);
  asm volatile ("nop; nop; nop; nop");
}				/* end ne_fatal_at */

int
main (int argc, char **argv)
{
  int status = -1;
  ne_progname = argv[0];
  ne_selfhandle = dlopen (NULL, RTLD_NOW);
  if (!ne_selfhandle)
    NE_FATAL ("failed to dlopen self handle: %s", stderror (errno));
  ne_process_program_arguments (argc, argv);

  return status;
}				/* end main */

// end of file truc-gpl/NanoEngine/ne_main.c
