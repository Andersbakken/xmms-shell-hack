#ifndef _XMMS_SHELL_FORMATTER_H_

#define _XMMS_SHELL_FORMATTER_H_

#include <string>

using namespace std;

class Formatter
{
	string values[256];
public:
    Formatter();
    ~Formatter();
    void associate(char id, const string value);
    void dissociate(char id);
    string expand(const string format) const;
};

#endif

