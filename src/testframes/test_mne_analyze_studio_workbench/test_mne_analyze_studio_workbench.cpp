//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_analyze_studio_workbench.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Offscreen workflow tests for the MNE Analyze Studio workbench.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mainwindow.h>
#include <irawdataview.h>
#include <neurokernelservice.h>
#include <pipelineparser.h>
#include <skillhostservice.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLineEdit>
#include <QListWidget>
#include <QLocalSocket>
#include <QPushButton>
#include <QRhiWidget>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QToolButton>
#include <QtTest>

//=============================================================================================================

using namespace MNEANALYZESTUDIO;

class TestMneAnalyzeStudioWorkbench : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void answersTerminalCommandsForOpenRaw();
    void reportsPreviewedWorkflowGraph();
    void switchesPrimarySidebarSections();
    void restoresOpenFilesAfterClose();
    void analysesRawThroughNeuroKernel();
    void loadsAndSavesWorkflowThroughSkillHost();

private:
    MainWindow* makeWindow();
    static QTextEdit* terminal(MainWindow* window);
    static QString runCommand(MainWindow* window, const QString& command);
    static QLocalSocket* backendSocket(MainWindow* window, int index);
    static QTabWidget* tabsWithFile(MainWindow* window, const QString& filePath);

    QString m_rawPath;
    QList<MainWindow*> m_windows;
};

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::initTestCase()
{
    // Offline: no keychain prompt and no neuro-kernel / skill-host processes.
    qputenv("MNE_ANALYZE_STUDIO_OFFLINE", "1");
    QStandardPaths::setTestModeEnabled(true);
    QSettings(QStringLiteral("MNE-CPP"), QStringLiteral("MNEAnalyzeStudio")).clear();

    m_rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

//=============================================================================================================

MainWindow* TestMneAnalyzeStudioWorkbench::makeWindow()
{
    // The offscreen QRhi backend cannot safely destroy this widget tree, so keep it process-owned.
    auto* window = new MainWindow;
    for (QRhiWidget* rhiWidget : window->findChildren<QRhiWidget*>())
        rhiWidget->setApi(QRhiWidget::Api::Null);
    m_windows.append(window);
    return window;
}

//=============================================================================================================

QTextEdit* TestMneAnalyzeStudioWorkbench::terminal(MainWindow* window)
{
    for (QLineEdit* lineEdit : window->findChildren<QLineEdit*>()) {
        if (lineEdit->placeholderText().startsWith("Run MCP command"))
            return lineEdit->parentWidget()->findChild<QTextEdit*>();
    }
    return nullptr;
}

//=============================================================================================================

QString TestMneAnalyzeStudioWorkbench::runCommand(MainWindow* window, const QString& command)
{
    QTextEdit* output = terminal(window);
    QLineEdit* input = output->parentWidget()->findChild<QLineEdit*>();
    const qsizetype before = output->toPlainText().size();
    input->setText(command);
    QMetaObject::invokeMethod(input, "returnPressed");
    return output->toPlainText().mid(before);
}

//=============================================================================================================

QLocalSocket* TestMneAnalyzeStudioWorkbench::backendSocket(MainWindow* window, int index)
{
    // Created in member order: Neuro-Kernel first, then Extension Host.
    const QList<QLocalSocket*> sockets = window->findChildren<QLocalSocket*>(Qt::FindDirectChildrenOnly);
    return sockets.size() == 2 ? sockets.at(index) : nullptr;
}

//=============================================================================================================

QTabWidget* TestMneAnalyzeStudioWorkbench::tabsWithFile(MainWindow* window, const QString& filePath)
{
    for (QTabWidget* tabs : window->findChildren<QTabWidget*>()) {
        for (int i = 0; i < tabs->count(); ++i) {
            if (tabs->tabToolTip(i) == filePath)
                return tabs;
        }
    }
    return nullptr;
}

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::answersTerminalCommandsForOpenRaw()
{
    if (!QFile::exists(m_rawPath))
        QSKIP("Sample test data not found");

    MainWindow* window = makeWindow();
    QVERIFY(runCommand(window, "raw.summary").contains("No active raw browser."));

    window->openInitialFiles({m_rawPath});
    QTabWidget* tabs = tabsWithFile(window, m_rawPath);
    QVERIFY(tabs);
    auto* raw = dynamic_cast<IRawDataView*>(tabs->currentWidget());
    QVERIFY(raw);

    QVERIFY(runCommand(window, "help").contains("raw.goto_sample"));
    QVERIFY(runCommand(window, "views.list").contains("sample_audvis_trunc_raw.fif"));
    QVERIFY(runCommand(window, "raw.summary").contains("Channels: 376"));

    const QString moved = runCommand(window, "raw.goto_sample 15000");
    QCOMPARE(raw->cursorSample(), 15000);
    QVERIFY(moved.contains("sample 15000"));
    QVERIFY(runCommand(window, "raw.goto_sample x").contains("Invalid sample: x"));

    QVERIFY(runCommand(window, "raw.zoom 2").contains("2.00 px/sample"));
    QCOMPARE(raw->pixelsPerSample(), 2.0);
    QVERIFY(runCommand(window, "raw.zoom -1").contains("Invalid zoom value: -1"));
    QVERIFY(runCommand(window, "raw.state").contains("Zoom: 2.00 px/sample"));

    QVERIFY(runCommand(window, "tools.call view.raw.goto {\"sample\":16000}").contains("sample 16000"));
    QCOMPARE(raw->cursorSample(), 16000);
    QVERIFY(runCommand(window, "tools.call view.raw.goto {oops").contains("Invalid JSON arguments for view.raw.goto"));
    QVERIFY(runCommand(window, "tools.call view.raw.zoom {}").contains("requires {\"pixels_per_sample\""));

    // Natural language goes through the rule-based planner when no LLM is configured.
    const QString planned = runCommand(window, "please zoom in");
    QVERIFY(planned.contains("Mapped your request to `view.raw.zoom`"));
    QCOMPARE(raw->pixelsPerSample(), 1.5);
    QVERIFY(runCommand(window, "go to sample 17000").contains("view.raw.goto"));
    QCOMPARE(raw->cursorSample(), 17000);

    QVERIFY(runCommand(window, "kernel.raw_stats abc").contains("Invalid window size: abc"));
    QVERIFY(runCommand(window, "kernel.psd 600 meg").contains("with match MEG"));

    // Without a Neuro-Kernel the request cannot be delivered, so it must surface as a problem.
    QListWidget* problems = nullptr;
    for (QTabWidget* tabs : window->findChildren<QTabWidget*>()) {
        for (int i = 0; i < tabs->count(); ++i) {
            if (tabs->tabText(i) == "Problems")
                problems = qobject_cast<QListWidget*>(tabs->widget(i));
        }
    }
    if (!problems)
        QFAIL("No Problems panel");
    QVERIFY(!problems->findItems("Neuro-Kernel is not connected for tool neurokernel.psd_summary.", Qt::MatchExactly).isEmpty());
}

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::reportsPreviewedWorkflowGraph()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString workflowPath = dir.filePath("filter.mna");
    QFile workflow(workflowPath);
    QVERIFY(workflow.open(QIODevice::WriteOnly));
    workflow.write(R"({
  "resources": [{"uid": "raw_01", "type": "fiff_raw", "uri": "mne://workspace/raw.fif"}],
  "pipeline": [{"uid": "op_filter", "skill_id": "mne.skills.temporal_filter",
                "inputs": {"raw_data": "raw_01"}, "parameters": {"highpass": 1.0, "lowpass": 40.0},
                "outputs": {"filtered_data": "filt_01"}}]
})");
    workflow.close();

    MainWindow* window = makeWindow();
    QVERIFY(runCommand(window, "workflow.graph").contains("No active workflow graph."));
    QCOMPARE(window->previewWorkflowFile(dir.filePath("missing.mna")).isEmpty(), false);
    QCOMPARE(window->previewWorkflowFile(workflowPath), QString());

    QVERIFY(runCommand(window, "workflow.graph").contains("1 resources, 1 nodes"));
    QVERIFY(runCommand(window, "tools.call studio.workflow.active_graph {}").contains("1 resources, 1 nodes"));
    QVERIFY(runCommand(window, "tools.call studio.workflow.load {}").contains("requires {\"file\""));
    QVERIFY(runCommand(window, "tools.call studio.workflow.load {\"file\":\"/no/such.mna\"}").contains("does not exist"));
}

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::switchesPrimarySidebarSections()
{
    MainWindow* window = makeWindow();
    const QList<QPair<QString, QString>> sections{{"Flow", "Workflow navigator active"},
                                                  {"Skills", "Skill explorer active"},
                                                  {"Ext", "Extension manager active"},
                                                  {"Files", "Workspace explorer active"}};
    for (const auto& [buttonText, message] : sections) {
        QToolButton* button = nullptr;
        for (QToolButton* candidate : window->findChildren<QToolButton*>()) {
            if (candidate->text() == buttonText)
                button = candidate;
        }
        if (!button)
            QFAIL(qPrintable(buttonText));
        button->click();
        QVERIFY(button->isChecked());
        QCOMPARE(window->statusBar()->currentMessage(), message);
    }
}

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::restoresOpenFilesAfterClose()
{
    if (!QFile::exists(m_rawPath))
        QSKIP("Sample test data not found");

    QSettings(QStringLiteral("MNE-CPP"), QStringLiteral("MNEAnalyzeStudio")).clear();
    MainWindow* first = makeWindow();
    first->openInitialFiles({m_rawPath});
    QVERIFY(first->close());

    const QString absoluteRawPath = QFileInfo(m_rawPath).absoluteFilePath();
    MainWindow* second = makeWindow();
    QTabWidget* tabs = tabsWithFile(second, absoluteRawPath);
    QVERIFY(tabs);
    QVERIFY(dynamic_cast<IRawDataView*>(tabs->currentWidget()));
}

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::analysesRawThroughNeuroKernel()
{
    if (!QFile::exists(m_rawPath))
        QSKIP("Sample test data not found");

    NeuroKernelService kernel;
    const QString socketName = QStringLiteral("test_studio_workbench_kernel_%1").arg(QCoreApplication::applicationPid());
    QVERIFY(kernel.start(socketName));

    MainWindow* window = makeWindow();
    window->openInitialFiles({m_rawPath});
    QLocalSocket* socket = backendSocket(window, 0);
    QVERIFY(socket);
    socket->connectToServer(socketName);
    QVERIFY(socket->waitForConnected(2000));
    QTRY_VERIFY(window->statusBar()->currentMessage().contains("Neuro-Kernel tools"));

    runCommand(window, "raw.goto_sample 15000");
    runCommand(window, "kernel.raw_stats 600");
    QTRY_VERIFY(terminal(window)->toPlainText().contains("Raw stats for sample_audvis_trunc_raw.fif samples 14700-15300: RMS="));

    runCommand(window, "kernel.channel_stats 400");
    QTRY_VERIFY(terminal(window)->toPlainText().contains("Channel stats for sample_audvis_trunc_raw.fif samples 14800-15200:"));

    runCommand(window, "show the power spectrum of meg");
    QTRY_VERIFY(terminal(window)->toPlainText().contains("PSD summary for sample_audvis_trunc_raw.fif using"));

    runCommand(window, "find the strongest eeg burst");
    QTRY_VERIFY(terminal(window)->toPlainText().contains("Peak window for sample_audvis_trunc_raw.fif: sample"));

    // Anything the workbench cannot route is executed by the kernel as a free-form command.
    runCommand(window, "status");
    QTRY_VERIFY(terminal(window)->toPlainText().contains("Neuro-Kernel is running."));

    socket->abort();
}

