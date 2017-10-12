/*
bosh system stuff (spawning processes etc)

$Id: system.h,v 1.24 2010/02/21 23:27:36 alexsisson Exp $

(C) Copyright 2004-2009 Alex Sisson (alexsisson@gmail.com)

*/

#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#include "bosh.h"

int bosh_popen(bosh_t *b, char *cmd, int flags);
int bosh_open(bosh_t *b);
int bosh_action(bosh_t *b, char *key);

int bosh_read(bosh_t *b);
int bosh_write(bosh_t *b, char c);
void bosh_close(bosh_t *b, int readvarfile);

char *bosh_savebuf(bosh_t *b);


/* syscall wrappers */
int bosh_kill(bosh_t *b);
int bosh_unlink(const char *path);

/* extra fd macros */
#define FD_INVALID                    -1
#define FD_ISVALID(fd)                (fd>=0)
#define FD_ISVALIDANDSET(fd,fdset)    (FD_ISVALID(fd)?FD_ISSET(fd,fdset):0)
#define FD_SETIFVALID(fd,fdset,nfds)  if(FD_ISVALID(fd)) { FD_SET(fd,fdset); if(fd+1>nfds) nfds = fd+1; }
#define FD_CLOSE(fd)                  do {close(fd);fd=-1;} while(0)


#endif
