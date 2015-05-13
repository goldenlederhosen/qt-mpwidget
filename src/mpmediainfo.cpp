#include "mpmediainfo.h"

#include "vregexp.h"

static unsigned QSToUInt(const QString &s)
{
    bool ok = false;
    unsigned ret = s.toUInt(&ok);

    if(ok == false) {
        PROGRAMMERERROR("could not convert \"%s\" to unsigned", qPrintable(s));
    }

    return ret;
}

void MpMediaInfo::set_crop(const QString &str)
{
    if(str.isEmpty()) {
        mc_crop.force_set(QRect());
        return;
    }

    static VRegExp rxcs("^(\\d+):(\\d+):(\\d+):(\\d+)$");

    if(rxcs.indexIn(str) < 0) {
        qWarning("bad cropstring %s", qPrintable(str));
        mc_crop.force_set(QRect());
        return;
    }

    const QString ws = rxcs.cap(1);
    const QString hs = rxcs.cap(2);
    const QString xs = rxcs.cap(3);
    const QString ys = rxcs.cap(4);
    const size_t w = QSToUInt(ws);
    const size_t h = QSToUInt(hs);
    const size_t x = QSToUInt(xs);
    const size_t y = QSToUInt(ys);

    QRect r(x, y, w, h);
    set_crop(r);
    return;
}

void  MpMediaInfo::set_crop(const QRect &rect)
{
    const int cw = rect.width();
    const int ch = rect.height();
    const int cl = rect.left();
    const int ct = rect.top();

    if(cw < 1) {
        PROGRAMMERERROR("cropped width %d too small", cw);
    }

    if(ch < 1) {
        PROGRAMMERERROR("cropped height %d too small", ch);
    }

    if(cl < 0) {
        PROGRAMMERERROR("cropped left %d too small", cl);
    }

    if(ct < 0) {
        PROGRAMMERERROR("cropped top %d too small", ct);
    }

    if(has_size()) {
        const int mw = width();
        const int mh = height();

        if(mw < cw) {
            PROGRAMMERERROR("cropped width %d>media width %d", cw, mw);
        }

        if(mh < ch) {
            PROGRAMMERERROR("cropped height %d>media height %d", ch, mh);
        }

        if(cl + cw > mw) {
            PROGRAMMERERROR("cropped width %d + left %d > media width %d", cw, cl, mw);
        }

        if(ct + ch > mh) {
            PROGRAMMERERROR("cropped height %d + top %d > media height %d", ch, ct, mh);
        }
    }

    // FIXME this is a bit of a fudge...
    mc_crop.force_set(rect);
}
