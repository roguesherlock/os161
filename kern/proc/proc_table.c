#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <spinlock.h>
#include <synch.h>
#include <limits.h>
#include <proc_table.h>

#define PID_IN(X) (PID_MIN + ((X) % MAX_ACTIVE_PROC))

extern struct proc_table *pt;

/*
 *  proc_table_create - create a new process table.
 *
 */
void
proc_table_create(struct proc_table **pt)
{
    int i;
    struct proc_table *npt;

    npt = kmalloc(sizeof(*npt));
    if (npt == NULL)
        panic("[!] In proc_table_create, Error: %d", ENOMEM);

    spinlock_init(&npt->pt_lock);
    npt->pnum = 0;
    npt->pid_count = 0;
    npt->next_pid = PID_MIN;

    for (i = 0; i < MAX_ACTIVE_PROC; ++i)
        npt->p_array[i] = NULL;
    *pt = npt;
}

/*
 *  proc_table_destroy - destroy process table
 *                      calls proc_destroy on every active process
 */
void
proc_table_destroy(struct proc_table *pt)
{
    KASSERT (pt != NULL);

    int i;

    spinlock_acquire(&pt->pt_lock);
    for (i = PID_MIN; i < MAX_ACTIVE_PROC; ++i)
        if (pt->p_array[i] != NULL)
            proc_destroy(pt->p_array[i]);
    spinlock_release(&pt->pt_lock);

    spinlock_cleanup(&pt->pt_lock);
    kfree(pt);
}


/*
 *  set_proc - map a process to a given pid
 *              return proper error code
 *
 */
int
set_proc(pid_t pid, struct proc *p)
{
    int result;
    // struct proc_table *pt;

    result = 0;
    // pt = &PROC_TABLE;

    KASSERT(pt != NULL);

    if (pid >= PID_MAX)
        return EINVAL;

    if (p == NULL)
        return EFAULT;

    spinlock_acquire(&pt->pt_lock);

    if (pt->p_array[PID_IN(pid)] != NULL)
        result = EINVAL;
    else
        pt->p_array[PID_IN(pid)] = p;
    pt->pnum++;
    spinlock_release(&pt->pt_lock);

    return result;
}


/*
 *  get_proc - get a process for a given pid
 *              return proper error code
 *
 */
int
get_proc(pid_t pid, struct proc *p)
{
    // struct proc_table *pt;
    // struct proc *proc;

    // pt = &PROC_TABLE;

    KASSERT(pt != NULL);

    if (pid >= PID_MAX)
        return EINVAL;

    spinlock_acquire(&pt->pt_lock);
    p = pt->p_array[PID_IN(pid)];
    spinlock_release(&pt->pt_lock);

    if (p == NULL)
        return ESRCH;
    // p = proc;

    return 0;
}


/*
 *  get_pid - get a new valid pid.
 *            on sucess pid contains valid pid.
 *            return proper error code on error
 *
 */
int
get_pid(pid_t *pid)
{
    int result;
    // struct proc_table *pt;
    pid_t npid;

    result = 0;
    // pt = &PROC_TABLE;

    KASSERT(pt != NULL);

    spinlock_acquire(&pt->pt_lock);
    if (pt->pid_count++ >= PID_MAX)
        return ENPROC;
    npid = pt->next_pid++;
    while (pt->p_array[PID_IN(npid)] != NULL)
        npid = pt->next_pid++;
    spinlock_release(&pt->pt_lock);

    *pid = npid;

    return result;
}


/*
 *  rel_pid - release given pid
 *            return process associated with given pid, if any.
 *
 */
struct proc *
rel_pid(pid_t pid)
{
    struct proc *p;
    // struct proc_table *pt;

    // pt = &PROC_TABLE;

    KASSERT(pt != NULL);

    if (pid >= PID_MAX)
        return NULL;

    spinlock_acquire(&pt->pt_lock);
    p = pt->p_array[PID_IN(pid)];
    pt->pnum--;
    spinlock_release(&pt->pt_lock);

    return p;
}

