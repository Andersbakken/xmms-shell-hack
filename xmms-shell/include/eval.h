#ifndef _XMMS_SHELL_EVAL_H_

#define _XMMS_SHELL_EVAL_H_

#include "script.h"

int eval_command(ScriptContext *context, string& expr, int& quit, bool interactive);
int eval_command_string(ScriptContext *context, string& expr, int& quit, bool interactive);

#endif

