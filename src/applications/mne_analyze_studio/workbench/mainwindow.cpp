//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the main workbench shell and agent orchestration helpers.
 */

#include "mainwindow.h"

#include "agentchatdockwidget.h"
#include "rawdatabrowserwidget.h"

#include <jsonrpcmessage.h>

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QAction>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLocalSocket>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QFrame>
#include <QSplitter>
#include <QStatusBar>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QStringList>

using namespace MNEANALYZESTUDIO;

namespace
{

const char* kKernelSocketName = "mne_analyze_studio.neuro_kernel";

QString formatToolDefinitions(const QJsonArray& tools)
{
    QStringList entries;
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = value.toObject();
        entries << QString("%1: %2")
                       .arg(tool.value("name").toString(),
                            tool.value("description").toString());
    }

    return entries.join(" | ");
}

}

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
, m_rootWidget(new QWidget(this))
, m_agentChatDock(new AgentChatDockWidget(this))
, m_activityBar(new QWidget(this))
, m_explorerButton(new QToolButton)
, m_skillsButton(new QToolButton)
, m_workspaceExplorer(new QTreeWidget)
, m_skillsExplorer(new QTreeWidget)
, m_leftSidebarStack(new QStackedWidget)
, m_mainSplitter(new QSplitter(Qt::Horizontal))
, m_centerSplitter(new QSplitter(Qt::Vertical))
, m_centerTabs(new QTabWidget)
, m_bottomPanelTabs(new QTabWidget)
, m_outputPanel(new QListWidget)
, m_problemPanel(new QListWidget)
, m_terminalTab(new QWidget(this))
, m_terminalStatusLabel(new QLabel("Active view state: idle"))
, m_terminalPanel(new QTextEdit)
, m_terminalInput(new QLineEdit)
, m_terminalRunButton(new QPushButton("Run"))
, m_activeStateItem(new QListWidgetItem("Active view state: idle"))
, m_kernelSocket(new QLocalSocket(this))
, m_sceneRegistry(this)
, m_viewManager(&m_sceneRegistry, this)
{
    setWindowTitle("MNE Analyze Studio");
    resize(1440, 900);

    createLayout();
    createConnections();
    restoreWorkspace();
    applyWorkbenchStyle();

    m_kernelSocket->connectToServer(kKernelSocketName);
}

void MainWindow::openInitialFiles(const QStringList& filePaths)
{
    for(const QString& filePath : filePaths) {
        if(filePath.isEmpty() || !QFileInfo::exists(filePath)) {
            continue;
        }

        addWorkspaceDirectory(QFileInfo(filePath).absolutePath());
        openFileInView(filePath);
    }
}

QWidget* MainWindow::createSidebarSection(const QString& title, QWidget* contentWidget)
{
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QLabel* titleLabel = new QLabel(title, container);
    titleLabel->setObjectName("sidebarTitle");
    layout->addWidget(titleLabel);
    layout->addWidget(contentWidget, 1);

    return container;
}

