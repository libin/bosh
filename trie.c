/*
generic trie

$Id: trie.c,v 1.5 2009/06/10 11:44:07 alexsisson Exp $

(C) Copyright 2009 Alex Sisson (alexsisson@gmail.com)

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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "trie.h"
#include "misc.h"

trie_node_t *trie_node_new() {
  trie_node_t *r;
  r = bosh_malloc(sizeof(trie_node_t));
  memset(r,0,sizeof(trie_node_t));
  return r;
}

trie_t *trie_new() {
  return trie_node_new();
}

static int trie_map(char c) {
  if(isdigit(c))
    return c - 48;
  if(isupper(c))
    return c - 55;
  if(islower(c))
    return toupper(c) - 55;
  return -1;
}

void trie_add(trie_node_t *t, char *s, void *data) {
  char *p = s;
  int c;

  while(*p) {
    c = trie_map(*p);
    if(c<0)
      return; /* TODO: error handling */
    if(!t->child[c])
      t->child[c] = trie_node_new();
    t = t->child[c];
    p++;
  }
  t->data = data;
}

void *trie_lookup(trie_node_t *t, char *s) {
  int c;
  while(*s) {
    c = trie_map(*s);
    if(c<0)
      return NULL; /* TODO: error handling */
    if(!t->child[c])
      return NULL;
    t = t->child[c];
    s++;
  }
  return t->data;
}
