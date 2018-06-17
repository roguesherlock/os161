/*
 * definition for execv syscall
 *
 */

#include <types.h>
#include <limits.h>
#include <kern/errno.h>
#include <proc.h>
#include <proc_table.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <vnode.h>
#include <vfs.h>
#include <addrspace.h>
#include <copyinout.h>
#include <files_table.h>

#define ALIGN sizeof(char *)

#define PAD(X) (((X) % ALIGN == 0) ? 0 : (ALIGN - ((X) % ALIGN)))


/*
 *	get_argc - return total number of arguments
 *				this includes program name itself
 *				get_argc doesn't fail
 *
 */
static
int
get_argc(char **args)
{
	int argc;
	char **tracer;

	argc = 0;
	tracer = args;

	/* first argument is alraedy program name. so no need */
	// argc++;		/* for program name arg */
	while (*tracer++)
		argc++;
	return argc;
}

/*
 *	free_args - free arguments pointed by args
 *
 */
static
void
free_args(int argc, char **args)
{
	int i;
	char **tracer;

	tracer = args;

	for (i = 0; i < argc; ++i)
		kfree(*tracer++);


	/* finally free args itself */
	kfree(args);
}


/*
 *	check_and_copy_args - check if arguments size exceeds ARG_MAX
 *						size calculation includes argument size,
 * 						padding (to allign to 4 bytes [typical pointer size]),
 * 						and pointer size (that points to argument)
 *						And copy arguments to kernel space.
 *						on success, argc contains total number of arguments
 *						and argv contains array of arguments.
 *						Also, argv args are **not** alligned to 4 bytes.
 *						this includes program name
 *						returns proper error code on failure
 *
 */
static
int
check_and_copy_args(char **args, int *argc, char ***argv, int *size)
{
	int i, result, nargs, args_size, pad_size;
	size_t str_size;
	char **head, **kdst, **usrc;

	KASSERT (curproc != NULL);

	i = 0;
	args_size = 0;
	pad_size = 0;
	usrc = args;
	nargs = get_argc(args);

	/* head to keep track of start of argv in kernel space */
	head = kdst = kmalloc(sizeof(char *) * (nargs + 1)); /* extra 1 for termination */
	if (kdst == NULL)
		return ENOMEM;

	kdst[nargs] = NULL;								/* define argv end */
	args_size += sizeof(userptr_t);

	while(*usrc) {
		/* check if userptr is a valid ptr */
		// result = check_userptr ((const_userptr_t) *usrc);
		if ((!as_is_addr_valid(curproc->p_addrspace, (vaddr_t) *usrc))) {
		// if (result) {
			free_args(i, head);
			return EFAULT;
		}
		str_size = strlen(*usrc) + 1; 	/* account for \0 */
		pad_size = PAD(str_size);
		args_size += str_size + pad_size + sizeof(userptr_t); /* include pointers too */;

		*kdst = kmalloc(sizeof(char) * str_size);
		if (*kdst == NULL) {
			free_args(i, head);
			return ENOMEM;
		}
		++i;
		/* str_size works because char is 1 byte */
		result = copyin((const_userptr_t) *usrc, *kdst, str_size);
		if (result) {
			free_args(i, head);
			return result;
		}

		/* unlikely */
		if (args_size >= ARG_MAX) {
			free_args(i, head);
			return E2BIG;
		}

		usrc++;
		kdst++;
	}

	*argc = nargs;
	*argv = head;	/* triple ref pointer casue, why not! */
	*size = args_size;

	return 0;
}


