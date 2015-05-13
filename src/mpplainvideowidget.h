#ifndef MPPLAINVIDEOWIDGET_H
#define MPPLAINVIDEOWIDGET_H

#include <QWidget>

class QPaintEvent;

// A plain video widget
class MpPlainVideoWidget : public QWidget {
    Q_OBJECT

public:
    explicit MpPlainVideoWidget(QWidget *parent = 0);

};

// other things to do if problems:
//    setUpdatesEnabled(false);
//    override paintEvent (and resizeEvent?) to do nothing
//    override paintEngine to return NULL

#endif // MPPLAINVIDEOWIDGET_H
