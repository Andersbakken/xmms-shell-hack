#include <stdio.h>
#include <stdlib.h>
#include <xmmsctrl.h>
#include <map>
#include <set>
#include <cctype>
#include "config.h"
#include "command.h"
#include "output.h"

class QuitCommand : public Command
{
public:
	QuitCommand(void) : Command("quit")
	{
		add_alias("exit");
	}
	virtual ~QuitCommand() {}

	virtual void execute(CommandContext& context) const
	{
		context.quit = true;
		context.result_code = 0;
	}

	COM_SYNTAX("QUIT")
	COM_SYNOPSIS("terminate the shell")
	COM_DESCRIPTION(
		"The QUIT command instructs XMMS-Shell to immediately exit with "
		"an exit code of 0."
	)
	COM_RETURN("Always 0")
};

class StatusCommand : public Command
{
public:
	COM_STRUCT(StatusCommand, "status")

	virtual void execute(CommandContext &context) const
	{
        Session session = context.session;
		string title;
		gint32 leftVol, rightVol, pos, rate, freq, nch, t, i;
#if HAVE_XMMS_REMOTE_GET_EQ
		float preamp;
        vector<float> bands;
#endif
        Playlist playlist = session.get_playlist();

		pos = playlist.position();
        title = playlist.title(pos);
        if(session.is_playing()) {
            session.get_playback_info(rate, freq, nch);
            t = session.get_playback_time();
			printf("Playing: %s (%d kbps, %d hz, %d channels)\n", title.size() ? title.c_str() : "<no title>", rate / 1000, freq, nch);
			printf("Time: %02d:%02d.%02d%s\n", t / 60000, (t / 1000) % 60, (t / 10) % 100,
					session.is_paused() ? " (paused)" : "");
		} else
			printf("Current song: %s\n", title.size() ? title.c_str() : "<no track selected>");
        session.get_volume(leftVol, rightVol);
#if HAVE_XMMS_REMOTE_IS_REPEAT
		printf("Repeat mode: %s\n", session.is_repeat() ? "on" : "off");
#endif
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
		printf("Shuffle mode: %s\n", session.is_shuffle() ? "on" : "off");
#endif
		printf("Balance: %d\n", session.get_balance());
		printf("Skin: %s\n", session.get_skin().c_str());
		printf("Left volume: %d\n", leftVol);
		printf("Right volume: %d\n", rightVol);
#if HAVE_XMMS_REMOTE_GET_EQ
        session.get_eq(preamp, bands);
		printf("Equalizer preamp: %.1f\n", preamp);
		printf("Equalizer bands:");
		for(i = 0; i < 10; i++)
			printf("%5d", i);
		printf("\n");
		printf("                  ");
		for(i = 0; i < 10; i++)
			if(bands[i] < 0)
				printf(" %3.1f", bands[i]);
			else
				printf("  %3.1f", bands[i]);
		printf("\n");
#endif
		context.result_code = COMRES_SUCCESS;
	}

	COM_SYNTAX("STATUS")
	COM_SYNOPSIS("display the current status of XMMS")
	COM_DESCRIPTION(
		"The STATUS command queries XMMS for general information, such as the current track, "
		"playback mode, and various other settings."
	)
	COM_RETURN("Always 0")
};

class HelpCommand : public Command
{
	const char *html_target(const char *str) const
	{
		static char buf[256];
		const char *p;
		char *q;

		for(p = str, q = buf; *p; p++)
			if(isalnum(*p))
				*q++ = *p;
		*q = 0;
		return buf;
	}

	const char *html(const char *str) const
	{
		static char *buf;
		const char *p;
		char *q;

		if(buf)
			free(buf);
		buf = (char *)malloc(strlen(str) * 5 + 1);
		for(p = str, q = buf; *p; p++)
			switch(*p) {
				case '<':
					strcpy(q, "&lt;");
					q += 4;
					break;
				case '>':
					strcpy(q, "&gt;");
					q += 4;
					break;
				case '&':
					strcpy(q, "&amp;");
					q += 5;
					break;
				default:
					*q++ = *p;
			}
		*q = 0;
		return buf;
	}

