/* part of GUIS */
/***   Copyright © 2004 Basile STARYNKEVITCH
   $Id: config_slang.c 1.1 Sun, 09 May 2004 21:43:24 +0200 basile $
   $ProjectHeader: Guis 1.62 Thu, 30 Dec 2004 14:14:05 +0100 basile $
   file config_slang.c */

#include <stdlib.h>
#include <stdio.h>
#include <slang.h>
int main(int argc, char**argv) {
 if (-1 == SLang_init_all()) 
    return(EXIT_FAILURE);
 if (argc>1) {
   if (-1 == SLang_load_string(argv[1]))
     return (EXIT_FAILURE);
 }
 return 0;
}

/* eof $Id: config_slang.c 1.1 Sun, 09 May 2004 21:43:24 +0200 basile $ */
