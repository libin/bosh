/*
bosh system stuff (spawning processes etc)

$Id: system.c,v 1.112 2010/02/25 14:21:04 alexsisson Exp $

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

/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <fcntl.h>

#include "system.h"
#include "bosh.h"
#include "misc.h"
#include "rc.h"
#include "ui.h"
#include "cmd.h"

#ifdef DEBUG
int debug_leavetmp = 1;
#endif
#ifdef LOG
int log_read = 0;
#endif

/*
 *  bosh_prepare_*
 *
 *  these generate the script, and returns the array for execv
 *  for a particular shell/interpreter.
 *
 */
static stray_t *bosh_prepare_bash(bosh_t *bosh, char *command, FILE *script) {
  int n;
  stray_t *execvargs;

  char *p = NULL;
#ifdef LOG
  p = bosh_malloc(32);
  sprintf(p,"bash");
#endif

  if(BOSH)
    bosh_fprintfl(script,p,"BOSH=\"%s\"\n",BOSH);
  if(BOSHPARAM)
    bosh_fprintfl(script,p,"BOSHPARAM=\"%s\"\n",BOSHPARAM);
  bosh_fprintfl(script,p,"BOSHCONF=\"%s\"\n",conf);
  bosh_fprintfl(script,p,"BOSHPID=%d\n",getpid());
  bosh_fprintfl(script,p,"BOSHPPID=%d\n",getppid());
  bosh_fprintfl(script,p,"BOSHCURSOR=%d\n",bosh->cursor);

  if(bosh->title)
    bosh_fprintfl(script,p,"BOSHTITLE=\"%s\"\n",bosh->title);
  for(n=0;n<boshuservars;n++)
    if(boshuservar[n])
      bosh_fprintfl(script,p,"BOSHVAR%d=\"%s\"\n",n+1,boshuservar[n]);

  /* function for script to send commands back to bosh */
  bosh_fprintfl(script,p,"bosh_write() {\n");
  bosh_fprintfl(script,p,"  echo \"$@\" >&3\n");
  bosh_fprintfl(script,p,"}\n");

  /* define "main" function */
  bosh_fprintfl(script,p,"BOSHFUNCTION() {\n");
  if(common)
    bosh_fprintfl(script,p,"# \"common\"\n%s\n",common);
  bosh_fprintfl(script,p,"# \"command\"\n%s\n",command);
  bosh_fprintfl(script,p,"}\n");

  /* setup "main" call */
  if(bosh->tmpfpipe)
    bosh_fprintfl(script,p,"cat %s |",bosh->tmpfpipe);
  bosh_fprintfl(script,p,"BOSHFUNCTION \"$@\"");
  if(bosh->pipe)
    bosh_fprintfl(script,p," | %s",bosh->pipe);
  bosh_fprintfl(script,p,"\nBOSHRETURN=$?\n");

  /* send variables down FD_COMMAND (3) back to bosh */
  bosh_fprintfl(script,p,"bosh_write \"set BOSHERR    $BOSHERR\"\n");
  bosh_fprintfl(script,p,"bosh_write \"set BOSHCURSOR $BOSHCURSOR\"\n");
  bosh_fprintfl(script,p,"bosh_write \"set BOSHTITLE  $BOSHTITLE\"\n");
  for(n=0;n<boshuservars;n++)
    bosh_fprintfl(script,p,"bosh_write \"set BOSHVAR%d $BOSHVAR$d\"\n",n+1,n+1);
  /* close FD_COMMAND */
  bosh_fprintfl(script,p,"exec 3>&-\n");

  /* return */
  bosh_fprintfl(script,p,"exit $BOSHRETURN\n");

  /* construct execv args */
  execvargs = stray_new(STRAY_EMPTY);
  stray_addstr_d(execvargs,"/bin/bash");
  stray_addstr_d(execvargs,"--");
  stray_addstr_d(execvargs,bosh->script);
  stray_addarr_d(execvargs,confargs);
  return execvargs;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  bosh_popen
 *
 *  this is where a lot of the business gets done.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int bosh_popen(bosh_t *b, char *cmd, int flags) {

  int fd[2],parent[4],child[4],i,n,r=-1;
  pid_t pid;

  bosh_log("popen","bosh[%p] cmd=\"%s\" b->command=\"%s\" flags=%d:\n",b,cmd,bosh_log_printify(b->command),flags);

  if(b->childpid) {
    bosh_log("popen","bosh[%p] existing child process %d\n",b,b->childpid);
    bosh_close(b,1);
  }

  /* check that std{in,out,err} are 0,1,2 */
  if(0!=STDIN_FILENO || 1!=STDOUT_FILENO || 2!=STDERR_FILENO)
    return -1;

  /* create a pipe for each stream */
  for(i=0;i<4;i++) {
    if(pipe(fd))
      goto pipecleanup;
    if(i==0) {
      /* stdin */
      parent[i] = fd[1];
      child [i] = fd[0];
    } else {
      /* stdout/stderr/command */
      parent[i] = fd[0];
      child [i] = fd[1];
    }
  }

  /* create tmp script filename */
  strcpy(b->script,"/tmp/boshs_XXXXXX");
  mktemp(b->script);

  bosh_log("popen","bosh[%p] forking...\n",b);

  pid = fork();

  /* PARENT */
  if(pid>0) {
    bosh_log("popen","bosh[%p] [parent] fork success: child pid=\"%d\"\n",b,pid);

    /* close child's ends of the pipes */
    for(i=0;i<=3;i++)
      close(child[i]);

    /* see if child has died already due to an invalid command */
    usleep(BOSH_USLEEP_POPEN);
    if(pid==waitpid(pid,&n,WNOHANG))
      if(WIFEXITED(n))
        if(WEXITSTATUS(n)==127)
          goto pipecleanup;

    /* clear the buffer */
    list_reinit(&b->list);

    /* set non-blockin on fd's that parent will read from */
    for(i=1;i<4;i++) {
      n = fcntl(parent[i],F_SETFL,O_NONBLOCK);
      bosh_log("popen","bosh[%p] [parent] fcntl() F_SETFL O_NONBLOCK on fd[%d] returned %d: \"%s\"\n",b,parent[i],n,(n==-1) ? errstr : "success");
    }

    /* save child details */
    b->childfd[FD_STDIN]   = parent[0];
    b->childfd[FD_STDOUT]  = parent[1];
    b->childfd[FD_STDERR]  = parent[2];
    b->childfd[FD_COMMAND] = parent[3];
    b->childpid            = pid;
    b->childstate          = BOSH_CHILDSTATE_RUNNING;

    if(flags & BOSH_POPEN_WAIT) {
      bosh_log("popen","bosh[%p] [parent] waiting for child pid \"%d\"\n",b,pid);
      waitpid(b->childpid,0,0);
      bosh_close(b,1);
    }

    return 0;
  }

  /* CHILD */
  if(pid==0) {

    FILE *f;
    stray_t *a;
    pid_t sid;

    /* reset signal handlers - we don't want a child signal to bring down bosh by calling finish() */
    signal(SIGTERM,SIG_DFL);
    signal(SIGHUP,SIG_DFL);
    signal(SIGSEGV,SIG_DFL);
    signal(SIGINT,SIG_DFL);
    signal(SIGPIPE,SIG_DFL);

    pid = getpid();
    sid = setsid();

    bosh_log_open("a");
    bosh_log("popen","bosh[%p] [child:%d] setsid() returned %d\n",b,pid,sid);

    /* close parent's ends of the pipes, and dup our ends onto our std streams before closing them */
    for(i=0;i<=3;i++) {
      close(parent[i]);
      if(!(flags & BOSH_POPEN_NODUP)) {
        n = dup2(child[i],i);
        bosh_log("popen","bosh[%p] [child:%d] dup2(%02d==>%02d) returned %d: %s\n",b,pid,child[i],i,n,(n==-1) ? errstr : "success");
        close(child[i]);
      }
    }

    if(flags & BOSH_POPEN_FILTER) {
      /* nothing */
    } else {
      // *b->tmpfpipe = 0;
      if(!cmd)
        cmd = b->command;
    }

    /* create and prepare tmpscript for the command/action */
    f = fopen(b->script,"w");
    if(!f)
      return -1;
    bosh_log("popen","bosh[%p] [child:%d] created script: \"%s\"\n",b,pid,b->script);
    a = bosh_prepare_bash(b,cmd,f);
    fclose(f);
    chmod(b->script,0700); /* TODO: we should probably set the umask first */

    bosh_log("popen","bosh[%p] [child:%d] execv args:\n",b,pid);
    bosh_log_stray(a);
    bosh_log("popen","bosh[%p] [child:%d] execv()ing...:\n",b,pid);

    execv(a->v[0],a->v);

    bosh_log("popen","bosh[%p] [child:%d] execv failed!\n",b,pid);

    /* exec failed if we get here - drop through to cleanup. */
    /* or not - this pipecleanup business needs sorting */
    _exit(1);
  }

  /* fork() failed */
  /* TODO indicate error somehow */;
pipecleanup:
  for(n=0;n<=2;n++) {
    close(parent[n]);
    close(child[n]);
  }

  return r;
}


int bosh_open(bosh_t *bosh) {
  return bosh_popen(bosh,NULL,0);
}


static char *bosh_mkdollarbosh(bosh_t *b) {
  char *r=NULL,*sep=NULL,*p;
  int i,n;
  if(b->cursorsize>1) {
     /* multiline $BOSH */
    sep = b->multilineseperator ? b->multilineseperator : "";
    for(n=1,i=0;i < b->cursorsize;i++) {
      p = GET(b,b->vx+b->cursor+i);
      if(p)
        n += strlen(p) + strlen(sep);
    }
    r = bosh_malloc(n);
    if(!r)
      return NULL;
    *r = 0;
    for(i=0;i < b->cursorsize;i++) {
      p = GET(b,b->vx+b->cursor+i);
      if(p)
        sprintf(p+strlen(p),"%s%s",p+1,sep);
    }
  } else {
    /* single line $BOSH */
    p = GET(b,b->vx+b->cursor);
    if(p)
      r = bosh_strdup(p+1);
  }
  return r;
}


/* * * * * * * * * * * * * * * * * * * * * * *
 *
 * bosh_savebuf
 *
 * saves bosh b to a temporary file
 *
 * returns temporary file name
 *
 * * * * * * * * * * * * * * * * * * * * * * */
char *bosh_savebuf(bosh_t *b) {
  char *p,*r;
  FILE *f;
  r = bosh_strdup("/tmp/bosht_XXXXXX");
  bosh_log("savebuf","bosh[%p] saving to \"%s\"\n",b,r);
  f = fdopen(mkstemp(r),"w");
  START(b,0);
  while(1) {
    p = NEXT(b);
    if(!p)
      break;
    fprintf(f,"%s\n",p+1);
  }
  fclose(f);
  return r;
}



/* * * * * * * * * * * * * * * * * * * * * * *
 *
 * bosh_action
 *
 * prepare $BOSH and action destination stuff
 * and then run action.
 *
 * returns <0 on error, >0 if needs redraw
 *
 * * * * * * * * * * * * * * * * * * * * * * */

int bosh_action(bosh_t *b, char *key) {

  int ai,ao,r=0;
  bosh_action_t *a;
  bosh_window_t *aw;
  bosh_panel_t  *ap;
  char *action,*p=NULL;

  a = trie_lookup(b->at,key);

  if(!a)
    return -1;

/* TODO
  if(action->type==BOSH_ACTION_SPECIAL_SHOWCONF) {
    // shov conf (^v)
    // TODO: put this in it's own special window/pane?
    at = BOSH_ACTION_OUTPUT_ADVANCE;
    if(confpath) {
      action = malloc(strlen(confpath)+7);
      if(action)
        sprintf(action,"cat '%s'",confpath);
    }
    if(!action)
      return -1;
*/

  /* normal action */
  if(b->preaction)
    action = strdup2(b->preaction,a->command);
  else
    action = bosh_strdup(a->command);
  ai = a->input;
  ao = a->output;

  if(a->panel) {
    ap = GET(PL,a->panel-1);
  } else {
    aw = b->parent;
    if(aw)
      ap = aw->parent;
  }
  if(!ap) {
    bosh_log("action","invalid target panel %c\n",a->panel+64);
    return -1; /* TODO: show error */
  }

  aw = a->window ? GET(ap,a->window) : b->parent;
  if(!aw) {
    bosh_log("action","invalid target window %d\n",a->window);
    return -1; /* TODO: show error */
  }

  bosh_log("action","target: %c%d\n",a->panel+'A',a->window);
  bosh_free(BOSH);
  switch(ai) {
    case BOSH_ACTION_INPUT_CURSOR:
      bosh_log("action","input: cursor\n");
      BOSH = bosh_mkdollarbosh(b);
      break;
    case BOSH_ACTION_INPUT_ALL:
      /* TODO */
      bosh_log("action","input: all lines\n");
      break;
    case BOSH_ACTION_INPUT_PIPE:
      bosh_log("action","input: pipe\n");
      p = bosh_savebuf(b);
      break;
    default:
      bosh_log("action","input: unknown[%d]\n",ai);
      break;
  }

  /* take appropriate action depending on output */
  switch(ao) {
    case BOSH_ACTION_OUTPUT_OVERWRITE:
      bosh_log("action","output: overwrite\n");
      //list_reinit(bosh->buf);  - TODO can't remember why this is commented out!
      aw->bosh->command = strdup(action);
      aw->bosh->line = aw->bosh->vx = aw->bosh->cursor = 0;
      if(p) {
        aw->bosh->tmpfpipe = p;
        bosh_popen(aw->bosh,NULL,BOSH_POPEN_ACTION|BOSH_POPEN_FILTER);
      } else
        bosh_popen(aw->bosh,NULL,BOSH_POPEN_ACTION);
      break;

    case BOSH_ACTION_OUTPUT_ADVANCE:
      bosh_log("action","output: advance\n");
      window_newbosh(aw,aw->listpos+1);
      window_next(aw);
      aw->bosh->command = bosh_strdup(action);
      bosh_popen(aw->bosh,NULL,BOSH_POPEN_ACTION);
      break;

    case BOSH_ACTION_OUTPUT_ENDCURSES:
      bosh_log("action","output: end curses\n");
      endwin();
      bosh_popen(b,action,BOSH_POPEN_ACTION|BOSH_POPEN_NODUP|BOSH_POPEN_WAIT);
      if(b->refresh)
        bosh_open(b);
      r = 1;
      break;

    case BOSH_ACTION_OUTPUT_NONE:
      bosh_log("action","output: none\n");
      bosh_popen(b,action,BOSH_POPEN_ACTION|BOSH_POPEN_WAIT);
      if(b->refresh)
        bosh_open(b);
      break;

    case BOSH_ACTION_OUTPUT_SEND:
      bosh_log("action","output: send\n");
      p = a->command;
      while(*p)
        bosh_write(aw->bosh,*p++);
      break;

    default:
      break;
  }

  bosh_free(action);
  bosh_free(BOSH);
  bosh_free(BOSHPARAM);
  BOSH = NULL;
  BOSHPARAM = NULL;
  return r;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * bosh_readfd
 *
 * reads from fd, which has data available.
 *
 * returns +1 if a new line is read, -1 if the fd is to be closed
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int bosh_readfd(bosh_t *b, int fd) {
  int r = 0, n;
  char c,*p;
  bosh_readbuf_t *readbuf = &(b->readbuf[fd-1]);
  fd = b->childfd[fd];

  /* initialize read buf */
  if(!readbuf->data) {
    readbuf->len  = BOSH_READBUF_DEFAULT;
    readbuf->data = bosh_malloc(readbuf->len);
  }

READ:
  n = read(fd,&c,1);
  switch(n) {
    case -1:
      /* error */
    case 0:
      /* eof */
      bosh_log("read","bosh[%p] pid[%05d] fd[%d] %s%s\n",b,b->childpid,fd,n?"error: ":"eof",n?errstr:"");
      if(fd==b->childfd[FD_STDERR] && b->stderr==BOSH_STDERR_EXIT_SEEN) {
        START(b,0);
        while(1) {
          p = NEXT(b);
          if(!p)
            break;
          if(*p=='e') {
            //fprintf(stderr,"%s: %s\n",PACKAGE,p+1);
            BOSHERR = strdup(p+1);
            bosh_finish(1);
          }
        }
      }
      r = -1;
      goto END;

    default:
      /* got data */
      #ifdef LOG
      if(log_read)
        bosh_log("read","bosh[%p] pid[%05d] fd[%d] buf[%d/%d] got %03d %c\n",b,b->childpid,fd,readbuf->pos,readbuf->len,c,isprint(c)?c:' ');
      #endif
      if(fd==b->childfd[FD_STDERR] && b->stderr==BOSH_STDERR_EXIT) {
        b->stderr = BOSH_STDERR_EXIT_SEEN;
      }
      if(c=='\n'||c=='\r') {
        /* handle end of line */
        readbuf->data[readbuf->pos] = 0;

        if(fd==b->childfd[FD_COMMAND]) {
          /* child has sent a command */
          bosh_log("read","bosh[%p] pid[%05d] received command: '%s'\n",b,b->childpid,readbuf->data);
          cmd_run(BOSH_MODE_CHILD,(char*)readbuf->data,NULL,CR_FULL);
          readbuf->pos = 0;
          break;
        }

        /* else add to output buffer */
        if(c=='\r' && SIZE(b))
          DEL(b,SIZE(b)-1);
        list_adddup(&b->list,readbuf->data,readbuf->pos+1);
        if(b->parent->ncols)
          if(b->parent->ncols < readbuf->pos)
            b->parent->ncols = readbuf->pos;
        #ifdef LOG
        if(log_read) {
          readbuf->data[readbuf->pos] = 0;
          bosh_log("read","line: %s\n",readbuf->data);
        }
        #endif
        readbuf->data[0] = (fd==b->childfd[FD_STDOUT]) ? 'o':'e';
        readbuf->pos = 1;
        r = 1;
        break;
      }

      /* else append to readbuf */
      readbuf->data[readbuf->pos] = c;
      readbuf->pos++;
      if(readbuf->pos >= readbuf->len) {
        /* extend readbuf */
        readbuf->len *= 2;
        readbuf = bosh_realloc(readbuf->data,readbuf->len);
      }
      break;
  }
  goto READ;
END:
  return r;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * bosh_read
 *
 * read from child process if data available
 *
 * returns non-zero if a new line is read, or child is closed
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int bosh_read(bosh_t *b) {
  fd_set fdsetr;
  int nfds=0,i=0,n=0,r=0;
  struct timeval tv = {0,0};

  /* add fd's to fdsets */
  FD_ZERO(&fdsetr);
  FD_SETIFVALID(b->childfd[FD_STDOUT], &fdsetr,nfds);
  FD_SETIFVALID(b->childfd[FD_STDERR], &fdsetr,nfds);
  FD_SETIFVALID(b->childfd[FD_COMMAND],&fdsetr,nfds);

  if(nfds==0) {
    bosh_log("read","bosh[%p] pid[%05d] read: no open child streams\n",b,b->childpid);
    return r;
  }

  /* select() */
  n = select(nfds+1,&fdsetr,0,0,&tv);
  if(n<=0) {
    /* TODO: handle error ? */
    bosh_log("read","bosh[%p] pid[%05d] read: select() returned %d: %s\n",b,b->childpid,n,n?errstr:"no fd's ready");
    return r;
  }

  bosh_log("read","bosh[%p] pid[%05d] read: select() returned %d fd's ready\n",b,b->childpid,n);

  /* read set fd's and count invalids (ie closed) */
  nfds = 0;
  for(i=1;i<4;i++) {
    if(FD_ISVALIDANDSET(b->childfd[i],&fdsetr)) {
      bosh_log("read","bosh[%p] pid[%05d] read: child fd[%d] (%d) ready\n",b,b->childpid,i,b->childfd[i]);
      n = bosh_readfd(b,i);
      switch(n) {
        case -1:
          FD_CLOSE(b->childfd[i]);
        case 1:
          r = 1;
          break;
        default:
          break;
      }
    }
    if(!FD_ISVALID(b->childfd[i])) {
      bosh_log("read","bosh[%p] pid[%05d] read: child fd[%d] invalid (closed)\n",b,b->childpid,i);
      nfds++;
    }
  }

  /* close if 3 closed fd's */
  if(nfds==3) {
    bosh_log("read","bosh[%p] pid[%05d] read: all child streams closed. closing...\n",b,b->childpid);
    bosh_close(b,1);
  }

  return r;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * bosh_write
 *
 * writes to child's stdin
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * */

int bosh_write(bosh_t *b, char c) {
  bosh_log("write","bosh[%p] pid[%05d] fd[%d] c=%c\n",b,b->childpid,b->childfd[FD_STDIN],isprint(c)?c:' ');
  return write(b->childfd[FD_STDIN],&c,1);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * bosh_close
 *
 * cleans up child process and reads user vars
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * */

void bosh_close(bosh_t *b, int readvarfile) {
  int i,n;
  FILE *vf;
  char s[256];

  if(FD_ISVALID(b->childfd[FD_STDIN])) {
    bosh_log("close","bosh[%p] pid[%05d] close()ing fd child stdin [%d]\n",b,b->childpid,b->childfd[FD_STDIN]);
    FD_CLOSE(b->childfd[FD_STDIN]);
  }
  if(FD_ISVALID(b->childfd[FD_STDOUT])) {
    bosh_log("close","bosh[%p] pid[%05d] close()ing fd child stdout[%d]\n",b,b->childpid,b->childfd[FD_STDOUT]);
    FD_CLOSE(b->childfd[FD_STDOUT]);
  }
  if(FD_ISVALID(b->childfd[FD_STDERR])) {
    bosh_log("close","bosh[%p] pid[%05d] close()ing fd child stderr[%d]\n",b,b->childpid,b->childfd[FD_STDERR]);
    FD_CLOSE(b->childfd[FD_STDOUT]);
  }
  if(FD_ISVALID(b->childfd[FD_COMMAND])) {
    bosh_log("close","bosh[%p] pid[%05d] close()ing fd child stderr[%d]\n",b,b->childpid,b->childfd[FD_COMMAND]);
    FD_CLOSE(b->childfd[FD_COMMAND]);
  }

  if(!b->command) /* contents was from stdin */
    goto unlink;

  #ifdef LOG
  if(b->childpid)
     bosh_log("close","bosh[%p] pid[%05d] reap loop:\n",b,b->childpid);
  #endif

  while(b->childpid) {
    /* need to reap child */
    bosh_log("close","bosh[%p] pid[%05d] waitpid()ing for child\n",b,b->childpid);
    n = waitpid(b->childpid,&i,WNOHANG);
    switch(n) {
      case -1:
        b->childpid = 0;
        bosh_log("close","bosh[%p] pid[%05d]   waitpid() error: %d:%s\n",b,b->childpid,n,errstr);
        break;
      case 0:
        bosh_log("close","bosh[%p] pid[%05d]   killing child\n",b,b->childpid);
        bosh_kill(b);
        usleep(BOSH_USLEEP_KILL);
        continue;
      default:
        if(WIFEXITED(i)) {
          b->childstate = WEXITSTATUS(i);
          bosh_log("close","bosh[%p] pid[%05d]    child exited ($?==%d)\n",b,b->childpid,b->childstate);
          b->childpid = 0;
        } else if(WIFSIGNALED(i)) {
          b->childstate = WTERMSIG(i) * -1;
          bosh_log("close","  child terminated by signal %d\n",b,b->childpid,b->childstate*-1);
          b->childpid = 0;
        }
        break;
    }
  }

  bosh_unlink(b->script);

  /* get VARs back from child */
  if(!readvarfile)
    goto unlink;

  vf = NULL; //fopen(bosh->E,"r");
  if(!vf) {
    /* TODO: Decide how to handle this:
       Currently, if actions that target a different window are used,
       BOSHVARFILE doesn't get set properly and so this fopen fails.
       Once that's fixed, this needs to be handled better with some
       kind of error indication */
    goto unlink;
  }

  /* read $BOSHERR */
  fgets(s,sizeof(s),vf);
  strswp(s,'\n',0);
  if(strlen(s)) {
    /* child has set BOSHERR */
    fclose(vf);
    BOSHERR = bosh_strdup(s);
    bosh_log("close","varfile: $BOSHERR \"%s\"\n",BOSHERR);
    bosh_finish(b->childstate);
  }

  /* read $BOSHCURSOR */
  fgets(s,sizeof(s),vf);
  strswp(s,'\n',0);
  if(strlen(s)) {
    ui_cursorset(b,atoi(s));
    bosh_log("close","varfile: BOSHCURSOR \"%d\"\n",b->cursor);
  }

  /* read $BOSHTITLE */
  fgets(s,sizeof(s),vf);
  strswp(s,'\n',0);
  bosh_free(b->title);
  if(strlen(s)) {
    b->title = bosh_strdup(s);
    bosh_log("close","varfile: BOSHTITLE \"%s\"\n",b->title);
  } else
    b->title = NULL;

  /* read user vars */
  for(i=0;i<boshuservars;i++) {
    bosh_free(boshuservar[i]);
    boshuservar[i] = bosh_malloc(256);
    if(!fgets(boshuservar[i],256,vf)) {
      bosh_free(boshuservar[i]);
      boshuservar[i] = NULL;
      break;
    }
    strswp(boshuservar[i],'\n',0);
    bosh_log("close","varfile: BOSHVAR%d/uservar[%d] \"%s\"\n",i+1,i,boshuservar[i]);
    if(!strlen(boshuservar[i])) {
      bosh_free(boshuservar[i]);
      boshuservar[i] = NULL;
    }
  }
  fclose(vf);

unlink:
  bosh_unlink(b->tmpfpipe);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_unlink
 *
 * unlink wrapper
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int bosh_unlink(const char *path) {
  int r=0;
  #ifdef DEBUG
  if(debug_leavetmp) {
    bosh_log("unlink","left \"%s\"\n",path);
  } else {
  #endif
    r = unlink(path);
    bosh_log("unlink","path=\"%s\" [%d:%s]\n",path,r,r?errstr:"success");
  #ifdef DEBUG
  }
  #endif
  return r;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_kill
 *
 * kill wrapper
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int bosh_kill(bosh_t *bosh) {
  int r = -1;
  if(bosh->childpid) {
    /* TODO progressively nastier signals (TERM -> ... -> KILL) ?? */
    r = kill(bosh->childpid*-1,15);
    bosh_log("kill","pid[%05] [%d:%s]\n",bosh->childpid,r,r?errstr:"success");
  }
  #ifdef LOG
  else
    bosh_log("kill","no child\n");
  #endif
  return r;
}
