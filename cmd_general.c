/*
internal cmd trie and handler functions

$Id: cmd_general.c,v 1.2 2010/02/21 23:27:36 alexsisson Exp $

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

#include <ctype.h>

#include "cmd.h"

/* action KEY ACTION *****************************************************************************/
/* takes a bosh key description string, and an action value (after the =) */
CMD_HANDLER(action) {
  int i,vo;
  bosh_action_t *action;
  char *key,*val;

  CMD_LOG();
  CMD_ARGC_CHECK(2);

  key = CMD_ARGV(0);
  val = CMD_ARGV(1);

  bosh_log("cmdhandler","[action] key=%s v=%s\n",key,bosh_log_printify(val));

  /* TODO: check its a valid key description */
  /* bosh_fatal_err(1,"[%s:%02d] invalid action '%c'",conf,line,a[0]); */

  /* check if already bound */
  action = trie_lookup(CB->at,key);
  if(action)
    bosh_fatal_err(1,"duplicate action for '%s'",key);

  action = bosh_malloc(sizeof(bosh_action_t));
  if(!action)
    bosh_fatal_err(1,"malloc() failed");

  memset(action,0,sizeof(bosh_action_t));

  action->input  = BOSH_ACTION_INPUT_CURSOR;
  action->output = BOSH_ACTION_OUTPUT_NONE;

  /* parse options */
  vo = 0;
  if(val[0]=='[') {
    vo++;
    if(val[vo]=='|') {
      action->input = BOSH_ACTION_INPUT_PIPE;
      vo++;
    }
    if(isupper(val[vo])) {
      /* panel specified */
      /* TODO - check it's valid */
      action->panel = val[vo] - 64;  /* A => 1, B +> 2 ... */
      bosh_log("cmdhandler","[action] target panel \"%c\"\n",action->panel+'A');
      vo++;
    }
    if(isdigit(val[vo])) {
      /* window specified */
      action->window = val[vo] - 48;
      action->output = BOSH_ACTION_OUTPUT_OVERWRITE;  /* default with an explicit window */
      bosh_log("cmdhandler","[action] target window=\"%d\" (%d)\n",action->window+1,action->window);
      vo++;
    } else {
      /* TODO: error */
    }
    switch(val[vo]) {
      /* type */
      case BOSH_ACTION_OUTPUT_NONE:      /* [ ] */
      case BOSH_ACTION_OUTPUT_OVERWRITE: /* [.] */
      case BOSH_ACTION_OUTPUT_ADVANCE:   /* [>] */
      case BOSH_ACTION_OUTPUT_ENDCURSES: /* [!] */
      case BOSH_ACTION_OUTPUT_SEND:      /* [~] */
        action->output = val[vo];
        bosh_log("cmdhandler","[action] type='%c'\n",action->output);
        vo++;
      case ':':
      case ']':
        /* use default */
        break;
      default:
        bosh_fatal_err(1,"[%s] invalid action destination '%c'",conf,val[vo]);
    }

    if(val[vo]==':') {
      /* action parameter */
      vo++;
      i=0;
      while(1) {
        if(!val[vo+i])
          bosh_fatal_err(1,"[%s] unexpected end of line",conf);
        if(val[vo+i]==']')
          break;
        i++;
      }
      val[vo+i] = 0;
      action->prompt = bosh_strdup(val+vo);
      bosh_log("cmdhandler","[action] prompt=%s\n",action->prompt);
      vo += i + 1;
    } else if(val[vo]!=']') {
      bosh_fatal_err(1,"[%s] ']' expected",conf);
    } else
      vo++;
  }

  if(!val[vo]) {
    bosh_fatal_err(1,"[%s] unexpected end of line",conf);
  }
  action->command = bosh_strdup(val+vo);
  bosh_log("cmdhandler","[action] command=%s\n",bosh_log_printify(action->command));
  trie_add(CB->at,key,action);
  return 0;
}

/* autorefresh SECONDS ***************************************************************************/
CMD_HANDLER(autorefresh) {
//  CB->autorefresh = bosh_atoi(optarg);
  CMD_LOG();
  return 0;
}

/* back [window] *********************************************************************************/
CMD_HANDLER(back) {
  CMD_LOG();
  if(a->c) {

  }
  window_prev(CW);
  return 0;
}

