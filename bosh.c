/*
bosh main file

$Id: bosh.c,v 1.153 2010/02/21 23:27:36 alexsisson Exp $

(C) Copyright 2004-2009 Alex Sisson (alexsisson@gmail.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>

#include "bosh.h"
#include "objects.h"
#include "list.h"
#include "rc.h"
#include "system.h"
#include "ui.h"
#include "cmd.h"
#include "misc.h"
#include "stray.h"

/* backtrace */
#ifdef DEBUG
#ifdef __GNUC__
#define BT
#include <execinfo.h>
void bosh_bt() {
  int i,n;
  void *buf[16];
  char **s;
  n = backtrace(buf,16);
  fprintf(stderr,"Backtrace:\n");
  fprintf(stderr,"  backtrace(): %d entries:\n", n);
  s = backtrace_symbols(buf,n);
  if(s)
    for(i=0;i<n;i++)
      fprintf(stderr,"    %02i: %s\n",i,s[i]);
}
#endif
#endif


/* globals */
int      ULINES;                       /* usable lines */
char    *conf              = NULL;
char    *confpath          = NULL;
stray_t *confargs          = NULL;
stray_t *mainargs          = NULL;
char    *search            = NULL;     /* previous search */
char    *shell             = "/bin/bash";

char             *BOSH              = NULL;
char             *BOSHPARAM         = NULL;
char             *BOSHERR           = NULL;
int               boshuservars;
char            **boshuservar;
char             *common            = NULL;

bosh_panellist_t *panellist = NULL;
int               stdinfd   = FD_INVALID;

regex_t          *bosh_regex_search = NULL;
regex_t          *bosh_regex_highlight[BOSH_COLORS];
regmatch_t        MATCH[100];

bosh_panel_t   *viewconf = NULL;


/*
 * allbosh()
 *
 * creates list of all boshes
 *
 */
static LIST *BL = NULL;
void allbosh() {
  bosh_panel_t  *p;
  bosh_window_t *w;
  bosh_t        *b;

  list_reinit(BL);

  START(panellist,0);
  while(1) {
    p = NEXT(panellist);
    if(!p)
      break;
    START(p,0);
    while(1) {
      w = NEXT(p);
      if(!w)
        break;
      START(w,0);
      while(1) {
        b = NEXT(w);
        if(!b)
          break;
        list_add(BL,b);
      }
    }
  }
}

#define ALLBOSH(b) allbosh(); list_start(BL,0); while(1) { b=list_next(BL); if(!(b)) break;

int bosh_service() {
  int n = 0;
  bosh_t *b;
  ALLBOSH(b)/*{*/
    /* read */
    n |= bosh_read(b); /* TODO: perhaps there should be a bosh_readall that select()s all children at once */
    /* autorefresh */
    if(b->autorefresh) {
      if(time(NULL) >= b->autorefreshtime + b->autorefresh) {
        bosh_log("bosh","bosh[%p] autorefresh due: rerunning\n",b);
        b->autorefreshtime = time(NULL);
        bosh_open(b);
      }
    }
  }
  return n;
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_init
 *
 * init anything that needs to be re-inited by "reload".
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void bosh_init() {
  bosh_t *b;

  /* TODO free panel/window lists */

  /* TODO free key binding */

  /* init panel list */
  panellist = panellist_new();

  /* init key binding */
  ui_key_init();

  bosh_parse_args(mainargs);

  bosh_log("init","=== starting boshes =======================================================\n");

  ALLBOSH(b)/*{*/
    bosh_log("init","allbosh bosh[%p]\n",b);
    if(b->command) {
      bosh_log("init","window[%d]->bosh[%p] running... \"%s\"\n",b->parent->index,b);
      bosh_open(b);
    }
    if(b->autorefresh)
      b->autorefreshtime = time(NULL);
    ui_cursorset(b,0);
  }

  /* call bosh_service so that if a quick BOSHERR is set
     we can exit without the UI being inited */
  bosh_service();

  viewconf = panel_new();
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_finish
 *
 * finish bosh cleanly - display BOSHERR if set
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void bosh_finish(int s) {
  bosh_t *b;

  bosh_log("finish","===========================================================================\n");
  bosh_log("finish","signal=%d BOSHERR=\"%s\"\n",s,BOSHERR?BOSHERR:"");

  if(s==SIGSEGV)
    BOSHERR = "received SIGSEGV";

  /* finish ncurses */
  ui_finish();

  /* clean-up any children / tmpfiles */
  ALLBOSH(b)
    bosh_close(b,0);
    bosh_unlink(b->script);
    bosh_unlink(b->tmpfpipe);
  }

  if(BOSHERR)
    fprintf(stderr,"%s: %s\n",PACKAGE,BOSHERR);

  #ifdef BT
  if(s==SIGSEGV)
    bosh_bt();
  #endif

  exit(s);
}

/* ctrlc handler */
void ctrlc(int s) {
  ungetch(3);
}

//#define debug(s) list_adddup(bosh->list,s,strlen(s)+1)



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_mode
 *
 * loops in the current mode until ui_run returns a value
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int bosh_mode(int mode) {
  int n,*r = NULL;

  bosh_log("bosh","=== entered mode %d =========================================================\n",mode);

  while(1) {
    n = bosh_service();
    //bosh_log("bosh","needredraw=%d\n",n);

/*  if the cursor is currently above the buffer length, it suggests that the input has
    been refreshed, so we wait a little while so that so data can arrive. This way, the
    refresh code won't just "correct" the problem, and lose cursor position.

    TODO: intergrate this into bosh_service loop
    if(window->bosh->line > SIZE(window->bosh))
      usleep(BOSH_USLEEP_INPUT);
*/
    r = ui_run(mode,n);
    if(r) {
      n = *r;
      bosh_free(r);
      bosh_log("bosh","=== leaving mode %d =====================================================\n",mode);
      return n;
    }
  }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * M A I N
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int main(int argc, char **argv) {

  bosh_log_open("w");
  bosh_log("bosh","pid=%d\n",getpid());

  mainargs = stray_new_d(argc,argv);

  if(!isatty(0)) {
    /* piped input */
    bosh_log("bosh","piped input detected\n");
    stdinfd = dup(0);
    dup2(1,0);
  }


  /* init various globals */
  BL = bosh_malloc(sizeof(LIST));
  list_init(BL);
  cmd_trie_init();
  bosh_init();

  /* set up signal handlers */
  signal(SIGTERM,bosh_finish);
  signal(SIGHUP, bosh_finish);
  signal(SIGSEGV,bosh_finish);
  signal(SIGINT, ctrlc);
  signal(SIGPIPE,SIG_IGN);
  signal(SIGQUIT,SIG_IGN);

  /* init ncurses */
  ui_init();
  bosh_log("bosh","=== end of initialization ==================================================\n");
  ui_redraw(BOSH_MODE_ACTION);

  /* start main loop */
  bosh_mode(BOSH_MODE_ACTION);
  return 0;
}
