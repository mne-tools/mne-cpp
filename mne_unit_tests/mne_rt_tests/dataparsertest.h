#ifndef DATAPARSERTEST_H
#define DATAPARSERTEST_H

#include <QObject>

#include <rtCommand/commandmanager.h>

using namespace RTCOMMANDLIB;

class DataParserTest : public QObject
{
    Q_OBJECT
public:
    explicit DataParserTest(QObject *parent = 0);

    void helpReceived()
    {
        qDebug() << "Help triggered in DataParserTest";
    }

private:
    CommandManager m_commandManager;

signals:
};


#endif // DATAPARSERTEST_H
