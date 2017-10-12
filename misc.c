/*
Misc functions

$Id: misc.c,v 1.24 2009/11/09 01:23:06 alexsisson Exp $

(C) Copyright 2005-2009 Alex Sisson (alexsisson@gmail.com)

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
#include <stdarg.h>
#include <sys/file.h>
#include <unistd.h>
#include <regex.h>

#include "misc.h"
#include "stray.h"
#include "trie.h"
#include "rc.h"

char *strdup2(const char *s1, const char *s2) {
  char *r;
  r = bosh_malloc(strlen(s1)+strlen(s2)+1);
  if(r) {
    sprintf(r,"%s%s",s1,s2);
  }
  return r;
}

/* makes a \0 terminated string of length len filled with c */
char *strmak(size_t len, int c) {
  static char *buf = NULL;
  bosh_free(buf);
  buf = bosh_malloc(len+1);
  if(buf) {
    memset(buf,c,len);
    buf[len] = 0;
  }
  return buf;
}

/* swaps first occurance of char 'c' with 'replace' */
char *strswp(char *s, int c, int replace) {
  char *p = strchr(s,c);
  if(p)
    *p = replace;
  return p;
}

/* strtrnc: returns copy of s truncated to n chars - not thread safe */
char *strtrnc(char *s, size_t n) {
  static char buf[1024];
  memcpy(buf,s,n);
#ifdef DEBUG
  buf[n-1]='@';
#endif
  buf[n]=0;
  return buf;
}

/* substr: returns copy of substring of s - not thread safe */
char *substr(char *s, size_t so, size_t eo) {
  return strtrnc(s+so,eo-so);
}


/* bosh_atoi */
long int bosh_atoi(const char *nptr) {
  char *e;
  long int r;
  r = strtol(nptr,&e,10);
  if(*e)
    bosh_fatal_err(1,"%s: invalid numerical argument",PACKAGE);
  return r;
}

/* fatal error message and exit */
void bosh_fatal_err(int code, char *format, ...) {
  va_list v;
  static char s[512];
  va_start(v,format);
  vsprintf(s,format,v);
  va_end(v);
  fprintf(stderr,"%s: %s\n",PACKAGE,s);
  bosh_log("fatal: %s\n",s);
  exit(code);
}

/* fatal error message and exit with file/line number */
void bosh_fatal_err_conf(int code, char *format, ...) {
  va_list v;
  static char s[512];
  sprintf(s,"[%s:%02d] ", conf,bosh_rc_line());
  va_start(v,format);
  vsprintf(s + strlen(s),format,v);
  va_end(v);
  fprintf(stderr,"%s: %s\n",PACKAGE,s);
  bosh_log("fatal: %s\n",s);
  exit(code);
}

/* fprintf which also logs */
void bosh_fprintfl(FILE *stream, char *logprefix, const char *format, ...) {
  va_list v;
#ifdef LOG
  char s[512];
  va_start(v,format);
  vsprintf(s,format,v);
  va_end(v);
  fprintf(stream,"%s",s);
  bosh_log(logprefix,"%s",bosh_log_printify(s));
#else
  va_start(v,format);
  vfprintf(stream,format,v);
  va_end(v);
#endif
}



/* regex */
int bosh_regex_init(regex_t **r, char *s) {
  int n;
  static char errbuf[128];
  if(*r) {
    #ifdef LOG
    bosh_log("regex","freeing() regex[%p]\n",*r);
    #endif
    regfree(*r);
    bosh_free(*r);
  }
  *r = bosh_malloc(sizeof(regex_t));
  n = regcomp(*r,s,REG_EXTENDED|REG_ICASE);
  if(n) {
    regerror(n,*r,errbuf,sizeof(errbuf));
    #ifdef LOG
    bosh_log("regex","regcomp() /%s/ returned error[n:%s]\n",s,n,errbuf);
    #endif
    /* TODO: report error back to interface */
    bosh_free(*r);
    *r = NULL;
    return -1;
  }
  #ifdef LOG
  bosh_log("regex","regcomp() /%s/ ok regex[%p]\n",s,*r);
  #endif
  return 0;
}

int bosh_regex_try(regex_t **r, char *s) {
//  return regexec(*r,s,sizeof(MATCH),MATCH,0);
  return regexec(*r,s,0,0,0);
}



/* logging */
#ifdef LOG
FILE *BOSHLOG = NULL;

