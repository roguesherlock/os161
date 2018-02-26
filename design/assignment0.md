# Assignment 0



## Questions



-   #### What function initializes the kernel during boot, and what subsystems are currently initialized?
    >   boot fucntion. It initializes ram, cpu, file system, threads,


-   #### What VM system does your kernel use by default? What is wrong with it?
    >   dumbvm. leaks memory, doesn't track page allocations, all Pages are ReadWrite, only 1 region supported


-   #### OS/161 ships with two working synchronization primitives. What are they?
    >   Semaphore, spinlocks [ok ,so it was spinlocks and not conditional variables, which I first wrote]


-   #### How do you create a thread in OS/161? Give some examples of code that currently creates multiple threads.
    >   calling thread_fork. most of the tests called thread_fork multiple times


-   #### OS/161 has a system for printing debugging messages to the console. How does it work? How could it be useful?
    >   kprintf function prints to console. It can be used to debug and pin point the errors. at the end of the day, it's putch to console.


-   #### What do copyin and copyout do? What is special about these functions compared to other approaches to copying memory in C, like memmove?
    >   copyin and copyout copy the block of memory from userlevel to kernel level and vice vers    >   They check for out of bounds region like overlaping.
    >   (although memmove does that too, but, this implementation checks that; as well as allows non overlapped region to be copied)


-   #### What is a zombie thread?
    >   A thread which is exited but still requires thread_destroy to be called on it.


-   #### What is the difference between a thread sleeping and a thread yielding?
    >   yielding a thread gives up cpu but, it still is runnable and can be taken by other cpu. Whereas sleeping a thread gives up on cpu and is put
    >   into non runnable(sleeping) state.


-   #### Explain the difference between machine dependent and machine independent code. Where would you put both in the OS/161 source tree?
    >   Machine dependent code is code which is geared toward specific architecutre, or in esence, towards specific machine. Whereas Machine
    >   indipendent code is code that can be run on different machine or in esence, independent of underlying machine. All MD code goes in arch
    >   sub directory Whereas MI code goes in it's respective directory.


-   #### What functions are used to enable and restore interrupts? Would this be sufficient to ensure mutual exclusion on an OS/161 kernel?
    >    splx is used to enable and restore interrupts. I guess.
