#ifndef _XMMS_SHELL_EXCEPTION_H_

#define _XMMS_SHELL_EXCEPTION_H_

#include <string>

using namespace std;

class Exception
{
    string type;
    string message;

public:
    Exception(const string& type, const string& message = "");
    ~Exception();

    virtual string to_string(void) const;
};

#endif

