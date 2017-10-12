/*
internal cmd trie

$Id: cmd.h,v 1.7 2010/02/19 00:42:30 alexsisson Exp $

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

#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#include "bosh.h"
#include "stray.h"
#include "misc.h"
#include "system.h"
#include "ui.h"
#ifdef LOG
#include "trie.h"
extern trie_t *cmdtrie[BOSH_MODE_MAX];
#endif

int   cmd_trie_init();
void *cmd_trie_lookup(int mode, char *cmd);

int   cmd_run  (int mode, char *cmd, void *opts, int flags);
void  cmd_error(int mode, char *name, char *format, ...);

#define BOSH_CMDTRIE_NOTFOUND -1

/* cmd_run flags */
#define CR_FULL         1
#define CR_TOK          2
#define CR_STRAY        3

/* cmd handler generic return values */
#define CMD_SUCCESS                 0
#define CMD_NOTFOUND               -1
#define CMD_ERR_HANDLED            -2
#define CMD_ERR_INSUFFICIENT_ARGS  -3
#define CMD_ERR_INVALID_ARG        -4
#define CMD_ERR_INCORRECT_CONTEXT  -5

/* cmd handler macros */
#define CMD_HANDLER(c)             int cmd_handler_ ## c (stray_t *a, int mode)
#define CMD_ARGC                   (a->c)
#define CMD_ARGV(ind)              (stray_get(a,(ind)))
#define CMD_ARGC_CHECK(n)          if(a->c<(n)) { return CMD_ERR_INSUFFICIENT_ARGS; }
#define CMD_ERR(exp,...)           if(exp) { cmd_error(mode,(char*)(__func__+12),__VA_ARGS__); return CMD_ERR_HANDLED; }
#define CMD_LOG()                  bosh_log("cmdhandler","[%s]\n",__func__+12); bosh_log_stray(a);
#define CMD_MODE_SWITCH            switch(mode)
#define CMD_CONTEXT_CHECK(exp)     if(!(exp)) { return CMD_ERR_INCORRECT_CONTEXT; }

#endif
