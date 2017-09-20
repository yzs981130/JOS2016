/*	$NetBSD: stdarg.h,v 1.12 1995/12/25 23:15:31 mycroft Exp $	*/

#ifndef JOS_INC_STDARG_H
#define	JOS_INC_STDARG_H

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)

#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define va_end(ap) __builtin_va_end(ap)

/*
#define __va_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_start(ap, last) \
  ((ap) = (va_list)&(last) - __va_size(last))

#define va_arg(ap, type) \
  (*(type *)((ap) -= __va_size(type), (ap) + __va_size(type)))

#define	va_end(ap) \
	((void) 0)
*/

#endif	/* !JOS_INC_STDARG_H */
