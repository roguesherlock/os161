# Assignment 1



## Questions



### Thread Questions


-   #### What happens to a thread when it calls thread_exit? What about when it sleeps?
    > When it calls thread_exit,
    >   1. it is removed from the process
    >   2. the thread is put into zombie thread list
    >   3. cpu finds another thread to switch to.
    >
    >   ok, so when it sleeps,
    >   1. it's state will be set to sleep and
    >   2. it will be put on to the tail of the given wait channel


-   #### What function—​or functions—​handle(s) a context switch?
    >   thread_switch on c level and switchframe on architecture level, handles context switch



-   #### What does it mean for a thread to be in each of the possible thread states?
    >   -   S_RUN = running
    >   -   S_READY = ready to run
    >   -   S_SLEEP = sleeping
    >   -   S_ZOMBIE = exited but not yet deleted.


-   #### What does it mean to turn interrupts off? How is this accomplished? Why is it important to turn off interrupts in the thread subsystem code?
    >   turning off interrupts mean cpu can't be interrupted. That is, cpu won't respond to any signals and just        execute the current code.
    >
    >   well, splx() turns interrupts on or off depending upon the argument. It uses other lower level functions to     do this.
    >
    >   because we don't want cpu to interrupt when we are starting a thread, or switching to other thread or           deleting a thread, etc.


-   #### What happens when a thread wakes up another thread? How does a sleeping thread get to run again?
    >   Well, the woke up thread is put to runnable state. The thread is put to the runnable queue of the cpu on        which it was last running.


<br/>



### Scheduling Questions



-   #### What function (or functions) choose the next thread to run?
    >   schedule, consider_thread_migration, thread_yield


-   #### How is the next thread to run chosen?
    >   head of curcpu->runqueue


-   #### What role does the hardware timer play in scheduling?
    >   switching of threads in kind of round robin scheduling fashion


-   #### What hardware independent function is called on a timer interrupt?
    >   thread_yield


<br/>



### Synchronization Questions



-   #### Describe how wchan_sleep and wchan_wakeone are used to implement semaphores.
    >   Well, when the P() is called on semaphore and it is unavailable, the current thread is put to sleep on it's wait channel
    >   by wchan_sleep(). And, when V() is called, one of the thread is started from the wait channel to complete it's P().


-   #### Why does the lock API in OS/161 provide lock_do_i_hold, but not lock_get_holder?
    >   ok, so lock_do_i_hold is used to test if the lock is actually held by the current thread. There's no point in knowing who
    >   the lock holder is because if we are not holding the lock, there's nothing we can do other than wait for it to release.


<br/>
<br/>



## Doubts



-   #### curcpu and curthread confusion
-   #### thread.c line 585, when can that not be true?
-   #### thread.c line 597, how is it possible?
-   #### thread.c line 678, wouldn't that always be the case?


<br/>
<br/>



## Design



-   #### Locks
    >


-   #### CVs
    >


-   #### Reader Writer Locks
    >


-   #### Stoplight
    >


-   #### Whalemating
    >

