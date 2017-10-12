/*
some bosh interface stuff

$Id: ui.h,v 1.11 2010/02/19 00:42:30 alexsisson Exp $

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

#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include "objects.h"

extern char *REPLY;
extern char *PROMPT;

void ui_init();
void ui_finish();

void ui_key_init();
int *ui_run(int mode, int needredraw);
void ui_redraw(int mode);
void ui_refresh();

int readstr(char *prompt);
int readstrp(char *prompt, char *previous);

void ui_cursorset(bosh_t *b, int n);
int  ui_cursorup(bosh_t *b, int n);
int  ui_cursordown(bosh_t *b, int n);
int  ui_search(bosh_t *b);
void ui_message(char *s);

int *keyhandler_normal (bosh_t *b, int key);
int *keyhandler_readstr(bosh_t *b, int key);
int *keyhandler_thru   (bosh_t *b, int key);

#endif
