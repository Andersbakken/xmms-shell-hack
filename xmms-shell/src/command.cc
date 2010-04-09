#include "config.h"
#include "command.h"
#include <strings.h>
#include <algorithm>

static vector<CommandReference> commands;

void command_add(Command *com)
{
	const vector<string> s = com->get_aliases();

	commands.push_back(CommandReference(com->get_primary_name(), com));
	for(vector<string>::const_iterator p = s.begin(); p != s.end(); p++)
		commands.push_back(CommandReference(*p, com));
}

void command_init(void)
{
	sort(commands.begin(), commands.end());
}

const Command *command_lookup(const string &name)
{
	vector<CommandReference>::iterator p;

	for(p = commands.begin(); p != commands.end(); p++)
		if(!strcasecmp((*p).get_name().c_str(), name.c_str()))
			return (*p).get_command();
	for(p = commands.begin(); p != commands.end(); p++)
		if(!strncasecmp((*p).get_name().c_str(), name.c_str(), name.length()))
			return (*p).get_command();
	return 0;
}

const vector<CommandReference> &command_list(void)
{
	return commands;
}

