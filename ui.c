/*
some bosh interface stuff

$Id: ui.c,v 1.32 2010/02/19 23:46:58 alexsisson Exp $

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
#include "system.h"
#include "misc.h"
#include "ui.h"
#include "trie.h"
#include "cmd.h"

/* key related globals */
static trie_t *keytrie = NULL; /* indexed by a bosh key description string; returns a bosh internal command. */
static char **keyarray = NULL; /* indexed by an ncurses key code; returns a bosh key description string */

/* mouse handling - ncurses only */
#ifdef NCURSES_MOUSE_VERSION
static MEVENT mouse;
#endif

/* readstr mode related globals */
char *REPLY  = NULL;
char *PROMPT = NULL;

/* ui messages */
static char *UI_MESSAGE = NULL;
static int   UI_MESSAGE_COUNT = 0;
#define UI_MESSAGE_SHOWN 10

/* key defining macros */
#define DEFINE_KEY(key,desc)               keyarray[(key)] = (desc)
#define DEFINE_KEY_AND_BIND(key,desc,def)  keyarray[(key)] = (desc); trie_add(keytrie,keyarray[(key)], (def))
#define DEFINE_CHARKEY(key)                keyarray[(int)(*(key))] = (key)
#define DEFINE_CHARKEY_AND_BIND(key,def)   keyarray[(int)(*(key))] = (key); trie_add(keytrie,keyarray[(int)(*(key))], (def))

/* ui init */
void ui_init() {
  bosh_log("ui","init\n");
  /* init curses */
  initscr();
  keypad(stdscr,TRUE);
  cbreak();
  noecho();
  halfdelay(2);
  curs_set(0);

  /* init curses colors */
  if(has_colors()) {
    bosh_log("ui","- has_colors() = true\n");
    start_color();
    use_default_colors();
    init_pair(COLOR_RED,COLOR_RED,-1);
    init_pair(COLOR_GREEN,COLOR_GREEN,-1);
    init_pair(COLOR_YELLOW,COLOR_YELLOW,-1);
    init_pair(COLOR_BLUE,COLOR_BLUE,-1);
    init_pair(COLOR_MAGENTA,COLOR_MAGENTA,-1);
    init_pair(COLOR_CYAN,COLOR_CYAN,-1);
  }
  memset(bosh_regex_highlight,0,sizeof(regex_t*)*BOSH_COLORS);

#ifdef NCURSES_MOUSE_VERSION
  /* init ncurses mouse interface */
  mousemask(BUTTON1_CLICKED,NULL);
#endif

}

/* finish */
void ui_finish() {
  mvaddstr(LINES-1,0,strmak(COLS,' '));
  refresh();
  endwin();
}


