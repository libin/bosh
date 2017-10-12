/*
Various structs and functions to operate on these

$Id: objects.h,v 1.25 2010/02/21 23:27:36 alexsisson Exp $

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


#ifndef OBJECTS_H_INCLUDED
#define OBJECTS_H_INCLUDED

#include "list.h"
#include "stray.h"
#include "trie.h"

struct struct_bosh_window_t;
struct struct_bosh_panel_t;


/*
 *  bosh action struct
 *
 */

typedef struct {
  char   *command;
  char   *prompt;
  short  panel,window;
  short  input,output;
} bosh_action_t;


/*
 *  bosh_readbuf_t struct
 *
 */
typedef struct {
  unsigned int   len;
  unsigned int   pos;
  unsigned char *data;
} bosh_readbuf_t;


/*
 *  bosh struct, defines a process and some window stuff
 *
 */

typedef struct {

  /* process */
  int    childfd[4];
  int    childstate;
  pid_t  childpid;
  char  *command;
  char   script[18]; /* made with mkstmp so always fixed length */
  char  *tmpfpipe;
  bosh_readbuf_t readbuf[3];

  /* options */
  int    autorefresh;
  int    cursorsize;
  int    cursormovement;
  char  *multilineseperator;
  char  *preaction;
  int    refresh;
  int    searchwrap;
  int    header;
  int    footer;
  int    stderr;

  /* interface */
  LIST   list;           /* lines buffer */
  int    vx,vy;          /* viewport offset */
  int    cursor;         /* cursor from top of screen */
  int    line;           /* viewport + cursor */
  char  *pipe;
  char  *title;
  struct struct_bosh_window_t *parent;  /* parent bosh_window_t */
  time_t autorefreshtime;
  int    fgcolor;

  /* actions */
  trie_t *at;

} bosh_t;

void    bosh_t_free(bosh_t *b);



/*
 * window struct
 *
 */

typedef struct struct_bosh_window_t {
  LIST    list;      /* linked list for bosh structs */
  int     listpos;   /* index of current position in list */
  bosh_t *bosh;      /* pointer to current position in list */
  int     index;
  int     flags;

  int     x,y,ncols,nlines;   /* read from conf, with special settings (0, % etc) */
  int     rx,ry,rcols,rlines; /* real current coordinates calculated at last refresh */

  struct  struct_bosh_panel_t *parent;

} bosh_window_t;

void           window_free(bosh_window_t *w);

bosh_t *       window_newbosh(bosh_window_t *w, int pos);

int            window_prev(bosh_window_t *w);
int            window_next(bosh_window_t *w);



/*
 *  panel struct
 *
 */

typedef struct struct_bosh_panel_t {
  LIST list;              /* linked list of window structs */
  int  listpos;           /* index of current position in list */
  bosh_window_t *window;  /* pointer to current position in list */
  int index;
} bosh_panel_t;


bosh_panel_t  *panel_new();
void           panel_free(bosh_panel_t *p);

bosh_window_t *panel_newwindow(bosh_panel_t *p, int pos);

void           panel_nextwindow(bosh_panel_t *p);
void           panel_prevwindow(bosh_panel_t *p);
void           panel_setwindow (bosh_panel_t *p, int ind);

/*
 *  panel list
 *
 */

typedef struct {
  LIST list;
  int  listpos;
  bosh_panel_t *panel;
} bosh_panellist_t;

bosh_panellist_t *panellist_new();
void              panellist_free(bosh_panellist_t *pl);

void              panellist_nextpanel(bosh_panellist_t *pl);
void              panellist_prevpanel(bosh_panellist_t *pl);
void              panellist_setpanel (bosh_panellist_t *pl, int ind);

#endif
