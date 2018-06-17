/*
 * Definition for open syscall
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
#include <addrspace.h>
#include <vfs.h>
#include <vnode.h>
#include <copyinout.h>


int
sys_open(const char *filename, int flags, int32_t *fd)
{
    struct vnode *file;
    struct file_handle *fh;
    struct files_table *ft;
    char *fname;
    mode_t mode;
    size_t len;
    int result;

    KASSERT (curproc != NULL);

    *fd = -1;
    file = NULL;
    ft = NULL;
    fh = NULL;
    mode = 0644;    /* Note: this is dummy. It's not used whatsoever in this implementation */

    // if (filename == NULL)
    //     return EFAULT;

    if (filename == NULL || (!as_is_addr_valid(curproc->p_addrspace, (vaddr_t) filename)))
        return EFAULT;

    // result = check_userptr((const_userptr_t) filename);
    // if (result)
    //     return result;

    len = strlen(filename) + 1;
    fname = kmalloc(len);
    if (fname == NULL)
        return ENOMEM;

    result = copyinstr((const_userptr_t)filename, fname, len, NULL);
    if (result)
        goto err;

    result = vfs_open(fname, flags, mode, &file);
    if (result)
        goto err;

    ft = curproc->files;

    KASSERT(ft != NULL);

    result = file_handle_create(file, flags, 0, &fh);
    if (result)
        goto err;

    result = files_table_get_next_fd(ft, (int *)fd);
    if (result)
        goto err;

    ft->fd_array[*fd] = fh;

    kfree(fname);
    return 0;

err:
    kfree(fname);
    return result;

}


/*
 * ignoring this for now
 * int
 * sys_open(const char *filename, int flags, mode_t mode);
 *
 */