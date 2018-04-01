# Assignment 2



## Questions



### Design Considerations

-   #### What primitive operations exist to support the transfer of data to and from kernel space? Do you want to implement more on top of these?
    >   uimove, copyin, copyout, copyinstr, copyoutstr


-   #### When implementing exec, how will you determine:

    -   the stack pointer initial value
        >   New Stacks always have initial values of USESRSTACK. It'll be decremented (as stack grows down) as I put the arguments for the user program on top of stack.

    -   the initial register contents
        >   we only need to set few registers. Namely arguments registers, program counter, stack pointer, etc Rest of them are zeored out.

    -   the return value
        >   execv does not return of sucess. On failure it returns -1 and sets proper errno.

    -   whether you can execute the program at all?
        >   load_elf will fail in case program is non executable (although, it isn't called an program then).


-   #### You will need to bullet-proof the OS/161 kernel from user program errors. There should be nothing a user program can do—​and we will try almost everything—to crash the operating system, with the exception of explicitly asking the system to halt.
    >   execv doesn't panic. kill_curthread is called anytime user tries to anything mallicius (either intentionally or unintentionally) which kills the current user process.

-   #### What new data structures will you need to manage multiple processes?
    >   A Process Table which keep tracks of Current active/inactive Processess in the system.


-   #### What relationships do these new structures have with the rest of the system?
    >   A Process Table maps pids to processess in the system.


-   #### How will you manage file accesses? When the shell invokes the cat command, and the cat command starts to read file1, what will happen if another program also tries to read file1? What would you like to happen?
    >   Allow read only to everyone (off course based on file permissions). Basically, use reader writer locks.


<br/>



### Code Reading Questions


<br/>



### Existing Process Support


-   #### What are the ELF magic numbers?
    >   ELF magic numbers identify the ELF files. They are first 4 bytes of an ELF file. Typically '0x7f', 'E', 'L', 'F'.


-   #### What is the difference between UIO_USERISPACE and UIO_USERSPACE? When should one use UIO_SYSSPACE instead?
    >   UIO_USERISPACE, UIO_USERSPACE, UIO_SYSSPACE allows us to identify weather the data pointed at is user process code, user process data or kernal data respectively.
    >   when moving kernel data around?

-   #### Why can the struct uio that is used to read in a segment be allocated on the stack in load_segment? Or, put another way, where does the memory read actually go?
    >   don't understand the question properly yet.

-   #### In runprogram why is it important to call vfs_close before going to user mode?
    >   because user didn't open the file? Anyways, the file shouldn't be open for the user process as it's no use to user

-   #### What function forces the processor to switch into user mode? Is this function machine dependent?
    >   asm_usermode. Which just calls exception_return which does the heavy work.
    >   Yes it's machine dependent.

-   #### In what files are copyin, copyout, and memmove defined? Why are copyin and copyout necessary? (As opposed to just using memmove.)
    >   copyin, copyout in kern/vm/copyinout.c and memmove in common/libc/string/memmove.c
    >   memove is a simple function that copies block of memory from one point in memory to the other. It doesn't do much of error checking or exception handling. Thus the need for copyin and copyout.

-   #### What is the purpose of userptr_t?
    >   for type safety and to distinguish kernel pointers from userpointers


<br/>



### Traps and System Calls



-   #### What is the numerical value of the exception code for a MIPS system call?
    >   8 bytes

-   #### How many bytes is an instruction in MIPS? Try to answer this by reading syscall carefully, not by looking somewhere else.
    >   4 bytes

-   #### Why do you probably want to change the implementation of kill_curthread?
    >   Because kernel currently doesn't handle killing user thread. It panics!

-   #### What would be required to implement a system call that took more than 4 arguments?
    >   further arguments must be fetched from the user-level stack, starting at sp+16


<br/>



### Support Code for User Programs



-   #### What is the purpose of the SYSCALL macro?
    >   it defines each syscall and sets up proper arguments for it?

-   #### What is the MIPS instruction that actually triggers a system call? (Answer this by reading the source in this directory, not looking somewhere else.)
    >   j syscall?

-   #### Now that OS/161 supports 64 bit values, lseek takes and returns a 64 bit offset value. Thus, lseek takes a 32 bit file handle (arg0), a 64 bit offset (arg1), a 32 bit whence (arg3), and needs to return a 64 bit offset. In void syscall(struct              trapframe *tf) where will you find each of the three arguments (in which registers) and how will you return the 64 bit offset?
    >   arg0 in a0, arg1 in a2 & a3 and arg3 at sp+16. return in v0 & v1 (v0 contains higher order bits, whilst v1 contains lower order bits as MIPS's byte order is set to BIG-ENDIAN order)



<br/>
<br/>

