#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/syscall.h>
#include <current.h>
#include <vfs.h>
#include <copyinout.h>
#include <proc.h>
#include <uio.h>
#include <spinlock.h>
#include <syscall.h>
#include <file_handle.h>
#include <files_table.h>


int
sys___getcwd(char *buf, size_t buflen, int32_t *retval)
{
    struct iovec bufdata;
    struct uio ubuf;
    int result;

    *retval = -1;

    if (buf == NULL)
        return EFAULT;

    bufdata.iov_ubase = (userptr_t) buf;
    bufdata.iov_len = buflen;
    ubuf.uio_iov = &bufdata;
    ubuf.uio_iovcnt = 1;
    ubuf.uio_offset = 0;
    ubuf.uio_resid = buflen;
    ubuf.uio_segflg = UIO_USERSPACE;
    ubuf.uio_rw = UIO_READ;
    ubuf.uio_space = curproc->p_addrspace;


    result = vfs_getcwd(&ubuf);
    if (result)
        return result;

    *retval = buflen - ubuf.uio_offset;
    return 0;
}