/* common COMMAND [bosh] *************************************************************************/
CMD_HANDLER(common) {
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  free(common);
  common = bosh_strdup(CMD_ARGV(0));
  bosh_log("cmdhandler","[common] \"%s\"\n",CB,bosh_log_printify(common));
  return 0;
}

/* command COMMAND [bosh] ************************************************************************/
CMD_HANDLER(command) {
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  CB->command = CMD_ARGV(0);
  bosh_log("cmdhandler","[command] bosh[%p]->command <== \"%s\"\n",CB,bosh_log_printify(CB->command));
  CMD_MODE_SWITCH {
    case BOSH_MODE_ACTION:
      bosh_open(CB);
    default:
      break;
  }
  return 0;
}

/* console ***************************************************************************************/
CMD_HANDLER(console) {
  CMD_LOG();
  readstr("");
  if(REPLY)
    cmd_run(mode,REPLY,NULL,CR_FULL);
  return 0;
}

/* cursordown [bosh] *****************************************************************************/
CMD_HANDLER(cursordown) {
  CMD_LOG();
  if(!CMD_ARGC) {

  }
  ui_cursordown(CB,0);
  return 0;
}

/* cursorpgdn [bosh] *****************************************************************************/
CMD_HANDLER(cursorpgdn) {
  int i;
  CMD_LOG();
  if(a->c) {

  }
  for(i=0;i<8;i++)
    ui_cursordown(CB,0);
  return 0;
}

/* cursorpgup [bosh] *****************************************************************************/
CMD_HANDLER(cursorpgup) {
  int i;
  CMD_LOG();
  if(a->c) {

  }
  for(i=0;i<8;i++)
    ui_cursorup(CB,0);
  return 0;
}

/* cursorup [bosh] *******************************************************************************/
CMD_HANDLER(cursorup) {
  CMD_LOG();
  if(a->c) {

  }
  ui_cursorup(CB,0);
  return 0;
}

/* thru ******************************************************************************************/
CMD_HANDLER(edit) {
  CMD_LOG();
  bosh_mode(BOSH_MODE_EDIT);
  return 0;
}

/* exit ******************************************************************************************/
CMD_HANDLER(exit) {
  /* TODO exit code argument */
  CMD_LOG();
  bosh_finish(0);
  return 0;
}

/* fgcolor ***************************************************************************************/
CMD_HANDLER(fgcolor) {
  int n;
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  n = bosh_atoi(CMD_ARGV(0));
  if(n>=0)
    CB->fgcolor = n+1;
  return 0;
}

/* focus [panel/window] **************************************************************************/
CMD_HANDLER(focus) {
  char *s;
  int i=0;
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  s = stray_get(a,0);
  if(isupper(*s)) {
    /* panel */
    panellist_setpanel(PL,*s-'A');
    s++;
  }
  if(isdigit(*s)) {
    /* window */
    panel_setwindow(CP,*s-'0');
    return 0;
  }
  switch(s[0]) {
    case '-':
    case '+':
      switch(s[1]) {
        case '-':
        case '+':
          if(s[0]!=s[1]) {
            /* +- or -+ */
            return CMD_ERR_INVALID_ARG;
          }
          s[1] = '1';
          /* drop through*/
        default:
          if(!isdigit(s[1]))
            return CMD_ERR_INVALID_ARG;
          i = s[1]-'0';
          if(s[0]=='+')
            while(i--)
              panel_nextwindow(CP);
          else
            while(i--)
              panel_prevwindow(CP);
          break;
      }
      break;
    case 0:
      break;
    default:
      return CMD_ERR_INVALID_ARG;
  }
  return 0;
}

/* forward [window] ******************************************************************************/
CMD_HANDLER(forward) {
  CMD_LOG();
  if(a->c) {

  }
  window_next(CW);
  return 0;
}

/* header VALUE **********************************************************************************/
CMD_HANDLER(header) {
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  CB->header = bosh_atoi(CMD_ARGV(0));
  return 0;
}

/* help ******************************************************************************************/
CMD_HANDLER(help) {
  CMD_LOG();
  CMD_MODE_SWITCH {
    case BOSH_MODE_INIT:
      printf("Please refer to the bosh(1) manpage for usage instructions\n");
      exit(0);
    default:
      /* TODO Help mode */
      break;
  }
  return 0;
}

