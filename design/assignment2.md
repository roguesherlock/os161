# Assignment 2



## Questions



-   ### What primitive operations exist to support the transfer of data to and from kernel space? Do you want to implement more on top of these?
    >   copyin, copyout, copyinstr, copyoutstr


-   ### When implementing exec, how will you determine:

    -   the stack pointer initial value

    -   the initial register contents

    -   the return value

    -   whether you can execute the program at all?


-   ### You will need to bullet-proof the OS/161 kernel from user program errors. There should be nothing a user program can do—​and we will try almost everything—to crash the operating system, with the exception of explicitly asking the system to halt.


-   ### What new data structures will you need to manage multiple processes?


-   ### What relationships do these new structures have with the rest of the system?


-   ### How will you manage file accesses? When the shell invokes the cat command, and the cat command starts to read file1, what will happen if another program also tries to read file1? What would you like to happen?

    >   Allow read only access?


<br/>
<br/>



## Design


