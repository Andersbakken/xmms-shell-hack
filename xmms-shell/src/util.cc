#include "util.h"
#include <sstream>

string int_to_string(int n)
{
    stringstream s;
    string str;

    s << n;
    s >> str;
    return str;
}

