/*
Config file related routines.

$Id: rc.h,v 1.17 2009/11/07 00:07:40 alexsisson Exp $

(C) Copyright 2002-2009 Alex Sisson (alexsisson@gmail.com)

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


#ifndef RC_H_INCLUDED
#define RC_H_INCLUDED

#include "bosh.h"
#include "stray.h"

int bosh_parse_args(stray_t *args);
int bosh_rc_read( char *path);
/* int bosh_rc_write(char *path); */
int bosh_rc_line();

#endif