/* highlight VALUE *******************************************************************************/
CMD_HANDLER(highlight) {
  int n;
  char *s;
  CMD_LOG();
  CMD_ARGC_CHECK(2);
  n = bosh_atoi(CMD_ARGV(0));
  if(n>=BOSH_COLORS)
    return -2;
  s = CMD_ARGV(1);
  bosh_regex_init(&bosh_regex_highlight[n],s);
  bosh_log("cmdhandler","[highlight] highlight[%d] == \"%s\"\n",n,s);
  return 0;
}

/* kill [TODO] ***********************************************************************************/
CMD_HANDLER(kill) {
  CMD_LOG();
  bosh_kill(CB);
  return 0;
}

/* message [MESSAGE] *****************************************************************************/
CMD_HANDLER(message) {
  CMD_LOG();
  if(CMD_ARGC)
    ui_message(CMD_ARGV(0));
  else
    ui_message(NULL);
  return 0;
}

/* pipe [bosh] ***********************************************************************************/
CMD_HANDLER(pipe) {
  CMD_LOG();
  /* TODO: handle arg */
  /* TODO context */
  readstrp("|",CB->pipe);
  if(REPLY) {
    if(strlen(REPLY)) {
      CB->tmpfpipe = bosh_savebuf(CB);
      bosh_popen(CB,REPLY,BOSH_POPEN_FILTER);
    }
  }
  return 0;
}

/* preaction PREACTION [bosh] ********************************************************************/
CMD_HANDLER(preaction) {
  CMD_LOG();
  CMD_ARGC_CHECK(0);
  CB->preaction = bosh_strdup(CMD_ARGV(0));
  bosh_log("cmdhandler","[preaction] bosh[%p]->preaction <== \"%s\"\n",CB,CB->preaction);
  return 0;
}
/* redraw ****************************************************************************************/
CMD_HANDLER(redraw) {
  CMD_LOG();
  ui_refresh();
  return 0;
}

/* refresh [value]********************************************************************************/
CMD_HANDLER(refresh) {
  CMD_LOG();
  if(CMD_ARGC) {
    CB->refresh = bosh_atoi(CMD_ARGV(0));
    bosh_log("cmdhandler","[refresh] bosh[%p]->refresh <== \"%d\"\n",CB,CB->refresh);
  } else if(CB->command)
    bosh_open(CB);
  return 0;
}

/* reload ****************************************************************************************/
CMD_HANDLER(reload) {
  CMD_LOG();
  if(conf)
    bosh_init();
  return 0;
}

/* repeat ****************************************************************************************/
CMD_HANDLER(repeat) {
  CMD_LOG();
  ui_search(CB);
  return 0;
}

/* search ****************************************************************************************/
CMD_HANDLER(search) {
  CMD_LOG();
  /* TODO context and errors */
  readstrp("search",search);
  if(REPLY) {
    bosh_log("cmdhandler","[search] readstrp returned \"%s\"\n",REPLY);
    if(strlen(REPLY)) {
      bosh_regex_init(&bosh_regex_search,REPLY);
      bosh_free(search);
      search = bosh_strdup(REPLY);
    }
    ui_search(CB);
  }
  return 0;
}

/* stderr N ************************************************************************************/
CMD_HANDLER(stderr) {
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  if(strcmp(CMD_ARGV(0),"exit")==0) {
    CB->stderr = BOSH_STDERR_EXIT;
  } else if(strcmp(CMD_ARGV(0),"ignore")==0) {
    CB->stderr = BOSH_STDERR_IGNORE;
  } else if(strcmp(CMD_ARGV(0),"merge")==0) {
    CB->stderr = BOSH_STDERR_MERGE;
  } else if(strcmp(CMD_ARGV(0),"only")==0) {
    CB->stderr = BOSH_STDERR_ONLY;
  } else {
    return CMD_ERR_INVALID_ARG;
  }
  bosh_log("cmdhandler","[stderr] bosh[%p]->stderr <== %s\n",CB,CMD_ARGV(0));
  return 0;
}