/*
 *	put_args_to_userspace_stack - copy arguments from kernel space to
 *								to userspace stack
 *								on success, uargs points to start of args
 *								in userspace stack. Also, stackptr is updated.
 *								returns proper error code on failure
 *
 *
 * 					before:
 * 					-----------------	0x800000000
 *					|				|	<------------ Stackptr, head
 *					|				|
 *				/\	|				|	 |
 *				|	|				|	 |
 *	copyout		|	|				|	 |	stack grows down
 *	copies		|	|				|	 |
 *	up			|	|				|	\|
 *					|				|
 *					|				|
 *					|				|	<------------ tail (args_size)
 *					|				|
 *					----------------- 	0x000000000 (STACK_SIZE)
 *
 *
 * 					after:
 * 					-----------------	0x800000000
 *					|				|
 *					|	"arg0"		|
 *				/\	|				|	 |
 *				|	|	"arg1"		|	 |
 *	copyout		|	|				|	 |	stack grows down
 *	copies		|	|				|	 |
 *	up			|	|	"arg2"		|	\|
 *					|	argv[2]		|	<------------ tail (argc pointers copied)
 *					|	argv[1]		|
 *					|	argv[0]		|	<------------ Stackptr, head (args_size)
 *					|				|
 *					----------------- 	0x000000000 (STACK_SIZE)

 *
 */
static
int
put_args_to_userspace_stack (char **kargs, int args_size, vaddr_t *stackptr, userptr_t *uargs)
{
	KASSERT(kargs != NULL);
	KASSERT(stackptr != NULL);

	int result;
	char **tracer;
	size_t str_size, pad_size;
	userptr_t head, tail;

	head = (userptr_t) *stackptr;
	tail = (userptr_t) (*stackptr - args_size);
	tracer = kargs;

	while (*tracer) {
		str_size = strlen(*tracer) + 1;		/* account for \0 */
		pad_size = PAD(str_size);
		if (pad_size) {
			head -= pad_size;
			bzero(head, pad_size);
		}
		head -= str_size;
		result = copyout(*tracer, head, str_size);
		if (result)
			return result;
		result = copyout(&head, tail, sizeof(userptr_t));
		if (result)
			return result;

		tail += sizeof(userptr_t);
		tracer++;
	}

	/* terminate argv */
	bzero(tail, sizeof(userptr_t));

	*stackptr -= args_size;
	*uargs = (userptr_t) *stackptr;

	return 0;
}



int
sys_execv (const char *program, char **args, int32_t *retval)
{
    int result, kargc, args_size;
	char *pname, **kargs;
	size_t psize;
    struct vnode *v;
    struct addrspace *as;
    vaddr_t entrypoint, stackptr;
	userptr_t uargs;

	KASSERT (curproc != NULL);

    *retval = -1;

	if ((!as_is_addr_valid(curproc->p_addrspace, (vaddr_t) program))
		|| (!as_is_addr_valid(curproc->p_addrspace, (vaddr_t) args)))
		return EFAULT;

	psize = strlen(program) + 1;
	pname = kmalloc(psize);
	if (pname == NULL)
		return ENOMEM;

	result = copyin((const_userptr_t) program, pname, psize);
	if (result) {
		kfree(pname);
		return result;
	}

	/* copy args to kernel space */
	result = check_and_copy_args(args, &kargc, &kargs, &args_size);
	if (result) {
		kfree(pname);
		return result;
	}

	/* *kargs points to program name copied to kernel space */
	result = vfs_open(pname, O_RDONLY, 0, &v);
	if (result) {
		free_args(kargc, kargs);
		kfree(pname);
		return result;
	}

	/* free program name */
	kfree(pname);

    /* destroy previous address space. */
    as = proc_getas();
    KASSERT(as != NULL);
    as_destroy(as);
    as = NULL;

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		free_args(kargc, kargs);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		free_args(kargc, kargs);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		free_args(kargc, kargs);
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	result = put_args_to_userspace_stack(kargs, args_size, &stackptr, &uargs);
	if (result) {
		free_args(kargc, kargs);
		return result;
	}

	/* free kargs */
	free_args(kargc, kargs);

	/* Warp to user mode. */
	enter_new_process(kargc /*argc*/, uargs /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("[!] sys_execv: enter_new_process returned\n");
	return EINVAL;

}