#ifndef _XMMS_SHELL_EVAL_H_

#define _XMMS_SHELL_EVAL_H_

#include <glib.h>

gint eval_command(gint session_id, gchar *expr, gint *quit, gboolean interactive);
gint eval_command_string(gint session_id, gchar *expr, gint *quit, gboolean interactive);

#endif