	void generate_help_html(void) const
	{
		vector<CommandReference>::const_iterator p;
		const vector<CommandReference> cl = command_list();
		const Command *com;
		string name;

		set<string> sections;
		map<string, map<string, const Command *> > sl;
		string prev;
		
		for(p = cl.begin(); p != cl.end(); p++)
			sections.insert((*p).get_command()->get_section());
		for(set<string>::const_iterator it = sections.begin(); it != sections.end(); it++)
			sl.insert(pair<string, map<string, const Command *> >(*it, map<string, const Command *>()));
		for(p = cl.begin(); p != cl.end(); p++) {
			name = (*p).get_name();
			com = (*p).get_command();

			sl[com->get_section()].insert(pair<string, const Command *>(name, com));
		}
		printf("<a name=\"comhelp_toc\"><h3>List of Commands</h3>\n");
		printf("<ul>\n");
		for(map<string, map<string, const Command *> >::const_iterator it = sl.begin(); it != sl.end(); it++) {
			string section = (*it).first;

			if(section != prev) {
				if(prev.length())
					printf("</ul>\n");
				prev = section;
				printf("<li><b>%s</b>\n<ul>\n", section.c_str());
			}
			for(map<string, const Command *>::const_iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); it2++) {
				name = (*it2).first;
				com = (*it2).second;
				printf("<li><a href=\"#%s\">%s</a>\n", html_target(com->get_primary_name().c_str()), name.c_str());
			}
		}
		if(prev.length())
			printf("</ul>\n");
		printf("</ul>\n");

		map<string, const Command *> sc;
		
		for(p = cl.begin(); p != cl.end(); p++)
			sc.insert(pair<string, const Command *>((*p).get_command()->get_primary_name(), (*p).get_command()));
		printf("<dl>\n");
		for(map<string, const Command *>::const_iterator it = sc.begin(); it != sc.end(); it++) {
			name = (*it).first;
			com = (*it).second;
			printf("<dt><a name=\"%s\"><a href=\"#comhelp_top\">%s</a>", html_target(name.c_str()), html(name.c_str()));
			printf(" - %s</dt>\n<dd>\n", html(com->get_synopsis().c_str()));
			printf("%s\n<p>\n", html(com->get_description().c_str()));
			if(com->get_aliases().size() > 0) {
				vector<string> a = com->get_aliases();

				printf("<b>Alias%s</b>: ", a.size() - 1 ? "s" : "");
				for(vector<string>::const_iterator q = a.begin(); q != a.end(); q++) {
					if(q != a.begin())
						printf(", ");
					printf("%s", html((*q).c_str()));
				}
				printf("<br>\n");
			}
			printf("<b>Returns</b>: %s\n</dd>\n", html(com->get_return().c_str()));
			printf("<p>\n");
		}
	}

public:
	HelpCommand(void) : Command("help")
	{
		add_alias("?");
	}
	virtual ~HelpCommand() { }

	virtual void execute(CommandContext &context) const
	{
		vector<CommandReference>::const_iterator p;
		const vector<CommandReference> cl = command_list();
		const Command *com;
		string name;

		if(context.args.size() < 2) {
			set<string> sections;
			map<string, map<string, const Command *> > sl;
			string prev;

			for(p = cl.begin(); p != cl.end(); p++)
				sections.insert((*p).get_command()->get_section());
			for(set<string>::const_iterator it = sections.begin(); it != sections.end(); it++)
				sl.insert(pair<string, map<string, const Command *> >(*it, map<string, const Command *>()));
			for(p = cl.begin(); p != cl.end(); p++) {
				name = (*p).get_name();
				com = (*p).get_command();

				sl[com->get_section()].insert(pair<string, const Command *>(name, com));
			}
			for(map<string, map<string, const Command *> >::const_iterator it = sl.begin(); it != sl.end(); it++) {
				string section = (*it).first;

				if(section != prev) {
					if(prev.length())
						printf("\n");
					prev = section;
					printf("%s:\n", section.c_str());
				}
				for(map<string, const Command *>::const_iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); it2++) {
					name = (*it2).first;
					com = (*it2).second;
					printf("  %-16.16s -", name.c_str());
					output_indented(com->get_synopsis().c_str(), 25, 21, 76, stdout);
					printf("\n");
				}
			}
			context.result_code = COMRES_SUCCESS;
			return;
		}
		if(!strcasecmp(context.args[1].c_str(), "html")) {
			generate_help_html();
			return;
		}
		if(!(com = command_lookup(context.args[1]))) {
			printf("Invalid help topic: `%s'\n", context.args[1].c_str());
			printf("Enter `help' with no arguments for a list of available help topics\n");
			return;
		}
		name = com->get_primary_name();
		printf("%s: ", name.c_str());
		output_indented(com->get_synopsis().c_str(), name.length() + 2, name.length() + 2, 75, stdout);
		printf("\n\n");
		printf("Syntax: ");
		output_indented(com->get_syntax().c_str(), 7, 7, 75, stdout);
		printf("\n");
		if(com->get_aliases().size() > 0) {
			vector<string>::const_iterator p;
			
			printf("Aliases:");
			for(p = com->get_aliases().begin(); p != com->get_aliases().end(); p++)
				printf(" %s", (*p).c_str());
			printf("\n");
		}
		printf("\n");
		output_indented(com->get_description().c_str(), 0, 0, 75, stdout);
		printf("\n\n");
		printf("Returns:");
		output_indented(com->get_return().c_str(), 8, 9, 75, stdout);
		printf("\n\n");
	}

	COM_SYNOPSIS("list commands and show their usage")
	COM_SYNTAX("HELP [command]")
	COM_DESCRIPTION(
		"Given no arguments, HELP displays a list of commands and a brief "
		"description for each. If a command name is given as the first argument, "
		"HELP will display the usage of that command and a more in-depth description."
	)
	COM_RETURN("127 if an argument is specified that does not correspond to any command, 0 otherwise")
};

