/*
 * Definition for read syscall
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
sys_read(int fd, void *buf, size_t buflen, int32_t *read)
{
    int result;
    struct iovec riov;
    struct uio ruio;
    struct file_handle *fh;

    *read = -1;

    if (fd < 0 || fd >= NR_OPEN_DEFAULT || (fh = curproc->files->fd_array[fd]) == NULL) {
        return EBADF;
    }

    if (buf == NULL || buflen < 1) {
        return EFAULT;
    }

    if (!file_handle_has_access(fh, READ)) {
        return EBADF;
    }

    riov.iov_ubase = (userptr_t) buf;
    riov.iov_len = buflen;
    ruio.uio_iov = &riov;
    ruio.uio_iovcnt = 1;
    ruio.uio_offset = fh->f_pos;
    ruio.uio_resid = buflen;
    ruio.uio_segflg = UIO_USERSPACE;
    ruio.uio_rw = UIO_READ;
    ruio.uio_space = curproc->p_addrspace;

    rwlock_acquire_read(fh->f_lock);

    result = VOP_READ(fh->file, &ruio);
    if (result) {
        rwlock_release_read(fh->f_lock);
        return result;
    }

    *read = buflen - ruio.uio_resid;

    spinlock_acquire(&fh->fh_lock);
    fh->f_pos = ruio.uio_offset;
    spinlock_release(&fh->fh_lock);

    rwlock_release_read(fh->f_lock);

// ENOSPC
// EIO
    return 0;
}