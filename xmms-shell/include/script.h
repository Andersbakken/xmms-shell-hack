#ifndef _XMMS_SHELL_SCRIPT_H_

#define _XMMS_SHELL_SCRIPT_H_

#include "exception.h"
#include "formatter.h"
#include "session.h"

#include <stdio.h>
#include <map>

using namespace std;

class ScriptContext
{
protected:
    map<string,string> env;
    Session sess;

public:
    typedef map<string,string> Env;

    ScriptContext();
    ~ScriptContext();

    const Session& session(void) const;
    void set_session(const Session& session);

    const Env& const_environment(void) const;
    Env& environment(void);

    const string get_env(string key) const;
    void set_env(string key, string value);
    bool unset_env(string key);
    bool has_env(string key) const;

    virtual string get_line(void) = 0;
};

class FileContext : public ScriptContext
{
    FILE *file;

public:
    FileContext(FILE *f);
    ~FileContext();

    virtual string get_line(void);
};

class InteractiveContext : public ScriptContext
{
    class PromptFormatter : public Formatter
    {
    public:
        PromptFormatter(const Session& session);
    };

public:
    InteractiveContext();
    ~InteractiveContext();

    virtual string get_line(void);
};

class EOFException : public Exception
{
public:
    EOFException() : Exception("EOFException") { }
};

class StringContext : public ScriptContext
{
    string str;
    bool end;

public:
    StringContext(const string& s);

    virtual string get_line(void);
};

#endif

