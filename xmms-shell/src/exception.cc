#include "config.h"
#include "exception.h"

Exception::Exception(const string& _type, const string& _message)
    : type(_type), message(_message)
{
}

Exception::~Exception()
{
}

string Exception::to_string(void) const
{
    string result(type);

    if(message.size()) {
        result.append(": ");
        result.append(message);
    }
    return result;
}

