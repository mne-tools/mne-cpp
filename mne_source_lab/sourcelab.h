#ifndef SOURCELAB_H
#define SOURCELAB_H


#include <QObject>

class SourceLab : public QObject
{
    Q_OBJECT
public:
    explicit SourceLab(QObject *parent = 0);

signals:
    void closeSourceLab();

public slots:

};

#endif // SOURCELAB_H
