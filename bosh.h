/*
$Id: bosh.h,v 1.91 2010/02/21 23:27:36 alexsisson Exp $

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


#ifndef BOSH_H_INCLUDED
#define BOSH_H_INCLUDED

#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include "list.h"
#include "objects.h"
#include "stray.h"

#define HEADER 2
#define FOOTER 2
#define FRAME  (HEADER+FOOTER)


#define BOSH_WIDTH_DEFAULT            32

/* usleep timings */
#define BOSH_USLEEP_INIT         1000000
#define BOSH_USLEEP_POPEN           1000
#define BOSH_USLEEP_INPUT         100000
#define BOSH_USLEEP_KILL           10000


/* bosh interface modes */
#define BOSH_MODE_INIT     0
#define BOSH_MODE_ACTION   1
#define BOSH_MODE_READSTR  2
#define BOSH_MODE_EDIT     3
#define BOSH_MODE_THRU     4
#define BOSH_MODE_VIEWCONF 5
#define BOSH_MODE_CHILD    6
#define BOSH_MODE_MAX      7 /* set to highest mode + 1 */

/* bosh action types */
#define BOSH_ACTION_COMMAND  0x01
#define BOSH_ACTION_BOSH     0x02

/* bosh action input types */
#define BOSH_ACTION_INPUT_CURSOR 0  /* traditional $BOSH */
#define BOSH_ACTION_INPUT_ALL    1  /* all lines in a big array or something like that TODO */
#define BOSH_ACTION_INPUT_PIPE   2  /* input piped to action */

/* bosh action output destinations */
#define BOSH_ACTION_OUTPUT_NONE       ' '
#define BOSH_ACTION_OUTPUT_OVERWRITE  '.'
#define BOSH_ACTION_OUTPUT_ADVANCE    '>'
#define BOSH_ACTION_OUTPUT_ENDCURSES  '!'
#define BOSH_ACTION_OUTPUT_SEND       '~'
#define BOSH_ACTION_OUTPUT_NEWWINDOW  /* not decided/implemented  */

/* TODO: bosh special actions */
#define BOSH_ACTION_SPECIAL_SHOWCONF  40

/* special values for bosh_t.exit */
#define BOSH_CHILDSTATE_NONE    -100
#define BOSH_CHILDSTATE_RUNNING -101
#define BOSH_CHILDSTATE_STDIN   -102

/* meaning of bosh_popen cmd argument */
#define BOSH_POPEN_ACTION             1
#define BOSH_POPEN_NODUP              2
#define BOSH_POPEN_WAIT               4
#define BOSH_POPEN_FILTER             16

#define BOSH_WINDOW_FLAG_HIDDEN       1
#define BOSH_WINDOW_FLAG_DISABLED     2

/* bosh_read */
#define BOSH_READBUF_DEFAULT 1024

/* how to handle child stderr */
#define BOSH_STDERR_MERGE      0  /* default behaviour just merges it in with stdout */
#define BOSH_STDERR_EXIT       1  /* exit bosh and print child stderr to our stderr */
#define BOSH_STDERR_IGNORE     2  /* ignore stderr */
#define BOSH_STDERR_ONLY       1  /* only show stderr */

#define BOSH_STDERR_EXIT_SEEN 11  /* for internal use: set by bosh_read when stderr is set to exit and data has come from stderr */

/* child fd's */
#define FD_STDIN   0
#define FD_STDOUT  1
#define FD_STDERR  2
#define FD_COMMAND 3

/* ncurses-style key defitions */
#define KEY_CTRLA       1
#define KEY_CTRLB       2
#define KEY_CTRLC       3
#define KEY_CTRLD       4
#define KEY_CTRLE       5
#define KEY_CTRLF       6
#define KEY_CTRLG       7
#define KEY_CTRLH       8
#define KEY_CTRLI       9
#define KEY_CTRLJ       10
#define KEY_CTRLK       11
#define KEY_CTRLL       12
#define KEY_CTRLM       13
#define KEY_CTRLN       14
#define KEY_CTRLO       15
#define KEY_CTRLP       16
#define KEY_CTRLQ       17
#define KEY_CTRLR       18
#define KEY_CTRLS       19
#define KEY_CTRLT       20
#define KEY_CTRLU       21
#define KEY_CTRLV       22
#define KEY_CTRLW       23
#define KEY_CTRLX       24
#define KEY_CTRLY       25
#define KEY_CTRLZ       26
#define KEY_TAB         9
#define KEY_SPACE       32
#define KEY_BKSPC       127

#define BOSH_COLORS     16


/* list wrappers */
#define GET(o,i)       list_get(&(o)->list,(i))
#define ADD(o,d)       list_add(&(o)->list,(d))
#define SIZE(o)        list_items(&(o)->list)
#define START(o,n)     list_start(&(o)->list,(n))
#define NEXT(o)        list_next(&(o)->list)
#define DEL(o,n)       list_del(&(o)->list,(n))


extern char    *conf;
extern stray_t *confargs;
extern char    *confpath;
extern char    *shell;

extern int     ULINES;        /* usable lines */
extern char    *search;

extern char             *BOSH;
extern char             *BOSHPARAM;
extern char             *BOSHERR;
extern int               boshuservars;
extern char            **boshuservar;
extern char             *common;
extern bosh_panellist_t *panellist;
extern int               stdinfd;

extern regex_t *bosh_regex_search;
extern regex_t *bosh_regex_highlight[BOSH_COLORS];
extern regmatch_t MATCH[100];

extern bosh_panel_t    *viewconf;

#define PL (panellist)
#define CP (PL->panel)
#define CW (CP->window)
#define CB (CW->bosh)


void bosh_init();
void bosh_finish(int s);
int  bosh_mode(int mode);

#endif
