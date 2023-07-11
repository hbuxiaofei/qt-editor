#include <QKeyEvent>

#include "completer.h"

bool MyCompleter::eventFilter(QObject *o, QEvent *e)
{
    return QCompleter::eventFilter(o, e);
}
