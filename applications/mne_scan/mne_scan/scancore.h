#ifndef SCANCORE_H
#define SCANCORE_H

#include <QObject>
#include <memory>

#include "mainwindow.h"
#include "scShared/Management/pluginmanager.h"

namespace MNESCAN
{

class ScanCore : public QObject
{
    Q_OBJECT
public:
    explicit ScanCore(QObject *parent = nullptr);

private:

    void registerMetatypes();
    void initPluginManager();
    void initMainWindow();

    std::unique_ptr<MainWindow> m_pMainWindow;
    std::shared_ptr<SCSHAREDLIB::PluginManager> m_pPluginManager;

    bool m_bIsRunning;
};
}//namespace
#endif // SCANCORE_H
