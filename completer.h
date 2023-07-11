#ifndef CUSTOMCOMPLETER_H
#define CUSTOMCOMPLETER_H

#include <QCompleter>

class MyCompleter : public QCompleter {
public:
    MyCompleter(const QStringList& completions, QObject *parent = nullptr) : QCompleter(completions, parent) {}

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
};


#endif // CUSTOMCOMPLETER_H
