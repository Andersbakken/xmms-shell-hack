#ifndef _XMMS_SHELL_COMMANDS_H_

#define _XMMS_SHELL_COMMANDS_H_

#include <glib.h>


typedef enum _CommandResult CommandResult;

enum _CommandResult {
	TerminateResult,
	OkResult,
	SyntaxErrorResult,
	ErrorResult
};

typedef enum _HelpMode HelpMode;

enum _HelpMode {
	Synopsis,
	Syntax,
	Description
};

typedef enum _CommandFlags CommandFlags;

enum _CommandFlags {
	InteractiveOnly = 0x1
};

typedef CommandResult (*CommandHandler)(gint session_id, gint argc, gchar **argv);
typedef gchar *(*HelpCommand)(HelpMode mode);
typedef gchar *(*CompletionHelper)(gchar **argv, gint arg, gint state);
typedef struct _Command Command;

struct _Command {
	gchar *command;
	CommandHandler handler;
	CompletionHelper completer;
	HelpCommand helper;
	CommandFlags flags;
};

Command *lookup_command(const gchar *command);
gchar **command_completion(gchar *text, int start, int end);
gchar *command_generator(gchar *text, int state);

#endif

