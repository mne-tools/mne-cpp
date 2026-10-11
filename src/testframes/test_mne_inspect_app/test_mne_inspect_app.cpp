//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_inspect_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Offscreen application workflow tests for mne_inspect.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mainwindow.h>

#include <mna/mna_io.h>
#include <mna/mna_project.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QRhiWidget>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

#include <functional>

//=============================================================================================================

class TestMneInspectApp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loadsSubjectDataLikeTheCommandLine();
    void exportsAndReopensProjects();
    void loadsFilesThroughTheFileMenu();

private:
    MainWindow* makeWindow();
    void loadSample(MainWindow* window) const;
    static QMap<QString, QString> loadedFiles(MainWindow* window);

    QString m_dataPath;
    QList<MainWindow*> m_windows;
};

//=============================================================================================================

namespace
{

/** Runs fn on the next modal widget as soon as it is shown. */
void answerNextModal(const std::function<void(QWidget*)>& fn)
{
    QTimer::singleShot(10, qApp, [fn]() {
        if (QWidget* pModal = QApplication::activeModalWidget()) {
            fn(pModal);
        } else {
            answerNextModal(fn);
        }
    });
}

void chooseFile(const QString& path)
{
    answerNextModal([path](QWidget* pModal) {
        auto* dialog = qobject_cast<QFileDialog*>(pModal);
        dialog->setDirectory(QFileInfo(path).absolutePath());
        dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(path).fileName());
        static_cast<QDialog*>(dialog)->accept();
    });
}

QAction* findAction(QWidget* window, const QString& text)
{
    for (QAction* action : window->findChildren<QAction*>()) {
        if (action->text() == text)
            return action;
    }
    return nullptr;
}

} // namespace

//=============================================================================================================

void TestMneInspectApp::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings(QStringLiteral("mne-cpp"), QStringLiteral("mne_inspect")).clear();
    m_dataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
}

//=============================================================================================================

MainWindow* TestMneInspectApp::makeWindow()
{
    // The offscreen QRhi backend cannot safely destroy this widget tree, so keep it process-owned.
    auto* window = new MainWindow;
    for (QRhiWidget* rhiWidget : window->findChildren<QRhiWidget*>())
        rhiWidget->setApi(QRhiWidget::Api::Null);
    m_windows.append(window);
    return window;
}

//=============================================================================================================

void TestMneInspectApp::loadSample(MainWindow* window) const
{
    const QString subjects = m_dataPath + "subjects";
    window->loadInitialData(subjects,
                            "sample",
                            subjects + "/sample/bem/sample-5120-bem.fif",
                            m_dataPath + "MEG/sample/all-trans.fif",
                            {},
                            m_dataPath + "MEG/sample/sample_audvis_trunc_raw.fif",
                            subjects + "/sample/bem/sample-oct-6-src.fif",
                            subjects + "/sample/label/lh.aparc.annot",
                            m_dataPath + "MEG/sample/sample_audvis-ave.fif");
}

//=============================================================================================================

QMap<QString, QString> TestMneInspectApp::loadedFiles(MainWindow* window)
{
    QMap<QString, QString> files;
    auto* dock = window->findChild<QDockWidget*>(QStringLiteral("loadedFilesDock"));
    auto* tree = dock ? qobject_cast<QTreeWidget*>(dock->widget()) : nullptr;
    for (int i = 0; tree && i < tree->topLevelItemCount(); ++i)
        files.insert(tree->topLevelItem(i)->text(0), tree->topLevelItem(i)->text(1));
    return files;
}

//=============================================================================================================

void TestMneInspectApp::loadsSubjectDataLikeTheCommandLine()
{
    if (!QFile::exists(m_dataPath + "subjects/sample/surf/lh.white"))
        QSKIP("Sample test data not found");

    MainWindow* window = makeWindow();
    loadSample(window);

    const QMap<QString, QString> files = loadedFiles(window);
    const QMap<QString, QString> expected{{"lh.white", "surface"},
                                          {"rh.white", "surface"},
                                          {"lh.aparc.annot", "annotation"},
                                          {"rh.aparc.annot", "annotation"},
                                          {"sample-5120-bem.fif", "bem"},
                                          {"all-trans.fif", "transform"},
                                          {"sample_audvis_trunc_raw.fif", "digitizer"},
                                          {"sample-oct-6-src.fif", "source_space"},
                                          {"sample_audvis-ave.fif", "evoked"}};
    QCOMPARE(files, expected);

    QComboBox* evokedSets = nullptr;
    for (QComboBox* combo : window->findChildren<QComboBox*>()) {
        if (combo->property("evokedPath").isValid())
            evokedSets = combo;
    }
    if (!evokedSets)
        QFAIL("No evoked set selector");
    QCOMPARE(evokedSets->count(), 4);
    QVERIFY(evokedSets->isEnabled());
}

//=============================================================================================================

