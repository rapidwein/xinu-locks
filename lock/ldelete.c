#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <stdio.h>
#include <q.h>
#include <lock.h>

int ldelete(int lockdescriptor)
{
	STATWORD ps;
	struct lentry *lptr;
	struct pentry *pptr;
        int lckid = lockdescriptor/LOCKSCALE;
        if(isbadlock(lckid)) return SYSERR;
	disable(ps);
	lptr = &locktab[lckid];
	if(lptr->lrcnt + lptr->lwcnt != 0){
		//kprintf("lock in use");
		/*restore(ps);
		return SYSERR;*/
		int j;
		int lckcnt = lptr->lrcnt + lptr->lwcnt;
		for(j = 0; j < lckcnt; j++){
			lptr->pids[j] = -1;
		}
		lptr->lrcnt = 0;
		lptr->lwcnt = 0;
	}
	int i;
	for(i = 0; i < lptr->waitcnt; i++){
		//kprintf("deleting");
		int pid = lptr->lwait[i].pid;
		//kprintf(" pid %d from waitq\n",pid);
		int item = locktab[NLOCKS-1].lcktail + NLOCKS*lckid + pid;
		//kprintf("removing item %d from queue for lock %d ",item,lckid);
		pptr = &proctab[pid];
		pptr->lwaitret = DELETED;
		lptr->lwait[i].pid = -1;
		lptr->lwait[i].lstate = LFREE;
		lptr->lwait[i].lprio = -1;
		dequeue(item);
		ready(pid,RESCHNO);
	}
	lptr->lstate = LFREE;
	lptr->pid = -1;
	lptr->waitcnt = 0;
	lptr->ldeleted++;
	//kprintf("end of delete %d",lptr->ldeleted);
	resched();
	restore(ps);
	return OK;
}
