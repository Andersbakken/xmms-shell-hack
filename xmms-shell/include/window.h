#ifndef _XMMS_SHELL_WINDOW_H_

#define _XMMS_SHELL_WINDOW_H_

#include "session.h"

class Window
{
    Session session;

public:
    Window(const Session& session);
    Window(const Window& window);
    ~Window();

    void ensure_running(void) const;

    bool main(void) const;
    bool playlist(void) const;
    bool equalizer(void) const;

    void show_main(bool value = false) const;
    void show_playlist(bool value = false) const;
    void show_equalizer(bool value = false) const;

    bool toggle_main(void) const;
    bool toggle_playlist(void) const;
    bool toggle_equalizer(void) const;

    void eject(void) const;
    void preferences(void) const;
};

void window_init(void);

#endif

