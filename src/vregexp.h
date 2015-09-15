#ifndef VREGEXP_H
#define VREGEXP_H

#include <QRegExp>

#include <valgrind/valgrind.h>
#include <QtGlobal>

// a QRegExp that is always valid - it dies at construction time otherwise

class VRegExp : public QRegExp
{
private:
    void check() const
    {
        if(Q_LIKELY(this->isValid())) {
            return;
        }

        VALGRIND_PRINTF_BACKTRACE("bad regexp %s: %s\n", qPrintable(this->pattern()), qPrintable(this->errorString()));
        qFatal("bad regexp %s: %s", qPrintable(this->pattern()), qPrintable(this->errorString()));
    }
    explicit VRegExp();
public:
    explicit VRegExp(const QRegExp &in): QRegExp(in)
    {
        check();
    }
    explicit VRegExp(const VRegExp &in): QRegExp(in)
    {
    }
    explicit VRegExp(char const *const in): QRegExp(QLatin1String(in))
    {
        check();
    }
    explicit VRegExp(const QString &in): QRegExp(in)
    {
        check();
    }
};

#endif // VREGEXP_H
