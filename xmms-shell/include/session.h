#ifndef _XMMS_SHELL_SESSION_H_

#define _XMMS_SHELL_SESSION_H_

#include "exception.h"
#include <string>
#include <vector>

using namespace std;

class Playlist;
class Window;

class Session
{
    int sid;

public:
    Session(int id = 0);
    Session(const Session& session);
    ~Session();

    Playlist playlist(void) const;
    Window window(void) const;
    int id(void) const;
    void ensure_running(void) const;
    bool running(void) const;
    int version(void) const;
    void stop(void) const;
    bool playing(void) const;
    void play(void) const;
    bool paused(void) const;
    void pause(bool value = true) const;
    void unpause(void) const;
    bool pause_toggle(void) const;
    void playback_info(int& rate, int& freq, int& nch) const;
    int playback_time(void) const;
    void jump_to_time(int t) const;
    void volume(int& left, int& right) const;
    void set_volume(int left, int right) const;
    int balance(void) const;
    void set_balance(int value) const;
#if HAVE_XMMS_REMOTE_IS_REPEAT
    bool repeat(void) const;
    void set_repeat(bool value = true) const;
    bool repeat_toggle(void) const;
#else
    void repeat_toggle(void) const;
#endif
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
    bool shuffle(void) const;
    void set_shuffle(bool value = true) const;
    bool shuffle_toggle(void) const;
#else
    void shuffle_toggle(void) const;
#endif
    string skin(void) const;
#if HAVE_XMMS_REMOTE_GET_EQ
    void eq(float& preamp, vector<float>& bands) const;
    float eq_preamp(void) const;
    float eq_band(int band) const;
    void set_eq_preamp(float value) const;
    void set_eq_band(int band, float value) const;
#endif
    void quit(void) const;
};

class XmmsNotRunningException : public Exception
{
public:
    XmmsNotRunningException(const Session& session);
};

#endif

