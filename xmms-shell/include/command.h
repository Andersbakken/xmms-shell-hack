#ifndef _XMMS_SHELL_COMMAND_H_

#define _XMMS_SHELL_COMMAND_H_

#include <vector>
#include <string>

#define COMFLAG_INTERACTIVE	0x1

#define COMERR_NOEFFECT	123
#define COMERR_NOTINTERACTIVE	124
#define COMERR_SYNTAX	125
#define COMERR_BADCOMMAND	126
#define COMERR_UNKNOWN	127

#define COMRES_SUCCESS	0

#define COM_STRUCT(x, y)	\
	x(void) : Command(y) { } \
	virtual ~x() { }

#define COM_SYNTAX(x)	virtual const string get_syntax(void) const { return x; }
#define COM_SYNOPSIS(x)	virtual const string get_synopsis(void) const { return x; }
#define COM_RETURN(x)	virtual const string get_return(void) const { return x; }
#define COM_DESCRIPTION(x)	virtual const string get_description(void) const { return x; }

class CommandContext
{
public:
	bool quit;
	int session_id;
	int result_code;
	vector<string> args;

	CommandContext(int _session_id) : session_id(_session_id), quit(false), result_code(0) { }
	void add_arg(const string &arg) { args.push_back(arg); }
};

class Command
{
	string primary_name;
	vector<string> aliases;
public:
	Command(const string& _primary_name) : primary_name(_primary_name) { }
	virtual ~Command() { }
	const string &get_primary_name(void) const { return primary_name; }
	void add_alias(const string& alias) { aliases.push_back(alias); }
	const vector<string> &get_aliases(void) const { return aliases; }
	virtual void execute(CommandContext& context) const = 0;
	virtual int get_flags(void) const { return 0; }
	COM_SYNTAX("<not specified>")
	COM_SYNOPSIS("<no synopsis specified>")
	COM_DESCRIPTION("<no description specified>")
	COM_RETURN("<not specified>")
	virtual const string get_section(void) const { return "General"; }
};

class CommandReference
{
	string name;
	Command *command;
public:
	CommandReference(const string &_name, Command *_command) : name(_name) { command = _command; }
	CommandReference(const CommandReference &r) : name(r.name) { command = r.command; }
	const string &get_name(void) const { return name; }
	const Command *get_command(void) const { return command; }
	int operator<(const CommandReference& c) const { return name < c.name; }
};  
    
void command_init(void);
void command_add(Command *com);
const Command *command_lookup(const string &name);
const vector<CommandReference> &command_list(void);

#endif

