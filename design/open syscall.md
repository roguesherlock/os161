# Open

open opens the file, device, or other kernel object named by the pathname filename. The flags argument specifies how to open the file. The optional mode argument provides the file permissions to use and is only meaningful in Unix, or if you choose to implement Unix-style security later on. it can be ignored in OS/161.


## Synopsis

```int open(const char *filename, int flags);```

### On kernel side

```int sys_open(const char *filename, int flags, int32_t *fd)```


The flags argument should consist of one of
-   **O_RDONLY**	Open for reading only.
-   **O_WRONLY**	Open for writing only.
-   **O_RDWR**  	Open for reading and writing.


## Return Values
On success, open returns a nonnegative file handle. On error, -1 is returned, and errno is set according to the error encountered.

### On kernel side
return 0 on success or error code on errors. Acctual return values are put in fd pointer

## Errors
The following error codes should be returned under the conditions given. Other [error][1] codes may be returned for other cases not mentioned here.

-   **ENODEV** : The device prefix of filename did not exist.
-   **ENOTDIR** : 	A non-final component of filename was not a directory.
-   **ENOENT** : A non-final component of filename did not exist.
-   **ENOENT** : The named file does not exist, and O_CREAT was not specified.
-   **EEXIST** : The named file exists, and O_EXCL was specified.
-   **EISDIR** : The named object is a directory, and it was to be opened for writing.
-   **EMFILE** : The process's file table was full, or a process-specific limit on open files was reached.
-   **ENFILE** : The system file table is full, if such a thing exists, or a system-wide limit on open files was                    reac*hed.
-   **ENXIO** : The named object is a block device with no filesystem mounted on it.
-   **ENOSPC** : The file was to be created, and the filesystem involved is full.
-   **EINVAL** : flags contained invalid values.
-   **EIO**    : A hard I/O error occurred.
-   **EFAULT** : filename was an invalid pointer.



<br/>



## Source

-   [open_syscall.c][2]



[1]:../kern/include/kern/errno.h
[2]:../kern/syscall/open_syscall.c