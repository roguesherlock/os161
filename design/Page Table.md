The data structure used to quickly map a virtual page number to a page table entry is called a page table.

Each process has a separate page table.
Why? Virtual addresses are private to each process and translated differently for each.


(We will see in a few slides why we call them page table entries.)
Can usually jam everything into one 32-bit machine word:
Location: 20 bits. (Physical Page Number or location on disk.)
Permissions: 3 bits. (Read, Write, Execute.)
Valid: 1 bits. Is the page located in memory?
Referenced: 1 bits. Has the page been read/written to recently?


https://www.ops-class.org/slides/2017-03-15-paging/deck.html