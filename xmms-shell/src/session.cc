#include "config.h"
#include "session.h"
#include "playlist.h"
#include "util.h"
#include "window.h"

#if HAVE_XMMS_SESSION_CONNECT
# define SESSION 1
# include <xmmssess.h>
#else
#include <xmmsctrl.h>
#endif

XmmsNotRunningException::XmmsNotRunningException(const Session& session)
    : Exception("XmmsNotRunningException", "No instance of XMMS running under session identifier " + int_to_string(session.get_id()))
{
}

XmmsNotRunningException::~XmmsNotRunningException()
{
}

#if SESSION
XmmsQueryFailureException::XmmsQueryFailureException(XMMSQueryResult result, const string& filename, int line)
    : Exception("XmmsQueryFailureException", string(xmms_session_query_result_name(result)) + string(" at ") + filename + string(":") + int_to_string(line))
{
}

XmmsQueryFailureException::~XmmsQueryFailureException()
{
}
#endif

Session::Session(int id) : sid(id)
{
#if SESSION
    xs = xmms_session_connect(id);
    xmms_session_watch(xs);
    xmms_session_authenticate(xs, NULL);
    //xmms_session_debug(true);
#endif
}

Session::Session(const Session& session) : sid(session.sid)
{
#if SESSION
    //xs = xmms_session_connect(sid);
    xs = session.xs;
#endif
}

Session::~Session()
{
#if SESSION
    /*
    if(xs) {
        xmms_session_disconnect(xs);
    }
    */
#endif
}

#if SESSION
void Session::update_state(void)
{
#define X_ASSERT(X) if((xqr = (X)) != QUERY_SUCCESS) throw XmmsQueryFailureException(xqr, __FILE__, __LINE__)
    XMMSQueryResult xqr;
    gboolean bv1, bv2;
    gchar *sv;
    gfloat *fv;

    X_ASSERT(xmms_session_get_version(xs, &version));
    X_ASSERT(xmms_session_get_play_status(xs, &bv1));
    X_ASSERT(xmms_session_get_pause_status(xs, &bv2));
    if(bv1) {
        if(bv2) {
            mode = PAUSED;
        } else {
            mode = PLAYING;
        }
    } else {
        mode = STOPPED;
    }
    X_ASSERT(xmms_session_get_track_info(xs, &rate, &freq, &nch));
    X_ASSERT(xmms_session_get_volume(xs, &left_volume, &right_volume));
    X_ASSERT(xmms_session_get_balance(xs, &balance));
    X_ASSERT(xmms_session_get_repeat_status(xs, &repeat));
    X_ASSERT(xmms_session_get_shuffle_status(xs, &shuffle));
    X_ASSERT(xmms_session_get_skin(xs, &sv));
    skin = sv;
    g_free(sv);
    X_ASSERT(xmms_session_get_eq(xs, &preamp, &fv));
    bands = vector<float>(10);
    for(int i = 0; i < 10; i++) {
        bands[i] = fv[i];
    }
    g_free(fv);
}
#endif

void Session::ensure_running(void) const
{
    if(!is_running()) {
        throw XmmsNotRunningException(*this);
    }
}

guint32 Session::get_id(void) const
{
    return sid;
}

Playlist Session::get_playlist(void) const
{
    return Playlist(*this);
}

Window Session::get_window(void) const
{
    return Window(*this);
}

guint32 Session::get_version(void)
{
#if SESSION
    return version;
#else
    ensure_running();
    return xmms_remote_get_version(sid);
#endif
}

gboolean Session::is_running(void) const
{
#if SESSION
    return xmms_session_ping(xs, 0) == QUERY_SUCCESS;
#else
    return xmms_remote_is_running(sid);
#endif
}

void Session::stop(void)
{
    ensure_running();
#if SESSION
    xmms_session_stop(xs);
#else
    xmms_remote_stop(sid);
#endif
}

void Session::play(void)
{
    ensure_running();
#if SESSION
    xmms_session_play(xs);
#else
    xmms_remote_play(sid);
#endif
}

gboolean Session::is_playing(void)
{
#if SESSION
    return mode != STOPPED;
#else
    ensure_running();
    return xmms_remote_is_playing(sid);
#endif
}

