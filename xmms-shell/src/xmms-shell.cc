#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <xmmsctrl.h>
#include "config.h"
#include "command.h"
#include "eval.h"
#include "general.h"
#include "getline.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "volume.h"
#include "window.h"

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

static int eval_loop(int session_id, FILE *in)
{
	int quit = 0, retval = 127;
    ScriptContext *context;

    if(isatty(fileno(in))) {
        context = new InteractiveContext();
    } else {
        context = new FileContext(in);
    }
    context->set_session(Session(session_id));
	//while(!quit && (line = getline("xmms-shell> ")))
    try {
        while(!quit) {
            string line = context->get_line();

		    retval = eval_command_string(context, line, quit, TRUE);
            usleep(100);
        }
    } catch(EOFException ex) {
    }
	if(!quit) {
		printf("\n");
    }
	return retval;
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

	general_init();
	getline_init();
	misc_init();
	playback_init();
	playlist_init();
    script_init();
	volume_init();
	window_init();

	command_init();

	if(do_expr) {
        ScriptContext *context = new StringContext(do_expr);
        string line = context->get_line();
        int quit;

		return eval_command_string(context, line, quit, FALSE);
    }
	return eval_loop(session_id, stdin);
}

