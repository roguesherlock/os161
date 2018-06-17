# File Handle

    ### A Data Structure that contains a reference to a file object (vnode)

    *Requirements:*
    -   Should be able to tell, where to read or write
    -   Should be able to prevent invalid read or write
    -   Should be able to determine if it's safe to be destroyed
    -   Should be able to synchronize


1. offset

2. check proper permissions

3. Could use reference count to count the number of refrences to the file handle

4. Synchronization: spinlock



## Proposal

-------------------------------------
| File Handle                       |
-------------------------------------
|   refeference counter             |
|   spinlock for synchronization    |
|   pointer to a file               |
|   flags for access mode           |
|   offset                          |
|   rwlock for file                 |
-------------------------------------

* inspired by linux


<br/>



## Functions on File Handle


-   #### file_handle_create
    create a new file handle pointing to a given file with a given file mode and at a given offset.
    return proper [errno][1] on error.

    `int file_handle_create(struct vnode *file, enum file_modes f_mode, unsigned int f_pos, struct file_handle **nfh);`


-   #### file_handle_create_std_handle
    helper function for creating standard file handles. Namely STDIN, STDOUT and STDERR.
    return proper [errno][1] on error.

    `int file_handle_create_std_handle(unsigned int fd, struct file_handle **nfh);`


-   #### file_handle_has_access
    check if process has access to the given operation on a file.

    `bool file_handle_has_access(struct file_handle *fh, int flag);`


-   #### file_handle_destroy
    dispose of a file handle.

    `void file_handle_destroy(struct file_handle *);`



<br/>



### Source

-   [file_handle.h][2]
-   [file_handle.c][3]


[1]:../kern/include/kern/errno.h
[2]:../kern/include/file_handle.h
[3]:../kern/proc/file_handle.c