/* init key array and trie */
void ui_key_init() {
  bosh_log("key","init\n");

  keyarray = calloc(KEY_MAX+1,sizeof(char*));
  keytrie = trie_new();
  /* TODO handle failure */

  /* set default bindings */
  DEFINE_CHARKEY("a");
  DEFINE_CHARKEY("b");
  DEFINE_CHARKEY("c");
  DEFINE_CHARKEY("d");
  DEFINE_CHARKEY("e");
  DEFINE_CHARKEY("f");
  DEFINE_CHARKEY("g");
  DEFINE_CHARKEY("h");
  DEFINE_CHARKEY("i");
  DEFINE_CHARKEY("j");
  DEFINE_CHARKEY("k");
  DEFINE_CHARKEY("l");
  DEFINE_CHARKEY("m");
  DEFINE_CHARKEY("n");
  DEFINE_CHARKEY("o");
  DEFINE_CHARKEY("p");
  DEFINE_CHARKEY("q");
  DEFINE_CHARKEY("r");
  DEFINE_CHARKEY("s");
  DEFINE_CHARKEY("t");
  DEFINE_CHARKEY("u");
  DEFINE_CHARKEY("v");
  DEFINE_CHARKEY("w");
  DEFINE_CHARKEY("x");
  DEFINE_CHARKEY("y");
  DEFINE_CHARKEY("z");

  DEFINE_CHARKEY("0");
  DEFINE_CHARKEY("1");
  DEFINE_CHARKEY("2");
  DEFINE_CHARKEY("3");
  DEFINE_CHARKEY("4");
  DEFINE_CHARKEY("5");
  DEFINE_CHARKEY("6");
  DEFINE_CHARKEY("7");
  DEFINE_CHARKEY("8");
  DEFINE_CHARKEY("9");


  DEFINE_KEY_AND_BIND( '`'        , "backtick" , "console"    );
  DEFINE_KEY_AND_BIND( '|'        , "pipe"     , "pipe"       );
  DEFINE_KEY_AND_BIND( '/'        , "slash"    , "search"     );


  DEFINE_KEY(          KEY_CTRLA  , "ca"                      );
  DEFINE_KEY(          KEY_CTRLB  , "cb"                      );
  DEFINE_KEY(          KEY_CTRLC  , "cc"                      );
  DEFINE_KEY(          KEY_CTRLD  , "cd"                      );
  DEFINE_KEY_AND_BIND( KEY_CTRLE  , "ce"       , "edit"       );
  DEFINE_KEY(          KEY_CTRLF  , "cf"                      );
  DEFINE_KEY(          KEY_CTRLG  , "cg"                      );
  DEFINE_KEY(          KEY_CTRLH  , "ch"                      );
  DEFINE_KEY(          KEY_CTRLI  , "ci"                      );
  DEFINE_KEY(          KEY_CTRLJ  , "cj"                      );
  DEFINE_KEY_AND_BIND( KEY_CTRLK  , "ck"       , "kill"       );
  DEFINE_KEY_AND_BIND( KEY_CTRLL  , "cl"       , "redraw"     );
  DEFINE_KEY_AND_BIND( KEY_CTRLN  , "cn"       , "repeat"     );

  DEFINE_KEY_AND_BIND( KEY_CTRLR  , "cr"       , "refresh"    );
  DEFINE_KEY_AND_BIND( KEY_CTRLT  , "ct"       , "thru"       );
  DEFINE_KEY_AND_BIND( KEY_CTRLW  , "cv"       , "viewconf"   );
  DEFINE_KEY_AND_BIND( KEY_CTRLW  , "cw"       , "search"     );
  DEFINE_KEY_AND_BIND( KEY_CTRLX  , "cx"       , "exit"       );

  DEFINE_KEY(          KEY_SPACE  , "space"                   );

  DEFINE_KEY_AND_BIND( KEY_UP     , "up"       , "cursorup"   );
  DEFINE_KEY_AND_BIND( KEY_DOWN   , "down"     , "cursordown" );
  DEFINE_KEY_AND_BIND( KEY_PPAGE  , "pgup"     , "cursorpgup" );
  DEFINE_KEY_AND_BIND( KEY_NPAGE  , "pgdn"     , "cursorpgdn" );
  DEFINE_KEY_AND_BIND( KEY_LEFT   , "left"     , "back"       );
  DEFINE_KEY_AND_BIND( KEY_RIGHT  , "right"    , "forward"    );
  DEFINE_KEY_AND_BIND( KEY_TAB    , "tab"      , "focus ++"   );

  DEFINE_KEY_AND_BIND( KEY_F(1)   , "f1"       , "focus A"    );
  DEFINE_KEY_AND_BIND( KEY_F(2)   , "f2"       , "focus B"    );
  DEFINE_KEY_AND_BIND( KEY_F(3)   , "f3"       , "focus C"    );
  DEFINE_KEY_AND_BIND( KEY_F(4)   , "f4"       , "focus D"    );
  DEFINE_KEY_AND_BIND( KEY_F(5)   , "f5"       , "refresh"    );
  DEFINE_KEY_AND_BIND( KEY_F(6)   , "f6"       , "reload"     );
  DEFINE_KEY(          KEY_F(7)   , "f7"                      );
  DEFINE_KEY(          KEY_F(8)   , "f8"                      );
  DEFINE_KEY(          KEY_F(9)   , "f9"                      );
  DEFINE_KEY(          KEY_F(10)  , "f10"                     );
  DEFINE_KEY(          KEY_F(11)  , "f11"                     );
  DEFINE_KEY_AND_BIND( KEY_F(12)  , "f12"      , "redraw"     );

  bosh_log("key","default global bindings:\n");
  bosh_log_trie(keytrie);
}


