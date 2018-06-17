/*
 * files_table.c
 * Definitions for files_table functions
 *
 */

#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <spinlock.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <files_table.h>
#include <file_handle.h>

extern int errno;



/*
 *    file_handle_create - create a new file handle pointing to a given file with
 *                        a given file mode and at a given offset.
 *                        return proper errno on error.
 *
 */
int
file_handle_create (struct vnode *file, int f_openflags, off_t f_pos, struct file_handle ** nfh)
{
    KASSERT(file != NULL);

    struct file_handle *fh;

    fh = kmalloc(sizeof(*fh));
    if (fh == NULL)
        return ENOMEM;

    spinlock_init(&fh->fh_lock);

    fh->f_lock = rwlock_create("f_lock");

    fh->refcount = 1;
    fh->file = file;
    VOP_INCREF(file);
    fh->f_openflags = f_openflags & O_ACCMODE; /* Reduce flags to only 3 values R, W, RW */
    fh->f_pos = f_pos;

    *nfh = fh;
    return 0;
}


/*
 * file_handle_create_std_handle - helper function for creating standard file handles
 *                                STDIN, STDOUT, STDERR
 *                                return proper errno on error.
 *
 */
int
file_handle_create_std_handle (unsigned int fd, struct file_handle ** nfh)
{
    char console_str [] = {'c', 'o', 'n', ':', '\0'};
    struct vnode *f;
    int openflags, result;
    mode_t mode;

    switch (fd) {
        case STDIN_FILENO:
            openflags = O_RDONLY;
            mode = 0444; /* read only permission for everyone */
            break;
        case STDOUT_FILENO:
        case STDERR_FILENO:
            openflags = O_WRONLY;
            mode = 0222; /* write only permission for everyone */
            break;
        default:
            return EINVAL;
    }

    result = vfs_open(console_str, openflags, mode, &f);
    if (result)
        return result;

    result = file_handle_create(f, openflags, 0, nfh);
    if (result)
        return result;

    return 0;

}


/*
 * file_handle_has_access - check if process has proper acces to the file for it's operation.
 *
 */
bool
file_handle_has_access (struct file_handle *fh, int flag)
{
    KASSERT (fh != NULL);

	switch (flag) {
	    case READ:
	    case WRITE:
            break;
	    default:
		    return false;
	}

    if (fh->f_openflags == O_RDWR)
        return true;

    return (fh->f_openflags == flag);
}


/*
 * file_handle_destroy - dispose of a file handle.
 *
 */
void
file_handle_destroy (struct file_handle *fh)
{
    // KASSERT(fh != NULL);
    if (fh == NULL)
        return;

    spinlock_acquire(&fh->fh_lock);
    fh->refcount--;
    if (fh->refcount != 0)
        spinlock_release(&fh->fh_lock);
    else {
        vfs_close(fh->file);
        rwlock_destroy(fh->f_lock);
        spinlock_release(&fh->fh_lock);
        spinlock_cleanup(&fh->fh_lock);
        kfree(fh);
    }

}