#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/syscall.h>
#include <current.h>
#include <vfs.h>
#include <copyinout.h>
#include <proc.h>
#include <spinlock.h>
#include <syscall.h>
#include <addrspace.h>
#include <file_handle.h>
#include <files_table.h>


int
sys_chdir(const char *pathname, int32_t *retval) {

    char *path;
    int result;
    size_t len;

    KASSERT (curproc != NULL);

    *retval = -1;

    // if (pathname == NULL)
    //     return EFAULT;

    if (pathname == NULL || (!as_is_addr_valid(curproc->p_addrspace, (vaddr_t) pathname)))
        return EFAULT;
    // result = check_userptr((const_userptr_t) pathname);
    // if (result)
    //     return result;

    len = strlen(pathname) + 1;

    path = kmalloc(len);
    if (path == NULL)
        return ENOMEM;

    result = copyinstr((const_userptr_t)pathname, path, len, NULL);
    if (result)
        goto err;

    result = vfs_chdir(path);
    if (result)
        goto err;

    *retval = 0;
    return 0;

err:
    kfree(path);
    return result;

}