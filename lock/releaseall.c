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

int lock_release(int lckpid, int lckid)
{
	if(isbadlock(lckid)) return SYSERR;
	struct lentry *lptr = &locktab[lckid];
	int lckcnt = lptr->lrcnt + lptr->lwcnt;
	//kprintf("lckcnt : %d",lckcnt);
	if(lptr->lstate == LFREE) return SYSERR;
	int i;
	int flag = 0;
	for(i = 0; i < lckcnt; i++){
		if(locktab[lckid].pids[i] == lckpid) flag = 1;;
	}
	if(flag == 0) return SYSERR;
	for(i = 0; i < lckcnt; i++){
		if(lptr->pids[i] == lckpid){
			int j;
			for(j = i; j < lckcnt-1; j++){
				lptr->pids[j] = lptr->pids[j+1];
				if(j == lckcnt-1) lptr->pids[j+1] = -1;
			}
			if(lptr->lstate == READ) lptr->lrcnt--;
			else lptr->lwcnt--;
			lckcnt--;
			break;
		}
	}
	if(lckcnt == 0) lptr->lstate = LUSED;
	int pid, item;
	if(q[lptr->lckhead].qnext != lptr->lcktail){
			item = q[lptr->lcktail].qprev;
		//kprintf("dequeueing item %d",item);
		int j,k;
		pid = (item - locktab[NLOCKS-1].lcktail - NLOCKS*lckid);
		//kprintf("pid : %d lstate : %d", pid, lptr->lstate);
		for(j = 0; j < lptr->waitcnt; j++){
			if(lptr->lwait[j].pid == pid){
				if(lptr->lstate == LUSED){
					//kprintf("proc %d acquired lock %d\n",pid,lckid);
					if(lptr->lwait[j].lstate == READ){
						lptr->lstate = READ;
						lptr->pids[lptr->lrcnt++] = pid;
					}
					else{
						lptr->lstate = WRITE;
						lptr->pids[lptr->lwcnt++] = pid;
					}
					for(k = j; k < lptr->waitcnt - 1; k++){
						lptr->lwait[k] = lptr->lwait[k+1];
						if(k == lptr->waitcnt - 1){
							lptr->lwait[k].lstate = LFREE;
							lptr->lwait[k].pid = -1;
							lptr->lwait[k].lprio = -1;
						}
					}
					lptr->waitcnt--;
					dequeue(item);
					//kprintf("pid : %d",pid);
					ready(pid, RESCHNO);
					proctab[pid].lwaitret = OK;
					break;
				}
			}
		}
	}
	return OK;
}
SYSCALL releaseall (int numlocks, int args)
{
	STATWORD ps;
	disable(ps);
	int ldes, ret = OK;
	int *a = (int *)&args + (numlocks-1);
	for(; numlocks > 0; numlocks--) {
		ldes = *a--;
		int lckid = ldes/LOCKSCALE;
		//kprintf("ldes : %d lckid %d pid %d",ldes, lckid, currpid);
		if(lock_release(currpid, lckid) == SYSERR) ret = SYSERR;
	}
	//kprintf("end of resched");
	resched();
	restore(ps);
	return ret;
}
