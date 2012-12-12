#ifndef DATACLIENT_H
#define DATACLIENT_H

#include <QThread>

class DataClient : public QThread
{
    Q_OBJECT
public:
    explicit DataClient(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // DATACLIENT_H
