# Assignment 3



## Questions



### Code Reading Questions


<br/>



### Virtual Memory


-   #### Assuming that a user program just attempted to access a virtual address, describe the conditions under which each of the following can arise. If the situation cannot happen, explain why it cannot occur.

    -   TLB miss, page fault
        >   Mapping doesn't exist in TLB as well as the page isn't mapped onto physical memory.

    -   TLB miss, no page fault
        >   Mapping doesn't exist in TLB, but page does exist in memory (i.e. mapped onto physical memory).

    -   TLB hit, page fault
        >   This situation cannot happen. There's TLB hit if and only if the page does exist in memeory (i.e. mapped onto physical memory).

    -   TLB hit, no page fault
        >   There's TLB hit if and only if the page does exist in memeory (i.e. mapped onto physical memory).

    P.S. what's the proper perposition? onto or in?


-   #### A friend of yours who foolishly decided not to take this class, but who likes OS/161, implemented a TLB that has room for only one entry, and experienced a bug that caused a user instruction to generate a TLB fault infinitely—​the instruction never completed executing! Explain how this could happen. Recall that after OS/161 handles an exception, it restarts the instruction that caused the exception.
    >   Consider the below instruction to load word from some memeory address not currently mapped onto physical memory. There'll be first a TLB miss as TLB had room for only one entry and it contained the mapping for the current program counter. Second, There'll also be page fault as the memory address isn't mapped onto the physical memory. Now suppose we handle this execption, load the reqeusted page (page containing requested memory address) onto physicall memory, update the page table, as well as TLB. Now TLB contains entry for the requested memeory address. And so, now we return back to userspace. As the instruction is restarted after the exception, we'll have another TLB miss because the address containing the instruction isn't in TLB (as TLB has room for only one entry). And thus the cycle continues infinitely.


-   #### How many memory-related exceptions—-including hardware exceptions and other software exceptional conditions—-can the following MIPS-like instruction raise? Explain the cause of each.
    ```
    # load word from $0 (contains zeros) offset 0x120 into register $3
    lw $3,0x0120($0)
     ```

    >   Number of Exceptions: 3

    >       -   TLB miss, page fault:
                Entry/Mapping doesn't exist in TLB as well as the page isn't mapped onto physical memory

    >       -   TLB miss, no page fault:
                Entry/Mapping doesn't exist in TLB. But, page is mapped onto physical memory

    >       -   TLB hit, no page fault
                Entry/Mapping exist in TLB.




<br/>



### The malloc Library Allocator


Once OS/161 has paging, you can support applications with larger address spaces. The malloc and free functions are provided in the standard C library. Read the code and answer the following questions.

Consider the following (useless) program:

```
/* This is bad code: it doesn't do any error-checking */
#include <stdio.h>
int main (int argc, char **argv) {
    int i;
    void *start, *finish;
    void *res[10];
    start = sbrk(0);
    for (i = 0; i < 10; i++) {
        res[i] = malloc(10);
    }
    finish = sbrk(0);
    /* INSERT */
    return 0;
}
```


-   #### How many times does the system call sbrk get called from within malloc?
    >   only once. Whenever malloc needs to call sbrk, it makes sure it gets memory in pages. So, total size of 10 pointers (40 bytes) + (8 bytes for each mheader) can easyly fit into a single 4k page


-   #### On the i386 platform, what is the numeric value of (finish - start)?
    >   Assuming it's a fresh malloc and no blocks are free, On i386 platform (considering 64-bit), the numberic value of (finish - start) = 260


<br/>



Now, suppose that in the example above we now insert the following code at location /* INSERT */ above:

```
  void *x;
	free(res[8]); free(res[7]); free(res[6]);
	free(res[1]); free(res[3]); free(res[2]);
	x = malloc(60); /* MARK */
```


-   #### Again on the i386, would malloc call sbrk when doing that last allocation at the marked line above? What can you say about x?
    >   Yes. As each and every block freed had size 10 and malloc alocates in blocks of size >= given size to malloc


-   #### It is conventional for libc internal functions and variables to be prefaced with __. Why do you think this is so?
    >   a. to signify that pariticular indentifier is for internal use only and not part of the public api. b. to avoid conflict with user programs.

-   #### The man page for malloc requires that "the pointer returned must be suitably aligned for use with any data type." How does our implementation of malloc guarantee this?
    >   size is always rounded to integral number of blocks (blocksize for a block is platform dependent).

