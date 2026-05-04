#include "shared/shared_parse.h"

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
  long val = strtol(str, &endptr, 0);
  if (errno || str == endptr || (endptr && *endptr)) {
    errorfn(1, "%s : %s\n", msg, endptr);
  }
  if (val < min || max < val) {
    errorfn(1, "%s: '%s': out of range: %lu <= value <= %lu\n", msg, str, min, max);
  }
  return val;
}
