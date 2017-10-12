/*
Generic linked list

$Id: list.h,v 1.8 2009/03/14 12:54:53 alexsisson Exp $

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

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#include <stdlib.h>
#include <unistd.h>

struct node {
  void *data;
  int size;
  struct node *next;
};

struct list {
  struct node *root,*last,*next,*iter;
  int items;
};

typedef struct list LIST; 

int list_init(struct list *l);
int list_free(struct list *l);
int list_reinit(struct list *l);
int list_add(struct list *l, void *data);
int list_adddup(struct list *l, void *data, int size);
int list_ins(struct list *l, int index, void *data);
int list_insdup(struct list *l, int index, void *data, int size);
int list_del(struct list *l, int index);
void *list_get(struct list *l, int index);
void *list_getdup(struct list *l, int index);
int list_items(struct list *l);
int list_isdup(struct list *l, int index);
int list_start(struct list *l, int index);
void *list_next(struct list *l);
int list_debug(struct list *l);

#endif
