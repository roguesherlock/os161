#ifndef _FILE_HANDLE_H_
#define _FILE_HANDLE_H_

/*
 * Definition of a File Handle.
 *
 */

#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <spinlock.h>
#include <synch.h>
#include <vnode.h>



#define READ O_RDONLY
#define WRITE O_WRONLY


struct file_handle {
    volatile unsigned int refcount;     /* reference counter */
    struct spinlock fh_lock;            /* file handle lock */
    struct vnode *file;                 /* file */
    int f_openflags;                    /* file opened as */
    off_t f_pos;                        /* file offset */
    struct rwlock *f_lock;              /* file lock */
};


/*
 * Functions in file_handle.c:
 *
 *    file_handle_create            - create a new file handle pointing to a given file with
 *                                  a given file mode and at a given offset.
 *                                  return proper errno on error.
 *
 *    file_handle_create_std_handle - helper function for creating standard file handles.
 *                                  Namely, STDIN, STDOUT and STDERR.
 *                                  return proper errno on error.
 *
 *    file_handle_has_access        - check if process has proper acces to the file for it's operation
 *
 *
 *    file_handle_destroy           - dispose of a file handle.
 *
 *
 */
int  file_handle_create(struct vnode *file, int f_openflags, off_t f_pos, struct file_handle **fh);
int  file_handle_create_std_handle(unsigned int fd, struct file_handle **fh);
bool file_handle_has_access(struct file_handle *fh, int flag);
void file_handle_destroy(struct file_handle *);




#endif /* _FILE_HANDLE_H_ */