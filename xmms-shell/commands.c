#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <xmmsctrl.h>

#if HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#define COMMAND(N)	static CommandResult N(gint session_id, gint argc, gchar **argv)
#define COMPLETION_HELPER(N)	static gchar *N(gchar **argv, gint arg, gint state)
#define HELP_COMMAND(N)	static gchar *N(HelpMode mode)

COMMAND(command_quit);
COMMAND(command_help);
COMMAND(command_status);
COMMAND(command_pause);
COMMAND(command_stop);
COMMAND(command_play);
COMMAND(command_forward);
COMMAND(command_backward);
COMMAND(command_jump);
COMMAND(command_volume_up);
COMMAND(command_volume_down);
COMMAND(command_volume);
COMMAND(command_clear);
COMMAND(command_playlist);
COMMAND(command_load);
COMMAND(command_save);
COMMAND(command_eject);
COMMAND(command_version);
COMMAND(command_window);
COMMAND(command_preferences);
COMMAND(command_repeat);
COMMAND(command_shuffle);
COMMAND(command_fade);
COMMAND(command_balance);
COMMAND(command_fakepause);
COMMAND(command_resetdevice);
#if HAVE_XMMS_REMOTE_GET_EQ_PREAMP
COMMAND(command_preamp);
#endif
#if HAVE_XMMS_REMOTE_GET_EQ_BAND
COMMAND(command_band);
#endif

COMPLETION_HELPER(complete_volume_modify);
COMPLETION_HELPER(complete_help);

HELP_COMMAND(help_quit);
HELP_COMMAND(help_help);
HELP_COMMAND(help_status);
HELP_COMMAND(help_pause);
HELP_COMMAND(help_stop);
HELP_COMMAND(help_play);
HELP_COMMAND(help_forward);
HELP_COMMAND(help_backward);
HELP_COMMAND(help_jump);
HELP_COMMAND(help_volume_up);
HELP_COMMAND(help_volume_down);
HELP_COMMAND(help_volume);
HELP_COMMAND(help_clear);
HELP_COMMAND(help_playlist);
HELP_COMMAND(help_load);
HELP_COMMAND(help_save);
HELP_COMMAND(help_eject);
HELP_COMMAND(help_version);
HELP_COMMAND(help_window);
HELP_COMMAND(help_preferences);
HELP_COMMAND(help_repeat);
HELP_COMMAND(help_shuffle);
HELP_COMMAND(help_fade);
HELP_COMMAND(help_balance);
HELP_COMMAND(help_fakepause);
HELP_COMMAND(help_resetdevice);
#if HAVE_XMMS_REMOTE_GET_EQ_PREAMP
HELP_COMMAND(help_preamp);
#endif
#if HAVE_XMMS_REMOTE_GET_EQ_BAND
HELP_COMMAND(help_band);
#endif

static Command commands[] = {
	{ "+", command_volume_up, complete_volume_modify, help_volume_up, 0 },
	{ "-", command_volume_down, complete_volume_modify, help_volume_down, 0 },
	{ "backward", command_backward, 0, help_backward, 0 },
	{ "balance", command_balance, 0, help_balance, 0 },
#if HAVE_XMMS_REMOTE_GET_EQ_BAND
	{ "band", command_band, 0, help_band, 0 },
#endif
	{ "clear", command_clear, 0, help_clear, 0 },
	{ "eject", command_eject, 0, help_eject, 0 },
	{ "fade", command_fade, 0, help_fade, 0 },
	{ "fakepause", command_fakepause, 0, help_fakepause, InteractiveOnly },
	{ "forward", command_forward, 0, help_forward, 0 },
	{ "help", command_help, complete_help, help_help, 0 },
	{ "jump", command_jump, 0, help_jump, 0 },
	{ "list", command_playlist, 0, help_playlist, 0 },
	{ "load", command_load, 0, help_load, 0 },
	{ "pause", command_pause, 0, help_pause, 0 },
	{ "play", command_play, 0, help_play, 0 },
	{ "playlist", command_playlist, 0, help_playlist, 0 },
#if HAVE_XMMS_REMOTE_GET_EQ_PREAMP
	{ "preamp", command_preamp, 0, help_preamp, 0 },
#endif
	{ "preferences", command_preferences, 0, help_preferences, 0 },
	{ "quit", command_quit, 0, help_quit, 0 },
	{ "repeat", command_repeat, 0, help_repeat, 0 },
	{ "resetdevice", command_resetdevice, 0, help_resetdevice, 0 },
	{ "save", command_save, 0, help_save, 0 },
	{ "shuffle", command_shuffle, 0, help_shuffle, 0 },
	{ "status", command_status, 0, help_status, 0 },
	{ "stop", command_stop, 0, help_stop, 0 },
	{ "version", command_version, 0, help_version, 0 },
	{ "volume", command_volume, 0, help_volume, 0 },
	{ "window", command_window, 0, help_window, 0 },
	{ 0, 0, 0, 0 }
};

Command *lookup_command(const gchar *command)
{
	Command *com;

	for(com = commands; com->command && g_strcasecmp(com->command, command); com++);
	if(com->command)
		return com;
	for(com = commands; com->command && g_strncasecmp(com->command, command, strlen(command)); com++);
	if(com->command)
		return com;
	return 0;
}

