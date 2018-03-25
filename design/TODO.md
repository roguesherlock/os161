# TODO

## To Implement

-   #### kill_curthread
-   #### A better scheduling algorithm.

## bugs

-   #### In [open syscall][0] related to bad memory refrences
        check with /testbin/badcall

-   #### In [read syscall][1] related to permissions
        check with /testbin/badcall

-   #### In [write syscall][2] related to permissions
        check with /testbin/badcall

-   #### In [lseek syscall][3] related to bad memory refrences
        check with /testbin/badcall

-   #### In [waitpid syscall][4] related to bad memory refrences
        check if status pointer is valid before sleeping.
        Note: NULL is valid.
        idea 1: try copying dummy value with copyout
        idea 2: see if copycheck can be made available as copyinoutapi
                and use it directly



[0]:../kern/syscall/open_syscall.c
[1]:../kern/syscall/read_syscall.c
[2]:../kern/syscall/write_syscall.c
[3]:../kern/syscall/lseek_syscall.c
[4]:../kern/syscall/waitpid_syscall.c