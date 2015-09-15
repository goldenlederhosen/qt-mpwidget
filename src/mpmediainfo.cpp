#include "mpmediainfo.h"

#include "vregexp.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "MPM"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MPMMYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

void MpMediaInfo::set_DAR(double D)
{
    MPMMYDBG("set_DAR(%f)", D);
    mc_DAR.set(D);
}

void  MpMediaInfo::set_PAR(double P)
{
    MPMMYDBG("set_PAR(%f)", P);
    mc_PAR.set(P);
}
double MpMediaInfo::PAR() const
{
#ifdef CAUTION

    if(mc_PAR.isset() && mc_DAR.isset()) {
        const double P = mc_PAR.get();
        const double D = mc_DAR.get();
        const int h = height();
        const int w = width();
        const double P2 = D * h / w;

        if(fabs(P * w - D * h) > 5) {
            qFatal("PAR isset: %f. But DAR isset: PAR %f = %f * %d / %d", P, P2, D, h, w);
        }
    }

#endif

    if(mc_PAR.isset()) {
        const double P = mc_PAR.get();
        MPMMYDBG("PAR isset: %f", P);
        return P;
    }

    if(mc_DAR.isset()) {
        const double D = mc_DAR.get();
        const int h = height();
        const int w = width();
        const double ret = D * h / w;
        MPMMYDBG("DAR isset: PAR %f = %f * %d / %d", ret, D, h, w);
        return ret;
    }

    MPMMYDBG("neither DAR nor PAR isset: 1.0");
    const double one = 1.0;
    return one;
}
double MpMediaInfo::DAR() const
{
#ifdef CAUTION

    if(mc_DAR.isset() && mc_PAR.isset()) {
        const double D = mc_DAR.get();
        const double P = mc_PAR.get();
        const int w = width();
        const int h = height();
        const double D2 = P * w / h;

        if(fabs(D * h - P * w) > 5) {
            qFatal("DAR isset: %f. But PAR isset: DAR %f = %f * %d / %d", D, D2, P, w, h);
        }
    }

#endif

    if(mc_DAR.isset()) {
        const double D = mc_DAR.get();
        MPMMYDBG("DAR isset: %f", D);
        return D;
    }

    if(mc_PAR.isset()) {
        const double P = mc_PAR.get();
        const int w = width();
        const int h = height();
        const double ret = P * w / h;
        MPMMYDBG("PAR isset: DAR %f = %f * %d / %d", ret, P, w, h);
        return ret;
    }

    const int w = width();
    const int h = height();
    const double ret = w / h;
    MPMMYDBG("neither DAR nor PAR isset: DAR %f = %d / %d", ret, w, h);
    return ret;
}

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