/* open log */
void bosh_log_open(const char *mode) {
  BOSHLOG = fopen("bosh.log",mode);
}

/* standard log */
void bosh_log(char *prefix, char *format, ...) {
  va_list v;
  static char p[256];
  if(BOSHLOG) {
    flock(fileno(BOSHLOG),LOCK_EX);
    fseek(BOSHLOG,0,SEEK_END);
    if(prefix) {
      sprintf(p,"%s:",prefix);
      fprintf(BOSHLOG,"%-12s",prefix);
    }
    va_start(v,format);
    vfprintf(BOSHLOG,format,v);
    va_end(v);
    fflush(BOSHLOG);
    flock(fileno(BOSHLOG),LOCK_UN);
  }
}

/* log stray_t */
void bosh_log_stray(stray_t *a) {
  int i;
  bosh_log("stray","->c: %d\n",a->c);
  for(i=0;i<=a->c;i++)
    bosh_log("stray","->v[%02d]: [%08p] %s\n",i,a->v[i],a->v[i]?bosh_log_printify(a->v[i]):"NULL");
}

/* log trie_t */
static char log_trie_buf[256];
static int  log_trie_len=0;
void bosh_log_trie(trie_t *t) {
  int i;
  int c=0;
  for(i=0;i<9;i++) {
    if(t->child[i]) {
      log_trie_buf[log_trie_len++] = i+48;
      log_trie_buf[log_trie_len] = 0;
      bosh_log_trie(t->child[i]);
      log_trie_len--;
      c++;
    }
  }
  for(i=10;i<36;i++) {
    if(t->child[i]) {
      log_trie_buf[log_trie_len++] = i+87;
      log_trie_buf[log_trie_len] = 0;
      bosh_log_trie(t->child[i]);
      log_trie_len--;
      c++;
    }
  }
  if(!c)
    bosh_log("","  %s\n",log_trie_buf);
}



char *bosh_log_printify(char *s) {
  static char *buf=NULL,*pb,*ps;
  if(!s)
    return NULL;
  bosh_free(buf);
  pb = buf = bosh_malloc(strlen(s)*2);
  ps = s;
  while(*ps) {
    if(isprint(*ps)) {
      *pb = *ps;
      pb++;
      ps++;
    } else {
      *pb++ = '\\';
      *pb++ = 'n';
      ps++;
    }
  }
  *pb=0;
  return buf;
}


#ifdef LOGMEM
static size_t ss = sizeof(size_t);
static size_t tb = 0;

/* wrappers for memory tracking */
void *bosh_malloc(size_t size) {
  void *ptr;
  ptr = malloc(size+ss);
  if(ptr) {
    memcpy(ptr,&size,ss);
    ptr += ss;
    tb += size;
    bosh_log("mem","malloc()ed %08d bytes ==> tb: %d\n",size,tb);
  } else {
    bosh_log("mem","malloc() failed\n",size);
  }
  return ptr;
}

void *bosh_realloc(void *ptr, size_t size) {
  void *new_ptr;
  size_t old_size;
  ptr -= ss;
  memcpy(&old_size,ptr,ss);
  new_ptr = realloc(ptr,size+ss);
  if(new_ptr) {
    memcpy(new_ptr,&size,ss);
    new_ptr += ss;
    tb -= old_size;
    tb += size;
    bosh_log("mem","realloc()ed %08d ==> %08d bytes ==> tb: %d\n",old_size,size,tb);
  } else {
    bosh_log("mem","realloc() failed\n",size);
  }
  return new_ptr;
}

char *bosh_strdup(const char *s) {
  char *ptr;
  size_t size;
  size = strlen(s) + 1 + ss;
  ptr = malloc(size);
  if(ptr) {
    memcpy(ptr,&size,ss);
    ptr += ss;
    memcpy(ptr,s,size-ss);
    tb += size;
    bosh_log("mem","strdup()ed %08d bytes ==> tb: %d\n",size,tb);
  } else {
    bosh_log("mem","strdup() failed\n",size);
  }
  return ptr;
}

void bosh_free(void *ptr) {
  size_t size;
  if(ptr) {
    ptr -= ss;
    memcpy(&size,ptr,ss);
    free(ptr);
    tb -= size;
    bosh_log("mem","free()ed %08d bytes   ==> tb: %d\n",size,tb);
  }
}
#endif

#endif

