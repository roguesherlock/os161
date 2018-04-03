# Page

    Each physical page in the system has a struct page associated with
    it to keep track of whatever it is we are using the page for at the
    moment.



## Proposal

--------------------------------------------------------------------------
| Page                                                                   |
--------------------------------------------------------------------------
|    p_lock:    lock for refrence count                                  |
|    refcount:  how many threads are refrenceing this page               |
|    flags:     access permissions for the page                          |
|    dirty:     does the page contents need to be written back to disk?  |
|    id:        page id. allows to detect contiguous pages               |
--------------------------------------------------------------------------



<br/>


-----------------------------------------------------
| flags                                             |
-----------------------------------------------------
|   PAGE_READ: A Page is used as read only          |
|   PAGE_WRITE: A Page is used as write only        |
|   PAGE_RDWR: A Page is being used  as read, write |
-----------------------------------------------------



<br/>



### Source

-   [vm.h][1]



[1]:../kern/include/vm.h