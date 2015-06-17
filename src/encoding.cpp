#include "encoding.h"

#include "util.h"

#define DEBUG_ENCODING

#ifdef DEBUG_ALL
#define DEBUG_ENCODING
#endif

#ifdef DEBUG_ENCODING
#define MYDBG(msg, ...) qDebug("UTIL " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

QString xbin_2_codec_qstring(bool doerr, char const *const blob, const size_t len, QTextCodec const *const codec)
{
    if(blob == NULL) {
        PROGRAMMERERROR("NULL blob-string?");
    }

    if(len == 0) {
        return QString();
    }

    if(codec == NULL) {
        qFatal("Blob-string \"%s\": no codec found?", blob);
    }

    QTextCodec::ConverterState state;
    const QString string = codec->toUnicode(blob, len, &state);

    if(state.invalidChars > 0) {
        if(doerr) {
            qFatal("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }
        else {
            qWarning("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }

    }

    if(state.remainingChars > 0) {
        if(doerr) {
            qFatal("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }
        else {
            qWarning("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }
    }

    return string;
}

