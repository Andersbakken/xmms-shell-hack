#include <glib.h>
#include <string.h>
#include "formatter.h"

Formatter::Formatter()
{
    associate('%', "%");
}

Formatter::~Formatter()
{
}

void Formatter::associate(char id, const string value)
{
    dissociate(id);
    values[id] = string(value);
}

void Formatter::dissociate(char id)
{
    values[id] = string();
}

string Formatter::expand(const string format) const
{
    string result;

    for(int i = 0; i < format.size(); i++) {
        if(format[i] == '%') {
            result.append(values[(char) format[++i]]);
        } else {
            result.append(1, format[i]);
        }
    }
    return result;
}

