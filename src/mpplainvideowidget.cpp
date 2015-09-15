#include "mpplainvideowidget.h"

#include <QPainter>

MpPlainVideoWidget::MpPlainVideoWidget(QWidget *parent): QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_NativeWindow);

    setMouseTracking(true);
    setObjectName(QLatin1String("QMPWidget_videotarget"));
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
QPaintEngine *MpPlainVideoWidget::paintEngine() const
{
    return NULL;
}
#endif
