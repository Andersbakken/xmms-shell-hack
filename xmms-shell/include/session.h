#ifndef _XMMS_SHELL_SESSION_H_

#define _XMMS_SHELL_SESSION_H_

#include "config.h"
#include "exception.h"
#include <string>
#include <vector>
#if HAVE_XMMS_SESSION_CONNECT
#include <xmmssess.h>
#else
#include <xmmsctrl.h>
#endif

using namespace std;

class Playlist;
class Window;

class Session
{
public:
    typedef enum { STOPPED, PLAYING, PAUSED } PlayMode;

private:
#if HAVE_XMMS_SESSION_CONNECT
    XMMSSession *xs;
    guint32 version;
    Session::PlayMode mode;
    gint32 rate;
    gint32 freq;
    gint32 nch;
    gint32 left_volume;
    gint32 right_volume;
    gint32 balance;
    gboolean repeat;
    gboolean shuffle;
    string skin;
    float preamp;
    vector<float> bands;

    void update_state(void);

#endif
    gint32 sid;

public:
    Session(int id = 0);
    Session(const Session& session);
    ~Session();

    Playlist get_playlist(void) const;
    Window get_window(void) const;
    gint32 get_id(void) const;
    void ensure_running(void) const;
    gboolean is_running(void) const;
    guint32 get_version(void);
    void stop(void);
    gboolean is_playing(void);
    void play(void);
    gboolean is_paused(void);
    void pause(gboolean value = true);
    void unpause(void);
    Session::PlayMode pause_toggle(void);
    Session::PlayMode get_play_mode(void);
    void get_playback_info(gint32& rate, gint32& freq, gint32& nch);
    gint32 get_playback_time(void);
    void jump_to_time(gint32 t);
    void get_volume(gint32& left, gint32& right);
    void set_volume(gint32 left, gint32 right);
    gint32 get_balance(void);
    void set_balance(gint32 value);
#if HAVE_XMMS_REMOTE_IS_REPEAT || HAVE_XMMS_SESSION_CONNECT
    gboolean is_repeat(void);
    void set_repeat(gboolean value = true);
    gboolean repeat_toggle(void);
#else
    void repeat_toggle(void);
#endif
#if HAVE_XMMS_REMOTE_IS_SHUFFLE || HAVE_XMMS_SESSION_CONNECT
    gboolean is_shuffle(void);
    void set_shuffle(gboolean value = true);
    gboolean shuffle_toggle(void);
#else
    void shuffle_toggle(void);
#endif
    string get_skin(void);
#if HAVE_XMMS_REMOTE_GET_EQ || HAVE_XMMS_SESSION_CONNECT
    void get_eq(float& preamp, vector<float>& bands);
    float get_eq_preamp(void);
    float get_eq_band(gint32 band);
    const vector<float>& get_eq_bands(void);
    void set_eq_preamp(float value);
    void set_eq_bands(const vector<float>& bands);
    void set_eq_band(gint32 band, float value);
#endif
    void quit(void);
};

class XmmsNotRunningException : public Exception
{
public:
    XmmsNotRunningException(const Session& session);
    virtual ~XmmsNotRunningException();
};

#if HAVE_XMMS_SESSION_CONNECT
class XmmsQueryFailureException : public Exception
{
public:
    XmmsQueryFailureException(XMMSQueryResult result, const string& filename, int line);
    virtual ~XmmsQueryFailureException();
};
#endif

#endif

