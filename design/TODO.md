# TODO

## To Implement

-   #### kill_curthread
-   #### A better scheduling algorithm.

## bugs

-   #### In [waitpid syscall][4] related to bad memory refrences
        check if status pointer is valid before sleeping.
        Note: NULL is valid.
        idea 1: try copying dummy value with copyout
        idea 2: see if copycheck can be made available as copyinoutapi
                and use it directly




[4]:../kern/syscall/waitpid_syscall.c
