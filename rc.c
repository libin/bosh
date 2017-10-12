/*
Config file and argument parsing related routines.

$Id: rc.c,v 1.70 2010/02/19 23:46:58 alexsisson Exp $

(C) Copyright 2002-2009 Alex Sisson (alexsisson@gmail.com)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rc.h"
#include "cmd.h"
#include "misc.h"
#include "bosh.h"

static int line = 0;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_parse_args
 *
 * - parses command lines arguments
 * - calls rc_read on first non option argument
 * - assembles remaining arguments in childargs
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int bosh_parse_args(stray_t *args) {
  int i,o=1;
  char *a,*p;
  void *h;

  bosh_log("parseargs","before processing:\n");
  bosh_log_stray(args);

  stray_free(confargs);
  confargs = stray_new(STRAY_EMPTY);

  /* find first non option arg (conf) */
  for(i=1;i< args->c;i++) {
    p = a = stray_get(args,i);
    if(!p)
      break;
    if(*p!='-') {
      confpath = bosh_strdup(p);
      stray_set_d(args,i,"--");
      break;
    }
  }

  if(confpath) {
    conf = strrchr(confpath,'/');
    conf = conf ? conf+1 : confpath;
    conf = bosh_strdup(conf);
    bosh_rc_read(confpath);
  }

  for(i=1;i< args->c;i++) {
    p = a = stray_get(args,i);
    if(!p)
      break;
    bosh_log("parseargs","processing: \"%s\"\n",p);
    if(o && *p=='-') {
      p++;
      if(*p=='-') {
        /* long opt */
        p++;
        if(*p==0) {
          bosh_log("parseargs","stopped processing (--)\n");
          o = 0;
        } else {
          p = strswp(p,'=',0);
          if(p)
            p++;
          else
            p = bosh_strdup(""); //doesnt get free()ed
          h = cmd_trie_lookup(BOSH_MODE_INIT,a+2);
          if(!h)
            bosh_fatal_err(1,"invalid argument: \"%s\"",a);
          cmd_run(BOSH_MODE_INIT,a+2,p,0);
        }
      } else {
        /* short opt */
        switch(*p) {
          case 'h':
            cmd_run(BOSH_MODE_INIT,"help",NULL,CR_FULL);
          case 'v':
            cmd_run(BOSH_MODE_INIT,"version",NULL,CR_FULL);
          case '\0':
            bosh_log("parseargs","stdin=0 (-)\n");
            cmd_run(BOSH_MODE_INIT,"stdin 0",p,CR_FULL);
            break;
          default:
            if(!isdigit(*p))
              bosh_fatal_err(1,"invalid argument: -%c",*p);
            bosh_log("parseargs","stdin=%s (-)\n",*p);
            cmd_run(BOSH_MODE_INIT,"stdin",p,0);
            break;
        }
      }
    } else {
      /* add to conf args */
      bosh_log("parseargs","  adding to confargs\n");
      stray_addstr_d(confargs,a);
    }
  }

  bosh_log("parseargs","confargs:\n");
  bosh_log_stray(confargs);
  return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * bosh_rc_read
 *
 * Reads a configuration
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int bosh_rc_read(char *path) {

  FILE *f;
  void *h;
  char s[256],*p,*a,*v,*k,*ao;
  int isaction;
  stray_t *aa;

  /* open config */
  f = fopen(path,"r");
  if(!f)
    bosh_fatal_err(1,"file not found '%s'",path);

  while(!feof(f)) {
    if(!fgets(s,sizeof(s),f)) {
      if(ferror(f))
        bosh_fatal_err_conf(1,"read error");
      break;
    }

    line++;

    /* it's a comment */
    if(*s=='#')
      continue;

    /* lose \n */
    strswp(s,'\n',0);

    /* it's an empty line */
    if(!*s)
      continue;

    /* try and find equals-sign */
    p = strchr(s,'=');
    if(p) {
      *p = 0;
      p++;
      a = bosh_strdup(s); /* attribute */
      v = bosh_strdup(p); /* value */

      /* handle value on next line */
      if(!*v) {
        if(!fgets(s,sizeof(s),f)) {
          if(ferror(f))
            bosh_fatal_err_conf(1,"read error\n");
          break;
        }
        bosh_free(v);
        if(*s=='#')
          continue;
        strswp(s,'\n',0);
        v = bosh_strdup(s);
      }

      /* handle multi-line value that uses \ */
      while(v[strlen(v)-1]=='\\') {
        v[strlen(v)-1]='\n';
        if(!fgets(s,sizeof(s),f)) {
          if(ferror(f))
            bosh_fatal_err_conf(1,"read error\n");
          break;
        }
        line++;
        if(*s=='#')
          continue;
        strswp(s,'\n',0);
        v = bosh_realloc(v,strlen(v)+strlen(s)+1);
        strcat(v,s);
      }

    } else {
      /* check for a {{ }} block */
      p = strstr(s,"{{");
      if(p) {
        *p = 0;
        p++;
        a = bosh_strdup(s); /* attribute */
        v = bosh_strdup("\0");
        while(1) {
          if(!fgets(s,sizeof(s),f)) {
            if(ferror(f))
              bosh_fatal_err_conf(1,"read error\n");
            break;
          }
          if(strncmp(s,"}}",2)==0)
            break;
          line++;
          v = bosh_realloc(v,strlen(v)+strlen(s)+1);
          strcat(v,s);
        }
      } else {
         /* error */
         bosh_fatal_err_conf(1,"= or {{ expected\n");
      }
    }

    bosh_log("rcread","a=\"%s\" v=\"%s\"\n",a,bosh_log_printify(v));
    /* process */
    isaction = 0;
    if(strlen(a)==1) {
      /* simple alpha numberic action */
      isaction = 1;
      k = a;
    } else {
      ao = NULL;
      if(a[0]=='.') {
        isaction = 1;
        k = a + 1;
        ao = strchr(k,'[');
      } else if(a[1]=='[') {
        isaction = 1;
        k = a;
        ao = a + 1;
      }
      if(ao) {
        p = strdup2(ao,v);
        bosh_free(v);
        v = p;
        *ao = 0;
      }
    }

    if(isaction) {
      bosh_log("rcread","action key=\"%s\" val=\"%s\"\n",k,bosh_log_printify(v));
      aa = stray_new(STRAY_EMPTY);
      stray_addstr(aa,k);
      stray_addstr(aa,v);
      cmd_run(BOSH_MODE_INIT,"action",aa,CR_STRAY);
    } else {
      h = cmd_trie_lookup(BOSH_MODE_INIT,a);
      if(!h)
        bosh_fatal_err_conf(1,"unrecognized option '%s'",a);
      cmd_run(BOSH_MODE_INIT,a,v,0);
    }

    bosh_free(a);
    bosh_free(v);
  }

  fclose(f);
  return 0;
}

/*
int bosh_rc_write(char *name, bosh_t *bosh) {
  return 0;
}
*/

int bosh_rc_line() {
  return line;
}
