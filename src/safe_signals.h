#ifndef SAFE_SIGNALS_H
#define SAFE_SIGNALS_H

#include "util.h"

#define INVOKE_DELAY(X)    QTimer::singleShot(0, this, SLOT(X));

#define QUEUEDCONN         Qt::QueuedConnection

#define xinvokeMethod(...) do{ \
       if(!QMetaObject::invokeMethod(__VA_ARGS__)){\
          PROGRAMMERERROR("method call failed");\
       }\
    } while(0)

#define XCONNECT(...)      do{ \
       if(!QObject::connect(__VA_ARGS__)){\
          PROGRAMMERERROR("connect failed");\
       }\
    } while(0)

#endif // SAFE_SIGNALS_H