/* cursor movement */
void ui_cursorset(bosh_t *b, int n) {
  if(n < b->header)
    n = b->header;
  if(n>0) {
//    n--;
    if(n < b->line)
      while(n < b->line)
        ui_cursorup(b,1);
    else
      while(n>b->line && ui_cursordown(b,1));
  }
  bosh_log("cursor","set %d: bosh[%p] v[%02d] c[%02d] l[%02d]\n",n,b,b->vx,b->cursor,b->line);
}

int ui_cursorup(bosh_t *b, int n) {
  bosh_log("cursor","up %d: bosh[%p] in:  v[%02d] c[%02d] l[%02d]\n",n,b,b->vx,b->cursor,b->line);
  if(!n)
    n = b->cursormovement;
  if(b->cursor >= n)
    b->cursor -= n;
  else if(b->vx >= n)
    b->vx -= n;
  if(b->vx==0 && b->cursor < b->header)
    b->cursor = b->header;
  b->line = b->vx + b->cursor;
  bosh_log("cursor","up %d: bosh[%p] out: v[%02d] c[%02d] l[%02d]\n",n,b,b->vx,b->cursor,b->line);
  return n;
}

int ui_cursordown(bosh_t *b, int n) {
  bosh_log("cursor","down %d: bosh[%p] in:  v[%02d] c[%02d] l[%02d]\n",n,b,b->vx,b->cursor,b->line);
  if(!n)
    n = b->cursormovement;
  if(b->line+b->cursormovement >= SIZE(b))
    return 0;
  if(b->cursor + n < b->parent->rlines)
    b->cursor += n;
  else
    b->vx += n;
  b->line = b->vx + b->cursor;
  bosh_log("cursor","down %d: bosh[%p] out: v[%02d] c[%02d] l[%02d]\n",n,b,b->vx,b->cursor,b->line);
  return n;
}

