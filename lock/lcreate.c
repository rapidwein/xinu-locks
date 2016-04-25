#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <stdio.h>
#include <lock.h>

LOCAL int newlock();

int lcreate()
{
	STATWORD ps;
	int	lock;

	disable(ps);
	if((lock=newlock()) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	locktab[lock].pid = currpid;
	int j;
	for(j = 0 ; j < NPROC; j++){
		struct pentry *pptr = &proctab[j];
		if(pptr->pstate != PRFREE && pptr->ldeleted[lock] == -1) pptr->ldeleted[lock] = locktab[lock].ldeleted; 
	}
	restore(ps);
	return(lock*LOCKSCALE+currpid);
}

LOCAL int newlock()
{
	int 	lock;
	int 	i;

	for(i = 0; i < NLOCKS; i++) {
		lock = nextlock--;
		if(nextlock < 0)
			nextlock = NLOCKS - 1;
		if(locktab[lock].lstate == LFREE) {
			locktab[lock].lstate = LUSED;
			return(lock);
		}
	}
	return SYSERR;
}
