#ifndef SHARED_PARSE_H
#define SHARED_PARSE_H

#include <errno.h>
#include <stdlib.h>

long parse_long(
    char const *const str,
    const char *const msg,
    const long min,
    const long max,
    void (*errorfn)(int, const char *, ...));

#endif /* SHARED_PARSE_H */
