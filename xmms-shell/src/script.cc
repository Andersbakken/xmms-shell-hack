#include "script.h"
#include "command.h"
#include "getline.h"
#include "playlist.h"
#include "util.h"
#include <glib.h>
#include <string.h>

ScriptContext::ScriptContext()
{
}

ScriptContext::~ScriptContext()
{
}

const Session& ScriptContext::session(void) const
{
    return sess;
}

void ScriptContext::set_session(const Session& session)
{
    sess = session;
}

const ScriptContext::Env& ScriptContext::const_environment(void) const
{
    return env;
}

ScriptContext::Env& ScriptContext::environment(void)
{
    return env;
}

const string ScriptContext::get_env(string key) const
{
    Env::const_iterator v = env.find(key);

    if(v == env.end()) {
        return "";
    }
    return v->second;
}

void ScriptContext::set_env(string key, string value)
{
    env[key] = value;
}

bool ScriptContext::unset_env(string key)
{
    return env.erase(key) > 0;
}

bool ScriptContext::has_env(string key) const
{
    return env.find(key) != env.end();
}

FileContext::FileContext(FILE *f)
{
    file = f;
}

FileContext::~FileContext()
{
    fclose(file);
}

string FileContext::get_line(void)
{
    char line[4096];

    if(!fgets(line, sizeof(line), file)) {
        throw EOFException();
    }
    return line;
}

InteractiveContext::InteractiveContext()
{
    set_env("PS1", "%x (%R)> ");
    set_env("RUNNING_PS1", "[%i/%N] %S (%m)> ");
}

InteractiveContext::~InteractiveContext()
{
}

InteractiveContext::PromptFormatter::PromptFormatter(Session& session)
{
    guint32 rate, freq, nch;
    guint32 l, r;

    associate('X', "XMMS-Shell");
    associate('x', "xmms-shell");
    if(session.is_running()) {
        associate('R', "running");
        associate('p', session.is_playing() ? "playing" : "not playing");
        associate('u', session.is_paused() ? "paused" : "not paused");
        if(session.is_playing()) {
            associate('m', session.is_paused() ? "paused" : "playing");
        } else {
            associate('m', "stopped");
        }
        session.get_playback_info(rate, freq, nch);
        associate('a', int_to_string(rate));
        associate('f', int_to_string(freq));
        associate('n', int_to_string(nch));
        associate('t', int_to_string(session.get_playback_time()));
        associate('T', int_to_string(session.get_playback_time() / 1000));
        session.get_volume(l, r);
        associate('l', int_to_string(l));
        associate('r', int_to_string(r));
        associate('b', int_to_string(session.get_balance()));
#if HAVE_XMMS_REMOTE_IS_REPEAT
        associate('c', session.is_repeat() ? "(repeat)" : "");
#endif
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
        associate('s', session.is_shuffle() ? "(shuffle)" : "");
#endif

        Playlist playlist = session.get_playlist();

        associate('i', int_to_string(playlist.position()));
        associate('F', playlist.current_filename());
        associate('S', playlist.current_title());
        associate('N', int_to_string(playlist.length()));
    } else {
        associate('R', "not running");
    }
}

string InteractiveContext::get_line(void)
{
    PromptFormatter formatter(sess);
    string promptvar = sess.is_running() ? "RUNNING_PS1" : "PS1";
    string promptval = has_env(promptvar) ? get_env(promptvar) : get_env("PS1");
    string prompt = formatter.expand(promptval);
    char *tmp = g_new(char, prompt.size() + 1);

    strcpy(tmp, prompt.c_str());

    char *buf = getline(tmp);

    g_free(tmp);
    if(!buf) {
        throw EOFException();
    }

    string line(buf);

    //g_free(buf);
    return line;
}

StringContext::StringContext(const string& s) : str(s), end(false)
{
}

StringContext::~StringContext()
{
}

string StringContext::get_line(void)
{
    if(end) {
        throw EOFException();
    }
    end = true;
    return str;
}

class SetCommand : public Command
{
    void display(CommandContext& cnx) const
    {
        const ScriptContext::Env& e = cnx.context->const_environment();

        for(ScriptContext::Env::const_iterator i = e.begin(); i != e.end(); i++) {
            printf("%s=%s\n", i->first.c_str(), i->second.c_str());
        }
    }

public:
    SetCommand(void) : Command("set") { }
    virtual ~SetCommand() { }

    virtual void execute(CommandContext& cnx) const
    {
        if(cnx.args.size() < 2) {
            display(cnx);
        } else {
            for(vector<string>::const_iterator i = cnx.args.begin(); ++i != cnx.args.end(); ) {
                const string s = *i;
                unsigned p1 = s.find('=');
                string name = s.substr(0, p1);
                string value = s.substr(p1 + 1);

                cnx.context->set_env(name, value);
            }
        }
    }

    COM_SYNOPSIS("set or display environment variables")
    COM_SYNTAX("SET [<name>[=<value>]]...")
    COM_DESCRIPTION(
            "If given no arguments, SET will display the current values of all defined "
            "environment variables.  Otherwise, each argument should be of the form "
            "<name> or <name>=<value>.  The former form is equivalent to <name>= (assigning "
            "an empty value to that particular variable."
    )
    COM_RETURN("Always 0")
};

class UnsetCommand : public Command
{
public:
    UnsetCommand(void) : Command("unset") { }
    virtual ~UnsetCommand() { }

    virtual void execute(CommandContext& cnx) const
    {
        if(cnx.args.size() < 2) {
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
        for(vector<string>::const_iterator i = cnx.args.begin(); ++i != cnx.args.end(); ) {
            cnx.context->unset_env(*i);
        }
    }

    COM_SYNOPSIS("unset environment variables")
    COM_SYNTAX("UNSET <name>...")
    COM_DESCRIPTION(
        "For each argument that is the name of an environment variable, UNSET removes that "
        "variable from the environment."
    )
    COM_RETURN("Always 0")
};

static Command *commands[] = {
    new SetCommand(),
    new UnsetCommand(),
};

void script_init(void)
{
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

