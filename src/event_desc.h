#ifndef EVENT_DESC_H
#define EVENT_DESC_H

class QEvent;
class QObject;
class QLoggingCategory;

void log_qevent(QLoggingCategory const &lcat, QObject *receiver, QEvent *event);
void log_qeventFilter(QLoggingCategory const &lcat, QObject *receiver, QEvent *event);
void init_signals_spy();

#endif /* EVENT_DESC_H */
