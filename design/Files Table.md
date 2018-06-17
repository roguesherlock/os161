# Files Table

    ### A Data Structure that maps a file discriptor to a file handle

    *Requirements:*
    -   Should be able to find an available file descriptor
    -   Given a file descriptor, the file table should be able to return the file handle of that index
    -   should be able to recycle file descriptor

1. a cache for next file descriptor
2. file handles array
3. loop through array to find next file descriptor if next fd is over NR_OPEN_DEFAULT



## Proposal

-----------------------------------------
| Files Table                           |
|---------------------------------------|
|   ~~a pointer to file handles table~~ |
|   ~~Base file handles table~~         |
|   cache for next file descriptor      |
|   spinlock for the file table         |
|   static file handles array           |
-----------------------------------------

* inspired by linux


I decided eventually to restrict open files to NR_OPEN_DEFAULT (64 hard coded currently). I could probably add another pointer to another file handles table and increase the number of open files for a given process. But, the complexity isn't worth it. Atleast for OS161.


<br/>



## Functions on Files Table

-   #### files_table_create
    create a new empty files table. return proper [errno][1] on error.

    `int files_table_create(struct files_table **nft);`


-   #### files_table_assign_default_handles
    helper function to create standard file handles. Namely STDIN, STDOUT and STDERR.
    return proper [errno][1] on error.

    `int files_table_assign_default_handles (struct files_table *ft)`


-   #### files_table_copy
    create a new files table that is an exact copy of an old one.

    `int files_table_copy(struct files_table *src, struct files_table **ret);`


-   #### files_table_get_next_fd
    function to get next available file descriptor.
    return proper [errno][1] on error.

    `int files_table_get_next_fd (struct files_table *ft, int *fd);`


-   #### files_table_destroy
    dispose of a files table.

    `void files_table_destroy(struct files_table *);`



<br/>



### Source

-   [files_table.h][2]
-   [files_table.c][3]



[1]:../kern/include/errno.h
[2]:../kern/include/files_table.h
[3]:../kern/proc/files_table.c