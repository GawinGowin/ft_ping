#ifndef SHARED_ERROR_H
#define SHARED_ERROR_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef TESTING
#include <setjmp.h>
extern int last_error_status;
extern char last_error_message[256];
extern jmp_buf test_err_jmp_buf;
extern int test_err_jmp_buf_set;
#endif

void error(int status, const char *format, ...);

#endif /* SHARED_ERROR_H */
