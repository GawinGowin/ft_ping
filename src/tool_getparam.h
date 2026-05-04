#ifndef TOOL_GETPARAM_H
#define TOOL_GETPARAM_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ft_ping.h"

#include <arpa/inet.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "shared/shared_error.h"
#include "shared/shared_parse.h"

void tool_parse_args(int *argc, char ***argv, t_ping_config *config);

#endif /* TOOL_GETPARAM_H */
