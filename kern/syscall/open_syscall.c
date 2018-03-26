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

    *fd = -1;
    file = NULL;
    ft = NULL;
    fh = NULL;
    mode = 0644;    /* Note: this is dummy. It's not used whatsoever in this implementation */

    if (filename == NULL)
        return EFAULT;

    result = check_userptr((const_userptr_t) filename);
    if (result)
        return result;

    len = strlen(filename) + 1;
    fname = kmalloc(len);
    if (fname == NULL)
        return ENOMEM;

    result = copyinstr((const_userptr_t)filename, fname, len, NULL);
    if (result)
        return result;

    result = vfs_open(fname, flags, mode, &file);
    if (result)
        return result;

    ft = curproc->files;

    KASSERT(ft != NULL);

    result = file_handle_create(file, flags, 0, &fh);
    if (result)
        return result;

    result = files_table_get_next_fd(ft, (int *)fd);
    if (result)
        return result;

    ft->fd_array[*fd] = fh;

    return 0;

}


/*
 * ignoring this for now
 * int
 * sys_open(const char *filename, int flags, mode_t mode);
 *
 */