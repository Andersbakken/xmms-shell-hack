#ifndef _XMMS_SHELL_PLAYLIST_H_

#define _XMMS_SHELL_PLAYLIST_H_

#include "session.h"

#include <vector>
#include <iterator>

using namespace std;

class Playlist
{
    Session session;
public:
    Playlist(const Session& session);
    ~Playlist();

    int length(void) const;
    int position(void) const;
    void set_position(int pos) const;
    void check_position(int pos, int min_value = 1) const;
    string current_title(void) const;
    string title(int pos) const;
    string current_filename(void) const;
    string filename(int pos) const;
    vector<string> filenames(void) const;
    vector<string> titles(void) const;
    int next(void) const;
    int prev(void) const;
    void clear(void) const;
    int load(vector<string>::const_iterator start, vector<string>::const_iterator end);
    void remove(int pos) const;
    int remove(int pos1, int pos2) const;
};

class PlaylistPositionOutOfBoundsException : public Exception
{
    int position, length, minv;

public:
    PlaylistPositionOutOfBoundsException(const Playlist& playlist, int position, int min_value = 1);
    virtual ~PlaylistPositionOutOfBoundsException();

    virtual string to_string(void) const;
};

void playlist_init(void);

#endif

