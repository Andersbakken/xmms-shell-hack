#include "getline.h"
#include <stdio.h>

#ifdef HAVE_LIBREADLINE

#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "commands.h"

static gchar *stripwhite(gchar *line)
{
	gchar *p;

	for(; *line && isspace(*line); line++);
	for(p = line + strlen(line) - 1; p >= line && isspace(*p); p--);
	*++p = 0;
	return line;
}

gchar *getline(gchar *prompt)
{
	static gchar *line;
	gchar *s;

	if(!line)
		rl_readline_name = "XMMS-Shell";
	else
		free(line);
	rl_attempted_completion_function = (CPPFunction *)command_completion;
	if(!(line = readline(prompt)))
		return 0;
	s = stripwhite(line);
	if(*s)
		add_history(s);
	return s;
}

#else

gchar *getline(gchar *prompt)
{
	static char line[4096];

	printf("%s", (const char *)prompt);
	fflush(stdout);
	return fgets(line, sizeof(line), stdin);
}

#endif

