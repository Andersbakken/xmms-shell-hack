#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <cctype>
#include <xmmsctrl.h>
#include "config.h"
#include "command.h"

#define SECTION virtual const string get_section(void) const { return "Playlist"; }

class JumpCommand : public Command
{
public:
	COM_STRUCT(JumpCommand, "jump")

	virtual void execute(CommandContext &cnx) const
	{
		int pos;

		if(cnx.args.size() < 2 || !isdigit(cnx.args[1][0])) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		pos = atoi(cnx.args[1].c_str());
		if(pos < 1 || pos > xmms_remote_get_playlist_length(cnx.session_id)) {
			fprintf(stderr, "Invalid position %d.  Valid positions are >= 1 and <= %d.\n",
			        pos, xmms_remote_get_playlist_length(cnx.session_id));
			cnx.result_code = COMERR_SYNTAX;
		} else {
			xmms_remote_set_playlist_pos(cnx.session_id, pos - 1);
			printf("Jumped to position %d in the playlist.\n", pos);
			cnx.result_code = 0;
		}
	}

	COM_SYNOPSIS("jump to a position in the playlist")
	COM_SYNTAX("JUMP <position>")
	COM_DESCRIPTION(
		"The JUMP command instructs XMMS to move to a given position in the "
		"playlist.  Positions are specified as numbers.  The first file in the "
		"playlist is position 1.  If XMMS is playing when the JUMP command is "
		"executed then playback will continue immediately on the new file."
	)
	COM_RETURN("Always 0")
	SECTION
};

class Traverse
{
	int jump;

public:
	Traverse(int _jump) : jump(_jump) { }

	int traverse_playlist(int session_id)
	{
		int pos = xmms_remote_get_playlist_pos(session_id);
		int len = xmms_remote_get_playlist_length(session_id);
		
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
		if(xmms_remote_is_shuffle(session_id)) {
			if(jump < 0)
				xmms_remote_playlist_prev(session_id);
			else
				xmms_remote_playlist_next(session_id);
			usleep(100);
			return xmms_remote_get_playlist_pos(session_id);
		}
#endif
		pos += jump;
		if(pos < 0)
			pos = len + pos;
		if(pos >= len)
			pos -= len;
		xmms_remote_set_playlist_pos(session_id, pos);
		return pos;
	}
};

class NextCommand : public Command
{
public:
	NextCommand(void) : Command("next") { add_alias("forward"); }
	virtual ~NextCommand() { }

	virtual void execute(CommandContext &cnx) const
	{
		int n = 1;

		if(cnx.args.size() > 1) {
			if(!isdigit(cnx.args[1][0])) {
				cnx.result_code = COMERR_SYNTAX;
				return;
			}
			n = atoi(cnx.args[1].c_str());
		}

		Traverse t(n);

		cnx.result_code = t.traverse_playlist(cnx.session_id);
		printf("Current track is now: %d\n", cnx.result_code);
	}

	COM_SYNOPSIS("advance forward in the playlist")
	COM_SYNTAX("NEXT [n]")
	COM_DESCRIPTION(
		"If [n] is not specified, it is 1 by default.  NEXT advances the playlist "
		"forward by n entries.  Whenever the end of the playlist is encountered "
		"during advancing, the beginning of the playlist is considered to be the "
		"next entry (hence the playlist is treated as circular).  If shuffle mode is "
		"enabled in XMMS, the parameter n is effectively ignored."
	)
	COM_RETURN("The new position of the playlist")
	SECTION
};

class PreviousCommand : public Command
{
public:
	PreviousCommand(void) : Command("previous") { add_alias("backward"); }
	virtual ~PreviousCommand() { }

	virtual void execute(CommandContext &cnx) const
	{
		int n = 1;

		if(cnx.args.size() > 1) {
			if(!isdigit(cnx.args[1][0])) {
				cnx.result_code = COMERR_SYNTAX;
				return;
			}
			n = atoi(cnx.args[1].c_str());
		}

		Traverse t(-n);

		cnx.result_code = t.traverse_playlist(cnx.session_id);
		printf("Current track is now: %d\n", cnx.result_code);
	}

	COM_SYNOPSIS("advance backward in the playlist")
	COM_SYNTAX("PREVIOUS [n]")
	COM_DESCRIPTION(
		"If [n] is not specified, it is 1 by default.  PREVIOUS advances the playlist "
		"backward by n entries.  Whenever the beginning of the playlist is encountered "
		"during advancing, the end of the playlist is considered to be the "
		"previous entry (hence the playlist is treated as circular).  If shuffle mode is "
		"enabled in XMMS, the parameter n is effectively ignored."
	)
	COM_RETURN("The new position of the playlist")
	SECTION
};

class ClearCommand : public Command
{
public:
	COM_STRUCT(ClearCommand, "clear")
	
