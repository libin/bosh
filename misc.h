/*
Misc

$Id: misc.h,v 1.20 2009/11/07 00:07:40 alexsisson Exp $

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


#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <regex.h>

#include "stray.h"
#include "trie.h"
#include "bosh.h"

#define strequ(s1,s2) (!strcmp(s1,s2))

#define lof(a,b) (a<b?a:b) /* lower of */
#define hof(a,b) (a>b?a:b) /* higher of */
#define errstr   (strerror(errno))

char *strdup2(const char *s1, const char *s2);
char *strmak(size_t len, int c);
char *strswp(char *s, int c, int replace);
char *strtrnc(char *s, size_t n);
char *substr(char *s, size_t so, size_t eo);

long int bosh_atoi(const char *nptr);
void bosh_fprintfl(FILE *stream, char *logprefix, const char *format, ...);

void bosh_fatal_err     (int code, char *format, ...);
void bosh_fatal_err_conf(int code, char *format, ...);

#ifdef DEBUG
#define bosh_assert(e) if(!(e)) { bosh_fatal_err(1,"assert (%s) failed in function %s()\n",#e,__func__); }
#else
#define bosh_assert(e)
#endif

int bosh_regex_init(regex_t **r, char *s);
int bosh_regex_try(regex_t **r, char *s);

#ifdef LOG
void  bosh_log_open(const char *mode);
void  bosh_log(char *prefix, char *format, ...);
void  bosh_log_stray(stray_t *a);
void  bosh_log_trie(trie_t *t);
char *bosh_log_printify(char *s);
#else
#define bosh_log_open(mode)
#define bosh_log(prefix,format,...)
#define bosh_log_stray(a)
#define bosh_log_trie(t)
#define bosh_log_printify(s)
#endif

#ifdef LOGMEM
void *bosh_malloc(size_t size);
void *bosh_realloc(void* ptr, size_t size);
char *bosh_strdup(const char *s);
void bosh_free(void *ptr);
#else
#define bosh_malloc(size)      malloc(size)
#define bosh_realloc(ptr,size) realloc((ptr),(size))
#define bosh_strdup(s)         strdup(s)
#define bosh_free(ptr)         free(ptr)
#endif

#endif
