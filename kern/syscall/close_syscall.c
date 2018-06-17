#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/syscall.h>
#include <current.h>
#include <spinlock.h>
#include <synch.h>
#include <kern/iovec.h>
#include <uio.h>
#include <copyinout.h>
#include <syscall.h>
#include <file_handle.h>
#include <files_table.h>
#include <proc.h>
#include <vfs.h>
#include <vnode.h>


int
sys_close(int fd, int32_t *result)
{
    struct file_handle *fh;

    *result = -1;

    if (fd < 0 || fd >= NR_OPEN_DEFAULT || (fh = curproc->files->fd_array[fd]) == NULL)
        return EBADF;

    file_handle_destroy(fh);
    spinlock_acquire(&curproc->p_lock);
    curproc->files->fd_array[fd] = NULL;
    spinlock_release(&curproc->p_lock);
    *result = 0;

    return 0;
}