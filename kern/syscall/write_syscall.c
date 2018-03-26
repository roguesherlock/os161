/*
 * Definition for write syscall
 *
 */

#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
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
sys_write(int fd, const void *buf, size_t buflen, int32_t *wrote)
{
    int result;
    struct iovec wiov;
    struct uio wuio;
    struct file_handle *fh;

    result = 0;
    *wrote = -1;

    if (fd < 0 || fd >= NR_OPEN_DEFAULT || (fh = curproc->files->fd_array[fd]) == NULL) {
        return EBADF;
    }

    if (buf == NULL || buflen < 1) {
        return EFAULT;
    }

    if (!file_handle_has_access(fh, WRITE)) {
        return EBADF;
    }

    rwlock_acquire_write(fh->f_lock);

    wiov.iov_ubase = (userptr_t) buf;
    wiov.iov_len = buflen;
    wuio.uio_iov = &wiov;
    wuio.uio_iovcnt = 1;
    spinlock_acquire(&fh->fh_lock);
    wuio.uio_offset = fh->f_pos;
    spinlock_release(&fh->fh_lock);
    wuio.uio_resid = buflen;
    wuio.uio_segflg = UIO_USERSPACE;
    wuio.uio_rw = UIO_WRITE;
    wuio.uio_space = curproc->p_addrspace;


    result = VOP_WRITE(fh->file, &wuio);
    if (result)
        goto done;

    *wrote = buflen - wuio.uio_resid;

    spinlock_acquire(&fh->fh_lock);
    fh->f_pos = wuio.uio_offset;
    spinlock_release(&fh->fh_lock);

done:
// ENOSPC
// EIO
    rwlock_release_write(fh->f_lock);
    return result;
}