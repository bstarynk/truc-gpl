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
//   This was from libgtk-4 example-1.c
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
bool ne_debug;
char *ne_progname;
void *ne_selfhandle;

static void
print_hello (GtkWidget *widget, gpointer data)
{
  g_print ("Hello World widget@%p, data@%p\n", widget, data);
} /* end print_hello */



static void
activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *window = NULL;
  GtkWidget *button = NULL;
  GtkWidget *box = NULL;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);

  gtk_window_set_child (GTK_WINDOW (window), box);

  button = gtk_button_new_with_label ("Hello World");

  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);
  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (gtk_window_destroy), window);

  gtk_box_append (GTK_BOX (box), button);

  gtk_window_present (GTK_WINDOW (window));
} /* end activate */



static void
process_program_arguments(int argc, char**argv)
{
  /// Debugging GTK can also be provided by the G_DEBUG environment
  /// variable.  See https://docs.gtk.org/glib/running.html
  if (argc>1 && !strcmp(argv[1], "--version"))
    {
      extern const char _ne_shortgit[];
      //extern const char _ne_fullgit[];
      extern const char _ne_timestamp[];
      //extern const long _ne_timelong;
      printf("%s version git %s built %s;\n",
	     ne_progname, _ne_shortgit, _ne_timestamp);
      printf("see NanoEngine under github.com/bstarynk/truc-gpl\n");
      exit(0);
    };
  for (int ix = 1; ix < argc; ix++)
    if (!strcmp (argv[ix], "-D") || !strcmp (argv[ix], "--debug"))
      ne_debug = true;
} /* ed process_program_arguments */

int
main (int argc, char **argv)
{
  GtkApplication *app = NULL;
  int status = -1;
  ne_progname = argv[0];
  ne_selfhandle = dlopen (NULL, RTLD_NOW);
  if (!ne_selfhandle)
    g_error ("dlopen self failed %s", dlerror ());
  process_program_arguments(argc, argv);
  {
    const char *gd = getenv ("G_DEBUG");
    if (gd && gd[0])
      ne_debug = true;
  };
  app = gtk_application_new ("net.starynkevitch.nanoengine",
			     G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
} /* end main */

// end of file truc-gpl/NanoEngine/ne_main.c
