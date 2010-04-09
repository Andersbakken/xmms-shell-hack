#include "config.h"
#include "eval.h"
#include "command.h"
#include <stdio.h>
#include <string.h>
#include <cctype>
#include <glib.h>

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

static void tokenize(const string &line, bool &completed, vector<string>& args, vector<string>& raw)
{
	string cur;
	string::const_iterator p = line.begin();
    int pos = 0;

	completed = true;
	while(p != line.end()) {
		cur = "";
		while(isspace(*p)) {
			p++;
            pos++;
        }
		if(p == line.end())
			break;
        raw.push_back(line.substr(pos));
		while(p != line.end() && !isspace(*p)) {
			if(*p == '\\') {
				if(pos++, ++p == line.end()) {
					completed = false;
					break;
				}
				cur += *p++;
                pos++;
			} else if(*p == '"' || *p == '\'')
				cur += dequote(line, p, completed);
			else {
				pos++;
                cur += *p++;
            }
		}
		args.push_back(cur);
	}
}

int eval_command(ScriptContext *scontext, const string& expr, int& quit, bool interactive)
{
	const Command *command;
	CommandContext context(scontext);
	bool completed;

    quit = 0;
	tokenize(expr, completed, context.args, context.raw);
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
		if(context.quit) {
			quit = 1;
        }
		if(context.result_code == COMERR_SYNTAX) {
			fprintf(stderr, "Usage: %s\n", command->get_syntax().c_str());
        }
	}
	return context.result_code;
}

int eval_command_string(ScriptContext *scontext, string& expr, int& quit, bool interactive)
{
	char **commands;
	int i, result = 0;
    char buf[strlen(expr.c_str()) + 1];

    strcpy(buf, expr.c_str());
	g_strdelimit(buf, "\r\n;", ';');
	commands = g_strsplit(buf, ";", 0);
	for(i = quit = 0; !quit && commands[i]; i++)
		result = eval_command(scontext, commands[i], quit, interactive);
	g_strfreev(commands);
	return result;
}

