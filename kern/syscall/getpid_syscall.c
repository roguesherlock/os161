/*
 * getpid syscall definition
 *
 */

#include <types.h>
#include <proc.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>


pid_t
sys_getpid(void)
{
    return curproc->pid;
}