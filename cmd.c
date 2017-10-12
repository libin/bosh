/*
internal cmd trie and handler functions

$Id: cmd.c,v 1.33 2010/02/25 14:21:04 alexsisson Exp $

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rc.h"
#include "cmd.h"
#include "misc.h"
#include "bosh.h"
#include "trie.h"
#include "stray.h"
#include "misc.h"
#include "ui.h"
#include "objects.h"
#include "system.h"

/* cmd handler includes */
#include "cmd_general.h"

#ifndef LOG
static
#endif
trie_t *cmdtrie[BOSH_MODE_MAX];

#define CMD_TRIE_ADD1(c,m1)       trie_add(cmdtrie[(m1)],#c,cmd_handler_ ## c)

#define CMD_TRIE_ADD2(c,m1,m2)    trie_add(cmdtrie[(m1)],#c,cmd_handler_ ## c);\
                                  trie_add(cmdtrie[(m2)],#c,cmd_handler_ ## c)

#define CMD_TRIE_ADD3(c,m1,m2,m3) trie_add(cmdtrie[(m1)],#c,cmd_handler_ ## c);\
                                  trie_add(cmdtrie[(m2)],#c,cmd_handler_ ## c);\
                                  trie_add(cmdtrie[(m3)],#c,cmd_handler_ ## c)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * cmd_trie_init()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int cmd_trie_init() {
  bosh_log("cmdtrie","init\n");
  cmdtrie[BOSH_MODE_INIT]   = trie_new();
  cmdtrie[BOSH_MODE_ACTION] = trie_new();
  cmdtrie[BOSH_MODE_EDIT]   = trie_new();
  cmdtrie[BOSH_MODE_CHILD]  = trie_new();

  CMD_TRIE_ADD2(action,      BOSH_MODE_INIT, BOSH_MODE_ACTION);
  CMD_TRIE_ADD2(autorefresh, BOSH_MODE_INIT, BOSH_MODE_ACTION);

  CMD_TRIE_ADD1(back,                        BOSH_MODE_ACTION);
  CMD_TRIE_ADD2(command,     BOSH_MODE_INIT, BOSH_MODE_ACTION);
  CMD_TRIE_ADD2(common,      BOSH_MODE_INIT, BOSH_MODE_ACTION);
  CMD_TRIE_ADD1(console,                     BOSH_MODE_ACTION);

  CMD_TRIE_ADD3(cursorup,                    BOSH_MODE_ACTION, BOSH_MODE_EDIT, BOSH_MODE_CHILD);
  CMD_TRIE_ADD3(cursordown,                  BOSH_MODE_ACTION, BOSH_MODE_EDIT, BOSH_MODE_CHILD);
  CMD_TRIE_ADD2(cursorpgup,                  BOSH_MODE_ACTION, BOSH_MODE_EDIT);
  CMD_TRIE_ADD2(cursorpgdn,                  BOSH_MODE_ACTION, BOSH_MODE_EDIT);

  CMD_TRIE_ADD1(edit,                        BOSH_MODE_ACTION);
  CMD_TRIE_ADD2(exit,                        BOSH_MODE_ACTION, BOSH_MODE_EDIT);

  CMD_TRIE_ADD3(fgcolor,     BOSH_MODE_INIT, BOSH_MODE_ACTION, BOSH_MODE_EDIT);
  CMD_TRIE_ADD3(focus,       BOSH_MODE_INIT, BOSH_MODE_ACTION, BOSH_MODE_EDIT);

  CMD_TRIE_ADD2(header,      BOSH_MODE_INIT, BOSH_MODE_ACTION);

  CMD_TRIE_ADD1(kill,                        BOSH_MODE_ACTION);

  CMD_TRIE_ADD1(pipe,                        BOSH_MODE_ACTION);
  CMD_TRIE_ADD2(preaction,   BOSH_MODE_INIT, BOSH_MODE_ACTION);
  CMD_TRIE_ADD2(redraw,                      BOSH_MODE_ACTION, BOSH_MODE_EDIT);
  CMD_TRIE_ADD2(refresh,     BOSH_MODE_INIT, BOSH_MODE_ACTION);

  CMD_TRIE_ADD2(search,                      BOSH_MODE_ACTION, BOSH_MODE_EDIT);
  CMD_TRIE_ADD2(stderr,      BOSH_MODE_INIT, BOSH_MODE_ACTION);

  CMD_TRIE_ADD2(uservars,    BOSH_MODE_INIT, BOSH_MODE_ACTION);
  CMD_TRIE_ADD1(window,      BOSH_MODE_INIT);

/*
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,forward);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,help);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,highlight);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,message);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,reload);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,repeat);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,stdin);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,thru);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,version);
  CMD_TRIE_ADD1(BOSH_MODE_ACTION,viewconf);
*/
  bosh_log_trie(cmdtrie[BOSH_MODE_INIT]);
  bosh_log_trie(cmdtrie[BOSH_MODE_ACTION]);
  bosh_log_trie(cmdtrie[BOSH_MODE_EDIT]);
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * cmd_trie_lookup()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void *cmd_trie_lookup(int mode, char *cmd) {
  void *f;
  f = trie_lookup(cmdtrie[mode],cmd);
  if(f)
    bosh_log("cmdtrie","lookup: success! \"%s\"\n",cmd);
  else
    bosh_log("cmdtrie","lookup: not found \"%s\"\n",cmd);
  return f;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * cmd_error()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void cmd_error(int mode, char *cmd, char *format, ...) {
  va_list v;
  static char s[256];
  sprintf(s,"[%s] ",cmd);
  va_start(v,format);
  vsprintf(s + strlen(s),format,v);
  va_end(v);

  bosh_log("cmderror","mode=%d %s\n",mode,s);

  switch(mode) {
    case BOSH_MODE_INIT:
      bosh_fatal_err(1,s);
    case BOSH_MODE_ACTION:
      ui_message(s);
      ui_redraw(BOSH_MODE_ACTION);
      break;
    default:
      break;
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * cmd_run()

flags:

CR_FULL
CR_TOK
CR_STRAY

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int cmd_run(int mode, char *cmd, void *args, int flag) {
  int (*f)(stray_t*, int context);
  stray_t *a = NULL;
  char *p = args;
  int r;

  bosh_log("cmdrun","start: cmd=\"%s\" mode=%d \n",cmd,mode);

  cmd = bosh_strdup(cmd);

  switch(flag) {
    case CR_FULL:
      /* cmd parameter contains a full command string with arguments.
         seperate command from arguments and drop through */
      bosh_log("cmdrun","  CR_FULL: \"%s\"\n",cmd);
      p = strchr(cmd,' ');
      if(p)
        *p++ = 0;
    case CR_TOK:
      /* cmd contains just the command, args contains arguments to be tokenized */
      bosh_log("cmdrun","  CR_TOK: cmd=\"%s\" args=\"%s\"\n",cmd,p);
      a = p ? stray_tok(p," ") : stray_new(STRAY_EMPTY);
      break;
    case CR_STRAY:
      /* args is a stray */
      a = args;
      break;
    default:
      /* else treat args as one arg */
      bosh_log("cmdrun","  noflag: cmd=\"%s\" args=%s\n",cmd,bosh_log_printify(p));
      a = stray_new(STRAY_EMPTY);
      if(p)
        stray_addstr_d(a,p);
      break;
  }

  bosh_log("cmdrun","args:\n");
  bosh_log_stray(a);

  f = cmd_trie_lookup(mode,cmd);
  if(!f) {
    r = CMD_NOTFOUND;
    cmd_error(mode,cmd,"command not found");
    goto cleanup;
  }

  bosh_log("cmdrun","calling handler \"%s\"...\n",cmd);
  r = f(a,mode);
  switch(r) {
    case CMD_SUCCESS:
      goto cleanup;
    case CMD_ERR_INSUFFICIENT_ARGS:
      cmd_error(mode,cmd,"insufficient arguments");
      break;
    case CMD_ERR_INVALID_ARG:
      cmd_error(mode,cmd,"invalid argument");
      break;
    case CMD_ERR_INCORRECT_CONTEXT:
      cmd_error(mode,cmd,"incorrect context");
      break;
    default:
      cmd_error(mode,cmd,"unknown error");
      break;
  }

cleanup:
  /* TODO :free stray if we created it */
  bosh_free(cmd); /* strdup()ed */
  return r;
}