/* search */
int ui_search(bosh_t *b) {
  char *p;
  int n;
  bosh_log("search","search string: \"%s\"\n",search);
  if(!search)
    return -2;
  n = CB->line; /* starting line */
  ui_cursordown(CB,1);
  while(1) {
    if(n==CB->line)
      break;
     p = GET(CB,CB->line);
     if(!p)
       break;
    bosh_log("search","  current line %d: \"%s\"\n",b->line,p);
    if(bosh_regex_try(&bosh_regex_search,p)==0) {
      /* match */
      bosh_log("search","match on line %d\n",b->line,p);
      break;
    }
    if(b->line+1 >= SIZE(b)) {
      /* end of buffer */
      if(b->searchwrap) {
        ui_cursorset(b,0);
        continue;
      }
      else
        break;
    }
    if(!ui_cursordown(b,1))
      break;
  }
  return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Key handlers:
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* for key handlers */
static int *intalloc(int n) {
  int *r = bosh_malloc(sizeof(int));
  if(r)
    memcpy(r,&n,sizeof(int));
  return r;
}


/* normal mode */
int *keyhandler_normal(bosh_t *b, int key) {
  char *kd,*cmd;
  bosh_action_t *a;

#ifdef NCURSES_MOUSE_VERSION
  bosh_window_t *w;
  if(key==KEY_MOUSE) {
    if(getmouse(&mouse)==OK) {
      START(CP,0);
      while(1) {
        w = NEXT(CP);
        if(!w)
          break;
        if(w->flags & BOSH_WINDOW_FLAG_HIDDEN)
          break;
        if( (mouse.x >= w->rx) && (mouse.x < w->rx + w->rcols) && (mouse.y >= w->ry) && (mouse.y < w->ry + w->rlines) ) {
          char s[4];
          sprintf(s,"focus %d",w->index+1);
          cmd_run(BOSH_MODE_ACTION,s,NULL,CR_FULL);
        }
      }
    }
  }
#endif

  kd = keyarray[key];
  if(!kd) {
    bosh_log("key","mode[normal]: not defined [%d]\n",key);
    return NULL;
  }

  bosh_log("key","mode[normal]: found description:\"%s\"\n",kd);

  cmd = trie_lookup(keytrie,kd);

  if(cmd) {
    bosh_log("key","mode[normal]: key[%s] ==> globally bound to \"%s\"\n",kd,cmd);
    cmd_run(BOSH_MODE_ACTION,cmd,NULL,CR_FULL);
  } else {
    bosh_log("key","mode[normal]: key[%s] not bound globally\n",kd);

    a = trie_lookup(b->at,kd);
    if(!a) {
      bosh_log("key","mode[normal]: key[%s] not bound in current bosh\n",kd);
      return NULL;
    }
    bosh_log("key","mode[normal]: key[%s] bound to action in current bosh\n",kd);
    if(b->cursor>=SIZE(b)) {
      bosh_log("key","mode[normal]: bosh cursor out of range\n",kd);
      return NULL;
    }
    if(a->prompt) {
      readstr(a->prompt);
      if(!REPLY)
        return NULL;
      BOSHPARAM = bosh_strdup(REPLY);
    }
    bosh_action(b,kd);
  }
  return NULL;
}



int *keyhandler_old(bosh_t *b, int key) {
  int n;
  switch(key) {
    /* set autorefresh */
    case KEY_CTRLA:
      if(b->autorefresh) {
        b->autorefresh = 0;
      } else {
        readstr("autorefresh");
        if(REPLY && b->command) {
          b->autorefresh = strtol(REPLY,0,10);
          b->autorefreshtime = time(NULL);
        }
      }
      break;

#ifdef DEBUG
    /* hide window */
    case KEY_CTRLH:
      CW->flags |= BOSH_WINDOW_FLAG_HIDDEN;
      break;
#endif

    /* ^J: jump to line */
    case KEY_CTRLJ:
      readstr("line");
      if(REPLY) {
        n = strtol(REPLY,0,10);
      }
      break;

    /* ^O: run new command */
    case KEY_CTRLO:
      readstr("command");
      if(REPLY) {
        bosh_free(b->command);
        b->command = bosh_strdup(REPLY);
        ungetch(KEY_CTRLR);
      }
      break;

}
  return NULL;
}

/* readstr mode key handler */
int *keyhandler_readstr(bosh_t *b, int key) {
  switch(key) {
    case KEY_CTRLX:
      bosh_finish(0);
    case KEY_CTRLC:
      return intalloc(-1);
    case KEY_CTRLH:
    case KEY_BKSPC:
    case KEY_BACKSPACE:
      if(strlen(REPLY)) {
        REPLY[strlen(REPLY)-1] = 0;
        mvaddch(stdscr->_cury,stdscr->_curx-1,' ');
        move(stdscr->_cury,stdscr->_curx-1);
      }
      break;
    case '\n':
      return intalloc(0);
    default:
      if(isprint(key)) {
        REPLY[strlen(REPLY)] = key;
        addch(key);
        refresh();
      }
      break;
  }
  return NULL;
}

/* thru mode key handler */
int *keyhandler_thru(bosh_t *b, int key) {
  if(b->childstate!=BOSH_CHILDSTATE_RUNNING)
    return intalloc(0);
  switch(key) {
    case KEY_CTRLT:
    case KEY_CTRLX:
      return intalloc(0);
    case ERR:
      break;
    default:
      bosh_write(b,key);
  }
  return NULL;
}

/* edit mode key handler */
int *keyhandler_edit(bosh_t *b, int key) {
  switch(key) {
    case KEY_CTRLE:
      return intalloc(0);

    /* insert blank line into list */
    case KEY_CTRLU:
    case KEY_IC:
      //list_insdup(&window->bosh->list,window->bosh->line,"\0",1);
      break;

    /* delete item from list */
    case KEY_CTRLK:
    case KEY_DC:
      DEL(b,b->line);
      if(b->line >= SIZE(b))
        ungetch(KEY_UP);
      break;
  }
  return NULL;
}


/* calculate the real x or y position of a bosh window */
static int ui_calcpos(int n, int m) {
  float f;
  if(n>=0)
    return n;
  f = (float)(n*-1) * (float)m/100.0;
  return (int)f;
}

/* calculate the real x or y dimension of a bosh window */
static int ui_calcdim(int n, int p, int m) {
  float f;
  if(n>=0) {
    if(!n)
      n = 0xffff;
    return lof(n,m-p);
  }
  f = (float)(n*-1) * (float)m/100.0;
  if(p+(int)f>m)
    f = m-p;
  return (int)f;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ui_redraw_window
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void ui_redraw_window(bosh_window_t *w, int mode) {
  int n,r=0,i,m,s,e;
  char *p;
  bosh_t *b = w->bosh;

  w->rx     = ui_calcpos(w->x,COLS);
  w->ry     = ui_calcpos(w->y,ULINES) + HEADER;
  w->rcols  = ui_calcdim(w->ncols,w->rx,COLS);
  w->rlines = ui_calcdim(w->nlines,w->ry,LINES-FOOTER);

  bosh_log("redraw","window[%d] bosh[%p]:\n",w->index+1,b);
  bosh_log("redraw","  %d,%d,%d,%d -> %d,%d,%d,%d\n",w->x,w->y,w->ncols,w->nlines,w->rx,w->ry,w->rcols,w->rlines);

  if(b->fgcolor) {
    bosh_log("redraw","  fgcolor[%d]\n",b->fgcolor-1);
    attron(COLOR_PAIR(b->fgcolor-1));
  }

  START(b,b->vx);
  for(n=0;n < w->rlines;n++) {
    p = NEXT(b);
    if(p) if((r = (n >= b->cursor && n < b->cursor + b->cursorsize))) {
      /* cursor */
      bosh_log("redraw","  line %d: cursor\n",n);
      attron(A_REVERSE);
    }
    mvaddstr(w->ry+n,w->rx,strmak(w->rcols,' '));
    if(p) {
      #ifndef DEBUG
      p++;
      #endif
      mvaddstr(w->ry+n,w->rx,strtrnc(p,w->rcols));
      //  this makes bosh crash :-( - linked list could be dodgy:   addstr(strmak(lof(COLS,bosh->ncols)-strlen(p),' '));

      for(i=0;i<BOSH_COLORS;i++) if(bosh_regex_highlight[i]) if(bosh_regex_try(&bosh_regex_highlight[i],p)) {
        bosh_log("redraw","  line[%d] highlight[%d] match",n,i);
        attron(COLOR_PAIR(i));
        for(m=0;m<sizeof(MATCH);m++) {
          s = MATCH[m].rm_so;
          e = MATCH[m].rm_eo;
          if(s>=0) {
            bosh_log("redraw","  match so[%d] eo[%d]",s,e);
            mvaddstr(w->ry+n,w->rx+s,substr(p,s,e));
          }
        }
        attroff(COLOR_PAIR(i));
      }
      if(r)
        attroff(A_REVERSE);
    }
#ifdef DEBUG
    else
      mvaddstr(w->ry+n,w->rx,"---");
#endif
  }

  if(b->fgcolor)
    attroff(COLOR_PAIR(b->fgcolor-1));
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ui_redraw
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define WIDTHOF(n) ((n<10?1:(n<100?2:(n<1000?3:(n<10000?5:6)))))
void ui_redraw(int mode) {
  int n;
  ULINES = LINES - FRAME;
  bosh_t *b = CB;
  bosh_window_t *w;

  erase();

//  Ensure cursor is in buffer range. Commented out because it was being called on a refresh
//  while(bosh->vx+bosh->cursor>list_items(bosh->list)+1)
//    cursor_up();

  /* draw top bar */
  attron(A_REVERSE);
  mvaddstr(0,0,strmak(COLS,' '));
  if(conf)
    mvprintw(0,0,"%s",conf);
  if(b->title)
    printw(" (%s)",b->title);
  mvprintw(0,COLS-(strlen(PACKAGE)+strlen(VERSION)+1),"%s %s",PACKAGE,VERSION);
  attroff(A_REVERSE);
  mvaddstr(1,0,strmak(COLS,' '));

  /* draw windows of current panel */
  bosh_log("redraw","panel (COLS=%d, LINES=%d):\n",COLS,LINES);
  START(CP,0);
  while(1) {
    w = NEXT(CP);
    if(!w)
      break;
    if(w->flags & BOSH_WINDOW_FLAG_HIDDEN)
      break;
    ui_redraw_window(w,mode);
  }

  /* draw bottom bar */
  mvaddstr(LINES-2,0,strmak(COLS,' '));
  attron(A_REVERSE);
  mvaddstr(LINES-1,0,strmak(COLS,' '));
  move(LINES-1,0);
  switch(mode) {
    case BOSH_MODE_ACTION:
    case BOSH_MODE_VIEWCONF:
      /* message set */
      if(UI_MESSAGE) {
        if(UI_MESSAGE_COUNT) {
          printw("* %s",UI_MESSAGE);
          if(--UI_MESSAGE_COUNT==0)
            ui_message(NULL);
        }
        break;
      }

      /* cursor pos */
      printw("[%c:%d] ",mode==BOSH_MODE_VIEWCONF?'V':PL->listpos+'A',CP->listpos);
      n = hof(WIDTHOF(SIZE(b)),2);
      if(b->cursorsize>1)
        printw("%0*d-%0*d/%0*d ",n,b->line+1,n,lof(b->line+1+b->cursorsize-1,n),n,SIZE(b));
      else
        printw("%0*d/%0*d ",n,b->line,n,SIZE(b));

      /* child state */
      printw("[");
      switch(b->childstate) {
        case BOSH_CHILDSTATE_NONE:
          printw(b->command?"unknown":"none");
          break;
        case BOSH_CHILDSTATE_RUNNING:
          printw("running");
          break;
        case BOSH_CHILDSTATE_STDIN:
          printw("stdin");
          break;
        default:
          printw("exited:");
          if(b->childstate>=0)
            printw("%2d",b->childstate);
          else
            printw("sig%d",b->childstate*-1);
      }
      printw("]");
      printw(" %s ",b->refresh?"R":" ");
      if(b->autorefresh)
        printw("%d",b->autorefresh - (time(NULL) - b->autorefreshtime));
      break;

    case BOSH_MODE_READSTR:
      if(strlen(PROMPT)==1)
        printw("%s %s",PROMPT,REPLY);
      else
        printw("%s: %s",PROMPT,REPLY);
      break;

    case BOSH_MODE_THRU:
      addstr("THRU");
      break;
  }
  attroff(A_REVERSE);
  /* done */
  refresh();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  ui_message: simple screen refresh
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void ui_refresh() {
  erase();
  refresh();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  ui_message: prints message to status bar
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void ui_message(char *s) {
  if(s) {
    free(UI_MESSAGE);
    UI_MESSAGE = strdup(s);
    UI_MESSAGE_COUNT = UI_MESSAGE_SHOWN;
  } else {
    free(UI_MESSAGE);
    UI_MESSAGE = NULL;
    UI_MESSAGE_COUNT = 0;
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  ui_run: does a getch and handles key
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int *ui_run(int mode, int needredraw) {
  int *r=NULL,key;
  bosh_t *b = CB;
  bosh_window_t *w = NULL;

  key = getch();

  //bosh_log("ui","getch() returned key: %d\n",key);

  switch(key) {
    case KEY_RESIZE:
      needredraw = 1;
    case ERR:
      goto redraw;
  }
  needredraw = 1;
  switch(mode) {
    case BOSH_MODE_ACTION:
      r = keyhandler_normal(b,key);
      break;
    case BOSH_MODE_READSTR:
      r = keyhandler_readstr(b,key);
      break;
    case BOSH_MODE_EDIT:
      r = keyhandler_edit(b,key);
      break;
    case BOSH_MODE_THRU:
      r = keyhandler_thru(b,key);
      break;
    case BOSH_MODE_VIEWCONF:
      w = list_get(&(viewconf->list),0);
      if(w) {
        b = list_get(&(w->list),0);
        r = keyhandler_normal(b,key);
      }
      break;
  }

redraw:
  if(needredraw)
    ui_redraw(mode);
  return r;
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  readstr mode
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int readstr(char *prompt) {
  int n;
  bosh_free(REPLY);
  REPLY = bosh_malloc(128);
  if(!REPLY)
    return -1;
  memset(REPLY,0,128);
  bosh_free(PROMPT);
  PROMPT = bosh_strdup(prompt);
  curs_set(1);
  ui_redraw(BOSH_MODE_READSTR);
  n = bosh_mode(BOSH_MODE_READSTR);
  curs_set(0);
  if(n) {
    /* cancelled */
    bosh_free(REPLY);
    REPLY = NULL;
    return -1;
  }
  bosh_log("readstr","success: REPLY=\"%s\"\n",REPLY);
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  readstr mode with previous input available
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int readstrp(char *prompt, char *previous) {
  char *s;
  int r;
  if(previous) {
    s = bosh_malloc(strlen(prompt)+strlen(previous)+4);
    sprintf(s,"%s [%s]",prompt,previous);
    r = readstr(s);
    bosh_free(s);
  } else {
    r = readstr(prompt);
  }
  return r;
}
