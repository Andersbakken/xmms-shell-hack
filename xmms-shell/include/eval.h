#ifndef _XMMS_SHELL_EVAL_H_

#define _XMMS_SHELL_EVAL_H_

#include "session.h"

int eval_command(const Session& session, char *expr, int& quit, bool interactive);
int eval_command_string(const Session& session, char *expr, int& quit, bool interactive);

#endif

