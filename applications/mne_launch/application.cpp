#include "application.h"
#include "mnelaunchcontrol.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Application::Application()
: m_pQMLEngine (new QQmlApplicationEngine)
, m_pLaunchControl (new MNELaunchControl)
{

}


//*************************************************************************************************************

int Application::init(int &argc, char **argv, QGuiApplication& app)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    Q_UNUSED(app)

    registerTypes();

    m_pQMLEngine->rootContext()->setContextProperty("launchControl", m_pLaunchControl);
    m_pQMLEngine->load(QUrl(QLatin1String("qrc:/qml/main.qml")));

    return 0;
}


//*************************************************************************************************************

void Application::registerTypes()
{
    // Register QML types hier
}


//*************************************************************************************************************

void Application::start()
{
}