void MainWindow::createLayout()
{
    QWidget* leftSidebar = new QWidget(this);
    QHBoxLayout* leftSidebarLayout = new QHBoxLayout(leftSidebar);
    leftSidebarLayout->setContentsMargins(0, 0, 0, 0);
    leftSidebarLayout->setSpacing(0);

    QVBoxLayout* activityLayout = new QVBoxLayout(m_activityBar);
    activityLayout->setContentsMargins(6, 8, 6, 8);
    activityLayout->setSpacing(6);
    m_explorerButton->setText("Files");
    m_skillsButton->setText("Skills");
    m_explorerButton->setToolTip("Explorer");
    m_skillsButton->setToolTip("Extensions");
    m_explorerButton->setCheckable(true);
    m_skillsButton->setCheckable(true);
    m_explorerButton->setChecked(true);
    activityLayout->addWidget(m_explorerButton);
    activityLayout->addWidget(m_skillsButton);
    activityLayout->addStretch(1);

    m_workspaceExplorer->setHeaderLabel("Workspace");
    m_workspaceExplorer->setAlternatingRowColors(true);
    m_workspaceExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_skillsExplorer->setHeaderLabel("Extensions");
    QTreeWidgetItem* skillsRoot = new QTreeWidgetItem(QStringList() << "Installed Skills");
    skillsRoot->addChild(new QTreeWidgetItem(QStringList() << "MCP Tool Registry (planned)"));
    skillsRoot->addChild(new QTreeWidgetItem(QStringList() << "Visualizer Skills (planned)"));
    m_skillsExplorer->addTopLevelItem(skillsRoot);
    skillsRoot->setExpanded(true);

    m_leftSidebarStack->addWidget(m_workspaceExplorer);
    m_leftSidebarStack->addWidget(m_skillsExplorer);

    leftSidebarLayout->addWidget(m_activityBar);
    leftSidebarLayout->addWidget(m_leftSidebarStack, 1);

    QWidget* leftPanel = createSidebarSection("Workspace", leftSidebar);

    m_centerTabs->setDocumentMode(true);
    m_centerTabs->setTabsClosable(true);
    m_centerTabs->setMovable(true);
    m_centerTabs->tabBar()->setExpanding(false);

    m_bottomPanelTabs->setDocumentMode(true);
    m_bottomPanelTabs->setMovable(false);
    m_bottomPanelTabs->addTab(m_outputPanel, "Output");
    m_bottomPanelTabs->addTab(m_problemPanel, "Problems");
    m_bottomPanelTabs->addTab(m_terminalTab, "Terminal");
    m_outputPanel->setAlternatingRowColors(true);
    m_problemPanel->setAlternatingRowColors(true);
    m_outputPanel->addItem(m_activeStateItem);
    m_terminalPanel->setReadOnly(true);
    m_terminalPanel->setLineWrapMode(QTextEdit::NoWrap);
    m_terminalStatusLabel->setObjectName("terminalStatusLabel");
    m_terminalInput->setPlaceholderText("Run MCP command or note, for example: status");

    QVBoxLayout* terminalLayout = new QVBoxLayout(m_terminalTab);
    terminalLayout->setContentsMargins(0, 0, 0, 0);
    terminalLayout->setSpacing(8);

    QHBoxLayout* terminalComposerLayout = new QHBoxLayout;
    terminalComposerLayout->setContentsMargins(0, 0, 0, 0);
    terminalComposerLayout->setSpacing(8);
    terminalComposerLayout->addWidget(m_terminalInput, 1);
    terminalComposerLayout->addWidget(m_terminalRunButton);

    terminalLayout->addWidget(m_terminalStatusLabel);
    terminalLayout->addWidget(m_terminalPanel, 1);
    terminalLayout->addLayout(terminalComposerLayout);

    QWidget* activitySection = createSidebarSection("Panel", m_bottomPanelTabs);
    QWidget* chatSection = createSidebarSection("Coworker", m_agentChatDock);

    m_centerSplitter->addWidget(m_centerTabs);
    m_centerSplitter->addWidget(activitySection);
    m_centerSplitter->setStretchFactor(0, 1);
    m_centerSplitter->setStretchFactor(1, 0);
    m_centerSplitter->setSizes({700, 180});

    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(m_centerSplitter);
    m_mainSplitter->addWidget(chatSection);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);
    m_mainSplitter->setSizes({280, 860, 320});

    QHBoxLayout* rootLayout = new QHBoxLayout(m_rootWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_mainSplitter);
    setCentralWidget(m_rootWidget);

    QMenu* fileMenu = menuBar()->addMenu("File");
    QAction* addWorkspaceAction = fileMenu->addAction("Add Folder To Workspace...");
    connect(addWorkspaceAction, &QAction::triggered, this, [this]() {
        const QString dirPath = QFileDialog::getExistingDirectory(this, "Add folder to workspace");
        if(!dirPath.isEmpty()) {
            addWorkspaceDirectory(dirPath);
        }
    });

    QAction* openAction = fileMenu->addAction("Open File...");
    connect(openAction, &QAction::triggered, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(this, "Open neuroscience asset");
        if(filePath.isEmpty()) {
            return;
        }

        addWorkspaceDirectory(QFileInfo(filePath).absolutePath());
        openFileInView(filePath);
    });

    statusBar()->showMessage("Workbench online");
    appendOutputMessage("Workbench online");
    appendTerminalMessage("$ studio boot complete");
}

