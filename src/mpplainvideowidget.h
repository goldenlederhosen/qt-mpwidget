#ifndef MPPLAINVIDEOWIDGET_H
#define MPPLAINVIDEOWIDGET_H

#include <QWidget>

class QPaintEvent;

// A plain video widget
class MpPlainVideoWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QWidget super;

private:
    // forbid
    MpPlainVideoWidget();
    MpPlainVideoWidget(const MpPlainVideoWidget &);
    MpPlainVideoWidget &operator=(const MpPlainVideoWidget &in);

public:
    explicit MpPlainVideoWidget(QWidget *parent);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    virtual QPaintEngine *paintEngine() const;
#endif

protected:
    virtual bool event(QEvent *event);

};

// other things to do if problems:
//    setUpdatesEnabled(false);
//    override paintEvent (and resizeEvent?) to do nothing
//    override paintEngine to return NULL?

#endif // MPPLAINVIDEOWIDGET_H
