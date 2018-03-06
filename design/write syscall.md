# Write

    *Considerations:*
    -   Check if file descriptor is valid
    -   Check if buffer pointer is valid
    -   Can the user write to the file?
    -   How to write to a file?
        -   Read and understand [uio][1]






## Source

[write_syscall.c][2]



[1]:../kern/include/uio.h
[2]:../kern/syscall/write_syscall.c