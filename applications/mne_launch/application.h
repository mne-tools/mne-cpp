#ifndef APPLICATION_H
#define APPLICATION_H


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPointer>


class MNELaunchControl;


class Application : public QObject
{
    Q_OBJECT
public:
    Application();

    int init(int &argc, char **argv, QGuiApplication& app);

    void registerTypes();

    void start();

private:
    QPointer<QQmlApplicationEngine> m_pQMLEngine;
    QPointer<MNELaunchControl>      m_pLaunchControl;

};

#endif // APPLICATION_H
