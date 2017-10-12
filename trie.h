/*
generic trie

$Id: trie.h,v 1.3 2009/06/04 16:13:35 alexsisson Exp $

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


#ifndef CMDTREE_H_INCLUDED
#define CMDTREE_H_INCLUDED

struct struct_trie_node_t {
  struct struct_trie_node_t *child[36];
  void *data;
};

typedef struct struct_trie_node_t trie_node_t;
typedef struct struct_trie_node_t trie_t;


trie_t *trie_new();
void  trie_add   (trie_t *t, char *s, void *data);
void *trie_lookup(trie_t *t, char *s);

#endif
