#include "ft_ping.h"

void error(int status, const char *format, ...) {
  va_list ap;

  fprintf(stderr, "%s: ", program_invocation_short_name);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  if (status)
    exit(status);
}

long parse_long(char const *const str, const char *const msg, const long min, const long max) {
  char *endptr;

  if (str == NULL || *str == '\0') {
    error(1, "%s: %s", msg, str);
  }
  long val = strtol(str, &endptr, 10);
  if (errno || str == endptr || (endptr && *endptr)) {
    error(1, "%s : %s", msg, endptr);
  }
  if (val < min || max < val) {
    error(1, "%s: '%s': out of range: %lu <= value <= %lu", msg, str, min, max);
  }
  return val;
}