void Session::pause(gboolean value)
{
    ensure_running();
#if SESSION
    if(mode != PAUSED) {
        xmms_session_pause(xs);
    }
#else
    if(is_paused() != value) {
        xmms_remote_pause(sid);
    }
#endif
}

void Session::unpause(void)
{
    ensure_running();
#if SESSION
    if(mode == PAUSED) {
        xmms_session_pause_toggle(xs);
    }
#else
    if(is_paused()) {
        xmms_remote_pause(sid);
    }
#endif
}

Session::PlayMode Session::pause_toggle(void)
{
    ensure_running();
#if SESSION
    xmms_session_pause_toggle(xs);
    switch(mode) {
        case STOPPED:
            break;
        case PLAYING:
            mode = PAUSED;
            break;
        case PAUSED:
            mode = PLAYING;
            break;
    }
    return mode;
#else
    xmms_remote_pause(sid);
    return is_paused();
#endif
}

gboolean Session::is_paused(void)
{
#if SESSION
    return mode == PAUSED;
#else
    ensure_running();
    return xmms_remote_is_paused(sid);
#endif
}

Session::PlayMode Session::get_play_mode(void)
{
#if SESSION
    return mode;
#else
    if(is_playing()) {
        if(is_paused()) {
            return PAUSED;
        } else {
            return PLAYING;
        }
    } else {
        return STOPPED;
    }
#endif
}

void Session::get_playback_info(guint32& rate, guint32& freq, guint32 &nch)
{
#if SESSION
    rate = this->rate;
    freq = this->freq;
    nch = this->nch;
#else
    ensure_running();
    xmms_remote_get_info(sid, &rate, &freq, &nch);
#endif
}

void Session::jump_to_time(guint32 t)
{
#if SESSION
    xmms_session_jump_to_time(xs, t);
#else
    ensure_running();
    xmms_remote_jump_to_time(sid, t);
#endif
}

