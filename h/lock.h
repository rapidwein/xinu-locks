
#ifndef _LOCK_H_
#define _LOCK_H_

/* lock table declarations and constants		*/
#ifndef NLOCKS		/* set number of locks 		*/
#define NLOCKS	50	/* if it is not already set	*/
#endif


#define DELETED	2
#define LFREE	3
#define LUSED	4
#define READ	5
#define WRITE	6

#define LOCKSCALE 100

typedef struct {
        int lstate;
        int pid;
        int lprio;
}lock_wait;

struct lentry {
	int		lstate;
	int		pid;
	int		pids[NPROC];
	int 		waitcnt;
	int 		lrcnt;
	int 		lwcnt;
	int 		lckhead;
	int		lcktail;
	int		ldeleted;
	lock_wait	lwait[NPROC];
};

extern struct lentry locktab[];
extern int nextlock;
extern int linit();
extern int lcreate();
extern int lock_release(int, int);
//extern SYSCALL releaseall(int, int);
extern int lock(int, int, int);
extern int ldelete(int);

#define isbadlock(l)	(l < 0 || l >= NLOCKS)
#define isbadtype(t)	(type != READ && type != WRITE)
#endif