void TestMneInspectApp::exportsAndReopensProjects()
{
    if (!QFile::exists(m_dataPath + "subjects/sample/surf/lh.white"))
        QSKIP("Sample test data not found");

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir dir(tempDir.path());
#ifdef Q_OS_UNIX
    // Projects are often saved through a symlinked folder (macOS /tmp, /var); linked paths must still resolve.
    QVERIFY(dir.mkdir("real"));
    QVERIFY(QFile::link(dir.filePath("real"), dir.filePath("link")));
    QVERIFY(dir.cd("link"));
#endif
    MainWindow* source = makeWindow();
    loadSample(source);
    const QMap<QString, QString> expected = loadedFiles(source);

    QAction* exportProject = findAction(source, "&Export Project...");
    QVERIFY(exportProject);
    for (const QString& name : {QStringLiteral("linked.mna"), QStringLiteral("embedded.mnx")}) {
        chooseFile(dir.filePath(name));
        exportProject->trigger();
        QVERIFY2(QFile::exists(dir.filePath(name)), qPrintable(name));

        const MNALIB::MnaProject project = MNALIB::MnaIO::read(dir.filePath(name));
        QCOMPARE(project.subjects.size(), 1);
        const auto& refs = project.subjects.first().sessions.first().recordings.first().files;
        QCOMPARE(refs.size(), expected.size());
        for (const MNALIB::MnaFileRef& ref : refs)
            QCOMPARE(ref.embedded, name.endsWith(".mnx"));

        MainWindow* reopened = makeWindow();
        QAction* openProject = findAction(reopened, "&Open Project...");
        QVERIFY(openProject);
        chooseFile(dir.filePath(name));
        openProject->trigger();
        QCOMPARE(loadedFiles(reopened), expected);
    }
}

//=============================================================================================================

void TestMneInspectApp::loadsFilesThroughTheFileMenu()
{
    if (!QFile::exists(m_dataPath + "subjects/sample/surf/lh.white"))
        QSKIP("Sample test data not found");

    MainWindow* window = makeWindow();
    const QString subject = m_dataPath + "subjects/sample/";
    // Each hemisphere's sibling is picked up with it
    const QList<QPair<QString, QString>> menuFiles{{"Load &Surface...", subject + "surf/lh.white"},
                                                   {"Load &Atlas...", subject + "label/rh.aparc.annot"},
                                                   {"Load &BEM...", subject + "bem/sample-5120-bem.fif"},
                                                   {"Load T&ransformation...", m_dataPath + "MEG/sample/all-trans.fif"},
                                                   {"Load Digi&tizer...", m_dataPath + "MEG/sample/sample_audvis_trunc_raw.fif"},
                                                   {"Load Source S&pace...", subject + "bem/sample-oct-6-src.fif"},
                                                   {"Load E&voked...", m_dataPath + "MEG/sample/sample_audvis-ave.fif"}};
    for (const auto& [text, path] : menuFiles) {
        QAction* action = findAction(window, text);
        QVERIFY2(action, qPrintable(text));
        chooseFile(path);
        action->trigger();
    }
    const QMap<QString, QString> expected{{"lh.white", "surface"},
                                          {"rh.white", "surface"},
                                          {"lh.aparc.annot", "annotation"},
                                          {"rh.aparc.annot", "annotation"},
                                          {"sample-5120-bem.fif", "bem"},
                                          {"all-trans.fif", "transform"},
                                          {"sample_audvis_trunc_raw.fif", "digitizer"},
                                          {"sample-oct-6-src.fif", "source_space"},
                                          {"sample_audvis-ave.fif", "evoked"}};
    QCOMPARE(loadedFiles(window), expected);

    // An evoked file without averages is reported in the set list instead of loading
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QComboBox* evokedSets = nullptr;
    for (QComboBox* combo : window->findChildren<QComboBox*>()) {
        if (combo->property("evokedPath").isValid())
            evokedSets = combo;
    }
    if (!evokedSets)
        QFAIL("No evoked set selector");
    QCOMPARE(evokedSets->count(), 4);
    chooseFile(subject + "bem/sample-oct-6-src.fif");
    findAction(window, "Load E&voked...")->trigger();
    QCOMPARE(evokedSets->count(), 1);
    QVERIFY(!evokedSets->isEnabled());

    // A FIFF file without BEM surfaces is not listed as a BEM
    chooseFile(m_dataPath + "MEG/sample/sample_audvis-cov.fif");
    findAction(window, "Load &BEM...")->trigger();
    QVERIFY(!loadedFiles(window).contains("sample_audvis-cov.fif"));

    // Electrode contacts from a CSV of name, x, y, z, array
    const QString csvPath = dir.filePath("contacts.csv");
    QFile csv(csvPath);
    QVERIFY(csv.open(QIODevice::WriteOnly | QIODevice::Text));
    csv.write("name,x,y,z,array\nA1,0.01,0.02,0.03,A\nA2,0.01,0.02,0.04,A\nB1,-0.01,0.0,0.05,B\n");
    csv.close();
    chooseFile(csvPath);
    findAction(window, "Load &Electrodes...")->trigger();
    QCOMPARE(window->electrodesPlugin().contactCount(), 3);

    // A file that holds no electrodes is refused with a message
    const QString emptyPath = dir.filePath("empty.csv");
    QFile empty(emptyPath);
    QVERIFY(empty.open(QIODevice::WriteOnly));
    empty.close();
    QString warning;
    answerNextModal([&warning, emptyPath](QWidget* pModal) {
        auto* dialog = qobject_cast<QFileDialog*>(pModal);
        dialog->setDirectory(QFileInfo(emptyPath).absolutePath());
        dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(emptyPath).fileName());
        answerNextModal([&warning](QWidget* pBox) {
            warning = qobject_cast<QMessageBox*>(pBox)->text();
            qobject_cast<QDialog*>(pBox)->accept();
        });
        static_cast<QDialog*>(dialog)->accept();
    });
    findAction(window, "Load &Electrodes...")->trigger();
    QVERIFY2(warning.contains("Failed to load electrode data"), qPrintable(warning));
    QCOMPARE(window->electrodesPlugin().contactCount(), 3);
}

//=============================================================================================================

QTEST_MAIN(TestMneInspectApp)
#include "test_mne_inspect_app.moc"
