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

    CommandManager& getCommandManager()
    {
        return m_commandManager;
    }

    void helpReceived(Command test)
    {
        qDebug() << "STILLL Help triggered in DataParserTest";
        qDebug() << "Command: " << test.command();
    }

private:
    CommandManager m_commandManager;

signals:
};


#endif // DATAPARSERTEST_H