static gchar **get_playlist_filenames(int session_id, int *length)
{
	int len = xmms_remote_get_playlist_length(session_id), i;
	gchar **playlist = 0;

	if(length) *length = len;
	if(len) {
		playlist = g_new(gchar *, len + 1);
		for(i = 0; i < len; i++)
			playlist[i] = xmms_remote_get_playlist_file(session_id, i);
		playlist[len] = 0;
	}
	return playlist;
}

static gchar **get_playlist_titles(int session_id, int *length)
{
	int len = xmms_remote_get_playlist_length(session_id), i;
	gchar **playlist = 0;

	if(length) *length = len;
	if(len) {
		playlist = g_new(gchar *, len + 1);
		for(i = 0; i < len; i++)
			playlist[i] = xmms_remote_get_playlist_title(session_id, i);
		playlist[len] = 0;
	}
	return playlist;
}

static void output_indented(const gchar *text, gint start, gint indent, gint max, FILE *f)
{
	int i, j, pos;

	i = 0;
	pos = start;
	while(text[i]) {
		while(text[i] && isspace(text[i]))
			i++;
		if(!text[i])
			break;
		while(pos < indent) {
			fprintf(f, " ");
			pos++;
		}
		for(j = i + 1; text[j] && !isspace(text[j]); j++);
		if(pos != indent && j - i + pos + 1 >= max) {
			fprintf(f, "\n");
			pos = 0;
			continue;
		}
		if(pos != indent) {
			fprintf(f, " ");
			pos++;
		}
		pos += j - i;
		while(i < j)
			fprintf(f, "%c", text[i++]);
	}
}

static gchar *build_command_list(gchar **argv, int arg, int state)
{
	static int index;
	char *com = argv[arg];

	if(!com)
		com = "";
	if(!state)
		index = 0;
	else
		if(commands[index].command)
			index++;
	while(commands[index].command && g_strncasecmp(commands[index].command, com, strlen(com)))
		index++;
	if(!commands[index].command)
		return 0;
	return commands[index].command;
}

static void free_vector(gchar **vec)
{
	int i;

	for(i = 0; vec[i]; i++)
		free(vec[i]);
	free(vec);
}

