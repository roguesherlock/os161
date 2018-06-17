#ifndef _PROC_TABLE_H_
#define _PROC_TABLE_H_
/*
 * Definition of a process table.
 *
 */

#include <spinlock.h>
#include <proc.h>

#define MAX_ACTIVE_PROC 1000
#define MAX_INACTIVE_PROC 10


/* Process Table */
struct proc_table {
	struct spinlock pt_lock;                	/* for synchronization */
	unsigned pnum;			                	/* number of active processess */
	unsigned next_pid;		                	/* cache for next pid */
    struct proc *p_array[MAX_ACTIVE_PROC];   	/* Process Array */
};

/* create and destroy proc_table. Must be empty at destroy */
void proc_table_create(struct proc_table **pt);
void proc_table_destroy(struct proc_table *pt);

/* check weather proc_table is empty */
bool proc_table_is_empty(void);


int             set_proc(pid_t pid, struct proc *p);
int             get_proc(pid_t pid, struct proc **p);
int             get_pid(pid_t *pid);
struct proc *   rel_pid(pid_t pid);

/* global proc table */
extern struct proc_table *pt;

#endif /* _PROC_TABLE_H_ */