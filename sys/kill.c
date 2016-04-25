/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	
	int i,j,k;
	//kprintf("killing process %d",pid);
	for(i = NLOCKS-1; i > 0; i--){
		pptr->ldeleted[i] = -1;
		//kprintf("lock %d",i);
		struct lentry *lptr = &locktab[i];
		//kprintf("lock id %d pid %d lstate %d",i,lptr->pid,lptr->lstate);
		if(lptr->lstate != LFREE && lptr->lstate != LUSED){
		//kprintf("lock %d is being used",i);
		for(j = 0; j < NPROC; j++){
			//kprintf("lwait pid %d pid %d",lptr->lwait[j].pid,pid);
			if(lptr->pids[j] == pid){
				//int ldes = i*LOCKSCALE + lptr->pid;
				/*int lckcnt = lptr->lrcnt + lptr->lwcnt;
				for(k = j; k < lckcnt-1; k++){
					lptr->pids[k] = lptr->pids[k+1];
					if(k == lckcnt-1) lptr->pids[k+1] = -1;
				}
				lckcnt--;
				if(lptr->lstate == READ) lptr->lrcnt--;
				else lptr->lwcnt--;
				if(lckcnt == 0) lptr->lstate = LUSED;
				*/lock_release(pid, i);
			}
			else if(lptr->lwait[j].pid == pid){
				int item = locktab[NLOCKS-1].lcktail + NLOCKS*i + pid;
				for(k = j; k < lptr->waitcnt-1; k++){
					lptr->lwait[k] = lptr->lwait[k+1];
					if(k == lptr->waitcnt-1) lptr->pids[k+1] = -1;
				}
				lptr->waitcnt--;
				int tmp_item = q[lptr->lcktail].qprev;
				dequeue(item);
				if(lptr->lwait[j].lstate == WRITE && tmp_item == item){
					int next = q[lptr->lcktail].qprev;
					while(next != lptr->lckhead && q[next].qstate == READ){
						int lck_pid = next - locktab[NLOCKS-1].lcktail - NLOCKS*i;
						for(j = 0; j < lptr->waitcnt; j++){
							if(lptr->lwait[j].pid == lck_pid){
								if(lptr->lstate == LUSED || lptr->lstate == READ){
									//kprintf("proc %d acquired lock %d\n",pid,lckid);
									if(lptr->lwait[j].lstate == READ){
										lptr->lstate = READ;
										lptr->pids[lptr->lrcnt++] = lck_pid;
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
									//kprintf("pid : %d",pid);
									ready(pid, RESCHNO);
									proctab[lck_pid].lwaitret = OK;
									break;
								}
							}
						}		
						dequeue(next);
						next = q[lptr->lcktail].qprev;
					}
				}
			}
		}
		}
	}
	switch (pptr->pstate) {

        case PRCURR:    pptr->pstate = PRFREE;  /* suicide */
                        resched();

        case PRWAIT:    semaph[pptr->psem].semcnt++;

        case PRREADY:   dequeue(pid);
                        pptr->pstate = PRFREE;
                        break;
        case PRTRECV:   unsleep(pid);
                                                /* fall through */
        default:        pptr->pstate = PRFREE;
        }
	restore(ps);
	return(OK);
}