void MainWindow::createConnections()
{
    connect(m_agentChatDock, &AgentChatDockWidget::commandSubmitted, this, &MainWindow::sendToolCall);
    connect(m_explorerButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("workspace");
    });
    connect(m_skillsButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("skills");
    });
    connect(m_workspaceExplorer, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        openWorkspaceItem(item);
    });
    connect(m_workspaceExplorer, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        QTreeWidgetItem* item = m_workspaceExplorer->itemAt(position);
        if(!item) {
            return;
        }

        const QString filePath = item->data(0, Qt::UserRole).toString();
        if(!QFileInfo(filePath).isFile()) {
            return;
        }

        m_workspaceExplorer->setCurrentItem(item);

        QMenu contextMenu(this);
        QAction* openInEditorAction = contextMenu.addAction("Open in Editor");
        connect(openInEditorAction, &QAction::triggered, this, [this, item]() {
            openWorkspaceItem(item);
        });
        contextMenu.exec(m_workspaceExplorer->viewport()->mapToGlobal(position));
    });
    connect(m_centerTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* widget = m_centerTabs->widget(index);
        m_centerTabs->removeTab(index);
        delete widget;
    });
    connect(m_terminalRunButton, &QPushButton::clicked, this, [this]() {
        const QString commandText = m_terminalInput->text().trimmed();
        if(commandText.isEmpty()) {
            return;
        }

        sendToolCall(commandText);
        m_terminalInput->clear();
    });
    connect(m_terminalInput, &QLineEdit::returnPressed, this, [this]() {
        m_terminalRunButton->click();
    });

    connect(m_kernelSocket, &QLocalSocket::readyRead, this, [this]() {
        while(m_kernelSocket->canReadLine()) {
            const QByteArray payload = m_kernelSocket->readLine();
            QJsonObject response;
            QString errorString;
            if(JsonRpcMessage::deserialize(payload, response, errorString)) {
                if(response.contains("error")) {
                    const QString message = response.value("error").toObject().value("message").toString();
                    m_agentChatDock->appendTranscript(QString("Kernel> %1").arg(message));
                    appendProblemMessage(QString("Kernel error: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                } else {
                    const QJsonObject result = response.value("result").toObject();
                    QString message = result.value("message").toString(QJsonDocument(result).toJson(QJsonDocument::Compact));
                    if(result.contains("tools") && result.value("tools").isArray()) {
                        message = QString("%1 | %2").arg(message, formatToolDefinitions(result.value("tools").toArray()));
                    }
                    m_agentChatDock->appendTranscript(QString("Kernel> %1").arg(message));
                    appendOutputMessage(QString("Kernel: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                }
            } else {
                const QString message = QString::fromUtf8(payload).trimmed();
                m_agentChatDock->appendTranscript(QString("Kernel> %1").arg(message));
                appendOutputMessage(QString("Kernel: %1").arg(message));
                appendTerminalMessage(QString("> %1").arg(message));
            }
        }
    });
}

void MainWindow::addWorkspaceDirectory(const QString& directoryPath)
{
    if(directoryPath.isEmpty()) {
        return;
    }

    for(int i = 0; i < m_workspaceExplorer->topLevelItemCount(); ++i) {
        QTreeWidgetItem* existingRoot = m_workspaceExplorer->topLevelItem(i);
        if(existingRoot->data(0, Qt::UserRole).toString() == directoryPath) {
            return;
        }
    }

    QFileInfo rootInfo(directoryPath);
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(QStringList() << rootInfo.fileName());
    rootItem->setData(0, Qt::UserRole, directoryPath);
    rootItem->setExpanded(true);

    QDirIterator it(directoryPath,
                    QDir::Files | QDir::NoDotAndDotDot | QDir::Dirs,
                    QDirIterator::Subdirectories);
    while(it.hasNext()) {
        const QString path = it.next();
        QFileInfo info(path);
        if(info.isDir()) {
            continue;
        }
        if(!isSupportedWorkspaceFile(path)) {
            continue;
        }

        QString relativePath = QDir(directoryPath).relativeFilePath(path);
        const QStringList segments = relativePath.split('/', Qt::SkipEmptyParts);
        QTreeWidgetItem* parentItem = rootItem;
        QString accumulatedPath = directoryPath;

        for(int segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex) {
            const QString& segment = segments.at(segmentIndex);
            accumulatedPath = QDir(accumulatedPath).filePath(segment);

            QTreeWidgetItem* matchingChild = nullptr;
            for(int childIndex = 0; childIndex < parentItem->childCount(); ++childIndex) {
                QTreeWidgetItem* candidate = parentItem->child(childIndex);
                if(candidate->text(0) == segment) {
                    matchingChild = candidate;
                    break;
                }
            }

            if(!matchingChild) {
                matchingChild = new QTreeWidgetItem(QStringList() << segment);
                matchingChild->setData(0, Qt::UserRole, accumulatedPath);
                parentItem->addChild(matchingChild);
            }

            parentItem = matchingChild;
        }
    }

    m_workspaceExplorer->addTopLevelItem(rootItem);
    appendOutputMessage(QString("Workspace added: %1").arg(directoryPath));
}

void MainWindow::openWorkspaceItem(QTreeWidgetItem* item)
{
    if(!item) {
        return;
    }

    const QString filePath = item->data(0, Qt::UserRole).toString();
    if(QFileInfo(filePath).isFile()) {
        openFileInView(filePath);
    }
}

void MainWindow::openFileInView(const QString& filePath)
{
    const QJsonObject dispatch = m_viewManager.dispatchFileSelection(filePath);
    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(RawDataBrowserWidget* rawBrowser = qobject_cast<RawDataBrowserWidget*>(m_centerTabs->widget(i))) {
            if(rawBrowser->filePath() == filePath) {
                m_centerTabs->setCurrentIndex(i);
                return;
            }
        }
        if(m_centerTabs->tabToolTip(i) == filePath) {
            m_centerTabs->setCurrentIndex(i);
            return;
        }
    }

    QWidget* viewWidget = nullptr;
    if(dispatch.value("view").toString() == "SignalBrowserView") {
        RawDataBrowserWidget* rawBrowser = new RawDataBrowserWidget;
        connect(rawBrowser, &RawDataBrowserWidget::outputMessage, this, [this](const QString& message) {
            appendOutputMessage(message);
            appendTerminalMessage(QString("> %1").arg(message));
        });
        connect(rawBrowser, &RawDataBrowserWidget::statusMessage, this, [this](const QString& message) {
            statusBar()->showMessage(message);
            setActivePanelState(message);
        });
        if(rawBrowser->loadFile(filePath)) {
            viewWidget = rawBrowser;
        } else {
            appendProblemMessage(QString("Failed to open FIFF raw browser: %1").arg(filePath));
            rawBrowser->deleteLater();
        }
    } else {
        QTextEdit* editor = new QTextEdit;
        editor->setReadOnly(true);
        editor->setPlainText(QJsonDocument(dispatch).toJson(QJsonDocument::Indented));
        viewWidget = editor;
    }

    if(!viewWidget) {
        statusBar()->showMessage(QString("Could not open %1").arg(QFileInfo(filePath).fileName()), 5000);
        return;
    }

    const int tabIndex = m_centerTabs->addTab(viewWidget, QFileInfo(filePath).fileName());
    m_centerTabs->setTabToolTip(tabIndex, filePath);
    m_centerTabs->setCurrentIndex(tabIndex);
    appendOutputMessage(QString("Opened %1 in %2").arg(filePath, dispatch.value("view").toString()));
    appendTerminalMessage(QString("$ open %1").arg(filePath));
}

void MainWindow::applyWorkbenchStyle()
{
    setStyleSheet(
        "QMainWindow { background: #1f232a; }"
        "QMenuBar { background: #181c22; color: #d7dce2; }"
        "QMenuBar::item:selected { background: #2d333b; }"
        "QMenu { background: #20252c; color: #d7dce2; border: 1px solid #313842; }"
        "QStatusBar { background: #181c22; color: #98a2ad; }"
        "QSplitter::handle { background: #181c22; }"
        "#sidebarTitle, #agentChatTitle {"
        "  background: #181c22; color: #c9d1d9; font-weight: 600; padding: 8px 12px;"
        "  border-bottom: 1px solid #30363d; }"
        "QTreeWidget, QListWidget, QTextEdit, QLineEdit, QTabWidget::pane {"
        "  background: #22272e; color: #d7dce2; border: none; }"
        "#terminalStatusLabel { background: #181c22; color: #8b949e; padding: 6px 8px; border: 1px solid #30363d; }"
        "QTreeWidget, QListWidget { alternate-background-color: #262c34; }"
        "QTreeWidget::item:selected, QListWidget::item:selected { background: #2f81f7; color: white; }"
        "QTabBar::tab { background: #181c22; color: #9da7b3; padding: 8px 12px; border: none; }"
        "QTabBar::tab:selected { background: #22272e; color: #ffffff; }"
        "QTabBar::close-button { subcontrol-position: right; margin-left: 8px; }"
        "QToolButton { background: transparent; color: #9da7b3; border: none; padding: 10px 8px; border-radius: 10px; }"
        "QToolButton:checked { background: #30363d; color: #ffffff; border-left: 2px solid #2f81f7; }"
        "QPushButton { background: #2f81f7; color: white; border: none; padding: 8px 14px; border-radius: 12px; }"
        "QPushButton:hover { background: #4c95f8; }"
        "QLineEdit { padding: 8px 10px; border: 1px solid #30363d; border-radius: 12px; }"
    );
}

void MainWindow::switchPrimarySidebar(const QString& sectionName)
{
    const bool workspaceSelected = sectionName == "workspace";
    m_explorerButton->setChecked(workspaceSelected);
    m_skillsButton->setChecked(!workspaceSelected);
    m_leftSidebarStack->setCurrentWidget(workspaceSelected ? static_cast<QWidget*>(m_workspaceExplorer)
                                                           : static_cast<QWidget*>(m_skillsExplorer));

    if(workspaceSelected) {
        statusBar()->showMessage("Workspace explorer active");
    } else {
        statusBar()->showMessage("Skill explorer active");
    }
}

void MainWindow::sendToolCall(const QString& commandText)
{
    bool handledStructured = false;
    const QString structuredResponse = handleStructuredToolCommand(commandText, handledStructured);
    if(handledStructured) {
        if(!structuredResponse.isEmpty()) {
            m_agentChatDock->appendTranscript(QString("Studio> %1").arg(structuredResponse));
            appendTerminalMessage(QString("> %1").arg(structuredResponse));
        }
        return;
    }

    bool handledLocally = false;
    const QString localResponse = handleLocalAgentCommand(commandText, handledLocally);
    if(handledLocally) {
        m_agentChatDock->appendTranscript(QString("Studio> %1").arg(localResponse));
        appendOutputMessage(QString("Agent handled locally: %1").arg(commandText));
        appendTerminalMessage(QString("> %1").arg(localResponse));
        return;
    }

    bool planned = false;
    QString plannedCommand;
    const QString planningNote = planAgentIntent(commandText, plannedCommand, planned);
    if(planned) {
        if(!planningNote.isEmpty()) {
            m_agentChatDock->appendTranscript(QString("Planner> %1").arg(planningNote));
            appendTerminalMessage(QString("> %1").arg(planningNote));
        }
        sendToolCall(plannedCommand);
        return;
    }

    sendKernelToolCall("neurokernel.execute", QJsonObject{{"command", commandText}});
    appendTerminalMessage(QString("$ mcp tools/call %1").arg(commandText));
}

QString MainWindow::planAgentIntent(const QString& commandText, QString& plannedCommand, bool& planned) const
{
    planned = false;
    plannedCommand.clear();

    const QString trimmed = commandText.trimmed();
    if(trimmed.isEmpty()) {
        return QString();
    }

    const QString lower = trimmed.toLower();

    auto containsAny = [&lower](const QStringList& needles) {
        for(const QString& needle : needles) {
            if(lower.contains(needle)) {
                return true;
            }
        }
        return false;
    };

    if(containsAny({"what tools", "list tools", "available tools", "which tools"})) {
        planned = true;
        plannedCommand = "tools.list";
        return "Mapped your request to `tools.list`.";
    }

    if(containsAny({"what views", "open views", "which views", "what is open"})) {
        planned = true;
        plannedCommand = "tools.call studio.views.list {}";
        return "Mapped your request to `studio.views.list`.";
    }

    if(containsAny({"summary", "metadata", "dataset info", "raw info"})) {
        planned = true;
        plannedCommand = "tools.call view.raw.summary {}";
        return "Mapped your request to `view.raw.summary`.";
    }

    if(containsAny({"state", "visible range", "where am i", "current position", "cursor position"})) {
        planned = true;
        plannedCommand = "tools.call view.raw.state {}";
        return "Mapped your request to `view.raw.state`.";
    }

    QRegularExpression samplePattern("(?:go to|goto|jump to|move to|set cursor to|cursor at|sample)\\s+(?:sample\\s+)?(\\d+)");
    QRegularExpressionMatch sampleMatch = samplePattern.match(lower);
    if(sampleMatch.hasMatch()) {
        planned = true;
        plannedCommand = QString("tools.call view.raw.goto {\"sample\":%1}").arg(sampleMatch.captured(1));
        return QString("Mapped your request to `view.raw.goto` for sample %1.").arg(sampleMatch.captured(1));
    }

    QRegularExpression zoomPattern("(?:zoom to|set zoom to)\\s+(\\d+(?:\\.\\d+)?)");
    QRegularExpressionMatch zoomMatch = zoomPattern.match(lower);
    if(zoomMatch.hasMatch()) {
        planned = true;
        plannedCommand = QString("tools.call view.raw.zoom {\"pixels_per_sample\":%1}").arg(zoomMatch.captured(1));
        return QString("Mapped your request to `view.raw.zoom` with %1 px/sample.").arg(zoomMatch.captured(1));
    }

    if(containsAny({"zoom in", "closer", "increase zoom"})) {
        planned = true;
        plannedCommand = "tools.call view.raw.zoom {\"pixels_per_sample\":1.5}";
        return "Mapped your request to `view.raw.zoom` with a closer view.";
    }

    if(containsAny({"zoom out", "farther", "decrease zoom"})) {
        planned = true;
        plannedCommand = "tools.call view.raw.zoom {\"pixels_per_sample\":0.75}";
        return "Mapped your request to `view.raw.zoom` with a wider view.";
    }

    QRegularExpression windowPattern("(\\d+)\\s*(?:samples|sample)");
    const QRegularExpressionMatch windowMatch = windowPattern.match(lower);
    const int windowSamples = windowMatch.hasMatch() ? windowMatch.captured(1).toInt() : 600;

    if(containsAny({"channel stats", "strongest channels", "top channels", "eeg channels"})) {
        planned = true;
        QStringList jsonParts;
        jsonParts << QString("\"window_samples\":%1").arg(windowSamples);
        if(lower.contains("eeg")) {
            jsonParts << "\"match\":\"EEG\"";
        } else if(lower.contains("meg")) {
            jsonParts << "\"match\":\"MEG\"";
        } else if(lower.contains("eog")) {
            jsonParts << "\"match\":\"EOG\"";
        }
        jsonParts << "\"limit\":5";
        plannedCommand = QString("tools.call neurokernel.channel_stats {%1}").arg(jsonParts.join(","));
        return "Mapped your request to `neurokernel.channel_stats`.";
    }

    if(containsAny({"raw stats", "signal stats", "compute stats", "window stats", "rms"})) {
        planned = true;
        plannedCommand = QString("tools.call neurokernel.raw_stats {\"window_samples\":%1}").arg(windowSamples);
        return "Mapped your request to `neurokernel.raw_stats`.";
    }

    return QString();
}

QString MainWindow::handleStructuredToolCommand(const QString& commandText, bool& handled)
{
    handled = true;

    const QString trimmed = commandText.trimmed();
    if(trimmed == "tools.list") {
        sendKernelToolCall("tools/list", QJsonObject());
        return QString("Studio tools: %1 | Requested kernel tools list.")
            .arg(formatToolDefinitions(localToolDefinitions()));
    }

    if(trimmed == "kernel.tools.list") {
        sendKernelToolCall("tools/list", QJsonObject());
        return "Requested kernel tools list.";
    }

    const QString prefix = "tools.call ";
    if(!trimmed.startsWith(prefix, Qt::CaseInsensitive)) {
        handled = false;
        return QString();
    }

    const QString invocation = trimmed.mid(prefix.size()).trimmed();
    const int separatorIndex = invocation.indexOf(' ');
    const QString toolName = separatorIndex >= 0 ? invocation.left(separatorIndex).trimmed()
                                                 : invocation;
    const QString argumentsText = separatorIndex >= 0 ? invocation.mid(separatorIndex + 1).trimmed()
                                                      : QString();

    if(toolName.isEmpty()) {
        return "Usage: tools.call <tool_name> {json_arguments}";
    }

    QJsonObject arguments;
    if(!argumentsText.isEmpty()) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(argumentsText.toUtf8(), &error);
        if(error.error != QJsonParseError::NoError || !document.isObject()) {
            return QString("Invalid JSON arguments for %1: %2").arg(toolName, error.errorString());
        }
        arguments = document.object();
    }

    if(toolName == "studio.views.list") {
        QStringList views;
        for(int i = 0; i < m_centerTabs->count(); ++i) {
            views << QString("%1: %2").arg(i + 1).arg(m_centerTabs->tabText(i));
        }
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    RawDataBrowserWidget* rawBrowser = activeRawBrowser();
    if(toolName == "view.raw.summary") {
        return rawBrowser ? rawBrowser->summaryText() : "No active raw browser.";
    }

    if(toolName == "view.raw.state") {
        return rawBrowser ? rawBrowser->stateText() : "No active raw browser.";
    }

    if(toolName == "view.raw.goto" || toolName == "view.raw.cursor") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        if(!arguments.contains("sample")) {
            return QString("Tool %1 requires {\"sample\": <int>}.").arg(toolName);
        }
        const int sample = arguments.value("sample").toInt(-1);
        if(sample < 0) {
            return QString("Tool %1 received an invalid sample.").arg(toolName);
        }
        rawBrowser->gotoSample(sample);
        return QString("Moved raw browser cursor to sample %1.").arg(rawBrowser->cursorSample());
    }

    if(toolName == "view.raw.zoom") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        const double pixelsPerSample = arguments.value("pixels_per_sample").toDouble(-1.0);
        if(pixelsPerSample <= 0.0) {
            return "Tool view.raw.zoom requires {\"pixels_per_sample\": <positive number>}.";
        }
        rawBrowser->setZoomPixelsPerSample(pixelsPerSample);
        return QString("Raw browser zoom set to %1 px/sample.")
            .arg(QString::number(rawBrowser->pixelsPerSample(), 'f', 2));
    }

    if(toolName == "neurokernel.raw_stats") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        const int windowSamples = arguments.value("window_samples").toInt(600);
        sendKernelToolCall(toolName, buildRawWindowArguments(windowSamples));
        return QString("Requested %1 for %2.")
            .arg(toolName, QFileInfo(rawBrowser->filePath()).fileName());
    }

    if(toolName == "neurokernel.channel_stats") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        QJsonObject kernelArguments = buildRawWindowArguments(arguments.value("window_samples").toInt(600));
        if(arguments.contains("limit")) {
            kernelArguments.insert("limit", arguments.value("limit").toInt(10));
        }
        if(arguments.contains("match")) {
            kernelArguments.insert("match", arguments.value("match").toString());
        }
        sendKernelToolCall(toolName, kernelArguments);
        return QString("Requested %1 for %2.")
            .arg(toolName, QFileInfo(rawBrowser->filePath()).fileName());
    }

    sendKernelToolCall(toolName, arguments);
    return QString("Forwarded tool call: %1").arg(toolName);
}

