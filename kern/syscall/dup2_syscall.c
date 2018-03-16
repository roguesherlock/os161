#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/syscall.h>
#include <current.h>
#include <proc.h>
#include <spinlock.h>
#include <syscall.h>
#include <file_handle.h>
#include <files_table.h>


int
sys_dup2(int oldfd, int newfd, int32_t *fd)
{
    int result;
    struct file_handle *oldfh, *newfh;

    *fd = -1;

    if (oldfd < 0 || oldfd >= NR_OPEN_DEFAULT || (oldfh = curproc->files->fd_array[oldfd]) == NULL)
        return EBADF;

    if (newfd < 0 || newfd >= NR_OPEN_DEFAULT)
        return EBADF;

    if (oldfd == newfd) {
        *fd = newfd;
        return 0;
    }


    if ((newfh = curproc->files->fd_array[newfd]) != NULL) {
        file_handle_destroy(newfh);
        curproc->files->fd_array[newfd] = NULL;
    }

    spinlock_acquire(&oldfh->fh_lock);
    result = file_handle_create(oldfh->file, oldfh->f_openflags,
                oldfh->f_pos, &newfh);
    if (result) {
        spinlock_release(&oldfh->fh_lock);
        return result;
    }
    spinlock_acquire(&curproc->p_lock);
    curproc->files->fd_array[newfd] = newfh;
    spinlock_release(&curproc->p_lock);
    spinlock_release(&oldfh->fh_lock);

    *fd = newfd;

    return 0;
}