guint32 Session::get_playback_time(void)
{
    ensure_running();
#if SESSION
    XMMSQueryResult xqr;
    guint32 t;

    if((xqr = xmms_session_get_output_time(xs, &t)) != QUERY_SUCCESS) {
        throw new XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    return t;
#else
    return xmms_remote_get_output_time(sid);
#endif
}

void Session::get_volume(guint32& left, guint32& right)
{
#if SESSION
    XMMSQueryResult xqr;
    
    if((xqr = xmms_session_get_volume(xs, &left_volume, &right_volume)) != QUERY_SUCCESS) {
        throw new XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    left = left_volume;
    right = right_volume;
#else
    ensure_running();
    xmms_remote_get_volume(sid, &left, &right);
#endif
}

void Session::set_volume(guint32 left, guint32 right)
{
    ensure_running();
#if SESSION
    xmms_session_set_volume(xs, left, right);
    left_volume = left;
    right_volume = right;
#else
    xmms_remote_set_volume(sid, left, right);
#endif
}

gint32 Session::get_balance(void)
{
#if SESSION
    return balance;
#else
    ensure_running();
    return xmms_remote_get_balance(sid);
#endif
}

void Session::set_balance(gint32 value)
{
    ensure_running();
#if SESSION
    xmms_session_set_balance(xs, value);
    balance = value;
#else
    xmms_remote_set_balance(sid, value);
#endif
}

#if HAVE_XMMS_REMOTE_IS_REPEAT || SESSION

gboolean Session::is_repeat(void)
{
    ensure_running();
#if SESSION
    XMMSQueryResult xqr;

    if((xqr = xmms_session_get_repeat_status(xs, &repeat)) != QUERY_SUCCESS) {
        throw XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    return repeat;
#else
    return xmms_remote_is_repeat(sid);
#endif
}

void Session::set_repeat(gboolean value)
{
#if SESSION
    ensure_running();
    xmms_session_set_repeat_status(xs, value);
    repeat = value;
#else
    gboolean v = is_repeat();

    if(v != value) {
        repeat_toggle();
    }
#endif
}

gboolean Session::repeat_toggle(void)
{
#if SESSION
    set_repeat(!is_repeat());
    return repeat;
#else
    ensure_running();
    xmms_remote_toggle_repeat(sid);
    return repeat();
#endif
}

#else

void Session::repeat_toggle(void)
{
    ensure_running();
    xmms_remote_toggle_repeat(sid);
}

#endif

#if HAVE_XMMS_REMOTE_IS_SHUFFLE || SESSION

gboolean Session::is_shuffle(void)
{
    ensure_running();
#if SESSION
    XMMSQueryResult xqr;

    if((xqr = xmms_session_get_shuffle_status(xs, &shuffle)) != QUERY_SUCCESS) {
        throw XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    return shuffle;
#else
    return xmms_remote_is_shuffle(sid);
#endif
}

void Session::set_shuffle(gboolean value)
{
#if SESSION
    ensure_running();
    xmms_session_set_shuffle_status(xs, value);
    shuffle = value;
#else
    gboolean v = shuffle();

    if(v != value) {
        shuffle_toggle();
    }
#endif
}

gboolean Session::shuffle_toggle(void)
{
#if SESSION
    set_shuffle(!is_shuffle());
    return shuffle;
#else
    ensure_running();
    xmms_remote_toggle_shuffle(sid);
    return shuffle();
#endif
}

#else

void Session::shuffle_toggle(void)
{
    ensure_running();
    xmms_remote_toggle_shuffle(sid);
}

#endif

string Session::get_skin(void)
{
#if SESSION
    XMMSQueryResult xqr;
    char *sv;

    if((xqr = xmms_session_get_skin(xs, &sv)) != QUERY_SUCCESS) {
        throw XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    skin = sv;
    g_free(sv);
    return skin;
#else
    ensure_running();
    return xmms_remote_get_skin(sid);
#endif
}

#if HAVE_XMMS_REMOTE_GET_EQ || SESSION

void Session::get_eq(float& preamp, vector<float>& bands)
{
#if SESSION
    XMMSQueryResult xqr;
    float *fv;

    if((xqr = xmms_session_get_eq(xs, &this->preamp, &fv)) != QUERY_SUCCESS) {
        throw XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    this->bands = vector<float>(10);
    for(int i = 0; i < 10; i++) {
        this->bands[i] = fv[i];
    }
    g_free(fv);
    preamp = this->preamp;
    bands = this->bands;
#else
    ensure_running();

    float *c_bands;

    xmms_remote_get_eq(sid, &preamp, &c_bands);
    bands = vector<float>(10);
    for(int i = 0; i < 10; i++) {
        bands[i] = c_bands[i];
    }
    g_free(c_bands);
#endif
}

float Session::get_eq_preamp(void)
{
#if SESSION
    XMMSQueryResult xqr;

    if((xqr = xmms_session_get_eq_preamp(xs, &preamp)) != QUERY_SUCCESS) {
        throw XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    return preamp;
#else
    ensure_running();
    return xmms_remote_get_eq_preamp(sid);
#endif
}

float Session::get_eq_band(guint32 band)
{
#if SESSION
    XMMSQueryResult xqr;
    float v;

    if((xqr = xmms_session_get_eq_band(xs, band, &v)) != QUERY_SUCCESS) {
        throw XmmsQueryFailureException(xqr, __FILE__, __LINE__);
    }
    bands[band] = v;
    return bands[band];
#else
    ensure_running();
    return xmms_remote_get_eq_band(sid, band);
#endif
}

void Session::set_eq_preamp(float value)
{
    ensure_running();
#if SESSION
    xmms_session_set_eq_preamp(xs, value);
    preamp = value;
#else
    xmms_remote_set_eq_preamp(sid, value);
#endif
}

void Session::set_eq_band(guint32 band, float value)
{
    ensure_running();
#if SESSION
    xmms_session_set_eq_band(xs, band, value);
    bands[band] = value;
#else
    xmms_remote_set_eq_band(sid, band, value);
#endif
}

#endif

void Session::quit(void)
{
    ensure_running();
#if SESSION
    xmms_session_quit(xs);
#else
    xmms_remote_quit(sid);
#endif
}