//=============================================================================================================

void TestMneAnalyzeStudioWorkbench::loadsAndSavesWorkflowThroughSkillHost()
{
    if (!QFile::exists(m_rawPath))
        QSKIP("Sample test data not found");

    // Loading a workflow runs it, and the filter writes next to its input, so work on a copy.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString rawCopy = dir.filePath("raw.fif");
    QVERIFY(QFile::copy(m_rawPath, rawCopy));
    const QString workflowPath = dir.filePath("filter.mna");
    QFile workflow(workflowPath);
    QVERIFY(workflow.open(QIODevice::WriteOnly));
    workflow.write(QString(R"({
  "resources": [{"uid": "raw_01", "type": "fiff_raw", "uri": "%1"}],
  "pipeline": [{"uid": "op_filter", "skill_id": "mne.skills.temporal_filter",
                "inputs": {"raw_data": "raw_01"}, "parameters": {"highpass": 1.0, "lowpass": 40.0},
                "outputs": {"filtered_data": "filt_01"}}]
})")
                       .arg(QUrl::fromLocalFile(rawCopy).toString())
                       .toUtf8());
    workflow.close();

    SkillHostService host;
    const QString socketName = QStringLiteral("test_studio_workbench_host_%1").arg(QCoreApplication::applicationPid());
    QVERIFY(host.start(socketName, QStringLiteral(MNE_STUDIO_EXTENSIONS_DIR)));

    MainWindow* window = makeWindow();
    QLocalSocket* socket = backendSocket(window, 1);
    QVERIFY(socket);
    socket->connectToServer(socketName);
    QVERIFY(socket->waitForConnected(2000));
    QTRY_VERIFY(window->statusBar()->currentMessage().startsWith("Extension host sessions"));

    runCommand(window, QString("tools.call studio.workflow.load {\"file\":\"%1\"}").arg(workflowPath));
    // Running the filter node publishes its output as a second resource.
    QTRY_VERIFY(runCommand(window, "workflow.graph").contains("2 resources, 1 nodes"));

    QPushButton* save = nullptr;
    for (QPushButton* button : window->findChildren<QPushButton*>()) {
        if (button->text() == "Save Workflow")
            save = button;
    }
    if (!save)
        QFAIL("No Save Workflow button");
    QVERIFY(QFile::remove(workflowPath));
    save->click();
    QTRY_VERIFY(QFile::exists(workflowPath));

    const WorkflowGraph saved = PipelineParser().parseFile(workflowPath);
    QVERIFY(saved.hasResource("raw_01"));
    QVERIFY(saved.hasNode("op_filter"));
    QCOMPARE(saved.nodes().size(), 1);

    socket->abort();
}

//=============================================================================================================

QTEST_MAIN(TestMneAnalyzeStudioWorkbench)
#include "test_mne_analyze_studio_workbench.moc"
