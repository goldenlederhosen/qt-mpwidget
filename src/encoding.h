#ifndef ENCODING_H
#define ENCODING_H

#include <QTextCodec>
#include <QString>
#include <QByteArray>

QString xbin_2_codec_qstring(bool doerr, char const *const blob, const size_t len, QTextCodec const *const codec);

inline QString warn_xbin_2_local_qstring(char const *const blob)
{
    const size_t len = ((blob == NULL) ? 0 : strlen(blob));
    return xbin_2_codec_qstring(false, blob, len, QTextCodec::codecForLocale());
}

inline QString warn_xbin_2_local_qstring(const QByteArray &vblob)
{
    return xbin_2_codec_qstring(false, vblob.constData(), vblob.size(), QTextCodec::codecForLocale());
}


inline QString err_xbin_2_local_qstring(char const *const blob)
{
    const size_t len = ((blob == NULL) ? 0 : strlen(blob));
    return xbin_2_codec_qstring(true, blob, len, QTextCodec::codecForLocale());
}

inline QString err_xbin_2_local_qstring(const QByteArray &vblob)
{
    return xbin_2_codec_qstring(true, vblob.constData(), vblob.size(), QTextCodec::codecForLocale());
}

inline QString err_xbin_2_utf8_qstring(char const *const blob)
{
    const size_t len = ((blob == NULL) ? 0 : strlen(blob));
    return xbin_2_codec_qstring(true, blob, len, QTextCodec::codecForName("UTF-8"));
}

#endif // ENCODING_H
