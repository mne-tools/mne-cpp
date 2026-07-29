//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scshared_management.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the mne_scan plugin and display management classes.
 *
 * These classes decide which plugins exist and which are running, and none of
 * them had a test. The interesting states are the empty ones and the ones a
 * user can reach by mistake: starting a scene with no plugins in it, asking
 * for a plugin that was never loaded, stopping something already stopped.
 * Those are the paths an unguarded container index goes wrong on, and they
 * are reachable from the running application rather than being contrived.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <scShared/Management/pluginscenemanager.h>
#include <scShared/Management/pluginmanager.h>
#include <scShared/Management/displaymanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestScSharedManagement
 *
 * @brief Checks the mne_scan plugin and display management classes.
 */
class TestScSharedManagement: public QObject
{
    Q_OBJECT

public:
    TestScSharedManagement() = default;

private slots:
    void sceneManager_startsEmpty();
    void sceneManager_startStopOnEmptySceneIsSafe();
    void sceneManager_removingUnknownPluginIsRejected();

    void pluginManager_startsEmpty();
    void pluginManager_findByNameOnEmptyReturnsNotFound_data();
    void pluginManager_findByNameOnEmptyReturnsNotFound();
    void pluginManager_loadingFromMissingDirectoryIsSafe();

    void displayManager_cleanOnEmptyIsSafe();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestScSharedManagement::sceneManager_startsEmpty()
{
    PluginSceneManager manager;

    // Nothing is loaded until the application adds something, and the plugin
    // list is what the toolbar and the scene are drawn from.
    QCOMPARE(manager.getPlugins().size(), 0);
}

//=============================================================================================================

void TestScSharedManagement::sceneManager_startStopOnEmptySceneIsSafe()
{
    PluginSceneManager manager;

    // Pressing start with an empty scene is a thing a user does. It must not
    // fault, and stopping afterwards has to be equally survivable.
    manager.startPlugins();
    manager.stopPlugins();

    // The scene is still empty and still usable afterwards.
    QCOMPARE(manager.getPlugins().size(), 0);
}

//=============================================================================================================

void TestScSharedManagement::sceneManager_removingUnknownPluginIsRejected()
{
    PluginSceneManager manager;

    // Removing something that was never added has to be reported rather than
    // silently succeeding, since a caller that believes the removal worked
    // would go on to drop its own reference.
    const bool removed = manager.removePlugin(AbstractPlugin::SPtr());
    QVERIFY2(!removed, "removing a plugin that is not in the scene reported success");

    QCOMPARE(manager.getPlugins().size(), 0);
}

//=============================================================================================================

void TestScSharedManagement::pluginManager_startsEmpty()
{
    PluginManager manager;

    // Before loadPlugins runs there is nothing to offer the user.
    QCOMPARE(manager.getPlugins().size(), 0);
}

//=============================================================================================================

void TestScSharedManagement::pluginManager_findByNameOnEmptyReturnsNotFound_data()
{
    QTest::addColumn<QString>("name");

    // findByName returns an index, so the empty case has to come back as a
    // miss rather than as 0, which would be a valid index into a populated
    // list and would hand the caller the wrong plugin.
    QTest::newRow("ordinary name") << "Covariance";
    QTest::newRow("empty name")    << "";
    QTest::newRow("unicode name")  << QString::fromUtf8("Ünïcödé");
    QTest::newRow("long name")     << QString(512, QChar('x'));
}

//=============================================================================================================

void TestScSharedManagement::pluginManager_findByNameOnEmptyReturnsNotFound()
{
    QFETCH(QString, name);

    PluginManager manager;

    const int index = manager.findByName(name);
    QVERIFY2(index < 0,
             qPrintable(QString("findByName returned index %1 on an empty manager, "
                                "which is a usable index into a populated list").arg(index)));
}

//=============================================================================================================

void TestScSharedManagement::pluginManager_loadingFromMissingDirectoryIsSafe()
{
    PluginManager manager;

    // A missing plugin directory is what a partial install looks like. Loading
    // has to cope and leave the manager empty rather than faulting on the way.
    manager.loadPlugins(QDir::tempPath() + "/no-such-mne-scan-plugin-dir");

    QCOMPARE(manager.getPlugins().size(), 0);
}

//=============================================================================================================

void TestScSharedManagement::displayManager_cleanOnEmptyIsSafe()
{
    DisplayManager manager;

    // clean is called on shutdown and on every scene change, including before
    // anything was ever shown.
    manager.clean();
    manager.clean();

    QVERIFY(true);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestScSharedManagement)
#include "test_scshared_management.moc"
