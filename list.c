/*
Generic linked list

$Id: list.c,v 1.16 2009/06/19 22:58:39 alexsisson Exp $

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "misc.h"



/*
 * list_init
 *
 * initialse list structure
 *
 */
int list_init(struct list *l) {
  /* the root is a pseudo node at the start which will never contain data */
  l->root = bosh_malloc(sizeof(struct node));
  l->root->data = NULL;
  l->root->size = 0;
  l->root->next = bosh_malloc(sizeof(struct node));
  l->root->next->data = NULL;
  l->root->next->size = 0;
  l->root->next->next = NULL;
  l->last = l->root->next;
  l->items = 0;
  l->next = NULL;
  return 0;
}


/*
 * list_free
 *
 * free list structure and any node data we allocated
 *
 */
int list_free(struct list *l) {
  l->iter = l->root;
  while(l->iter) {
    l->root = l->root->next;
    if(l->iter->size)
      bosh_free(l->iter->data);
    bosh_free(l->iter);
    l->iter = l->root;
  }
  return 0;
}

/* free and init the list */
int list_reinit(struct list *l) {
  list_free(l);
  list_init(l);
  return 0;
}

/* add an item */
int list_add(struct list *l, void *data) {
  l->last->data = data;
  l->last->next = bosh_malloc(sizeof(struct node));
  l->last->next->next = NULL;
  l->last->next->data = NULL;
  l->last->next->size = 0;
  l->last = l->last->next;
  l->items++;
  return 0;
}

/* duplicate data and add */
int list_adddup(struct list *l, void *data, int size) {
  l->last->data = bosh_malloc(size);
  memcpy(l->last->data,data,size);
  l->last->size = size;
  l->last->next = bosh_malloc(sizeof(struct node));
  l->last->next->next = NULL;
  l->last->next->data = NULL;
  l->last->next->size = 0;
  l->last = l->last->next;
  l->items++;
  return 0;
}

/*
insert an item after index
NOT WORKING!
*/
int list_ins(struct list *l, int index, void *data)
{
  struct node *temp;
  if(index>=l->items)
    return -1;
  l->iter = l->root;
  while(index) {
    if(!l->iter->next)
      return -1;
    l->iter = l->iter->next;
    index--;
  }
  l->iter->data = data;
  l->iter->size = 0;
  temp = l->iter->next;
  l->iter->next = bosh_malloc(sizeof(struct node));
  l->iter->next->next = temp;
  l->iter->next->data = NULL;
  l->last->next->size = 0;
  l->items++;
  return 0;
}

/*
insert an item after index
*/
int list_insdup(struct list *l, int index, void *data, int size)
{
  struct node *temp;
  if(index>=l->items)
    return -1;
  l->iter = l->root;
  while(index) {
    if(!l->iter->next)
      return -1;
    l->iter = l->iter->next;
    index--;
  }
  temp = l->iter->next;
  l->iter->next = bosh_malloc(sizeof(struct node));
  l->iter->next->next = temp;
  l->iter->next->data = bosh_malloc(size);
  memcpy(l->iter->next->data,data,size);
  l->iter->next->size = size;
  l->items++;
  return 0;
}

/*
delete item
*/
int list_del(struct list *l, int index) {

  struct node *temp;

  if(index>=l->items)
    return -1;

  if(index) {
    index--;
    l->iter = l->root->next;
    while(index-- && l->iter->next->next) {
      l->iter = l->iter->next;

//    fprintf(stderr,"del: index=%d current=%s next=%s\n",index,l->iter?l->iter->data:"NULL",l->iter->next?l->iter->next->data:"NULL");

    }
  }
  else {
    l->iter = l->root;
  }

//fprintf(stderr,"del: index=%d current=%s next=%s\n",index,l->iter?l->iter->data:"NULL",l->iter->next?l->iter->next->data:"NULL");

  if(l->iter->next->size)
    bosh_free(l->iter->next->data);
  temp = l->iter->next;
  l->iter->next = l->iter->next->next;
  bosh_free(temp);
  l->items--;
  return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  list_get*
 *
 *  get item from list
 *
 */
void *list_get(struct list *l, int index) {
  bosh_assert(index>=0);
  l->iter = l->root->next;
  while(index-- && l->iter->next)
    l->iter = l->iter->next;
  return l->iter->data;
}

/* get item and duplicate (caller needs to free)  NOT FINISHED
 *
 */
void *list_getdup(struct list *l, int index) {
//  void *data;
  l->iter = l->root;
  while(--index && l->iter->next->next)
    l->iter = l->iter->next;
  //l->iter->next->data;
  return NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */





/*
return number of items
*/
int list_items(struct list *l) {
  return l->items;
}

/*
was this data duplicated when added?
*/
int list_isdup(struct list *l, int index)
{
  l->iter = l->root;
  while(--index && l->iter->next->next)
    l->iter = l->iter->next;
  return l->iter->next->size>0;
}

/*
start iterating through loop
*/
int list_start(struct list *l, int index)
{
  l->next = l->root->next;
  while(index && l->next->next) {
    l->next = l->next->next;
    index--;
  }
  return 0;
}

/*
get next item
*/
void *list_next(struct list *l)
{
  l->iter = l->next;
  if(l->next->next)
    l->next = l->next->next;
  return l->iter->data;
}

/*
debug
*/
int list_debug(struct list *l)
{
  int i = 0;
  l->iter = l->root->next;
  printf("items: %d\n",l->items);
  for(i=0;i<l->items;i++) {
    printf("%4d:contents=%s\n",i,(char*)l->iter->data);
    l->iter = l->iter->next;
  }
  return 0;
}

#ifdef TEST

#include <ctype.h>

int main()
{
  struct list l;
  void *d;
  int i,n;
  char s[256];

  list_init(&l);

  while(1) {
    fputc('>',stdout);
    fgets(s,sizeof(s),stdin);
    *strchr(s,'\n')=0;
    fprintf(stderr,"%s\n",s);
    if(s[0]=='x')
      break;
    if(isdigit(s[0])) {
      if(s[1])
        list_insdup(&l,s[0]-'0',s+1,strlen(s+1)+1);
      else
        list_del(&l,s[0]-'0');
    }
    else if(s[0]=='R')
      list_reinit(&l);
    else
      list_adddup(&l,s,strlen(s)+1);
    list_debug(&l);
//    for(i=0;i<5;i++)
//      list_adddup(&l,"hello",6);
//    list_reinit(&l);
  }

  return 0;
}

#endif
