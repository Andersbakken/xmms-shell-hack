#include "config.h"
#include "session.h"
#include "playlist.h"
#include "window.h"
#include <xmmsctrl.h>

XmmsNotRunningException::XmmsNotRunningException(const Session& session)
    : Exception("XmmsNotRunningException", string("No instance of XMMS running under session identifier " + session.id()))
{
}

XmmsNotRunningException::~XmmsNotRunningException()
{
}

Session::Session(int id) : sid(id)
{
}

Session::Session(const Session& session) : sid(session.sid)
{
}

Session::~Session()
{
}

void Session::ensure_running(void) const
{
    if(!running()) {
        throw XmmsNotRunningException(*this);
    }
}

int Session::id(void) const
{
    return sid;
}

Playlist Session::playlist(void) const
{
    return Playlist(*this);
}

Window Session::window(void) const
{
    return Window(*this);
}

int Session::version(void) const
{
    ensure_running();
    return xmms_remote_get_version(sid);
}

bool Session::running(void) const
{
    return xmms_remote_is_running(sid);
}

void Session::stop(void) const
{
    ensure_running();
    xmms_remote_stop(sid);
}

void Session::play(void) const
{
    ensure_running();
    xmms_remote_play(sid);
}

bool Session::playing(void) const
{
    ensure_running();
    return xmms_remote_is_playing(sid);
}

void Session::pause(bool value) const
{
    ensure_running();
    if(paused() != value) {
        xmms_remote_pause(sid);
    }
}

void Session::unpause(void) const
{
    ensure_running();
    if(paused()) {
        xmms_remote_pause(sid);
    }
}

bool Session::pause_toggle(void) const
{
    ensure_running();
    xmms_remote_pause(sid);
    return paused();
}

bool Session::paused(void) const
{
    ensure_running();
    return xmms_remote_is_paused(sid);
}

void Session::playback_info(int& rate, int& freq, int &nch) const
{
    ensure_running();
    xmms_remote_get_info(sid, &rate, &freq, &nch);
}

void Session::jump_to_time(int t) const
{
    ensure_running();
    xmms_remote_jump_to_time(sid, t);
}

int Session::playback_time(void) const
{
    ensure_running();
    return xmms_remote_get_output_time(sid);
}

void Session::volume(int& left, int& right) const
{
    ensure_running();
    return xmms_remote_get_volume(sid, &left, &right);
}

void Session::set_volume(int left, int right) const
{
    ensure_running();
    xmms_remote_set_volume(sid, left, right);
}

int Session::balance(void) const
{
    ensure_running();
    return xmms_remote_get_balance(sid);
}

void Session::set_balance(int value) const
{
    ensure_running();
    xmms_remote_set_balance(sid, value);
}

#if HAVE_XMMS_REMOTE_IS_REPEAT

bool Session::repeat(void) const
{
    ensure_running();
    return xmms_remote_is_repeat(sid);
}

void Session::set_repeat(bool value) const
{
    bool v = repeat();

    if(v != value) {
        repeat_toggle();
    }
}

bool Session::repeat_toggle(void) const
{
    ensure_running();
    xmms_remote_toggle_repeat(sid);
    return repeat();
}

#else

void Session::repeat_toggle(void) const
{
    ensure_running();
    xmms_remote_toggle_repeat(sid);
}

#endif

#if HAVE_XMMS_REMOTE_IS_SHUFFLE

bool Session::shuffle(void) const
{
    ensure_running();
    return xmms_remote_is_shuffle(sid);
}

void Session::set_shuffle(bool value) const
{
    bool v = shuffle();

    if(v != value) {
        shuffle_toggle();
    }
}

bool Session::shuffle_toggle(void) const
{
    ensure_running();
    xmms_remote_toggle_shuffle(sid);
    return shuffle();
}

#else

void Session::shuffle_toggle(void) const
{
    ensure_running();
    xmms_remote_toggle_shuffle(sid);
}

#endif

string Session::skin(void) const
{
    ensure_running();

    return xmms_remote_get_skin(sid);
}

#if HAVE_XMMS_REMOTE_GET_EQ
void Session::eq(float& preamp, vector<float>& bands) const
{
    ensure_running();

    float *c_bands;

    xmms_remote_get_eq(sid, &preamp, &c_bands);
    bands = vector<float>(10);
    for(int i = 0; i < 10; i++) {
        bands[i] = c_bands[i];
    }
    g_free(c_bands);
}

float Session::eq_preamp(void) const
{
    ensure_running();
    return xmms_remote_get_eq_preamp(sid);
}

float Session::eq_band(int band) const
{
    ensure_running();
    return xmms_remote_get_eq_band(sid, band);
}

void Session::set_eq_preamp(float value) const
{
    ensure_running();
    xmms_remote_set_eq_preamp(sid, value);
}

void Session::set_eq_band(int band, float value) const
{
    ensure_running();
    xmms_remote_set_eq_band(sid, band, value);
}

#endif

void Session::quit(void) const
{
    ensure_running();
    xmms_remote_quit(sid);
}

