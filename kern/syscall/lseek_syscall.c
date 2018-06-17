#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/syscall.h>
#include <kern/seek.h>
#include <kern/stat.h>
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
sys_lseek(int fd, off_t pos, int whence, int32_t *retval, int32_t *retval2)
{
    int result;
    off_t npos;
    struct stat statbuf;
    struct file_handle *fh;

    *retval = -1;

    if (fd < 0 || fd > NR_OPEN_DEFAULT || (fh = curproc->files->fd_array[fd]) == NULL)
        return EBADF;

    if (!VOP_ISSEEKABLE(fh->file))
        return ESPIPE;

    spinlock_acquire(&fh->fh_lock);
    npos = fh->f_pos;

    switch (whence) {
        case SEEK_SET:
        case SEEK_CUR:
        case SEEK_END:
            break;
        default:
            spinlock_release(&fh->fh_lock);
            return EINVAL;
    }

    if (whence == SEEK_SET) {
        if (pos < 0) {
            spinlock_release(&fh->fh_lock);
            return EINVAL;
        }
        npos = pos;
    }
    else if (whence == SEEK_CUR) {
        if ((pos < 0) && -pos > npos) { /* can't have negative positions */
            spinlock_release(&fh->fh_lock);
            return EINVAL;
        }
        npos += pos;
    }
    else if (whence == SEEK_END) {
        spinlock_release(&fh->fh_lock);
        result = VOP_STAT(fh->file, &statbuf);
        spinlock_acquire(&fh->fh_lock);
        if (result) {
            spinlock_release(&fh->fh_lock);
            return result;
        }
        npos = statbuf.st_size;
        if ((pos < 0) && -pos > npos) { /* can't have negative positions */
            spinlock_release(&fh->fh_lock);
            return EINVAL;
        }
        npos += pos;
    }

    fh->f_pos = npos;
    spinlock_release(&fh->fh_lock);

    /* devide 64bit number into two 32 bit numbers to store in two variables */
    *retval = (int32_t) (npos >> 32);                 /* most significant bits */
    *retval2 = (int32_t) (npos & 0x00000000ffffffff); /* least significant bits */

    return 0;
}