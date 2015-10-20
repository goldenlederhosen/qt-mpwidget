#ifndef VREGULAREXPRESSION_H
#define VREGULAREXPRESSION_H

#include <QRegularExpression>

#include <valgrind/valgrind.h>
#include <QtGlobal>

// a QRegularExpression that is always valid - it dies at construction time otherwise

class VRegularExpression : public QRegularExpression
{
public:
    typedef QRegularExpression super;
private:
    void check() const
    {
        if(Q_LIKELY(this->isValid())) {
            optimize();
            return;
        }

        qFatal("bad regexp %s: %s", qPrintable(this->pattern()), qPrintable(this->errorString()));
    }
    explicit VRegularExpression();
public:
    explicit VRegularExpression(const QRegularExpression &in): super(in)
    {
        check();
    }
    explicit VRegularExpression(const VRegularExpression &in): super(in)
    {
    }
    explicit VRegularExpression(char const *const in_latin1lit): super(QLatin1String(in_latin1lit), QRegularExpression::UseUnicodePropertiesOption)
    {
        check();
    }
    explicit VRegularExpression(const QString &in): super(in, QRegularExpression::UseUnicodePropertiesOption)
    {
        check();
    }
};

#endif // VREGULAREXPRESSION_H
