/*
stray (STRing arrAY)

ugly set of reusable routines for manipulating argc/argv style arrays
of strings. probably v. buggy and not particularly fast. also memleaks!

$Id: stray.h,v 1.6 2009/06/06 14:44:49 alexsisson Exp $

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

#ifndef STRAY_H_INCLUDED
#define STRAY_H_INCLUDED


#include <stdio.h>
#include <stdint.h>

#define STRAY_TYPE char*
#define STRAY_EMPTY 0,0

typedef struct {
  int c;
  char **v;
  int shift;
} stray_t;

stray_t *stray_new(int c, char **v);
stray_t *stray_new_d(int c, char **v);

stray_t *stray_dup(stray_t *a);

void     stray_debug(stray_t *a, FILE *stream);

void     stray_addstr(stray_t *a, char *s);
void     stray_addstr_d(stray_t *a, const char *s);

void     stray_addarr(stray_t *a, stray_t *b);
void     stray_addarr_d(stray_t *a, stray_t *b);

void     stray_clr(stray_t *a);
void     stray_free(stray_t *a);

STRAY_TYPE stray_get(stray_t *a, uint32_t ind);
void       stray_set(stray_t *a, uint32_t ind, STRAY_TYPE s);
void       stray_set_d(stray_t *a, uint32_t ind, STRAY_TYPE s);


void     stray_del(stray_t *a, uint32_t ind);

void     stray_shift(stray_t *a);

stray_t *stray_tok(char *s, char *delim);

#endif