RawDataBrowserWidget* MainWindow::activeRawBrowser() const
{
    return qobject_cast<RawDataBrowserWidget*>(m_centerTabs->currentWidget());
}

QString MainWindow::handleLocalAgentCommand(const QString& commandText, bool& handled)
{
    handled = true;

    const QString trimmed = commandText.trimmed();
    const QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
    if(parts.isEmpty()) {
        return "No command provided.";
    }

    const QString command = parts.first().toLower();

    if(command == "help" || command == "agent.help") {
        return "Commands: help, tools.list, tools.call <tool> {json}, views.list, raw.summary, raw.state, raw.goto_sample <n>, raw.cursor <n>, raw.zoom <px_per_sample>, kernel.raw_stats [window_samples], kernel.channel_stats [window_samples].";
    }

    if(command == "views.list") {
        QStringList views;
        for(int i = 0; i < m_centerTabs->count(); ++i) {
            views << QString("%1: %2").arg(i + 1).arg(m_centerTabs->tabText(i));
        }
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    RawDataBrowserWidget* rawBrowser = activeRawBrowser();

    if(command == "raw.summary") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        return rawBrowser->summaryText();
    }

    if(command == "raw.state") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        return rawBrowser->stateText();
    }

    if(command == "raw.goto_sample" || command == "raw.cursor") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        if(parts.size() < 2) {
            return "Usage: raw.goto_sample <sample>";
        }
        bool ok = false;
        const int sample = parts.at(1).toInt(&ok);
        if(!ok) {
            return QString("Invalid sample: %1").arg(parts.at(1));
        }
        rawBrowser->gotoSample(sample);
        return QString("Moved raw browser cursor to sample %1.").arg(rawBrowser->cursorSample());
    }

    if(command == "raw.zoom") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        if(parts.size() < 2) {
            return "Usage: raw.zoom <pixels_per_sample>";
        }
        bool ok = false;
        const double pixelsPerSample = parts.at(1).toDouble(&ok);
        if(!ok || pixelsPerSample <= 0.0) {
            return QString("Invalid zoom value: %1").arg(parts.at(1));
        }
        rawBrowser->setZoomPixelsPerSample(pixelsPerSample);
        return QString("Raw browser zoom set to %1 px/sample.")
            .arg(QString::number(rawBrowser->pixelsPerSample(), 'f', 2));
    }

    if(command == "kernel.raw_stats") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }

        int windowSamples = 600;
        if(parts.size() >= 2) {
            bool ok = false;
            const int parsedWindow = parts.at(1).toInt(&ok);
            if(!ok || parsedWindow <= 0) {
                return QString("Invalid window size: %1").arg(parts.at(1));
            }
            windowSamples = parsedWindow;
        }

        const int cursorSample = rawBrowser->cursorSample() >= 0 ? rawBrowser->cursorSample() : 0;
        sendKernelToolCall("neurokernel.raw_stats", buildRawWindowArguments(windowSamples));
        return QString("Requested neuro-kernel raw statistics for %1 around sample %2.")
            .arg(QFileInfo(rawBrowser->filePath()).fileName())
            .arg(cursorSample);
    }

    if(command == "kernel.channel_stats") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }

        int windowSamples = 600;
        if(parts.size() >= 2) {
            bool ok = false;
            const int parsedWindow = parts.at(1).toInt(&ok);
            if(!ok || parsedWindow <= 0) {
                return QString("Invalid window size: %1").arg(parts.at(1));
            }
            windowSamples = parsedWindow;
        }

        const int cursorSample = rawBrowser->cursorSample() >= 0 ? rawBrowser->cursorSample() : 0;
        sendKernelToolCall("neurokernel.channel_stats", buildRawWindowArguments(windowSamples));
        return QString("Requested neuro-kernel channel statistics for %1 around sample %2.")
            .arg(QFileInfo(rawBrowser->filePath()).fileName())
            .arg(cursorSample);
    }

    handled = false;
    return QString();
}

