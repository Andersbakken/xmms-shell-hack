#include "config.h"
#include "eval.h"
#include "command.h"
#include <stdio.h>

static string dequote(const string &line, string::const_iterator &p, bool &completed)
{
	string arg;
	char qt = *p++;

	while(p != line.end() && *p != qt) {
		if(qt == '"' && *p == '\\') {
			if(++p == line.end()) {
				completed = false;
				break;
			}
		}
		arg += *p++;
	}
	if(p == line.end())
		completed = false;
	else
		p++;
	return arg;
}

static vector<string> tokenize(const string &line, bool &completed)
{
	vector<string> args;
	string cur;
	string::const_iterator p = line.begin();

	completed = true;
	while(p != line.end()) {
		cur = "";
		while(isspace(*p))
			p++;
		if(p == line.end())
			break;
		while(p != line.end() && !isspace(*p)) {
			if(*p == '\\') {
				if(++p == line.end()) {
					completed = false;
					break;
				}
				cur += *p++;
			} else if(*p == '"' || *p == '\'')
				cur += dequote(line, p, completed);
			else
				cur += *p++;
		}
		args.push_back(cur);
	}
	return args;
}

gint eval_command(gint session_id, gchar *expr, gint *quit, gboolean interactive)
{
	const Command *command;
	CommandContext context(session_id);
	bool completed;

	context.args = tokenize(expr, completed);
	if(!completed) {
		fprintf(stderr, "Incomplete command.  Multi-line entry of commands not yet implemented.\n");
		return COMERR_SYNTAX;
	}
	if(context.args.size()) {
		if(!(command = command_lookup(context.args[0]))) {
			fprintf(stderr, "Invalid command: %s\n", context.args[0].c_str());
			return COMERR_BADCOMMAND;
		}
		if(!interactive && (command->get_flags() & COMFLAG_INTERACTIVE)) {
			fprintf(stderr, "The `%s' command is available only in interactive mode\n", command->get_primary_name().c_str());
			return COMERR_NOTINTERACTIVE;
		}
		command->execute(context);
		if(quit && context.quit)
			*quit = 1;
		if(context.result_code == COMERR_SYNTAX)
			fprintf(stderr, "Usage: %s\n", command->get_syntax().c_str());
	}
	return context.result_code;
}

gint eval_command_string(gint session_id, gchar *expr, gint *quit, gboolean interactive)
{
	gchar **commands;
	gint i, q, result = 0;

	g_strdelimit(expr, "\r\n;", ';');
	commands = g_strsplit(expr, ";", 0);
	for(i = q = 0; !q && commands[i]; i++)
		result = eval_command(session_id, commands[i], &q, interactive);
	if(quit)
		*quit = q;
	g_strfreev(commands);
	return result;
}

