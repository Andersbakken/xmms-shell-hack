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
        Session session = cnx.session;
        Playlist playlist = session.playlist();
        int pos;

        if(cnx.args.size() < 2 || !isdigit(cnx.args[1][0])) {
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
        pos = atoi(cnx.args[1].c_str());
        try {
            playlist.set_position(pos);
            printf("Jumped to position %d in the playlist.\n", pos);
            cnx.result_code = 0;
        } catch(PlaylistPositionOutOfBoundsException ex) {
            fprintf(stderr, "%s\n", ex.to_string().c_str());
            cnx.result_code = COMERR_SYNTAX;
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

    int traverse_playlist(const Session& session)
    {
        Playlist playlist = session.playlist();
        int pos = playlist.position();
        int len = playlist.length();

#if HAVE_XMMS_REMOTE_IS_SHUFFLE
        if(session.shuffle()) {
            if(jump < 0)
                playlist.prev();
            else
                playlist.next();
            usleep(100);
            return playlist.position();
        }
#endif
        pos += jump;
        if(pos < 1)
            pos = len + pos + 1;
        if(pos > len)
            pos -= len;
        playlist.set_position(pos);
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

        cnx.result_code = t.traverse_playlist(cnx.session);
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

        cnx.result_code = t.traverse_playlist(cnx.session);
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
        cnx.session.playlist().clear();
    }

    COM_SYNOPSIS("clear the current playlist")
    COM_SYNTAX("CLEAR")
    COM_DESCRIPTION("Clears all entries from the playlist.")
    COM_RETURN("Always 0")
    SECTION
};

class ListCommand : public Command
{
public:
    COM_STRUCT(ListCommand, "list")
    
    virtual void execute(CommandContext &cnx) const
    {
        Session session = cnx.session;
        Playlist playlist = session.playlist();
        vector<string> list;
        int start = 0, stop = -1, x = 1, digs, pos;
        bool filenames = false;

        if(cnx.args.size() > 1 && !strncasecmp("filenames", cnx.args[1].c_str(), cnx.args[1].length())) {
            filenames = true;
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
        if(filenames) {
            list = playlist.filenames();
        } else {
            list = playlist.titles();
        }
        pos = playlist.position();
        if(list.size()) {
            for(int i = digs = 1; i < (int) list.size(); i *= 10, digs++);
            for(int i = start; i < (int) list.size() && (stop == -1 || i <= stop); i++)
                printf("%c%*d. %s\n", pos == i + 1 ? '*' : ' ', digs, i + 1, list[i].c_str());
        } else {
            printf("Playlist is empty\n");
        }
        cnx.result_code = (stop != -1 && stop < (int) list.size() ? stop + 1 : list.size()) - start;
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

int Playlist::load(vector<string>::const_iterator start, vector<string>::const_iterator end) const
{
    session.ensure_running();

    GList *list = 0;
    int n = 0;

    while(start != end) {
        list = g_list_append(list, g_strdup(start->c_str()));
        start++;
        n++;
    }
    xmms_remote_playlist_add(session.id(), list);
    g_list_free(list);
    return n;
}

class LoadCommand : public Command
{
public:
    COM_STRUCT(LoadCommand, "load")
    
    virtual void execute(CommandContext &cnx) const
    {
        Session session = cnx.session;
        Playlist playlist = session.playlist();
        vector<string>::const_iterator i = cnx.args.begin();

        if(cnx.args.size() < 2) {
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
        i++;
        cnx.result_code = playlist.load(i, cnx.args.end());
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
        Session session = cnx.session;
        Playlist playlist = session.playlist();
        FILE *f;
        vector<string> list;

        if(cnx.args.size() < 2) {
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
        if(!(f = fopen(cnx.args[1].c_str(), "w"))) {
            fprintf(stderr, "Unable to open `%s': %s\n", cnx.args[1].c_str(), strerror(errno));
            cnx.result_code = COMERR_UNKNOWN;
            return;
        }
        list = playlist.filenames();
        for(vector<string>::const_iterator i = list.begin(); i != list.end(); i++) {
            fprintf(f, "%s\n", i->c_str());
        }
        fclose(f);
        cnx.result_code = list.size();
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
        Session session = cnx.session;
        Playlist playlist = session.playlist();
        int len = playlist.length();

        if(len < 2) {
            if(!len)
                printf("This command has no effect on an empty playlist.\n");
            else
                printf("This command has no effect when there is only one entry in the playlist.\n");
            cnx.result_code = COMERR_NOEFFECT;
            return;
        }

        int t = rand() % (len - 1) + 1;

        if(t >= playlist.position()) {
            t++;
        }
        playlist.set_position(t);
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
        Session session = cnx.session;
        Playlist playlist = session.playlist();

        if(playlist.length() == 0) {
            printf("The playlist is empty.\n");
            cnx.result_code = 0;
            return;
        }

        int pos = playlist.position();
        string title = playlist.title(pos);

        printf("Current song: %d. %s\n", pos, title.c_str());
        cnx.result_code = pos;
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
        Session session = cnx.session;
        Playlist playlist = session.playlist();
        int pos, pos2;

        if(cnx.args.size() < 2 || !isdigit(cnx.args[1][0])) {
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
        pos = atoi(cnx.args[1].c_str());
        if(cnx.args.size() > 2 && isdigit(cnx.args[2][0])) {
            pos2 = atoi(cnx.args[2].c_str());
        } else {
            pos2 = pos;
        }
        try {
            cnx.result_code = playlist.remove(pos, pos2);
        } catch(PlaylistPositionOutOfBoundsException ex) {
            fprintf(stderr, "%s\n", ex.to_string().c_str());
            cnx.result_code = COMERR_SYNTAX;
            return;
        }
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

Playlist::Playlist(const Session& _session) : session(_session)
{
}

Playlist::~Playlist()
{
}

void Playlist::clear(void) const
{
    session.ensure_running();
    xmms_remote_playlist_clear(session.id());
}

int Playlist::position(void) const
{
    session.ensure_running();
    return xmms_remote_get_playlist_pos(session.id()) + 1;
}

void Playlist::set_position(int pos) const
{
    check_position(pos);
    xmms_remote_set_playlist_pos(session.id(), pos - 1);
}

int Playlist::length(void) const
{
    session.ensure_running();
    return xmms_remote_get_playlist_length(session.id());
}

string Playlist::title(int pos) const
{
    check_position(pos);

    char *c_str = xmms_remote_get_playlist_title(session.id(), pos - 1);
    string str(c_str);

    g_free(c_str);
    return str;
}

string Playlist::current_title(void) const
{
    return title(position());
}

string Playlist::filename(int pos) const
{
    check_position(pos);

    char *c_str = xmms_remote_get_playlist_file(session.id(), pos - 1);
    string str(c_str);

    g_free(c_str);
    return str;
}

string Playlist::current_filename(void) const
{
    return filename(position());
}

int Playlist::next(void) const
{
    session.ensure_running();
    xmms_remote_playlist_next(session.id());
    return position();
}

int Playlist::prev(void) const
{
    session.ensure_running();
    xmms_remote_playlist_prev(session.id());
    return position();
}

void Playlist::check_position(int pos, int min_value) const
{
    if(pos < min_value || pos > length()) {
        throw PlaylistPositionOutOfBoundsException(*this, pos, min_value);
    }
}

PlaylistPositionOutOfBoundsException::PlaylistPositionOutOfBoundsException(const Playlist& playlist, int _position, int min_value)
    : Exception("PlaylistPositionOutOfBoundsException"), position(_position), length(playlist.length()), minv(min_value)
{
}

PlaylistPositionOutOfBoundsException::~PlaylistPositionOutOfBoundsException()
{
}

string PlaylistPositionOutOfBoundsException::to_string(void) const
{
    char buf[4096];
    if(length) {
        sprintf(buf, "Invalid position %d.  Valid positions for the second argument are >= %d and <= %d.", position, minv, length);
    } else {
        sprintf(buf, "Invalid position %d.  The playlist is empty.", position);
    }
    return buf;
}

vector<string> Playlist::filenames(void) const
{
    int len = length();
    vector<string> list(len);
    char *c_str;

    for(int i = 0; i < len; i++) {
        c_str = xmms_remote_get_playlist_file(session.id(), i);
        list[i] = c_str;
        g_free(c_str);
    }
    return list;
}

vector<string> Playlist::titles(void) const
{
    int len = length();
    vector<string> list(len);
    char *c_str;

    for(int i = 0; i < len; i++) {
        c_str = xmms_remote_get_playlist_title(session.id(), i);
        list[i] = c_str;
        g_free(c_str);
    }
    return list;
}

void Playlist::remove(int pos) const
{
    remove(pos, pos);
}

int Playlist::remove(int pos1, int pos2) const
{
    int n = 0;

    session.ensure_running();
    check_position(pos1);
    check_position(pos2, pos1);
    while(pos2 >= pos1) {
        xmms_remote_playlist_delete(session.id(), --pos2);
        n++;
    }
    return n;
}

