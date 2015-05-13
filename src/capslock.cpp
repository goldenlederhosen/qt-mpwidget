#include "capslock.h"
#include <QtGlobal>

#if defined Q_WS_WIN
# include <windows.h>
#elif defined Q_WS_X11
# include <X11/XKBlib.h>
# undef KeyPress
# undef KeyRelease
# undef FocusIn
# undef FocusOut
#else
# error Unsupported window system
#endif

#define DEBUG_CAPSLOCK

#ifdef DEBUG_ALL
#define DEBUG_CAPSLOCK
#endif

#ifdef DEBUG_CAPSLOCK
#define MYDBG(msg, ...) qDebug("CAPSLOCK " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

bool checkCapsLock()
{
    // platform dependent method of determining if CAPS LOCK is on
#ifdef Q_OS_WIN32 // MS Windows version
    const bool caps_state = (GetKeyState(VK_CAPITAL) == 1);
#else // X11 version (Linux/Unix/Mac OS X/etc...)
    Display *d = XOpenDisplay((char *)0);

    if(d == NULL) {
        MYDBG("could not open display");
        return false;
    }

    unsigned n;
    XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
    const bool caps_state = (n & 0x01) == 1;
#endif
    MYDBG("Capslock state: %s", caps_state ? "active" : "inactive");
    return caps_state;
}


