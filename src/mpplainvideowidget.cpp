#include "mpplainvideowidget.h"

#include <QPainter>

#include "event_desc.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "MPPVW"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

MpPlainVideoWidget::MpPlainVideoWidget(QWidget *parent): super()
{
    setObjectName(QStringLiteral("QMPWidget_videotar"));
    setParent(parent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_NativeWindow);

    setFocusPolicy(Qt::NoFocus);

    setMouseTracking(true);
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
QPaintEngine *MpPlainVideoWidget::paintEngine() const
{
    return NULL;
}
#endif

bool MpPlainVideoWidget::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}

