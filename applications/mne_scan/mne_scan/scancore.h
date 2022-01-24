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
    void loadPlugins();

    void loadGui();

    std::unique_ptr<MainWindow> p_MainWindow;
    std::shared_ptr<SCSHAREDLIB::PluginManager> p_PluginManager;
};
}//namespace
#endif // SCANCORE_H
