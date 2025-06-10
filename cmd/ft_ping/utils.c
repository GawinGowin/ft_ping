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
int last_error_status = 0;
int test_err_jmp_buf_set = 0;
char last_error_message[256];
jmp_buf test_err_jmp_buf;

void error(int status, const char *format, ...) {
  va_list ap;

  memset(last_error_message, 0, sizeof(last_error_message));
  last_error_status = status;
  va_start(ap, format);
  vsnprintf(last_error_message, sizeof(last_error_message), format, ap);
  va_end(ap);
  fprintf(stderr, "%s: %s (%d)\n", "test_ft_ping", last_error_message, status);
  if (status && test_err_jmp_buf_set) {
    longjmp(test_err_jmp_buf, status);
  }
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
    errorfn(1, "%s: %s\n", msg, str);
  }
  long val = strtol(str, &endptr, 10);
  if (errno || str == endptr || (endptr && *endptr)) {
    errorfn(1, "%s : %s\n", msg, endptr);
  }
  if (val < min || max < val) {
    errorfn(1, "%s: '%s': out of range: %lu <= value <= %lu\n", msg, str, min, max);
  }
  return val;
}
