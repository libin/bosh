/*
Various structs and functions to operate on these

$Id: objects.c,v 1.24 2010/02/25 14:21:04 alexsisson Exp $

(C) Copyright 2004-2009 Alex Sisson (alexsisson@gmail.com)

This file is part of bosh.

bosh is free software; you can redistribute it and/or modify
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

bosh is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with bosh; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

#include "objects.h"
#include "bosh.h"
#include "system.h"
#include "misc.h"


/* frees a bosh_t struct */
void bosh_t_free(bosh_t *b) {
  int i;
  if(b->childpid)
    bosh_close(b,1);
  bosh_free(b->command);
  bosh_free(b->pipe);
  bosh_free(b->preaction);
  if(b->multilineseperator)
    if(strlen(b->multilineseperator))
      bosh_free(b->multilineseperator);
  /* free buffer */
  list_free(&b->list);
  /* ensure fd's are closed: probably done already by this stage */
  for(i=0;i<4;i++)
    close(b->childfd[i]);
  /* free read buffers */
  for(i=0;i<3;i++)
    bosh_free(b->readbuf[i].data);
  bosh_free(b);
}



void window_free(bosh_window_t *w) {
  /* TODO */
}

/* alloc and init bosh_t struct in window w */
bosh_t *window_newbosh(bosh_window_t *w, int pos) {
  bosh_t *b;
  b = bosh_malloc(sizeof(bosh_t));
  if(!b)
    return NULL; /* TODO handle properly */
  memset(b,0,sizeof(bosh_t));
  b->childstate = BOSH_CHILDSTATE_NONE;
  b->childfd[FD_STDIN] = b->childfd[FD_STDOUT] = b->childfd[FD_STDERR] = b->childfd[FD_COMMAND] = FD_INVALID;
  b->cursorsize = 1;
  b->cursormovement = 1;
  list_init(&b->list);
  b->searchwrap = 1;
  b->at = trie_new();
  b->parent = w;
  bosh_log("new","bosh: window:%d SIZE(window):%d pos:%d\n",w->index,SIZE(w),pos);
  if(pos>=0 && SIZE(w)>pos) {
    /* TODO free and delete entire list (once we have the ability for lists > 2) */
    bosh_t_free(GET(w,pos));
    DEL(w,pos);
  }
  ADD(w,b);
  return b;
}

int window_prev(bosh_window_t *w) {
  if(w->listpos) {
    w->listpos--;
    w->bosh = GET(w,w->listpos);
    if(w->bosh->refresh)
      bosh_open(w->bosh);
    return 1;
  }
  return 0;
}

int window_next(bosh_window_t *w) {
  if(w->listpos+1 < w->list.items) {
    w->listpos++;
    w->bosh = GET(w,w->listpos);
    if(w->bosh->refresh)
      bosh_open(w->bosh);
    return 1;
  }
  return 0;
}


/*
 *  panel
 *
 */

bosh_panel_t *panel_new() {
  bosh_panel_t *p;
  p = bosh_malloc(sizeof(bosh_panel_t));
  if(p) {
    memset(p,0,sizeof(bosh_panel_t));
    list_init(&p->list);
    p->window = panel_newwindow(p,-1);
    /* TODO handle error */
  }
  return p;
}

/* TODO panel_free() */

/* alloc and init bosh_window_t struct in panel p */
bosh_window_t *panel_newwindow(bosh_panel_t *p, int pos) {
  bosh_window_t *w;
  w = bosh_malloc(sizeof(bosh_window_t));
  if(w) {
    memset(w,0,sizeof(bosh_window_t));
    list_init(&w->list);
    w->bosh = window_newbosh(w,-1);
    /* TODO handle error */
    w->parent = p;
    bosh_log("new","window: panel:%c SIZE(panel):%d pos:%d\n",p->index+'A',SIZE(p),pos);
    if(pos>=0 && SIZE(p)>pos) {
      /* never used at the moment */
      window_free(GET(p,pos));
      DEL(p,pos);
    }
    w->index = SIZE(p)-1;
  }
  ADD(p,w);
  return w;
}

void panel_nextwindow(bosh_panel_t *p) {
  p->listpos++;
  if(p->listpos >= SIZE(p))
    p->listpos = 0;
  p->window = GET(p,p->listpos);
}

void panel_prevwindow(bosh_panel_t *p) {

}

void panel_setwindow(bosh_panel_t *p, int ind) {
  bosh_window_t *w;
  w = GET(p,ind);
  if(w) {
    p->window = w;
    p->listpos = ind;
  }
}

/*
 *  panel list
 *
 */

bosh_panellist_t *panellist_new() {
  bosh_panellist_t *pl;
  pl = bosh_malloc(sizeof(bosh_panellist_t));
  if(pl) {
    memset(pl,0,sizeof(bosh_panellist_t));
    list_init(&pl->list);
    pl->panel = panel_new();
    ADD(pl,pl->panel);
  }
  return pl;
}

void panellist_free(bosh_panellist_t *pl) {
  /* TODO */
}

void panellist_nextpanel(bosh_panellist_t *pl) {

}

void panellist_prevpanel(bosh_panellist_t *pl) {

}

void panellist_setpanel (bosh_panellist_t *pl, int ind) {
  bosh_panel_t *p;
  p = GET(pl,ind);
  if(p) {
    pl->panel = p;
    pl->listpos = ind;
  }
}
