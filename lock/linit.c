#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <stdio.h>
#include <lock.h>

int linit()
{
	int i, j;
	for(i = 0; i < NLOCKS; i++)
	{
		struct lentry *lptr = &locktab[i];
		lptr->lstate = LFREE;
		for(j = 0; j < NPROC; j++)
		{
			lptr->pids[j] = -1;
			lptr->lwait[j].lstate = LFREE;
			lptr->lwait[j].lprio = -1;
			lptr->lwait[j].pid = -1;
		}
		lptr->waitcnt = 0;
		lptr->lrcnt = 0;
		lptr->lwcnt = 0;
		lptr->pid = -1;
		lptr->lcktail = 1 + (lptr->lckhead=newqueue());/* initialize ready list */
		lptr->ldeleted = 0;
	}
	return OK;
}
