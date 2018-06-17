#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <spinlock.h>
#include <synch.h>
#include <current.h>
#include <limits.h>
#include <proc_table.h>

#define PID_IN(X) ((X) < MAX_ACTIVE_PROC ? (X) : PID_MIN + ((X) % (MAX_ACTIVE_PROC - PID_MIN)))

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
    npt->next_pid = PID_MIN;

    for (i = 0; i < MAX_ACTIVE_PROC; ++i)
        npt->p_array[i] = NULL;

    *pt = npt;
}

/*
 *  proc_table_destroy - destroy process table
 *                      calls proc_destroy on every active process
 *                      except kproc
 */
void
proc_table_destroy(struct proc_table *pt)
{
    KASSERT(pt != NULL);
    /* only kproc is allowed to call */
    KASSERT(curthread->t_proc == kproc);

    int i;

    if (pt->pnum != 0)
        for (i = PID_MIN; i < MAX_ACTIVE_PROC; ++i)
            if (pt->p_array[i] != NULL)
                proc_destroy(pt->p_array[i]);

    spinlock_cleanup(&pt->pt_lock);
    kfree(pt);
}


/*
 *  proc_table_is_empty - checks weather proc_table is empty or not
 *
 */
bool
proc_table_is_empty()
{
    bool is_empty;

    KASSERT(pt != NULL);

    spinlock_acquire(&pt->pt_lock);
    is_empty = (pt->pnum == 0);
    spinlock_release(&pt->pt_lock);

    return is_empty;
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

    result = 0;

    KASSERT(pt != NULL);

    if (pid < PID_MIN || pid >= PID_MAX)
        return EINVAL;

    if (p == NULL)
        return EFAULT;

    spinlock_acquire(&pt->pt_lock);
    if (pt->p_array[PID_IN(pid)] != NULL) {
        result = EINVAL;
        goto done;
    }

    pt->p_array[PID_IN(pid)] = p;
    pt->pnum++;

done:
    spinlock_release(&pt->pt_lock);
    return result;
}


/*
 *  get_proc - get a process for a given pid
 *              return proper error code
 *
 */
int
get_proc(pid_t pid, struct proc **p)
{
    KASSERT(pt != NULL);

    struct proc *np;

    if (pid < PID_MIN || pid >= PID_MAX)
        return ESRCH;

    spinlock_acquire(&pt->pt_lock);
    np = pt->p_array[PID_IN(pid)];
    spinlock_release(&pt->pt_lock);

    if (np == NULL)
        return ESRCH;

    *p = np;

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
    pid_t npid;

    result = 0;

    KASSERT(pt != NULL);

    spinlock_acquire(&pt->pt_lock);
    /* is process table full */
    if (pt->pnum >= (MAX_ACTIVE_PROC - PID_MIN)) {
        result = ENPROC;
        goto done;
    }
    /* wrap around */
    if ((pt->next_pid) >= PID_MAX)
        pt->next_pid = PID_MIN;

    npid = pt->next_pid++;
    while (pt->p_array[PID_IN(npid)] != NULL)
        npid = pt->next_pid++;

    *pid = npid;

done:
    spinlock_release(&pt->pt_lock);
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

    KASSERT(pt != NULL);

    if (pid < PID_MIN || pid >= PID_MAX)
        return NULL;

    spinlock_acquire(&pt->pt_lock);
    p = pt->p_array[PID_IN(pid)];
    pt->p_array[PID_IN(pid)] = NULL;
    /* if there was process to begin with, decrese active process counter */
    if (p)
        pt->pnum--;
    spinlock_release(&pt->pt_lock);

    return p;
}

