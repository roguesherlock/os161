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



/*
 *    files_table_create - create a new empty files table. May
 *                      return proper errno on error.
 *
 */
int
files_table_create (struct files_table **nft)
{
    struct files_table *ft;
    int i;

    ft = kmalloc(sizeof(*ft));

    if (ft == NULL)
        return ENOMEM;

    spinlock_init(&ft->ft_lock);

    for (i = 0; i < NR_OPEN_DEFAULT; ++i)
        ft->fd_array[i] = NULL;

    ft->next_fd = 3;
    *nft = ft;

    return 0;

}


/*
 *    files_table_assign_default_handles - helper function to create standard file handles.
 *                                       STDIN, STDOUT, STDERR
 *
 */
int
files_table_assign_default_handles (struct files_table *ft)
{
    KASSERT(ft != NULL);

    int result;

    /* create default file handles. STDIN, STDOUT, STDERR */
    result = file_handle_create_std_handle(STDIN_FILENO, &ft->fd_array[STDIN_FILENO]);
    if (result)
        return result;

    result = file_handle_create_std_handle(STDOUT_FILENO, &ft->fd_array[STDOUT_FILENO]);
    if (result)
        return result;

    result = file_handle_create_std_handle(STDERR_FILENO, &ft->fd_array[STDERR_FILENO]);
    if (result)
        return result;

    return 0;
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

    int result, i;
    struct files_table *ft;

	result = files_table_create(&ft);
	if (result)
		return result;

    for (i = 0; i < NR_OPEN_DEFAULT; ++i)
        if (src->fd_array[i] != NULL) {
            ft->fd_array[i] = src->fd_array[i];

            spinlock_acquire(&ft->fd_array[i]->fh_lock);    /* update refcount for file handle */
            ft->fd_array[i]->refcount++;
            spinlock_release(&ft->fd_array[i]->fh_lock);
        }

    ft->next_fd = src->next_fd;

	*ret = ft;
	return 0;
}

/*
 *    files_table_get_next_fd - function to get next available file descriptor.
 *                          return proper errno on error.
 */
int
files_table_get_next_fd (struct files_table *ft, int *fd)
{
    int i;
    bool found_fd;

    found_fd = false;

    spinlock_acquire(&ft->ft_lock);
    if (ft->next_fd < NR_OPEN_DEFAULT) {
        *fd = ft->next_fd++;
        spinlock_release(&ft->ft_lock);
        return 0;
    }

    for (i = 3; i < NR_OPEN_DEFAULT; ++i) {
        if (ft->fd_array[i] == NULL) {
            if (!found_fd) {
                *fd = i;
                continue;
            } else {
                ft->next_fd = i;
                break;
            }
        }
    }
    spinlock_release(&ft->ft_lock);

    if (!found_fd)
        return EMFILE;

    return 0;
}


/*
 * files_table_destroy - dispose of a files table.
 *
 */
void
files_table_destroy (struct files_table *ft)
{
    if (ft == NULL)
        return;

    int i;

    for (i = 0; i < NR_OPEN_DEFAULT; ++i)
        file_handle_destroy(ft->fd_array[i]);

    spinlock_cleanup(&ft->ft_lock);

    kfree(ft);

}