/* stdin [panel/window] ************************************************************************/
CMD_HANDLER(stdin) {
  char *z;
  bosh_panel_t *p = CP;
  bosh_window_t *w = CW;
  bosh_t *b;

  CMD_LOG();
  CMD_ERR(!FD_ISVALID(stdinfd),"no stdin");

  if(CMD_ARGC) {
    z = CMD_ARGV(0);
    if(isalpha(*z)) {
      p = GET(PL,*z-'A');
      CMD_ERR(!p,"invalid panel '%c'",*z);
      z++;
    }
    if(isdigit(*z)) {
      w = GET(p,*z-'0');
      CMD_ERR(!w,"invalid window '%c'",*z);
    }
  }
  b = GET(w,0);
  CMD_ERR(!b,"error");
  b->childfd[FD_STDIN] = stdinfd;
  b->childstate = BOSH_CHILDSTATE_STDIN;
  bosh_log("cmdhandler","[stdin] bosh[%p]->childout <== stdin[%d]\n",CB,stdinfd);
  return 0;
}

/* thru ******************************************************************************************/
CMD_HANDLER(thru) {
  CMD_LOG();
  bosh_mode(BOSH_MODE_THRU);
  return 0;
}

/* uservars N ************************************************************************************/
CMD_HANDLER(uservars) {
  int i;
  CMD_LOG();
  CMD_ARGC_CHECK(1);
  if(boshuservars) {
    for(i=0;i<boshuservars;i++)
      bosh_free(boshuservar[i]);
    bosh_free(boshuservar);
  }
  boshuservars = bosh_atoi(CMD_ARGV(0));
  if(boshuservars) {
    boshuservar = bosh_malloc(boshuservars*sizeof(char*));
    memset(boshuservar,0,boshuservars*sizeof(char*));
  }
  return 0;
}

/* version ***************************************************************************************/
CMD_HANDLER(version) {
  CMD_LOG();
  printf("%s %s",PACKAGE,VERSION);
  #ifdef DEBUG
  printf(" (DEBUG)");
  #endif
  #ifdef LOG
  printf(" (LOG)");
  #endif
  printf("\n");
  exit(0);
}

/* viewconf **************************************************************************************/
CMD_HANDLER(viewconf) {
  CMD_LOG();
  bosh_mode(BOSH_MODE_VIEWCONF);
  return 0;
}

/* window ****************************************************************************************/
/* can either accept 5 arguments, or 1 argument containing a string of those 5 arguments. */

static int bosh_window_parse(char *p) {
  return (*p=='%') ? bosh_atoi(p+1) * -1 : bosh_atoi(p);
}

CMD_HANDLER(window) {
  char *z;
  int pi=0,wi=0;
  bosh_panel_t  *p;
  bosh_window_t *w;

  CMD_LOG();

  if(CMD_ARGC!=1 && CMD_ARGC!=5)
    return -1;

  if(CMD_ARGC==1) {
    z = bosh_strdup(CMD_ARGV(0));
    bosh_free(a);
    a = stray_tok(z,",");
    if(a->c!=5)
      return -1;
    bosh_free(z);
  }

  z = stray_get(a,0);

  bosh_log("cmdhandler","[window] args:\n");
  bosh_log_stray(a);

  /* decode first arg */
  if(isalpha(*z)) {
    /* panel */
    pi = *z - (isupper(*z) ? 'A':'a');
    z++;
  }
  if(isdigit(*z)) {
    /* window number */
    wi = *z - '0';
  } else {
    /* TODO: some kind of error */
  }

  bosh_log("cmdhandler","[window] panel:%c   pi=%d\n",pi+'A',pi);
  bosh_log("cmdhandler","[window] window:%d wi=%d\n",wi+1,wi);

  /* make panel/window lists larger if needed */
  while(SIZE(PL)<=pi) {
    bosh_log("cmdhandler","[window] creating new panel");
    p = panel_new();
    ADD(PL,p);
  }

  p = GET(PL,pi);
  if(!p) {
    /* TODO: some kind of error */
  }

  while(SIZE(p)<=wi) {
    bosh_log("cmdhandler","[window] creating new window");
    w = panel_newwindow(p,-1);
  }

  w = GET(p,wi);
  w->x      = bosh_window_parse(stray_get(a,1));
  w->y      = bosh_window_parse(stray_get(a,2));
  w->ncols  = bosh_window_parse(stray_get(a,3));
  w->nlines = bosh_window_parse(stray_get(a,4));

  stray_free(a);
  return 0;
}
