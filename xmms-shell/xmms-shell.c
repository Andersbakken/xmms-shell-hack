#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <xmmsctrl.h>
#include "eval.h"
#include "getline.h"

static struct option long_options[] = {
	{ "session", 1, 0, 'n' },
	{ "eval", 1, 0, 'e' },
	{ "help", 0, 0, 'h' },
	{ 0, 0, 0, 0 }
};

static char *program_name;

static void display_usage(FILE *f)
{
	fprintf(f, "\n");
	fprintf(f, "Usage: %s [OPTIONS]\n", program_name);
	fprintf(f, "\n");
	fprintf(f, "Options:\n");
	fprintf(f, "\n");
	fprintf(f, "  -e [expr], --eval [expr] Evaluate expr and exit\n");
	fprintf(f, "  -h, --help               Display this message and exit\n");
	fprintf(f, "  -n [s], --session [s]    Specify session ID\n");
	fprintf(f, "\n");
	fprintf(f, "If no expression is specified on the command line the program will\n");
	fprintf(f, "switch into interactive mode.\n");
	fprintf(f, "\n");
}

static void eval_loop(int session_id, FILE *in)
{
	char *line;
	int quit = 0;

	while(!quit && (line = getline("xmms-shell> ")))
		eval_command_string(session_id, line, &quit, TRUE);
}

int main(int argc, char **argv)
{
	int opt, optind;
	int session_id = 0;
	char *do_expr = NULL;

	program_name = argv[0];
	while((opt = getopt_long(argc, argv, "n:e:h", long_options, &optind)) != -1) {
		switch(opt) {
			case 'h':
			case 0:
			case ':':
			case '?':
				display_usage(stderr);
				return 0;
			case 'n':
				session_id = atoi(optarg);
				break;
			case 'e':
				do_expr = optarg;
				break;
		}
	}
	if(!xmms_remote_is_running(session_id)) {
		fprintf(stderr, "XMMS is not running under the session identifier ``%d''\n", session_id);
		return 1;
	}
	if(do_expr)
		return eval_command_string(session_id, do_expr, 0, FALSE);
	eval_loop(session_id, stdin);
	return 0;
}

