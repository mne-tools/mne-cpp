//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_analyze_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Starts MNE Analyze offscreen with its plugins and a raw file, and drives the main window.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <analyzecore.h>
#include <mainwindow.h>

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QMenuBar>
#include <QSettings>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QtTest>

#include <memory>

//=============================================================================================================

using namespace MNEANALYZE;

namespace
{

QString rawPath()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

QMenu* menu(QMainWindow* window, const QString& title)
{
    for (QAction* action : window->menuBar()->actions()) {
        if (action->menu() && action->text() == title)
            return action->menu();
    }
    return nullptr;
}

QAction* findAction(QWidget* root, const QString& text)
{
    for (QAction* action : root->findChildren<QAction*>()) {
        if (action->text() == text)
            return action;
    }
    return nullptr;
}

/** Triggers the action labelled text, which must belong to root. */
bool trigger(QWidget* root, const QString& text)
{
    QAction* action = findAction(root, text);
    if (!action)
        return false;
    action->trigger();
    return true;
}

} // namespace

class TestMneAnalyzeApp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loadsPluginsAndTheFileFromTheCommandLine();
    void switchesStyleAndMode();
    void reloadsPluginsWithoutDuplicatingTheirUi();

private:
    std::unique_ptr<AnalyzeCore> m_core;
    QMainWindow* m_window = nullptr;
};

//=============================================================================================================

void TestMneAnalyzeApp::initTestCase()
{
    if (!QFile::exists(rawPath()))
        QSKIP("Sample raw file not found");
    m_core = std::make_unique<AnalyzeCore>();
    m_core->showMainWindow();
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (widget->objectName() == "mainwindow")
            m_window = qobject_cast<QMainWindow*>(widget);
    }
    QVERIFY(m_window);
}

//=============================================================================================================

void TestMneAnalyzeApp::loadsPluginsAndTheFileFromTheCommandLine()
{
    QStringList names;
    for (ANSHAREDLIB::AbstractPlugin* plugin : m_core->getLoadedPlugins())
        names << plugin->getName();
    for (const char* name : {"Data Loader", "Data", "Signal Viewer", "Filter", "Averaging", "Dipole Fit"})
        QVERIFY2(names.contains(name), qPrintable(names.join(", ")));

    // Each plugin control becomes a docked tab with a toggle under Control
    QVERIFY(menu(m_window, "Control"));
    QVERIFY(menu(m_window, "Control")->actions().size() >= 5);
    QVERIFY(findAction(m_window, "Reload Plugins"));
    QVERIFY(menu(m_window, "File")->actions().size() > 2);

    // --file hands the recording to the Data Loader
    QVERIFY(QTest::qWaitFor([this]() {
        return !m_window->findChild<QTextBrowser*>()->toPlainText().isEmpty() && m_window->findChild<QTextBrowser*>()->toPlainText().contains("sample_audvis_trunc_raw.fif");
    },
                            20000));
}

//=============================================================================================================

void TestMneAnalyzeApp::switchesStyleAndMode()
{
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(trigger(m_window, "Dark"));
    QVERIFY2(!app->styleSheet().isEmpty(), "the dark stylesheet is not bundled");
    QCOMPARE(app->font().family(), QStringLiteral("Roboto"));
    QVERIFY(trigger(m_window, "Default"));
    QVERIFY(app->styleSheet().isEmpty());

    QVERIFY(trigger(m_window, "Clinical"));
    QVERIFY(findAction(m_window, "Clinical")->isChecked());
    QVERIFY(trigger(m_window, "Research"));
    QVERIFY(findAction(m_window, "Research")->isChecked());

    QVERIFY(trigger(m_window, "About"));
    QWidget* about = m_window->findChild<QWidget*>("AboutWindow");
    QVERIFY(about && about->isVisible());
    about->close();
}

//=============================================================================================================

void TestMneAnalyzeApp::reloadsPluginsWithoutDuplicatingTheirUi()
{
    const int docks = m_window->findChildren<QDockWidget*>().size();
    const int controls = menu(m_window, "Control")->actions().size();
    QVERIFY(trigger(m_window, "Reload Plugins"));
    QCOMPARE(m_window->findChildren<QDockWidget*>().size(), docks);
    QCOMPARE(menu(m_window, "Control")->actions().size(), controls);
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings("MNECPP").clear();

    // AnalyzeCore parses the application arguments, so give it a --file rather than the QTest options
    std::string self = argv[0];
    std::string file = "--file=" + QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath().toStdString() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    int appArgc = 2;
    char* appArgv[] = {self.data(), file.data(), nullptr};
    QApplication app(appArgc, appArgv);

    TestMneAnalyzeApp test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mne_analyze_app.moc"
