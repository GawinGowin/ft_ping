#include "ft_ping.h"

#ifndef TESTING
void error(int status, const char *format, ...) {
  va_list ap;

  fprintf(stderr, "%s: ", program_invocation_short_name);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  if (status)
    exit(status);
}
#else
void error(int status, const char *format, ...) {
  char buffer[1024];
  va_list args;
  fprintf(stderr, "%s: (%d) ", program_invocation_short_name, status);
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  fprintf(stderr, "%s", buffer);
}
#endif

long parse_long(
    char const *const str,
    const char *const msg,
    const long min,
    const long max,
    void (*errorfn)(int, const char *, ...)) {
  char *endptr;

  if (str == NULL || *str == '\0') {
    errorfn(1, "%s: %s", msg, str);
  }
  long val = strtol(str, &endptr, 10);
  if (errno || str == endptr || (endptr && *endptr)) {
    errorfn(1, "%s : %s", msg, endptr);
  }
  if (val < min || max < val) {
    errorfn(1, "%s: '%s': out of range: %lu <= value <= %lu", msg, str, min, max);
  }
  return val;
}