QJsonArray MainWindow::localToolDefinitions() const
{
    return QJsonArray{
        QJsonObject{
            {"name", "studio.views.list"},
            {"description", "List open center views in the workbench."}
        },
        QJsonObject{
            {"name", "view.raw.summary"},
            {"description", "Return summary metadata for the active raw browser."}
        },
        QJsonObject{
            {"name", "view.raw.state"},
            {"description", "Return visible range, cursor, and zoom for the active raw browser."}
        },
        QJsonObject{
            {"name", "view.raw.goto"},
            {"description", "Move the active raw browser to a given sample with {\"sample\": <int>}."}
        },
        QJsonObject{
            {"name", "view.raw.zoom"},
            {"description", "Set raw browser zoom with {\"pixels_per_sample\": <number>}."}
        }
    };
}

QJsonObject MainWindow::buildRawWindowArguments(int windowSamples) const
{
    RawDataBrowserWidget* rawBrowser = activeRawBrowser();
    if(!rawBrowser) {
        return QJsonObject();
    }

    const int cursorSample = rawBrowser->cursorSample() >= 0 ? rawBrowser->cursorSample() : 0;
    const int halfWindow = qMax(1, windowSamples) / 2;
    return QJsonObject{
        {"file", rawBrowser->filePath()},
        {"from_sample", qMax(0, cursorSample - halfWindow)},
        {"to_sample", qMax(0, cursorSample + halfWindow)},
        {"cursor_sample", cursorSample}
    };
}

