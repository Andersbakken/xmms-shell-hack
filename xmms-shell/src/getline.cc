#include "config.h"
#include "getline.h"
#include <stdio.h>
#include <string>

#ifdef HAVE_LIBREADLINE

#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "command.h"

typedef char *(*CompletionHelper)(char **argv, int arg, int state);

static gchar *stripwhite(gchar *line)
{
	char *p;

	for(; *line && isspace(*line); line++);
	for(p = line + strlen(line) - 1; p >= line && isspace(*p); p--);
	*++p = 0;
	return line;
}

static char *build_command_list(char **argv, int arg, int state)
{
	static int index;
	char *com = argv[arg];
	static char **commands;

	if(!commands) {
		const vector<CommandReference> cl = command_list();
		int i = 0;

		commands = (char **)malloc((cl.size() + 1) * sizeof(char *));
		for(vector<CommandReference>::const_iterator p = cl.begin(); p != cl.end(); p++)
			commands[i++] = g_strdup((*p).get_name().c_str());
		commands[i] = 0;
	}
	if(!com)
		com = "";
	if(!state)
		index = 0;
	else
		if(commands[index])
			index++;
	while(commands[index] && g_strncasecmp(commands[index], com, strlen(com)))
		index++;
	if(!commands[index])
		return 0;
	return commands[index];
}

static void free_vector(char **vec)
{
	int i;

	for(i = 0; vec[i]; i++)
		free(vec[i]);
	free(vec);
}

static char **split_text(char *text, int start, int end, int *index)
{
	char **argv;
	int i, j;
	int argc, args;

	argc = 0;
	args = 16;
	argv = (char **)malloc(args * sizeof(char *));
	*index = -1;
	for(i = 0; text[i]; ) {
		for(; text[i] && isspace(text[i]); i++);
		if(!text[i])
			break;
		for(j = i + 1; text[j] && !isspace(text[j]); j++);
		if(argc >= args - 1) {
			args *= 2;
			argv = (char **)realloc(argv, args * sizeof(char *));
		}
		argv[argc] = (char *)malloc(j - i + 1);
		memcpy(argv[argc], text + i, j - i);
		argv[argc][j - i] = 0;
		if(i <= start && j >= end)
			*index = argc;
		argc++;
		i = j;
	}
	argv[argc] = 0;
	if(*index == -1)
		*index = argc;
	return argv;
}

static char *filename_quote(char *text, int match_type, char *quote_ptr)
{
	char *buf = (char *)malloc(strlen(text) * 2 + 1);
	char *p, *q;

	for(p = text, q = buf; *p; p++) {
		if(strchr(rl_filename_quote_characters, *p))
			*q++ = '\\';
		*q++ = *p;
	}
	*q = 0;
	return buf;
}

static char *filename_dequote(char *text, int quote_char)
{
	char *buf = (char *)malloc(strlen(text) + 1);
	char *p, *q;

	for(p = text, q = buf; *p; p++) {
		if(*p == '\\')
			p++;
		*q++ = *p;
	}
	*q = 0;
	return buf;
}

#if 0
/* Filename quoting for completion. */
/* A function to strip quotes that are not protected by backquotes.  It
 * allows single quotes to appear within double quotes, and vice versa.
 * It should be smarter. */
static char * bash_dequote_filename (char *text, int quote_char)
{
	char *ret, *p, *r;
	int l, quoted;
	
	l = strlen (text);
	ret = (char*)malloc (l + 1);
	for (quoted = quote_char, p = text, r = ret; p && *p; p++)
	{
		/* Allow backslash-quoted characters to pass through unscathed. */
		if (*p == '\\')
		{
			*r++ = *++p;
			if (*p == '\0')
				break;
			continue;
		}
		/* Close quote. */
		if (quoted && *p == quoted)
		{
			quoted = 0;
			continue;
		}
		/* Open quote. */
		if (quoted == 0 && (*p == '\'' || *p == '"'))
		{
			quoted = *p;
			continue;
		}
		*r++ = *p;
	}
	*r = '\0';
	return ret;
}
#endif

char **command_completion(const char *text, int start, int end)
{
	int index;
	char **argv;
	CompletionHelper helper;
	char **completions, *str;
	int ncompletions, scompletions;

	argv = split_text(rl_line_buffer, start, end, &index);
	if(index == 0)
		helper = build_command_list;
	else {
		free_vector(argv);
		goto filename_complete;
	}
	ncompletions = 0;
	scompletions = 16;
	completions = (char **)malloc(scompletions * sizeof(char *));
	while(1) {
		if(ncompletions >= scompletions - 1) {
			scompletions *= 2;
			completions = (char **)realloc(completions, scompletions * sizeof(char *));
		}
		str = helper(argv, index, ncompletions);
		if(!str)
			break;
		completions[ncompletions++] = strcpy((char *)malloc(strlen(str) + 1), str);
	}
	free_vector(argv);
	if(!ncompletions) {
		free(completions);
		goto filename_complete;
	}
	completions[ncompletions] = 0;
	return completions;

filename_complete:
	return 0;
}

static int char_is_quoted(char *text, int index)
{
	return index && text[index - 1] == '\\' && !char_is_quoted(text, index - 1);
}

gchar *getline(char *prompt)
{
	static char *line;
	char *s;

	if(line)
		free(line);
	rl_filename_completion_desired = 1;
	rl_filename_quoting_desired = 1;
	if(!(line = readline(prompt)))
		return 0;
	s = stripwhite(line);
	if(*s)
		add_history(s);
	return s;
}

static void getline_rl_init(void)
{
	rl_readline_name = "XMMS-Shell";
	rl_attempted_completion_function = /*(CPPFunction *)*/command_completion;
	rl_filename_completion_desired = 1;
	rl_filename_quoting_desired = 1;
	rl_completer_word_break_characters = " \t\n\"'";
	rl_completer_quote_characters = "\"'";
	rl_filename_quote_characters = " \t\n\\\"'>;|&()*?[]~";
	rl_filename_quoting_function = filename_quote;
	rl_filename_dequoting_function = filename_dequote;
	rl_char_is_quoted_p = char_is_quoted;
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

void getline_init(void)
{
#if HAVE_LIBREADLINE
	getline_rl_init();
#endif
}

