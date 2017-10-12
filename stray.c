/*
stray (STRing arrAY)

ugly set of reusable routines for manipulating argc/argv style arrays
of strings. probably v. buggy and not particularly fast. also memleaks!

$Id: stray.c,v 1.9 2009/06/10 11:44:07 alexsisson Exp $

(C) Copyright 2009 Alex Sisson (alexsisson@gmail.com)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stray.h"
#include "misc.h"

static void stray_realloc(stray_t *a, int e) {
  a->v = bosh_realloc(a->v,(a->c+2)*sizeof(STRAY_TYPE));
}

stray_t *stray_new(int c, char **v) {
  stray_t *r = bosh_malloc(sizeof(stray_t));
  if(r) {
    if(c||v) {
      r->c = c;
      r->v = v;
    } else {
      r->c = 0;
      r->v = bosh_malloc(sizeof(STRAY_TYPE));
      r->v[0] = NULL;
    }
    r->shift = 0;
  }
  return r;
}

stray_t *stray_new_d(int c, char **v) {
  stray_t *r = NULL, *a = stray_new(c,v);
  if(a) {
    r = stray_dup(a);
    bosh_free(a);
  }
  return r;
}

stray_t *stray_dup(stray_t *a) {
  int i;
  stray_t *r = bosh_malloc(sizeof(stray_t));
  r->c = a->c;
  r->v = bosh_malloc(sizeof(STRAY_TYPE)*r->c+1);
  for(i=0;i<=r->c;i++)
    r->v[i] = a->v[i] ? bosh_strdup(a->v[i]) : NULL;
  return r;
}

void stray_free(stray_t *a) {
  /* TODO free strings */
  if(!a)
    return;
  bosh_free(a->v - a->shift);
}

void stray_clr(stray_t *a) {
  stray_free(a);
  a->c = 0;
  a->v = bosh_malloc(sizeof(STRAY_TYPE));
  a->v[0] = NULL;
}

void stray_debug(stray_t *a, FILE *stream) {
  int i;
  fprintf(stream,"== ->c: %d\n",a->c);
  for(i=0;i<=a->c;i++) {
    fprintf(stream,"== ->v[%02d]: [%p] %s\n",i,a->v[i],a->v[i]?a->v[i]:"NULL");
  }
}

void stray_addstr(stray_t *a, char *s) {
  stray_realloc(a,2);
  if(a->v) {
    a->v[a->c] = s;
    a->c++;
    a->v[a->c] = NULL;
  }
}

void stray_addstr_d(stray_t *a, const char *s) {
  stray_addstr(a,bosh_strdup(s));
}

void stray_addarr(stray_t *a, stray_t *b) {
  stray_realloc(a,b->c);
  if(a->v) {
    char **p = b->v;
    while(*p) {
      stray_addstr(a,*p);
      p++;
    }
  }
}

void stray_addarr_d(stray_t *a, stray_t *b) {
  stray_realloc(a,b->c);
  if(a->v) {
    char **p = b->v;
    while(*p) {
      stray_addstr_d(a,*p);
      p++;
    }
  }
}

STRAY_TYPE stray_get(stray_t *a, uint32_t ind) {
  return a->v[ind];
}

void stray_set(stray_t *a, uint32_t ind, STRAY_TYPE s) {
  a->v[ind] = s;
}

void stray_set_d(stray_t *a, uint32_t ind, STRAY_TYPE s) {
  a->v[ind] = bosh_strdup(s);
}

void stray_del(stray_t *a, uint32_t ind) {
  /* TODO free string */
  char **p = a->v + ind;
  if(ind<=a->c) {
    while(*p) {
      p[0]=p[1];
      p++;
    }
    if(a->c)
      a->c--;
  }
}


/* TODO */
stray_t *stray_cut(stray_t *a) {
  return NULL;
}


void stray_shift(stray_t *a) {
  /* TODO: preserve a->v for freeing later */
  a->c--;
  a->v++;
  a->shift++;
}



stray_t *stray_tok(char *s, char *delim) {
  stray_t *a = stray_new(STRAY_EMPTY);
  char *p,*t;
  t = p = s = bosh_strdup(s);
  while(1) {
    if(p-t) {
      if(!*p) {
        stray_addstr_d(a,t);
        break;
      }
      if(strchr(delim,*p)) {
        *p = 0;
        stray_addstr_d(a,t);
        t = p + 1;
      }
    }
    p++;
  }
  bosh_free(s);
  return a;
}

stray_t *stray_tok_q(char *s, char *delim) {
  stray_t *a = stray_new(STRAY_EMPTY);
  char *p,*t;
  int q;
  t = p = s = bosh_strdup(s);
  if(*t=='\'') {
    q = 1;
    t++;
    p++;
  }
  while(1) {
    if(p-t) {
      if(!*p) {
        stray_addstr_d(a,t);
        break;
      }
      if(strchr(delim,*p)) {
        if(!q || p-t>1) {
          *p = 0;
          stray_addstr_d(a,t);
          t = p + 1;
          q = 0;
          if(*t=='\'') {
            q = 1;
            t++;
            p++;
          }
        }
      }
    }
    p++;
  }
  bosh_free(s);
  return a;
}



#ifdef STRAY_TEST
int main(int argc, char **argv) {
  stray_t *a = stray_new(argc,argv);
  printf("new a:\n");
  stray_debug(a,stderr);
  stray_t *b = stray_new_d(argc,argv);
  printf("\nnew_d b:\n");
  stray_debug(b,stderr);
  stray_t *c = stray_dup(a);
  printf("\ndup(a)=c:\n");
  stray_debug(c,stderr);
  stray_t *d = stray_dup(b);
  printf("\ndup(b)=d:\n");
  stray_debug(d,stderr);

  printf("\naddstr(b,str):\n");
  stray_addstr_d(b,"the cat sat on the mat");
  stray_debug(b,stderr);

  printf("\naddarr(b,a):\n");
  stray_addarr_d(b,a);
  stray_debug(b,stderr);

  printf("\ndel(b,0):\n");
  stray_del(b,0);
  stray_debug(b,stderr);

  printf("\ndel(b,2):\n");
  stray_del(b,2);
  stray_debug(b,stderr);

  printf("\ndel(b,0):\n");
  stray_del(b,0);
  stray_debug(b,stderr);
  printf("\ndel(b,0):\n");
  stray_del(b,0);
  stray_debug(b,stderr);
  printf("\ndel(b,0):\n");
  stray_del(b,0);
  stray_debug(b,stderr);
  printf("\ndel(b,0):\n");
  stray_del(b,0);
  stray_debug(b,stderr);
  printf("\ndel(b,0):\n");
  stray_del(b,0);
  stray_debug(b,stderr);
  printf("\naddstr(b,str):\n");
  stray_addstr_d(b,"the cat sat on the horse");
  stray_debug(b,stderr);

  return 0;
}

#endif

#ifdef STRAY_TOK_TEST

int main(int argc, char **argv) {

  stray_t *a;

  if(argc<3) {
    fprintf(stderr,"usage: tok STING DELIMITERS\n");
    return 1;
  }

  a = stray_tok_q(argv[1],argv[2]);

  stray_debug(a,stdout);

  return 0;

}

#endif