	virtual void execute(CommandContext &cnx) const
	{
		xmms_remote_playlist_clear(cnx.session_id);
	}

	COM_SYNOPSIS("clear the current playlist")
	COM_SYNTAX("CLEAR")
	COM_DESCRIPTION("Clears all entries from the playlist.")
	COM_RETURN("Always 0")
	SECTION
};

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

class ListCommand : public Command
{
public:
	COM_STRUCT(ListCommand, "list")
	
	virtual void execute(CommandContext &cnx) const
	{
		gchar **playlist;
		int start = 0, stop = -1, len, x = 1, digs, pos;
		gboolean filenames = FALSE;

		if(cnx.args.size() > 1 && !strncasecmp("filenames", cnx.args[1].c_str(), cnx.args[1].length())) {
			filenames = TRUE;
			x = 2;
		}
		if(cnx.args.size() - x > 0) {
			if(!isdigit(cnx.args[x][0])) {
				cnx.result_code = COMERR_SYNTAX;
				return;
			}
			start = atoi(cnx.args[x].c_str()) - 1;
			if(cnx.args.size() - x > 1) {
				if(!isdigit(cnx.args[x + 1][0])) {
					cnx.result_code = COMERR_SYNTAX;
					return;
				}
				stop = atoi(cnx.args[x + 1].c_str()) - 1;
			}
		}
		if(filenames)
			playlist = get_playlist_filenames(cnx.session_id, &len);
		else
			playlist = get_playlist_titles(cnx.session_id, &len);
        pos = xmms_remote_get_playlist_pos(cnx.session_id);
        for(int i = digs = 1; i < len; i *= 10, digs++);
		if(playlist) {
			for(int i = start; i < len && (stop == -1 || i <= stop); i++)
				printf("%c%*d. %s\n", pos == i ? '*' : ' ', digs, i + 1, playlist[i]);
			g_strfreev(playlist);
		} else
			printf("Playlist is empty\n");
		cnx.result_code = (stop != -1 && stop < len ? stop + 1 : len) - start;
	}

	COM_SYNOPSIS("display the playlist")
	COM_SYNTAX("LIST [FILENAMES] [start] [stop]")
	COM_DESCRIPTION(
		"The PLAYLIST command lists all of the entries in XMMS's playlist.  If a "
		"single number is provided as an argument, the PLAYLIST command lists all "
		"entries beginning at the position specified by that number.  If a second "
		"number is specified as an argument, the PLAYLIST command will stop listing "
		"entries when it reaches that position in the playlist.  By default PLAYLIST "
		"will list the titles of songs in the playlist.  If [FILENAMES] is specified, "
		"filenames will be listed instead."
	)
	COM_RETURN("The number of playlist entries displayed")
	SECTION
};

class LoadCommand : public Command
{
public:
	COM_STRUCT(LoadCommand, "load")
	
	virtual void execute(CommandContext &cnx) const
	{
		GList *playlist = 0;
		int olen = xmms_remote_get_playlist_length(cnx.session_id);

		if(cnx.args.size() < 2) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		for(unsigned i = 1; i < cnx.args.size(); i++)
			playlist = g_list_append(playlist, g_strdup(cnx.args[i].c_str()));
		xmms_remote_playlist_add(cnx.session_id, playlist);
		g_list_free(playlist);
		cnx.result_code = xmms_remote_get_playlist_length(cnx.session_id) - olen;
		printf("Loaded %d file%s\n", cnx.result_code, cnx.result_code - 1 ? "s" : "");
	}

	COM_SYNOPSIS("add music or playlist files to the playlist")
	COM_SYNTAX("LOAD filename...")
	COM_DESCRIPTION(
		"Attempts to load each of the filenames given as arguments.  If a file is "
		"a playlist file, the files specified within that playlist file are each "
		"appended to the playlist.  Otherwise the file itself is appended to the playlist."
	)
	COM_RETURN("The number of successfully loaded")
	SECTION
};

class SaveCommand : public Command
{
public:
	COM_STRUCT(SaveCommand, "save")
	
	virtual void execute(CommandContext &cnx) const
	{
		FILE *f;
		gint i, len;
		gchar **playlist;

		if(cnx.args.size() < 2) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		if(!(f = fopen(cnx.args[1].c_str(), "w"))) {
			fprintf(stderr, "Unable to open `%s': %s\n", cnx.args[1].c_str(), strerror(errno));
			cnx.result_code = COMERR_UNKNOWN;
			return;
		}
		if((playlist = get_playlist_filenames(cnx.session_id, &len))) {
			for(i = 0; i < len; i++)
				fprintf(f, "%s\n", playlist[i]);
			g_strfreev(playlist);
		}
		fclose(f);
		cnx.result_code = len;
	}

