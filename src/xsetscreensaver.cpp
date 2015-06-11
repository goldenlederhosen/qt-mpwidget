#include "xsetscreensaver.h"
#include <QtGlobal>

#define DEBUG_XSSS

#ifdef DEBUG_ALL
#define DEBUG_XSSS
#endif

#ifdef DEBUG_XSSS
#define MYDBG(msg, ...) qDebug("XSSS " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

#if defined Q_WS_X11
# include <X11/Xlib.h>
# include <X11/X.h>
#elif defined Q_OS_LINUX
# include <X11/Xlib.h>
# include <X11/X.h>
#else
#error Unsupported window system
#endif

void xsetscreensaver_enable()
{

    Display *d = XOpenDisplay(NULL);

    if(d == NULL) {
        MYDBG("XOpenDisplay() failed");
        return;
    }

    int ret = XSetScreenSaver(d, 600, 600, PreferBlanking, AllowExposures);
    MYDBG("called XSetScreenSaver(600, 600, PreferBlanking, AllowExposures) = %d", ret);

    // int timeout, int interval, int prefer_blanking, int allow_exposures
    //   timeout: in seconds till turn on. 0 = not, -1=default
    //   interval: seconds between screen saver alterations, -1=default
    //   blank: 0 don't, 1 prefer, 2 default
    //   exp: 0 do't allow, 1 allow, 2 default

    // xset s on; xset s blank
    // observed this: XSetScreenSaver(-1, -1, 2, 2); XSetScreenSaver(600, 600, 1, 1)
}

void xsetscreensaver_disable()
{
    Display *d = XOpenDisplay(NULL);

    if(d == NULL) {
        MYDBG("XOpenDisplay() failed");
        return;
    }

    int ret = XSetScreenSaver(d, 0, 600, PreferBlanking, AllowExposures);
    MYDBG("called XSetScreenSaver(0, 600, PreferBlanking, AllowExposures) = %d", ret);

    // xset s off
    // observed: XSetScreenSaver(0, 600, 1, 1)


}

