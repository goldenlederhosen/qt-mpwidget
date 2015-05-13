#ifndef KEEPFOCUS_H
#define KEEPFOCUS_H

class QWidget;
class QObject;

bool focus_is_outside();

void focus_should_go_to(QWidget *top, char const *const file = __builtin_FILE(), int line = __builtin_LINE());
void focus_should_definitely_not_be(QObject *bad, char const *const file = __builtin_FILE(), int line = __builtin_LINE());

QWidget *focus_should_currently_be_at();

#endif // KEEPFOCUS_H