void MainWindow::sendKernelToolCall(const QString& toolName, const QJsonObject& arguments)
{
    QJsonObject params{
        {"name", toolName},
        {"arguments", arguments}
    };

    const QJsonObject message = JsonRpcMessage::createRequest("workbench-1", "tools/call", params);
    m_kernelSocket->write(JsonRpcMessage::serialize(message));
    m_kernelSocket->flush();
}

void MainWindow::appendOutputMessage(const QString& message)
{
    m_outputPanel->addItem(message);
    m_outputPanel->scrollToBottom();
}

void MainWindow::appendProblemMessage(const QString& message)
{
    m_problemPanel->addItem(message);
    m_problemPanel->scrollToBottom();
    if(m_bottomPanelTabs->currentWidget() != m_problemPanel) {
        m_bottomPanelTabs->setCurrentWidget(m_problemPanel);
    }
}

void MainWindow::appendTerminalMessage(const QString& message)
{
    m_terminalPanel->append(message);
    m_terminalPanel->moveCursor(QTextCursor::End);
}

void MainWindow::setActivePanelState(const QString& message)
{
    const QString stateText = QString("Active view state: %1").arg(message);
    if(m_activeStateItem) {
        m_activeStateItem->setText(stateText);
    }
    if(m_terminalStatusLabel) {
        m_terminalStatusLabel->setText(stateText);
    }
}

