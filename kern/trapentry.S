/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps


/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

#define ec(name, num)\
.data;				\
	.long name;				\
.text;\
	.globl name;				\
	.type name, @function;				\
	.align 2;				\
name:				\
	pushl $(num);				\
	jmp _alltraps

#define noec(name, num)				\
.data;				\
	.long name;				\
.text;				\
	.globl name;				\
	.type name, @function;				\
	.align 2;				\
name:				\
	pushl $0;				\
	pushl $(num);				\
	jmp _alltraps

#define padding()				\
.data;				\
	.long 0

.data
	.p2align 2
	.globl funs



funs:
.text
	noec(th0, 0)
	noec(th1, 1)
	noec(th2, 2)
	noec(th3, 3)
	noec(th4, 4)
	noec(th5, 5)
	noec(th6, 6)
	noec(th7, 7)
	ec(th8, 8)
	padding()
	ec(th10, 10)
	ec(th11, 11)
	ec(th12, 12)
	ec(th13, 13)
	ec(th14, 14)
	padding()
	noec(th16, 16)
	noec(th17, 17)
	noec(th18, 18)
	noec(th19, 19)
.data
	.space 112
.text
	noec(th48, 48)


/*
 * Lab 3: Your code here for _alltraps
 */


_alltraps:
	pushl %ds
	pushl %es
	pushal
	movl $GD_KD, %eax
	movl %eax, %ds
	movl %eax, %es
	pushl %esp
	call trap
