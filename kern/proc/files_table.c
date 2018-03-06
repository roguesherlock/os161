/*
 * files_table.c
 * Definitions for files_table functions
 *
 */

#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <spinlock.h>
#include <file_handle.h>
#include <files_table.h>

extern int errno;



/*
 *    files_table_create - create a new empty files table. May
 *                      return NULL on out-of-memory error.
 *                      Will set proper errno on error
 *
 */
struct files_table *
files_table_create (void)
{
    struct files_table *ft;

    ft = kmalloc(sizeof(*ft));

    if (ft == NULL) {
        kprintf("[!] ERROR: [%d], files_table_create: out of memmory\n", ENOMEM);
        // errno = ENOMEM;
        return NULL;
    }

    spinlock_init(&ft->ft_lock);

    ft->next_fd = 3;

    return ft;

}


/*
 *    files_table_assign_default_handles - helper function to create standard file handles.
 *                                       STDIN, STDOUT, STDERR
 *
 */
void
files_table_assign_default_handles (struct files_table *ft)
{
    KASSERT(ft != NULL);

    /* create default file handles. STDIN, STDOUT, STDERR */
    ft->fd_array[STDIN_FILENO] = file_handle_create_std_handle(STDIN_FILENO);
    KASSERT(ft->fd_array[STDIN_FILENO] != NULL);


    ft->fd_array[STDOUT_FILENO] = file_handle_create_std_handle(STDOUT_FILENO);
    KASSERT(ft->fd_array[STDOUT_FILENO] != NULL);


    ft->fd_array[STDERR_FILENO] = file_handle_create_std_handle(STDERR_FILENO);
    KASSERT(ft->fd_array[STDERR_FILENO] != NULL);
}


/*
 *    files_table_copy - create a new files table that is an exact copy of
 *                      an old one.
 *
 */
int
files_table_copy (struct files_table *src, struct files_table **ret)
{
	KASSERT(src != NULL);

    int i;
    struct files_table *ft;

	ft = files_table_create();
	if (ft == NULL) {
        kprintf("[!] ERROR: [%d], files_table_copy: out of memmory\n", ENOMEM);
        // errno = ENOMEM;
		return -1;
	}

    spinlock_acquire(&ft->ft_lock);
    for (i = 0; i < NR_OPEN_DEFAULT; ++i)
        if (src->fd_array[i] != NULL) {
            ft->fd_array[i] = src->fd_array[i];

            spinlock_acquire(&ft->fd_array[i]->fh_lock);    /* update refcount for file handle */
            ft->fd_array[i]->refcount++;
            spinlock_release(&ft->fd_array[i]->fh_lock);
        }

    ft->next_fd = src->next_fd;
    spinlock_release(&ft->ft_lock);

	*ret = ft;
	return 0;
}

/*
 *    files_table_get_next_fd - function to get next available file descriptor.
 *                          return -1 if no file descriptor is available.
 *                          Will set proper errno on error
 */
int
files_table_get_next_fd (struct files_table *ft)
{
    int fd, i;
    bool found_fd;

    found_fd = false;

    spinlock_acquire(&ft->ft_lock);
    if (ft->next_fd < NR_OPEN_DEFAULT) {
        fd = ft->next_fd++;
        spinlock_release(&ft->ft_lock);
        return fd;
    }

    for (i = 3; i < NR_OPEN_DEFAULT; ++i) {
        if (ft->fd_array[i] == NULL) { /* TODO: CHECK IF FD_ARRAY IS INITIALIZED TO NULL BY DEFAULT */
            if (!found_fd) {
                fd = i;
                continue;
            } else {
                ft->next_fd = i;
                break;
            }
        }
    }
    spinlock_release(&ft->ft_lock);

    if (found_fd) {
        return fd;
    }

    // errno = EMFILE;
    return -1;
}


/*
 * files_table_destroy - dispose of a files table.
 *
 */
void
files_table_destroy (struct files_table *ft)
{
    KASSERT(ft != NULL);

    int i;

    for (i = 0; i < NR_OPEN_DEFAULT; ++i)
        file_handle_destroy(ft->fd_array[i]);

    spinlock_cleanup(&ft->ft_lock);

    kfree(ft);

}