static gchar **split_text(gchar *text, gint start, gint end, gint *index)
{
	gchar **argv;
	gint i, j;
	gint argc, args;

	argc = 0;
	args = 16;
	argv = (gchar **)malloc(args * sizeof(gchar *));
	*index = -1;
	for(i = 0; text[i]; ) {
		for(; text[i] && isspace(text[i]); i++);
		if(!text[i])
			break;
		for(j = i + 1; text[j] && !isspace(text[j]); j++);
		if(argc >= args - 1) {
			args *= 2;
			argv = (gchar **)realloc(argv, args * sizeof(gchar *));
		}
		argv[argc] = malloc(j - i + 1);
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

#if HAVE_LIBREADLINE

gchar **command_completion(gchar *text, int start, int end)
{
	gint index;
	gchar **argv;
	Command *com;
	CompletionHelper helper;
	gchar **completions, *str;
	gint ncompletions, scompletions;

	argv = split_text(rl_line_buffer, start, end, &index);
	/*
	if(argv[0] == 0) {
		free(argv);
		return 0;
	}
	*/
	if(index == 0)
		helper = build_command_list;
	else {
		com = lookup_command(argv[0]);
		if(!com || !com->completer) {
			free_vector(argv);
			return 0;
		}
		helper = com->completer;
	}
	ncompletions = 0;
	scompletions = 16;
	completions = (gchar **)malloc(scompletions * sizeof(gchar *));
	while(1) {
		if(ncompletions >= scompletions - 1) {
			scompletions *= 2;
			completions = (gchar **)realloc(completions, scompletions * sizeof(gchar *));
		}
		str = helper(argv, index, ncompletions);
		if(!str)
			break;
		completions[ncompletions++] = (gchar *)strcpy(malloc(strlen(str) + 1), str);
	}
	free_vector(argv);
	if(!ncompletions) {
		free(completions);
		return 0;
	}
	completions[ncompletions] = 0;
	return completions;
}

#else

gchar **command_completion(gchar *text, int start, int end)
{
	return 0;
}

#endif

COMPLETION_HELPER(complete_volume_modify)
{
	static int index;

	if(!state)
		index = 0;
	if(arg != 1)
		return 0;
	if(argv[1] && strlen(argv[1])) {
		if(state)
			return 0;
		if(!g_strncasecmp("left", argv[1], strlen(argv[1])))
			return "left";
		if(!g_strncasecmp("right", argv[1], strlen(argv[1])))
			return "right";
		return 0;
	} else if(state == 0) {
		return "left";
	} else if(index)
		return 0;
	else {
		index = 1;
		return "right";
	}
}

COMMAND(command_quit)
{
	return TerminateResult;
}

HELP_COMMAND(help_quit)
{
	switch(mode) {
		case Synopsis:
			return "terminate the shell";
		case Syntax:
			return "QUIT";
		default:
			return 0;
	}
}

static CommandResult traverse_playlist(gint session_id, gint jump)
{
	int pos = xmms_remote_get_playlist_pos(session_id);
	int len = xmms_remote_get_playlist_length(session_id);

	pos += jump;
	if(pos < 0)
		pos = len + pos;
	if(pos >= len)
		pos -= len;
	xmms_remote_set_playlist_pos(session_id, pos);
	return OkResult;
}

static CommandResult modify_volume(gint direction, gint session_id, gint argc, gchar **argv)
{
	int lv, rv, i;

	xmms_remote_get_volume(session_id, &lv, &rv);
	if(argc == 1) {
		lv += direction * 2;
		rv += direction * 2;
	} else {
		if(isdigit(argv[1][0])) {
			i = atoi(argv[1]);
			lv += direction * i;
			rv += direction * i;
		} else if((g_strncasecmp("left", argv[1], strlen(argv[1])) &&
		           g_strncasecmp("right", argv[1], strlen(argv[1]))) ||
		          (argc > 2 && !isdigit(argv[2][0]))) {
			return SyntaxErrorResult;
		} else {
			if(argc > 2)
				i = atoi(argv[2]);
			else
				i = 2;
			if(!g_strncasecmp("left", argv[1], strlen(argv[1])))
				lv += direction * i;
			else
				rv += direction * i;
		}
	}
	if(lv < 0)
		lv = 0;
	if(rv < 0)
		rv = 0;
	xmms_remote_set_volume(session_id, lv, rv);
	return OkResult;

}

COMPLETION_HELPER(complete_help)
{
	if(arg == 1)
		return build_command_list(argv, arg, state);
	return 0;
}

static void output_encoded_html(const gchar *str, FILE *f)
{
	while(*str) {
		switch(*str) {
			case '&':
				fprintf(f, "&amp;");
				break;
			case '<':
				fprintf(f, "&lt;");
				break;
			case '>':
				fprintf(f, "&gt;");
				break;
			default:
				fprintf(f, "%c", *str);
				break;
		}
		str++;
	}
}

static void generate_help_html(void)
{
	Command *com;
	gchar *txt;

	printf("<a name=\"comhelp_toc\"><h3>List of Commands</h3>\n");
	printf("<ul>\n");
	for(com = commands; com->command; com++) {
		printf("<li><a href=\"#%s\">", com->command);
		output_encoded_html(com->command, stdout);
		printf("</a>");
		if(com->helper && (txt = com->helper(Synopsis))) {
			printf(" - ");
			output_encoded_html(txt, stdout);
		}
		printf("\n");
	}
	printf("</ul>\n<p>\n");
	printf("<h3>Commands</h3>\n");
	printf("<dl>\n");
	for(com = commands; com->command; com++) {
		printf("<dt><a name=\"%s\"><a href=\"#comhelp_toc\">", com->command);
		output_encoded_html(com->command, stdout);
		printf("</a>\n");
		if(com->helper) {
			if((txt = com->helper(Synopsis))) {
				printf(" - ");
				output_encoded_html(txt, stdout);
				printf("<p>\n");
			}
			printf("<dd><dl>\n");
			if((txt = com->helper(Syntax))) {
				printf("<dt>Usage:<dd>");
				output_encoded_html(txt, stdout);
				printf("<p>\n");
			}
			if((txt = com->helper(Description))) {
				printf("<dt>Description:<dd>");
				output_encoded_html(txt, stdout);
				printf("<p>\n");
			}
			printf("</dl>\n");
		} else
			printf("<dd>No help available\n");
		printf("<p>\n");
	}
	printf("</dl>\n");
}

COMMAND(command_help)
{
	Command *com;
	gchar *txt;

	if(argc < 2) {
		for(com = commands; com->command; com++) {
			printf("%-12.12s", com->command);
			if(com->helper && (txt = com->helper(Synopsis))) {
				printf(" - ");
				output_indented(txt, 15, 15, 75, stdout);
			}
			printf("\n");
		}
		return OkResult;
	}
	if(!g_strcasecmp("html", argv[1])) {
		generate_help_html();
		return OkResult;
	}
	if(!(com = lookup_command(argv[1]))) {
		printf("Invalid help topic: `%s'\n", argv[1]);
		printf("Enter `help' with no arguments for a list of available help topics\n");
		return ErrorResult;
	}
	if(!com->helper) {
		printf("No help available for the `%s' command\n", com->command);
		return ErrorResult;
	}
	if((txt = com->helper(Synopsis))) {
		printf("%s: ", com->command);
		output_indented(txt, strlen(com->command) + 2, strlen(com->command) + 2, 75, stdout);
		printf("\n\n");
	}
	if((txt = com->helper(Syntax))) {
		printf("Usage: ");
		output_indented(txt, 7, 7, 75, stdout);
		printf("\n\n");
	}
	if((txt = com->helper(Description))) {
		output_indented(txt, 0, 0, 75, stdout);
		printf("\n\n");
	}
	return OkResult;
}

HELP_COMMAND(help_help)
{
	switch(mode) {
		case Synopsis:
			return "list commands and show their usage";
		case Syntax:
			return "HELP [command]";
		case Description:
			return "
Given no arguments, HELP displays a list of commands and a brief
description for each.  If a command name is given as the first
argument, HELP will display the usage of that command and a more
in-depth description.";
		default:
			return 0;
	}
}

COMMAND(command_status)
{
	gint leftVol, rightVol, pos, rate, freq, nch, t, i;
#if HAVE_XMMS_REMOTE_GET_EQ
	gfloat preamp, *bands;
#endif

	pos = xmms_remote_get_playlist_pos(session_id);
	if(xmms_remote_is_playing(session_id)) {
		xmms_remote_get_info(session_id, &rate, &freq, &nch);
		t = xmms_remote_get_output_time(session_id);
		printf("Playing: %s (%d kbps, %d hz, %d channels)\n",
		       xmms_remote_get_playlist_title(session_id, pos), rate / 1000, freq, nch);
		printf("Time: %02d:%02d.%02d%s\n", t / 60000, (t / 1000) % 60, (t / 10) % 100,
		       xmms_remote_is_paused(session_id) ? " (paused)" : "");
	} else
		printf("Current song: %s\n", xmms_remote_get_playlist_title(session_id, pos));

	xmms_remote_get_volume(session_id, &leftVol, &rightVol);
	printf("Left volume: %d\n", leftVol);
	printf("Right volume: %d\n", rightVol);

#if HAVE_XMMS_REMOTE_GET_EQ
	xmms_remote_get_eq(session_id, &preamp, &bands);
	printf("Equalizer preamp: %.1f\n", preamp);
	printf("Equalizer bands:");
	for(i = 0; i < 10; i++)
		printf("%5d", i);
	printf("\n");
	printf("                  ");
	for(i = 0; i < 10; i++)
		if(bands[i] < 0)
			printf(" %3.1f", bands[i]);
		else
			printf("  %3.1f", bands[i]);
	printf("\n");
	g_free(bands);
#endif

	printf("Balance: %d\n", xmms_remote_get_balance(session_id));
	printf("Skin: %s\n", xmms_remote_get_skin(session_id));

	return OkResult;
}

HELP_COMMAND(help_status)
{
	switch(mode) {
		case Synopsis:
			return "display the current status of XMMS";
		case Syntax:
			return "STATUS";
		default:
			return 0;
	}
}

COMMAND(command_pause)
{
	xmms_remote_pause(session_id);
	return OkResult;
}

HELP_COMMAND(help_pause)
{
	switch(mode) {
		case Synopsis:
			return "pause or unpause playback";
		case Syntax:
			return "PAUSE";
		case Description:
			return "
The PAUSE command behaves as a toggle.  If XMMS is currently paused, the
PAUSE command will have it resume playback.  If XMMS is currently playing,
the PAUSE command will cause it to pause.  If no song is currently playing,
the PAUSE command will have no effect.";
		default:
			return 0;
	}
}


COMMAND(command_play)
{
	xmms_remote_play(session_id);
	return OkResult;
}

HELP_COMMAND(help_play)
{
	switch(mode) {
		case Synopsis:
			return "begin or resume playback";
		case Syntax:
			return "PLAY";
		case Description:
			return "
If XMMS is currently paused or stopped, the PLAY command will resume
playback.  Otherwise, when XMMS is currently playing, the PLAY command
will cause XMMS to return to the beginning of the track (and continue
playing).";
		default:
			return 0;
	}
}

COMMAND(command_stop)
{
	xmms_remote_stop(session_id);
	return OkResult;
}

HELP_COMMAND(help_stop)
{
	switch(mode) {
		case Synopsis:
			return "stop playback and return to beginning of playlist";
		case Syntax:
			return "STOP";
		default:
			return 0;
	}
}

COMMAND(command_forward)
{
	return traverse_playlist(session_id, 1);
}

HELP_COMMAND(help_forward)
{
	switch(mode) {
		case Synopsis:
			return "advance forward one file in the playlist";
		case Syntax:
			return "FORWARD";
		case Description:
			return "
The FORWARD command causes XMMS to advance forward one file in the
current playlist.  If the last file in the playlist is the current
file when FORWARD is executed, the first file in the playlist will
become the new current file.  If the FORWARD command is executed
during playback, playback of the new file will immediately begin.";
		default:
			return 0;
	}
}

COMMAND(command_backward)
{
	return traverse_playlist(session_id, -1);
}

HELP_COMMAND(help_backward)
{
	switch(mode) {
		case Synopsis:
			return "move back one file in the playlist";
		case Syntax:
			return "BACKWARD";
		case Description:
			return "
The BACKWARD command causes XMMS to move back to the previous file
in the current playlist.  If the first file in the playlist is the
current file when BACKWARD is executed, the last file in the playlist
will become the new current file.  If the BACKWARD command is executed
during playback, playback of the new file will immediately begin.";
		default:
			return 0;
	}
}

COMMAND(command_jump)
{
	int pos;

	if(argc < 2 || !isdigit(argv[1][0]))
		return SyntaxErrorResult;
	pos = atoi(argv[1]);
	if(pos < 1 || pos > xmms_remote_get_playlist_length(session_id)) {
		fprintf(stderr, "Invalid position (1..%d)\n",
		        xmms_remote_get_playlist_length(session_id));
		return SyntaxErrorResult;
	}
	xmms_remote_set_playlist_pos(session_id, pos - 1);
	return OkResult;
}

HELP_COMMAND(help_jump)
{
	switch(mode) {
		case Synopsis:
			return "jump to a position in the playlist";
		case Syntax:
			return "JUMP <position>";
		case Description:
			return "
The JUMP command instructs XMMS to move to a given position in the
playlist.  Positions are specified as numbers.  The first file in the
playlist is position 1.  If XMMS is playing when the JUMP command is
executed then playback will continue immediately on the new file.";
		default:
			return 0;
	}
}

COMMAND(command_volume_up)
{
	return modify_volume(1, session_id, argc, argv);
}

HELP_COMMAND(help_volume_up)
{
	switch(mode) {
		case Synopsis:
			return "increase volume";
		case Syntax:
			return "+ [left|right] [value]";
		case Description:
			return "
The + command increments the volume level of XMMS.  If no [value] is
indicated a default value of 2 is assumed.  If neither [left] nor
[right] is specified then the volume of both output channels is
increased.  Otherwise only the volume level of the given output
channel is increased.";
		default:
			return 0;
	}
}

COMMAND(command_volume_down)
{
	return modify_volume(-1, session_id, argc, argv);
}


HELP_COMMAND(help_volume_down)
{
	switch(mode) {
		case Synopsis:
			return "decrease volume";
		case Syntax:
			return "- [left|right] [value]";
		case Description:
			return "
The + command decrements the volume level of XMMS.  If no [value] is
indicated a default value of 2 is assumed.  If neither [left] nor
[right] is specified then the volume of both output channels is
decreased.  Otherwise only the volume level of the given output
channel is decreased.";
		default:
			return 0;
	}
}

COMMAND(command_volume)
{
	int lv, rv;

	xmms_remote_get_volume(session_id, &lv, &rv);
	if(argc < 2)
		return SyntaxErrorResult;
	if(isdigit(argv[1][0]))
		lv = rv = atoi(argv[1]);
	else if(argc < 3 || !isdigit(argv[2][0]))
		return SyntaxErrorResult;
	else if(!g_strncasecmp("left", argv[1], strlen(argv[1])))
		lv = atoi(argv[2]);
	else if(!g_strncasecmp("right", argv[1], strlen(argv[1])))
		rv = atoi(argv[2]);
	else
		return SyntaxErrorResult;

	xmms_remote_set_volume(session_id, lv, rv);
	return OkResult;
}

HELP_COMMAND(help_volume)
{
	switch(mode) {
		case Synopsis:
			return "set the volume to a given level";
		case Syntax:
			return "VOLUME [left|right] <value>";
		case Description:
			return "
The VOLUME command explicitly sets the volume level of XMMS.  The <value>
argument is mandatory and must be specified as an integer from 0 through
100.  If neither [left] nor [right] is specified then the volume level of
both channels is set to <value>.  Otherwise only the volume level of the
given channel is set.";
		default:
			return 0;
	}
}

COMMAND(command_clear)
{
	xmms_remote_playlist_clear(session_id);
	return OkResult;
}

HELP_COMMAND(help_clear)
{
	switch(mode) {
		case Synopsis:
			return "clear the current playlist";
		case Syntax:
			return "CLEAR";
		default:
			return 0;
	}
}

COMMAND(command_playlist)
{
	gchar **playlist;
	int start = 0, stop = -1, len;
	gboolean filenames = FALSE;

	if(argc > 1 && !g_strncasecmp("filenames", argv[1], strlen(argv[1]))) {
		argc--;
		argv++;
		filenames = TRUE;
	}
	if(argc > 1) {
		if(!isdigit(argv[1][0]))
			return SyntaxErrorResult;
		start = atoi(argv[1]) - 1;
		if(argc > 2) {
			if(!isdigit(argv[2][0]))
				return SyntaxErrorResult;
			stop = atoi(argv[2]) - 1;
		}
	}
	if(filenames)
		playlist = get_playlist_filenames(session_id, &len);
	else
		playlist = get_playlist_titles(session_id, &len);
	if(playlist) {
		for(; start < len && (stop == -1 || start <= stop); start++)
			printf("%d. %s\n", start + 1, playlist[start]);
		g_strfreev(playlist);
	} else
		printf("Playlist is empty\n");
	return OkResult;
}

HELP_COMMAND(help_playlist)
{
	switch(mode) {
		case Synopsis:
			return "view XMMS's playlist";
		case Syntax:
			return "PLAYLIST [FILENAMES] [start] [stop]";
		case Description:
			return "\
The PLAYLIST command lists all of the entries in XMMS's playlist.  If a
single number is provided as an argument, the PLAYLIST command lists all
entries beginning at the position specified by that number.  If a second
number is specified as an argument, the PLAYLIST command will stop listing
entries when it reaches that position in the playlist.  By default PLAYLIST
will list the titles of songs in the playlist.  If [FILENAMES] is specified,
filenames will be listed instead.";
		default:
			return 0;
	}
}

COMMAND(command_load)
{
	GList *playlist;

	if(argc < 2)
		return SyntaxErrorResult;
	playlist = g_list_append(0, g_strdup(argv[1]));
	xmms_remote_playlist_add(session_id, playlist);
	g_list_free(playlist);
	return OkResult;
}

HELP_COMMAND(help_load)
{
	switch(mode) {
		case Synopsis:
			return "add a file or playlist to XMMS's playlist";
		case Syntax:
			return "LOAD <filename>";
		case Description:
			return "
The LOAD command instructs XMMS to load <filename> into its playlist.
If the file is itself a playlist, the contents of the playlist will
be appended to XMMS's playlist.  Otherwise the file will be appended
itself.";
		default:
			return 0;
	}
}

COMMAND(command_save)
{
	FILE *f;
	gint i, len;
	gchar **playlist;

	if(argc < 2)
		return SyntaxErrorResult;
	if(!(f = fopen(argv[1], "w"))) {
		fprintf(stderr, "Unable to open `%s': %s\n", argv[1], strerror(errno));
		return ErrorResult;
	}
	if((playlist = get_playlist_filenames(session_id, &len))) {
		for(i = 0; i < len; i++)
			fprintf(f, "%s\n", playlist[i]);
		g_strfreev(playlist);
	}
	fclose(f);
	return OkResult;
}

HELP_COMMAND(help_save)
{
	switch(mode) {
		case Synopsis:
			return "save XMMS's playlist to a file";
		case Syntax:
			return "SAVE <filename>";
		case Description:
			return "
The SAVE command saves the playlist currently loaded by XMMS to a file of
your choice.  The contents of the file will consist of the filenames of
each of the files in the playlist, one per line.";
		default:
			return 0;
	}
}

COMMAND(command_eject)
{
	xmms_remote_eject(session_id);
	return OkResult;
}

HELP_COMMAND(help_eject)
{
	switch(mode) {
		case Synopsis:
			return "cause XMMS to display the file load dialog";
		case Syntax:
			return "EJECT";
		default:
			return 0;
	}
}

COMMAND(command_version)
{
	printf("XMMS-Shell v%s by Logan Hanks <logan@vt.edu>\n", VERSION);
	printf("Build info: %s %s with %s\n", __TIME__, __DATE__, __VERSION__);
	return OkResult;
}

HELP_COMMAND(help_version)
{
	switch(mode) {
		case Synopsis:
			return "display XMMS-Shell version information";
		case Syntax:
			return "VERSION";
		default:
			return 0;
	}
}

COMMAND(command_window)
{
	gboolean status[3], apply[3];
	gint command = -1;

	if(argc < 3)
		return SyntaxErrorResult;

	if(!g_strncasecmp("hide", argv[2], strlen(argv[2])))
		command = 0;
	else if(!g_strncasecmp("show", argv[2], strlen(argv[2])))
		command = 1;
	else if(!g_strncasecmp("toggle", argv[2], strlen(argv[2])))
		command = 2;
	else
		return SyntaxErrorResult;

	if(!g_strncasecmp("all", argv[1], strlen(argv[1])))
		apply[0] = apply[1] = apply[2] = TRUE;
	else if(!g_strncasecmp("equalizer", argv[1], strlen(argv[1])))
		apply[0] = !(apply[1] = apply[2] = FALSE);
	else if(!g_strncasecmp("main", argv[1], strlen(argv[1])))
		apply[1] = !(apply[0] = apply[2] = FALSE);
	else if(!g_strncasecmp("playlist", argv[1], strlen(argv[1])))
		apply[2] = !(apply[0] = apply[1] = FALSE);
	else
		return SyntaxErrorResult;

	status[0] = xmms_remote_is_eq_win(session_id);
	status[1] = xmms_remote_is_main_win(session_id);
	status[2] = xmms_remote_is_pl_win(session_id);

	switch(command) {
		case 0:
			if(apply[0])
				status[0] = FALSE;
			if(apply[1])
				status[1] = FALSE;
			if(apply[2])
				status[2] = FALSE;
			break;
		case 1:
			if(apply[0])
				status[0] = TRUE;
			if(apply[1])
				status[1] = TRUE;
			if(apply[2])
				status[2] = TRUE;
			break;
		case 2:
			if(apply[0])
				status[0] = !status[0];
			if(apply[1])
				status[1] = !status[1];
			if(apply[2])
				status[2] = !status[2];
			break;
	}

	xmms_remote_eq_win_toggle(session_id, status[0]);
	xmms_remote_main_win_toggle(session_id, status[1]);
	xmms_remote_pl_win_toggle(session_id, status[2]);

	return OkResult;
}

HELP_COMMAND(help_window)
{
	switch(mode) {
		case Synopsis:
			return "show or hide XMMS windows";
		case Syntax:
			return "WINDOW ALL|EQUALIZER|MAIN|PLAYLIST HIDE|SHOW|TOGGLE";
		case Description:
			return "
The WINDOW command hides and shows XMMS windows.  The first argument must
be the window to operate on (the main, playlist, or equalizer window, or
all three) and the second argument must be a command (HIDE, SHOW, or TOGGLE).
The TOGGLE command will toggle the display of a window.";
		default:
			return 0;
	}
}

COMMAND(command_preferences)
{
	xmms_remote_show_prefs_box(session_id);
	return OkResult;
}

HELP_COMMAND(help_preferences)
{
	switch(mode) {
		case Synopsis:
			return "cause XMMS to display its preferences dialog";
		case Syntax:
			return "PREFERENCES";
		default:
			return 0;
	}
}

COMMAND(command_repeat)
{
#if HAVE_XMMS_REMOTE_IS_REPEAT
	gboolean status, newstatus;

	if(argc < 2)
		return SyntaxErrorResult;

	status = xmms_remote_is_repeat(session_id);
	if(!g_strncasecmp("off", argv[1], strlen(argv[1])))
		newstatus = FALSE;
	else if(!g_strncasecmp("on", argv[1], strlen(argv[1])))
		newstatus = TRUE;
	else if(!g_strncasecmp("toggle", argv[1], strlen(argv[1])))
		newstatus = !status;
	else
		return SyntaxErrorResult;

	if(status != newstatus)
#endif
	xmms_remote_toggle_repeat(session_id);
	return OkResult;
}

HELP_COMMAND(help_repeat)
{
	switch(mode) {
		case Synopsis:
#if HAVE_XMMS_REMOTE_IS_REPEAT
			return "set repeat mode";
#else
			return "toggle repeat mode";
#endif
		case Syntax:
#if HAVE_XMMS_REMOTE_IS_REPEAT
			return "REPEAT <OFF|ON|TOGGLE>";
#else
			return "REPEAT";
#endif
		case Description:
#if HAVE_XMMS_REMOTE_IS_REPEAT
			return 0;
#else
			return "
The REPEAT command causes XMMS to toggle repeat mode.  Unfortunately XMMS
provides no feedback on whether or not this mode is set, so it is impossible
for XMMS-Shell to explicitly set repeat mode to a particular value.  Only
toggling is possible.";
#endif
		default:
			return 0;
	}
}

COMMAND(command_shuffle)
{
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
	gboolean status, newstatus;

	if(argc < 2)
		return SyntaxErrorResult;

	status = xmms_remote_is_shuffle(session_id);
	if(!g_strncasecmp("off", argv[1], strlen(argv[1])))
		newstatus = FALSE;
	else if(!g_strncasecmp("on", argv[1], strlen(argv[1])))
		newstatus = TRUE;
	else if(!g_strncasecmp("toggle", argv[1], strlen(argv[1])))
		newstatus = !status;
	else
		return SyntaxErrorResult;

	if(status != newstatus)
#endif
	xmms_remote_toggle_shuffle(session_id);
	return OkResult;
}

HELP_COMMAND(help_shuffle)
{
	switch(mode) {
		case Synopsis:
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
			return "set shuffle mode";
#else
			return "toggle shuffle mode";
#endif
		case Syntax:
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
			return "SHUFFLE <OFF|ON|TOGGLE>";
#else
			return "SHUFFLE";
#endif
		case Description:
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
			return 0;
#else
			return "
The SHUFFLE command causes XMMS to toggle shuffle mode.  Unfortunately XMMS
provides no feedback on whether or not this mode is set, so it is impossible
for XMMS-Shell to explicitly set shuffle mode to a particular value.  Only
toggling is possible.";
#endif
		default:
			return 0;
	}
}

COMMAND(command_fade)
{
	gint target = 0, step = 1, delay = 100000, channel = 2, pos;
	gint lv, rv, olv, orv, v;

	if(!xmms_remote_is_playing(session_id)) {
		fprintf(stderr, "No song is playing, ignoring fade request\n");
		return OkResult;
	}
	if(xmms_remote_is_paused(session_id)) {
		fprintf(stderr, "Song is paused, ignoring fade request\n");
		return OkResult;
	}

	if(argc > 1) {
		if(!g_strncasecmp("left", argv[1], strlen(argv[1]))) {
			channel = 0;
			argc--;
			argv++;
		} else if(!g_strncasecmp("right", argv[1], strlen(argv[1]))) {
			channel = 1;
			argc--;
			argv++;
		}
	}

	pos = xmms_remote_get_playlist_pos(session_id);
	xmms_remote_get_volume(session_id, &lv, &rv);

	if(lv > rv)
		v = lv;
	else
		v = rv;

	olv = lv;
	orv = rv;
	
	if(argc > 1) {
		if(!isdigit(argv[1][0]))
			return SyntaxErrorResult;
		target = atoi(argv[1]);
		if(argc > 2) {
			if(!isdigit(argv[2][0]))
				return SyntaxErrorResult;
			step = atoi(argv[2]);
			if(!step) {
				fprintf(stderr, "The stepsize must be greater than zero.\n");
				return SyntaxErrorResult;
			}
			if(argc > 3) {
				if(!isdigit(argv[3][0]))
					return SyntaxErrorResult;
				delay = atoi(argv[3]);
			}
		}
	}
	if(v > target)
		while(v > target && xmms_remote_get_playlist_pos(session_id) == pos) {
			v -= step;
			if(channel != 1)
				lv -= step;
			if(channel != 0)
				rv -= step;
			xmms_remote_set_volume(session_id, lv, rv);
			usleep(delay);
		}
	else
		while(v < target && xmms_remote_get_playlist_pos(session_id) == pos) {
			v += step;
			if(channel != 1)
				lv += step;
			if(channel != 0)
				rv += step;
			xmms_remote_set_volume(session_id, lv, rv);
			usleep(delay);
		}
	xmms_remote_set_volume(session_id, olv, orv);
	return OkResult;
}

HELP_COMMAND(help_fade)
{
	switch(mode) {
		case Synopsis:
			return "slowly adjust volume to specified level, then move to next track";
		case Syntax:
			return "FADE [LEFT|RIGHT] [target] [stepsize] [delay]";
		case Description:
			return "
The FADE command performs a simple fade effect on the currently playing track.
If no song is being played, the command has no effect.  Specifying either LEFT
or RIGHT will cause the effect to be isolated to only the specified channel.
The [target] specifies at which volume the fade effect should end.  The volume
is restored to its previous settings once the target has been reached.
The [stepsize]
indicates by how much to change the volume at a time.  The [delay] specifies
how long to wait (in microseconds) before decreasing or increasing the volume
towards the target by the specified stepsize.  By default the effect is applied
to both channels, with a target of 0, stepsize of 1, and delay of 100000.";
		default:
			return 0;
	}
}

COMMAND(command_balance)
{
	gint bal;

	if(argc < 2 || (!isdigit(argv[1][0]) && (argv[1][0] != '-' || !isdigit(argv[1][1]))))
		return SyntaxErrorResult;
	
	bal = atoi(argv[1]);
	/* Let XMMS handle this
	if(bal < -100) bal = -100;
	if(bal > 100) bal = 100;
	*/
	xmms_remote_set_balance(session_id, bal);
	return OkResult;
}

HELP_COMMAND(help_balance)
{
	switch(mode) {
		case Synopsis:
			return "set balance level";
		case Syntax:
			return "BALANCE <value>";
		case Description:
			return "
Sets the balance to <value> (valid range is -100 through 100).  A value of 0
specifies the same volume for both the left and right channels.";
		default:
			return 0;
	}
}

COMMAND(command_fakepause)
{
	gint pos, offset, ch;

	if(!xmms_remote_is_playing(session_id)) {
		fprintf(stderr, "No song is being played\n");
		return ErrorResult;
	}
	pos = xmms_remote_get_playlist_pos(session_id);
	offset = xmms_remote_get_output_time(session_id);
	xmms_remote_stop(session_id);
	printf("Press [ENTER] to resume: ");
	fflush(stdout);
	while((ch = getc(stdin)) != EOF && ch != '\n');
	xmms_remote_set_playlist_pos(session_id, pos);
	xmms_remote_play(session_id);
	xmms_remote_jump_to_time(session_id, offset);

	return OkResult;
}

HELP_COMMAND(help_fakepause)
{
	switch(mode)
	{
		case Synopsis:
			return "pause XMMS and release the output device";
		case Syntax:
			return "FAKEPAUSE";
		case Description:
			return "
The FAKEPAUSE is similar to the PAUSE command in that it pauses playback.
However, FAKEPAUSE is an interactive command.  When you press [ENTER] at
the prompt FAKEPAUSE provides, playback will resume where it left off.
The usefulness of this command is that it forces XMMS to release the
output device until you unpause it.  Technically it does this by saving
the current song and the position in it before executing the STOP command.
Then, when it unpauses, it jumps back to the saved position in the song
and resumes playback.";
		default:
			return 0;
	}
}

COMMAND(command_resetdevice)
{
	gint pos, offset;
	gboolean paused;

	if(!xmms_remote_is_playing(session_id))
		return OkResult;

	paused = xmms_remote_is_paused(session_id);
	pos = xmms_remote_get_playlist_pos(session_id);
	offset = xmms_remote_get_output_time(session_id);
	xmms_remote_stop(session_id);
	xmms_remote_set_playlist_pos(session_id, pos);
	xmms_remote_play(session_id);
	xmms_remote_jump_to_time(session_id, offset);
	if(paused)
		xmms_remote_pause(session_id);

	return OkResult;
}

HELP_COMMAND(help_resetdevice)
{
	switch(mode)
	{
		case Synopsis:
			return "force XMMS to reset the output device";
		case Syntax:
			return "RESETDEVICE";
		case Description:
			return "
The RESETDEVICE command forces XMMS to reset the output device.  This is
useful if you wish to apply new output device settings immediately without
interrupting playback.  This is technically implemented in the same manner
as the FAKEPAUSE command.";
		default:
			return 0;
	}
}

#if HAVE_XMMS_REMOTE_GET_EQ_PREAMP

COMMAND(command_preamp)
{
	if(argc < 2) {
		printf("Preamp: %.1f\n", xmms_remote_get_eq_preamp(session_id));
		return OkResult;
	}

	xmms_remote_set_eq_preamp(session_id, atof(argv[1]));
	return OkResult;
}

HELP_COMMAND(help_preamp)
{
	switch(mode)
	{
		case Synopsis:
			return "view or set the equalizer's preamp value";
		case Syntax:
			return "PREAMP [value]";
		case Description:
			return "
The PREAMP command will output the equalizer's preamp value if no arguments
are given.  Otherwise its argument, which should be a decimal value between
-20 and 20, is assigned.";
		default:
			return 0;
	}
}

#endif

#if HAVE_XMMS_REMOTE_GET_EQ_BAND

COMMAND(command_band)
{
	gint band;
	gfloat *values;

	if(argc < 2) {
		xmms_remote_get_eq(session_id, 0, &values);
		for(band = 0; band < 10; band++)
			printf("Band %d: %.1f\n", band, values[band]);
		g_free(values);
		return OkResult;
	}

	if(!isdigit(argv[1][0]))
		return SyntaxErrorResult;

	band = atoi(argv[1]);

	if(argc < 3) {
		printf("Band %d: %.1f\n", band, xmms_remote_get_eq_band(session_id, band));
		return OkResult;
	}

	xmms_remote_set_eq_band(session_id, band, atof(argv[2]));
	return OkResult;
}

HELP_COMMAND(help_band)
{
	switch(mode)
	{
		case Synopsis:
			return "view or set the value of an equalizer band";
		case Syntax:
			return "BAND [band] [value]";
		case Description:
			return "
The BAND command outputs or sets the value of an equalizer band.
If given no arguments, the BAND command outputs the current values
of all 10 equalizer bands.  If only a band value is specified
(a value from 0 through 9) then the value of that particular band
is output.  Otherwise the second argument, which should be a decimal
value between -20 and 20, is assigned to the given band.";
		default:
			return 0;
	}
}

#endif