void MainWindow::restoreWorkspace()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QByteArray payload = settings.value("workspace/scenes").toByteArray();
    if(!payload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(payload);
        if(document.isArray()) {
            m_sceneRegistry.restore(document.array());
        }
    }

    const QStringList workspaceFiles = settings.value("workspace/files").toStringList();
    const QStringList workspaceDirectories = settings.value("workspace/directories").toStringList();
    for(const QString& directoryPath : workspaceDirectories) {
        addWorkspaceDirectory(directoryPath);
    }
    for(const QString& filePath : workspaceFiles) {
        openFileInView(filePath);
    }
}

void MainWindow::persistWorkspace() const
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    QStringList workspaceFiles;
    QStringList workspaceDirectories;
    for(int i = 0; i < m_workspaceExplorer->topLevelItemCount(); ++i) {
        QTreeWidgetItem* rootItem = m_workspaceExplorer->topLevelItem(i);
        const QString rootPath = rootItem->data(0, Qt::UserRole).toString();
        if(QFileInfo(rootPath).isDir()) {
            workspaceDirectories.append(rootPath);
        }
    }
    for(int i = 0; i < m_centerTabs->count(); ++i) {
        const QString filePath = m_centerTabs->tabToolTip(i);
        if(!filePath.isEmpty()) {
            workspaceFiles.append(filePath);
        }
    }

    settings.setValue("workspace/directories", workspaceDirectories);
    settings.setValue("workspace/files", workspaceFiles);
    settings.setValue("workspace/scenes", QJsonDocument(m_sceneRegistry.serialize()).toJson(QJsonDocument::Compact));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    persistWorkspace();
    QMainWindow::closeEvent(event);
}

bool MainWindow::isSupportedWorkspaceFile(const QString& filePath) const
{
    const auto kind = ViewManager::viewKindForFile(filePath);
    return kind != ViewManager::ViewKind::Unsupported;
}
