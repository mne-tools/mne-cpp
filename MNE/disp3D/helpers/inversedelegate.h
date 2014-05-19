#ifndef INVERSEDELEGATE_H
#define INVERSEDELEGATE_H

#include <QAbstractItemDelegate>

class InverseDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    InverseDelegate(QObject *parent = 0);

private:


};

#endif // INVERSEDELEGATE_H
