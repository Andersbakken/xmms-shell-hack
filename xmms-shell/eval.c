#include "eval.h"
#include "commands.h"
#include <stdio.h>

gint eval_command(gint session_id, gchar *expr, gint *quit, gboolean interactive)
{
	Command *command;
	CommandResult result;
	gint argc;
	gchar **argv, *txt;

	g_strdelimit(expr, " \t", ' ');
	argv = g_strsplit(expr, " ", 0);
	for(argc = 0; argv[argc]; argc++);
	if(argc) {
		if(!(command = lookup_command(argv[0]))) {
			fprintf(stderr, "Invalid command: %s\n", argv[0]);
			g_strfreev(argv);
			return 1;
		}
		if(!interactive && (command->flags & InteractiveOnly)) {
			fprintf(stderr, "The `%s' command is available only in interactive mode\n", command->command);
			g_strfreev(argv);
			return 1;
		}
		result = command->handler(session_id, argc, argv);
		if(quit && result == TerminateResult)
			*quit = 1;
		if(result == SyntaxErrorResult && command->helper && 
		   (txt = command->helper(Syntax)))
			fprintf(stderr, "Usage: %s\n", txt);
	}
	g_strfreev(argv);
	return 0;
}

gint eval_command_string(gint session_id, gchar *expr, gint *quit, gboolean interactive)
{
	gchar **commands;
	gint i, q;

	g_strdelimit(expr, "\r\n;", ';');
	commands = g_strsplit(expr, ";", 0);
	for(i = q = 0; !q && commands[i]; i++)
		eval_command(session_id, commands[i], &q, interactive);
	if(quit)
		*quit = q;
	g_strfreev(commands);
	return 0;
}

