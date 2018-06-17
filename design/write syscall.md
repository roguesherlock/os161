# Write
write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.

The current seek position of the file is advanced by the number of bytes written.



## Synopsis

```ssize_t write(int fd, const void *buf, size_t buflen);```

### On kernel side:
```int sys_write(int fd, const void *buf, size_t buflen, int32_t *wrote)```


## Return Values
The count of bytes written is returned. This count should be positive. A return value of 0 means that nothing could be written, but that no error occurred; this only occurs at end-of-file on fixed-size objects. On error, write returns -1 and sets errno to a suitable error code for the error condition encountered.

### On kernel side:
return 0 on success or error code on errors. Acctual return values are put in wrote pointer



## Errors
The following error codes should be returned under the conditions given. Other [error][1] codes may be returned for other cases not mentioned here.

-   **EBADF**  fd is not a valid file descriptor, or was not opened for writing.
-   **EFAULT** Part or all of the address space pointed to by buf is invalid.
-   **ENOSPC** There is no free space remaining on the filesystem containing the file.
-   **EIO**    A hardware I/O error occurred reading the data.



<br/>




## Source

-   [write_syscall.c][2]



[1]:../kern/include/kern/errno.h
[2]:../kern/syscall/write_syscall.c