#include "script.h"
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

InteractiveContext::PromptFormatter::PromptFormatter(const Session& session)
{
    int rate, freq, nch;
    int l, r;

    associate('X', "XMMS-Shell");
    associate('x', "xmms-shell");
    if(session.running()) {
        associate('R', "running");
        associate('p', session.playing() ? "playing" : "not playing");
        associate('u', session.paused() ? "paused" : "not paused");
        if(session.playing()) {
            associate('m', session.paused() ? "paused" : "playing");
        } else {
            associate('m', "stopped");
        }
        session.playback_info(rate, freq, nch);
        associate('a', int_to_string(rate));
        associate('f', int_to_string(freq));
        associate('n', int_to_string(nch));
        associate('t', int_to_string(session.playback_time()));
        associate('T', int_to_string(session.playback_time() / 1000));
        session.volume(l, r);
        associate('l', int_to_string(l));
        associate('r', int_to_string(r));
        associate('b', int_to_string(session.balance()));
#if HAVE_XMMS_REMOTE_IS_REPEAT
        associate('c', session.repeat() ? "(repeat)" : "");
#endif
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
        associate('s', session.shuffle() ? "(shuffle)" : "");
#endif

        Playlist playlist = session.playlist();

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
    string promptvar = sess.running() ? "RUNNING_PS1" : "PS1";
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

