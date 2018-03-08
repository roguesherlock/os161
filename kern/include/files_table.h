#ifndef _FILES_TABLE_H_
#define _FILES_TABLE_H_

/*
 * Definition of a Files Table.
 *
 *  Note: See design/Files Table.md for design choices.
 */

#include <spinlock.h>
#include <file_handle.h>

#define NR_OPEN_DEFAULT 64


struct files_table {
    struct spinlock ft_lock;                        /* spinlock for synchronization */
    int next_fd;                                    /* cache for the next file descriptor */
    struct file_handle *fd_array[NR_OPEN_DEFAULT];  /* files handles array */
};


/*
 * Functions in files_table.c:
 *
 *    files_table_create - create a new empty files table. May
 *                      return proper errno on error.
 *
 *    files_table_assign_default_handles - helper function to create standard file handles.
 *                                       STDIN, STDOUT, STDERR
 *                                       May panic if unable to assign any of the std file handles.
 *
 *    files_table_copy - create a new files table that is an exact copy of
 *                      an old one.
 *
 *    files_table_get_next_fd - function to get next available file descriptor.
 *                          return proper errno on error..
 *
 *    files_table_destroy - dispose of a files table.
 *
 *
 */
int  files_table_create (struct files_table **ft);
int  files_table_assign_default_handles (struct files_table *ft);
int  files_table_copy (struct files_table *src, struct files_table **ret);
int  files_table_get_next_fd (struct files_table *ft, int *fd);
void files_table_destroy (struct files_table *);



#endif /* _FILES_TABLE_H_ */