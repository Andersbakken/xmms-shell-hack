#include <stdio.h>
#include <xmmsctrl.h>
#include "config.h"
#include "command.h"
#include "window.h"

#define SECTION virtual const string get_section(void) const { return "Window Control"; }

class WindowCommand : public Command
{
public:
	COM_STRUCT(WindowCommand, "window")

	virtual void execute(CommandContext &cnx) const
	{
        Window window = cnx.session.get_window();
		int apply, status, command;

		if(cnx.args.size() < 3)  {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		
		if(!strncasecmp("hide", cnx.args[2].c_str(), cnx.args[2].size()))
			command = 0;
		else if(!strncasecmp("show", cnx.args[2].c_str(), cnx.args[2].size()))
			command = 1;
		else if(!strncasecmp("toggle", cnx.args[2].c_str(), cnx.args[2].size()))
			command = 2;
		else {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		
		if(!strncasecmp("all", cnx.args[1].c_str(), cnx.args[1].size()))
			apply = 7;
		else if(!strncasecmp("equalizer", cnx.args[1].c_str(), cnx.args[1].size()))
			apply = 1;
		else if(!strncasecmp("main", cnx.args[1].c_str(), cnx.args[1].size()))
			apply = 2;
		else if(!strncasecmp("playlist", cnx.args[1].c_str(), cnx.args[1].size()))
			apply = 4;
		else {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		
        status = window.equalizer() |
            (window.main() << 1) |
            (window.playlist() << 2);

		switch(command)
		{
			case 0:
				status &= ~apply;
				break;
			case 1:
				status |= apply;
				break;
			case 2:
				status ^= ~(status & ~apply) | apply;
				break;
		}

        window.show_equalizer(status & 1);
        window.show_main((status >> 1) & 1);
        window.show_playlist((status >> 2) & 1);
		cnx.result_code = status & 7;
	}

	COM_SYNOPSIS("show or hide XMMS windows")
	COM_SYNTAX("WINDOW ALL|EQUALIZER|MAIN|PLAYLIST HIDE|SHOW|TOGGLE")
	COM_DESCRIPTION(
		"The WINDOW command hides and shows XMMS windows.  The first argument must "
		"be the window to operate on (the main, playlist, or equalizer window, or "
		"all three) and the second argument must be a command (HIDE, SHOW, or TOGGLE).  "
		"The TOGGLE command will toggle the display of a window."
	)
	COM_RETURN(
		"A 3-bit integer, the bits of which represent the display status of each window.  "
		"The least significant bit represents the equalizer window, the second bit "
		"the main window, and the third bit the playlist window."
	)
	SECTION
};

class PreferencesCommand : public Command
{
public:
	COM_STRUCT(PreferencesCommand, "preferences")
	
	virtual void execute(CommandContext &cnx) const
	{
        cnx.session.get_window().preferences();
		cnx.result_code = 0;
	}

	COM_SYNOPSIS("cause XMMS to display its preferences dialog")
	COM_SYNTAX("PREFERENCES")
	COM_DESCRIPTION("Causes XMMS to display its preferences dialog.")
	COM_RETURN("Always 0")
	SECTION
};

class EjectCommand : public Command
{
public:
	COM_STRUCT(EjectCommand, "eject")
	
	virtual void execute(CommandContext &cnx) const
	{
        cnx.session.get_window().eject();
		cnx.result_code = 0;
	}

	COM_SYNOPSIS("cause XMMS to display its file load dialog")
	COM_SYNTAX("EJECT")
	COM_DESCRIPTION("Causes XMMS to display its file load dialog.")
	COM_RETURN("Always 0")
	SECTION
};

static Command *commands[] = {
	new WindowCommand(),
	new PreferencesCommand(),
	new EjectCommand(),
};

void window_init(void)
{
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

Window::Window(const Session& _session) : session(_session)
{
}

Window::Window(const Window& window) : session(window.session)
{
}

Window::~Window()
{
}

void Window::ensure_running(void) const
{
    session.ensure_running();
}

bool Window::main(void) const
{
    ensure_running();
    return xmms_remote_is_main_win(session.get_id());
}

bool Window::playlist(void) const
{
    ensure_running();
    return xmms_remote_is_pl_win(session.get_id());
}

bool Window::equalizer(void) const
{
    ensure_running();
    return xmms_remote_is_eq_win(session.get_id());
}

void Window::show_main(bool value) const
{
    if(main() != value) {
        toggle_main();
    }
}

void Window::show_playlist(bool value) const
{
    if(playlist() != value) {
        toggle_playlist();
    }
}

void Window::show_equalizer(bool value) const
{
    if(equalizer() != value) {
        toggle_equalizer();
    }
}

bool Window::toggle_main(void) const
{
    ensure_running();

    bool v = !main();

    xmms_remote_main_win_toggle(session.get_id(), v);
    return v;
}

bool Window::toggle_playlist(void) const
{
    ensure_running();

    bool v = !playlist();

    xmms_remote_pl_win_toggle(session.get_id(), v);
    return v;
}

bool Window::toggle_equalizer(void) const
{
    ensure_running();

    bool v = !equalizer();

    xmms_remote_eq_win_toggle(session.get_id(), v);
    return v;
}

void Window::eject(void) const
{
    ensure_running();
    xmms_remote_eject(session.get_id());
}

void Window::preferences(void) const
{
    ensure_running();
    xmms_remote_show_prefs_box(session.get_id());
}

