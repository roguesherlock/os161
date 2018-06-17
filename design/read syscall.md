# Read
read reads up to buflen bytes from the file specified by fd, at the location in the file specified by the current seek position of the file, and stores them in the space pointed to by buf. The file must be open for reading.

The current seek position of the file is advanced by the number of bytes read.


## Synopsis

```ssize_t read(int fd, void *buf, size_t buflen);```

### On kernel side:
```int sys_read(int fd, void *buf, size_t buflen, int32_t *read)```


## Return Values
The count of bytes read is returned. This count should be positive. A return value of 0 should be construed as signifying end-of-file. On error, read	returns -1 and sets errno to a suitable error code for the error condition encountered.

Note that in some cases, particularly on devices, fewer than buflen (but greater than zero) bytes may be returned. This depends on circumstances and does not necessarily signify end-of-file.

### On kernel side:
return 0 on success or error code on errors. Acctual return values are put in read pointer


## Errors
The following error codes should be returned under the conditions given. Other [error][1] codes may be returned for other cases not mentioned here.

-   **EBADF** fd is not a valid file descriptor, or was not opened for reading.
-   **EFAULT** Part or all of the address space pointed to by buf is invalid.
-   **EIO** A hardware I/O error occurred reading the data.



<br/>




## Source

-   [read_syscall.c][2]



[1]:../kern/include/kern/errno.h
[2]:../kern/syscall/read_syscall.c