class VersionCommand : public Command
{
public:
	COM_STRUCT(VersionCommand, "version")

	virtual void execute(CommandContext &context) const
	{
		int xver = context.session.get_version();

		printf("XMMS-Shell v%s by Logan Hanks <logan@vt.edu>\n", VERSION);
		printf("Build info: %s %s with %s\n", __TIME__, __DATE__, __VERSION__);
		printf("XMMS: %04X\n", xver);
		context.result_code = COMRES_SUCCESS;
	}

	COM_SYNOPSIS("display XMMS-Shell and XMMS version information")
	COM_SYNTAX("VERSION")
	COM_DESCRIPTION(
		"Displays version information regarding XMMS-Shell, and attempts to obtain and "
		"display version information regarding XMMS (the XMMS version number may or may not mean anything)."
	)
	COM_RETURN("Always 0")
};

class EchoCommand : public Command
{
public:
	EchoCommand(void) : Command("echo") { }
	virtual ~EchoCommand() { }

	virtual void execute(CommandContext &context) const
	{
		for(unsigned i = 1; i < context.args.size(); i++) {
			if(i != 1)
				printf(" ");
			printf("%s", context.args[i].c_str());
		}
		printf("\n");
	}

	COM_SYNOPSIS("display a string")
	COM_SYNTAX("ECHO [str...]")
	COM_DESCRIPTION(
		"Displays all of the given arguments as strings, separating each by a single space, and "
		"followed by a newline.  If no arguments are given, simply displays a newline."
	)
	COM_RETURN("Always 0")
};

class XMMSQuitCommand : public Command
{
public:
	XMMSQuitCommand(void) : Command("xmmsquit") { add_alias("xmmsexit"); }
	virtual ~XMMSQuitCommand() { }

	virtual void execute(CommandContext &cnx) const
	{
        cnx.session.quit();
		cnx.quit = true;
		cnx.result_code = 0;
	}

	COM_SYNOPSIS("force XMMS and XMMS-Shell to exit")
	COM_SYNTAX("XMMSQUIT")
	COM_DESCRIPTION(
		"Forces XMMS to exit, followed by XMMS-Shell exiting.  If you don't want to kill XMMS, "
		"just use the QUIT command instead."
	)
	COM_RETURN("Always 0")
};

class ExecCommand : public Command
{
public:
    ExecCommand(void) : Command("exec") { }
    virtual ~ExecCommand() { }

    virtual void execute(CommandContext& cnx) const
    {
        if(cnx.args.size() < 2) {
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
        system(cnx.raw[1].c_str());
    }

    COM_SYNOPSIS("executes a system command")
    COM_SYNTAX("EXEC <command>")
    COM_DESCRIPTION(
        "Passes its command-line to the system shell by executing /bin/sh -c <command>."
    )
    COM_RETURN("Always 0")
};

static Command *commands[] = {
	new StatusCommand(),
	new QuitCommand(),
	new HelpCommand(),
	new VersionCommand(),
	new EchoCommand(),
	new XMMSQuitCommand(),
    new ExecCommand(),
};

void general_init(void)
{
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

