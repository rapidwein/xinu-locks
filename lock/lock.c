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

int lock_insert(int proc, int head, int key, int state)
{
        int     next;                   /* runs through list            */
        int     prev;
        
	next = q[head].qnext;
        while (q[next].qkey < key)      /* tail has maxint as key       */
                next = q[next].qnext;
	if(q[next].qkey == key && state == WRITE){
		while(q[next].qstate == READ)
			next = q[next].qnext;
	}
        q[proc].qnext = next;
        q[proc].qprev = prev = q[next].qprev;
        q[proc].qkey  = key;
	q[proc].qstate = state;
        q[prev].qnext = proc;
        q[next].qprev = proc;
        return(OK);
}

int lock(int ldes1, int type, int priority)
{
	STATWORD ps;
	int lpid = ldes1%LOCKSCALE;
	int lckid = ldes1/LOCKSCALE;
	//kprintf("lpid : %d lckid : %d\n",lpid, lckid);
	if(locktab[lckid].pid != lpid) return SYSERR;
	if(isbadlock(lckid) || isbadtype(type)) return SYSERR;
	disable(ps);
	struct lentry *lptr = &locktab[lckid];
	if(lptr->lstate != LUSED && lptr->pid == -1) {
		restore(ps);
		return SYSERR;
	}
	if(lptr->lstate == LUSED) {
		lptr->lstate = type;
		int lckcnt = lptr->lrcnt + lptr->lwcnt;
		lptr->pids[lckcnt] = currpid;
		if(type == READ) lptr->lrcnt++;
		else lptr->lwcnt++;
		if(proctab[currpid].ldeleted[lckid] == -1) proctab[currpid].ldeleted[lckid] = lptr->ldeleted;
		//kprintf("acquired free lock %d lckcnt %d where ldeleted : %d vs %d",lckid,lckcnt,proctab[currpid].ldeleted[lckid],lptr->ldeleted);
		restore(ps);
		proctab[currpid].lwaitret = OK;
		if(lptr->ldeleted != proctab[currpid].ldeleted[lckid] && currpid != 49){ 
			return SYSERR;
		}
		return OK;
	}
	if(type == READ && lptr->lstate == READ) {
		int next = q[lptr->lckhead].qnext, index = 0;
		while(next != lptr->lcktail && q[next].qkey < priority){
			next = q[next].qnext;
			index++;
		}
		if(next != lptr->lcktail){
			if(lptr->lwait[index].lstate == WRITE){
				int item = locktab[NLOCKS-1].lcktail + NLOCKS*lckid+currpid;
				lock_insert(item, lptr->lckhead, priority, type);
				//kprintf("inserting at %d pid %d",item,currpid);
				lptr->lwait[lptr->waitcnt].pid = currpid;
				lptr->lwait[lptr->waitcnt].lstate = type;
				lptr->lwait[lptr->waitcnt].lprio = priority;
		                lptr->waitcnt++;
				struct pentry *pptr = &proctab[currpid];
				pptr->pstate = PRLCKWT;
				//pptr->ldeleted = lptr->ldeleted;
				//kprintf("ldeleted for pid %d : %d",currpid,pptr->ldeleted);
				//kprintf("put process %d into wait q for lock %d",currpid,lckid);
				resched();
				//if(lptr->ldeleted != pptr->ldeleted && lptr->lstate != LFREE) pptr->lwaitret = SYSERR;
				restore(ps);
				return pptr->lwaitret;
			}
			else{
				lptr->pids[lptr->lrcnt+lptr->lwcnt] = currpid;
				lptr->lrcnt++;
			}
			proctab[currpid].lwaitret = OK;
			if(proctab[currpid].ldeleted[lckid] == -1) proctab[currpid].ldeleted[lckid] = lptr->ldeleted;
			//kprintf("acquired free lock %d where ldeleted : %d vs %d",lckid,proctab[currpid].ldeleted[lckid],lptr->ldeleted);
			restore(ps);
			if(lptr->ldeleted != proctab[currpid].ldeleted[lckid] && currpid != 49){ 
				return SYSERR;
			}
			return OK;
		}
		else{
			lptr->pids[lptr->lrcnt+lptr->lwcnt] = currpid;
				//kprintf("inserting at %d pid %d",lptr->waitcnt,currpid);
			if(proctab[currpid].ldeleted[lckid] == -1) proctab[currpid].ldeleted[lckid] = lptr->ldeleted;
		//kprintf("acquired free lock %d where ldeleted : %d vs %d",lckid,proctab[currpid].ldeleted[lckid],lptr->ldeleted);
			proctab[currpid].lwaitret = OK;
			lptr->lrcnt++;
			restore(ps);
			if(lptr->ldeleted != proctab[currpid].ldeleted[lckid] && currpid != 49){ 
				return SYSERR;
			}
                        return OK;
		}
	}
	else {
		int lckcnt = lptr->lrcnt + lptr->lwcnt;
		int item = locktab[NLOCKS-1].lcktail + NLOCKS*lckid+currpid;
		lock_insert(item, lptr->lckhead, priority, type);
		lptr->lwait[lptr->waitcnt].pid = currpid;
				//kprintf("inserting at %d pid %d",item,currpid);
		lptr->lwait[lptr->waitcnt].lstate = type;
		lptr->lwait[lptr->waitcnt].lprio = priority;
		lptr->waitcnt++;
		struct pentry *pptr = &proctab[currpid];
                pptr->pstate = PRLCKWT;
		//pptr->ldeleted = lptr->ldeleted;
				//kprintf("\nldeleted for pid %d : %d",currpid,pptr->ldeleted);
		//kprintf("put process %d into wait q for write lock %d",currpid, lckid);
		resched();
		//if(lptr->ldeleted != pptr->ldeleted && lptr->lstate != LFREE) pptr->lwaitret = SYSERR;
		restore(ps);
		//kprintf("comparing ldeleted : %d :: %d",lptr->ldeleted,pptr->ldeleted);
		//kprintf("DELETED : %d waitret : %d",DELETED, pptr->lwaitret);
		return pptr->lwaitret;
	}
	restore(ps);
	return SYSERR;
}