	COM_SYNOPSIS("save playlist to file")
	COM_SYNTAX("SAVE <filename>")
	COM_DESCRIPTION(
		"The SAVE command saves the playlist currently loaded by XMMS to a file of "
		"your choice.  The contents of the file will consist of the filenames of "
		"each of the files in the playlist, one per line."
	)
	COM_RETURN(
		"The number of entries saved to the file"
	)
	SECTION
};

class RandomTrackCommand : public Command
{
public:
    RandomTrackCommand() : Command("random-track")
    {
        add_alias("randomtrack");
    }
	
	virtual void execute(CommandContext &cnx) const
	{
		int len = xmms_remote_get_playlist_length(cnx.session_id);

		if(len < 2) {
			if(!len)
				printf("This command has no effect on an empty playlist.\n");
			else
				printf("This command has no effect when there is only one entry in the playlist.\n");
			cnx.result_code = COMERR_NOEFFECT;
			return;
		}

		int t = rand() % (len - 1);

		if(t >= xmms_remote_get_playlist_pos(cnx.session_id))
			t++;
		xmms_remote_set_playlist_pos(cnx.session_id, t);
		printf("Set playlist position to %d\n", t);
		cnx.result_code = 0;
	}

	COM_SYNOPSIS("jump to random track")
	COM_SYNTAX("RANDOM-TRACK")
	COM_DESCRIPTION(
		"The RANDOM-TRACK command causes XMMS to jump to a random track in the playlist.  "
		"Its effect is the same as the JUMP command.  If the playlist contains fewer than "
		"two entries, this command has no effect."
	)
	COM_RETURN("Always 0")
	SECTION
};

class CurrentTrackCommand : public Command
{
public:
    CurrentTrackCommand() : Command("current-track")
    {
        add_alias("currenttrack");
    }

    virtual void execute(CommandContext &cnx) const
    {
        int pos = xmms_remote_get_playlist_pos(cnx.session_id);
        char *title = xmms_remote_get_playlist_title(cnx.session_id, pos);

		printf("Current song: %d. %s\n", pos + 1, title ? title : "<no track selected>");
        cnx.result_code = pos + 1;
    }

    COM_SYNOPSIS("display current track")
    COM_SYNTAX("CURRENT-TRACK")
    COM_DESCRIPTION(
        "The CURRENT-TRACK command displays the name of the current track and "
        "its current position in the playlist."
    )
    COM_RETURN("The position of the current track (counting from 1)")
    SECTION
};

class RemoveCommand : public Command
{
public:
    RemoveCommand() : Command("remove") { }

    virtual void execute(CommandContext &cnx) const
    {
        int len, pos, pos2;

        len = xmms_remote_get_playlist_length(cnx.session_id);
		if(cnx.args.size() < 2 || !isdigit(cnx.args[1][0])) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		pos = atoi(cnx.args[1].c_str());
		if(pos < 1 || pos > len) {
			fprintf(stderr, "Invalid position %d.  Valid positions are >= 1 and <= %d.\n", pos, len);
			cnx.result_code = COMERR_SYNTAX;
        }
        if(cnx.args.size() > 2 && isdigit(cnx.args[2][0])) {
            pos2 = atoi(cnx.args[2].c_str());
            if(pos2 < pos || pos2 > len) {
			    fprintf(stderr, "Invalid position %d.  Valid positions for the second argument "
                                "are >= %d and <= %d.\n", pos2, pos, len);
                cnx.result_code = COMERR_SYNTAX;
                return;
            }
        } else {
            pos2 = pos;
        }
        while(pos2 >= pos) {
            xmms_remote_playlist_delete(cnx.session_id, --pos2);
        }
        cnx.result_code = len - xmms_remote_get_playlist_length(cnx.session_id);
    }

    COM_SYNOPSIS("remove track(s) from the playlist")
    COM_SYNTAX("REMOVE pos [pos2]")
    COM_DESCRIPTION(
        "The REMOVE command removes a track or range of tracks from the playlist.  "
        "The first parameter given specifies the position of the first track to "
        "remove.  If no second parameter is given, only the track at the given "
        "position is removed.  If the second parameter is given, and pos2 >= pos1, "
        "then all tracks with positions between pos and pos2 inclusive are removed "
        "from the playlist."
    )
    COM_RETURN("The number of tracks removed from the playlist")
    SECTION
};

static Command *commands[] = {
	new JumpCommand(),
	new NextCommand(),
	new PreviousCommand(),
	new ClearCommand(),
	new ListCommand(),
	new LoadCommand(),
	new SaveCommand(),
	new RandomTrackCommand(),
    new CurrentTrackCommand(),
    new RemoveCommand(),
};

void playlist_init(void)
{
	srand(time(0));
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

