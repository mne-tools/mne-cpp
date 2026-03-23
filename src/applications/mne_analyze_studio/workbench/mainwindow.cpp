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
#include "editortabbar.h"
#include "editortabwidget.h"
#include "llmsettingsdialog.h"
#include "rawdatabrowserwidget.h"

#include <jsonrpcmessage.h>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
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
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSpinBox>
#include <QFrame>
#include <QHeaderView>
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

QTreeWidgetItem* makeToolTreeItem(const QJsonObject& tool)
{
    const QString name = tool.value("name").toString().trimmed();
    const QString description = tool.value("description").toString().trimmed();

    QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << (name.isEmpty() ? QString("Unnamed Tool") : name));
    item->setToolTip(0, description);
    item->setData(0, Qt::UserRole, name);
    item->setData(0, Qt::UserRole + 1, tool);

    return item;
}

QTreeWidgetItem* buildJsonTreeItem(const QString& key, const QJsonValue& value)
{
    QString displayValue;
    if(value.isObject()) {
        displayValue = "{...}";
    } else if(value.isArray()) {
        displayValue = QString("[%1]").arg(value.toArray().size());
    } else if(value.isString()) {
        displayValue = value.toString();
    } else if(value.isDouble()) {
        displayValue = QString::number(value.toDouble(), 'g', 8);
    } else if(value.isBool()) {
        displayValue = value.toBool() ? QString("true") : QString("false");
    } else if(value.isNull()) {
        displayValue = "null";
    } else {
        displayValue = QString::fromUtf8(QJsonDocument::fromVariant(value.toVariant()).toJson(QJsonDocument::Compact));
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << key << displayValue);
    if(value.isObject()) {
        const QJsonObject object = value.toObject();
        for(auto it = object.constBegin(); it != object.constEnd(); ++it) {
            item->addChild(buildJsonTreeItem(it.key(), it.value()));
        }
    } else if(value.isArray()) {
        const QJsonArray array = value.toArray();
        for(int i = 0; i < array.size(); ++i) {
            item->addChild(buildJsonTreeItem(QString("[%1]").arg(i), array.at(i)));
        }
    }

    return item;
}

QJsonObject objectSchema(const QJsonObject& properties,
                         const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QJsonObject integerSchema(const QString& title,
                          int minimum,
                          int maximum,
                          int defaultValue,
                          const QString& description = QString())
{
    QJsonObject schema{
        {"type", "integer"},
        {"title", title},
        {"minimum", minimum},
        {"maximum", maximum},
        {"default", defaultValue}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject numberSchema(const QString& title,
                         double minimum,
                         double maximum,
                         double defaultValue,
                         const QString& description = QString())
{
    QJsonObject schema{
        {"type", "number"},
        {"title", title},
        {"minimum", minimum},
        {"maximum", maximum},
        {"default", defaultValue}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject stringSchema(const QString& title,
                         const QJsonArray& values = QJsonArray(),
                         const QString& defaultValue = QString(),
                         const QString& description = QString())
{
    QJsonObject schema{
        {"type", "string"},
        {"title", title},
        {"default", defaultValue}
    };

    if(!values.isEmpty()) {
        schema.insert("enum", values);
    }

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject resultStringSchema(const QString& title,
                               const QString& description = QString())
{
    return stringSchema(title, QJsonArray(), QString(), description);
}

QJsonObject arraySchema(const QString& title,
                        const QJsonObject& itemSchema,
                        const QString& description = QString())
{
    QJsonObject schema{
        {"type", "array"},
        {"title", title},
        {"items", itemSchema}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
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
, m_skillsPage(new QWidget(this))
, m_skillDetailsView(new QPlainTextEdit(this))
, m_skillRunButton(new QPushButton("Run Tool...", this))
, m_leftSidebarStack(new QStackedWidget)
, m_mainSplitter(new QSplitter(Qt::Horizontal))
, m_centerSplitter(new QSplitter(Qt::Vertical))
, m_centerTabs(new EditorTabWidget)
, m_centerTabBar(new EditorTabBar(this))
, m_bottomPanelTabs(new QTabWidget)
, m_outputPanel(new QListWidget)
, m_problemPanel(new QListWidget)
, m_resultsTab(new QWidget(this))
, m_resultsTitleLabel(new QLabel("No structured results yet."))
, m_resultsActionButton(new QPushButton("Run Action", this))
, m_resultsStack(new QStackedWidget(this))
, m_resultsTree(new QTreeWidget(this))
, m_resultsTable(new QTableWidget(this))
, m_terminalTab(new QWidget(this))
, m_terminalStatusLabel(new QLabel("Active view state: idle"))
, m_terminalPanel(new QTextEdit)
, m_terminalInput(new QLineEdit)
, m_terminalRunButton(new QPushButton("Run"))
, m_activeStateItem(new QListWidgetItem("Active view state: idle"))
, m_kernelSocket(new QLocalSocket(this))
, m_llmPlanner(this)
, m_sceneRegistry(this)
, m_viewManager(&m_sceneRegistry, this)
{
    setWindowTitle("MNE Analyze Studio");
    resize(1440, 900);

    createLayout();
    createConnections();
    loadAgentSettings();
    restoreWorkspace();
    applyWorkbenchStyle();
    refreshAgentPlannerStatus();

    connect(m_kernelSocket, &QLocalSocket::connected, this, &MainWindow::requestKernelToolDefinitions);
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
    m_skillsExplorer->setAlternatingRowColors(true);
    m_skillsExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_skillDetailsView->setReadOnly(true);
    m_skillDetailsView->setPlaceholderText("Select a workbench or Neuro-Kernel tool to inspect it here.");
    m_skillRunButton->setEnabled(false);

    QVBoxLayout* skillsPageLayout = new QVBoxLayout(m_skillsPage);
    skillsPageLayout->setContentsMargins(0, 0, 0, 0);
    skillsPageLayout->setSpacing(8);
    skillsPageLayout->addWidget(m_skillsExplorer, 2);
    skillsPageLayout->addWidget(new QLabel("Tool Inspector", m_skillsPage));
    skillsPageLayout->addWidget(m_skillDetailsView, 1);
    skillsPageLayout->addWidget(m_skillRunButton);
    rebuildSkillsExplorer();

    m_leftSidebarStack->addWidget(m_workspaceExplorer);
    m_leftSidebarStack->addWidget(m_skillsPage);

    leftSidebarLayout->addWidget(m_activityBar);
    leftSidebarLayout->addWidget(m_leftSidebarStack, 1);

    QWidget* leftPanel = createSidebarSection("Workspace", leftSidebar);

    m_centerTabs->setDocumentMode(true);
    m_centerTabs->setTabsClosable(true);
    m_centerTabs->setMovable(true);
    m_centerTabs->setTabBar(m_centerTabBar);
    m_centerTabs->tabBar()->setExpanding(false);

    m_bottomPanelTabs->setDocumentMode(true);
    m_bottomPanelTabs->setMovable(false);
    m_bottomPanelTabs->addTab(m_outputPanel, "Output");
    m_bottomPanelTabs->addTab(m_problemPanel, "Problems");
    m_bottomPanelTabs->addTab(m_resultsTab, "Results");
    m_bottomPanelTabs->addTab(m_terminalTab, "Terminal");
    m_outputPanel->setAlternatingRowColors(true);
    m_problemPanel->setAlternatingRowColors(true);
    m_outputPanel->addItem(m_activeStateItem);
    m_resultsActionButton->setVisible(false);
    m_resultsActionButton->setEnabled(false);
    m_resultsTable->setColumnCount(4);
    m_resultsTable->setHorizontalHeaderLabels(QStringList() << "Name" << "RMS" << "Mean |x|" << "Peak |x|");
    m_resultsTable->setSortingEnabled(true);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTree->setColumnCount(2);
    m_resultsTree->setHeaderLabels(QStringList() << "Field" << "Value");
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTitleLabel->setObjectName("terminalStatusLabel");
    m_resultsStack->addWidget(m_resultsTree);
    m_resultsStack->addWidget(m_resultsTable);

    QVBoxLayout* resultsLayout = new QVBoxLayout(m_resultsTab);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    resultsLayout->setSpacing(8);
    resultsLayout->addWidget(m_resultsTitleLabel);
    resultsLayout->addWidget(m_resultsActionButton);
    resultsLayout->addWidget(m_resultsStack, 1);

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

    QMenu* agentMenu = menuBar()->addMenu("Agent");
    QAction* agentSettingsAction = agentMenu->addAction("Planner Settings...");
    connect(agentSettingsAction, &QAction::triggered, this, &MainWindow::openAgentSettings);

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
    connect(m_skillsExplorer, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) {
        updateSelectedSkillTool(current);
    });
    connect(m_skillsExplorer, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        updateSelectedSkillTool(item);
        if(!m_selectedSkillToolName.isEmpty()) {
            runSelectedSkillTool();
        }
    });
    connect(m_skillsExplorer, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        QMenu contextMenu(this);
        QTreeWidgetItem* item = m_skillsExplorer->itemAt(position);
        if(item) {
            updateSelectedSkillTool(item);
        }

        QAction* refreshToolsAction = contextMenu.addAction("Refresh Tool Registry");
        connect(refreshToolsAction, &QAction::triggered, this, &MainWindow::requestKernelToolDefinitions);

        if(!m_selectedSkillToolName.isEmpty()) {
            QAction* runToolAction = contextMenu.addAction("Run Tool...");
            connect(runToolAction, &QAction::triggered, this, &MainWindow::runSelectedSkillTool);
        }

        contextMenu.exec(m_skillsExplorer->viewport()->mapToGlobal(position));
    });
    connect(m_skillRunButton, &QPushButton::clicked, this, &MainWindow::runSelectedSkillTool);
    connect(m_resultsActionButton, &QPushButton::clicked, this, &MainWindow::runResultPrimaryAction);
    connect(m_resultsTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateResultPrimaryActionForSelection);
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
    connect(m_centerTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeCenterTab);
    connect(m_centerTabBar, &EditorTabBar::closeButtonClicked, this, &MainWindow::closeCenterTab);
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
                    const QString toolName = result.value("tool_name").toString();
                    if(toolName == "tools/list" && result.value("tools").isArray()) {
                        m_cachedKernelToolDefinitions = result.value("tools").toArray();
                        rebuildSkillsExplorer();
                        statusBar()->showMessage(QString("Discovered %1 Neuro-Kernel tools.")
                                                     .arg(result.value("tools").toArray().size()),
                                                 4000);
                        continue;
                    }
                    rememberToolResult(toolName, result);
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
        "#agentChatStatus { background: #181c22; color: #8b949e; padding: 8px 12px; border-bottom: 1px solid #30363d; }"
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
                                                           : static_cast<QWidget*>(m_skillsPage));

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

    bool plannedSteps = false;
    QStringList plannedCommands;
    const QString multiStepNote = planAgentSteps(commandText, plannedCommands, plannedSteps);
    if(plannedSteps) {
        if(!multiStepNote.isEmpty()) {
            m_agentChatDock->appendTranscript(QString("Planner> %1").arg(multiStepNote));
            appendTerminalMessage(QString("> %1").arg(multiStepNote));
        }

        for(int i = 0; i < plannedCommands.size(); ++i) {
            const QString plannedCommand = resolvePlannerReferences(plannedCommands.at(i));
            const QString stepNote = QString("Running step %1/%2: %3")
                                         .arg(i + 1)
                                         .arg(plannedCommands.size())
                                         .arg(plannedCommand);
            m_agentChatDock->appendTranscript(QString("Planner> %1").arg(stepNote));
            appendTerminalMessage(QString("> %1").arg(stepNote));
            sendToolCall(plannedCommand);
        }
        return;
    }

    if(m_llmPlanner.isConfigured()) {
        const LlmPlanResult llmPlan = m_llmPlanner.plan(commandText,
                                                       availableToolDefinitions(),
                                                       llmPlanningContext(commandText));
        if(llmPlan.success) {
            const QString plannerMessage = llmPlan.summary.isEmpty()
                ? QString("LLM planner (%1) proposed %2 steps.").arg(llmPlan.providerName).arg(llmPlan.plannedCommands.size())
                : QString("LLM planner (%1): %2").arg(llmPlan.providerName, llmPlan.summary);
            m_agentChatDock->appendTranscript(QString("Planner> %1").arg(plannerMessage));
            appendTerminalMessage(QString("> %1").arg(plannerMessage));

            for(int i = 0; i < llmPlan.plannedCommands.size(); ++i) {
                const QString plannedCommand = resolvePlannerReferences(llmPlan.plannedCommands.at(i));
                const QString stepNote = QString("Running LLM step %1/%2: %3")
                                             .arg(i + 1)
                                             .arg(llmPlan.plannedCommands.size())
                                             .arg(plannedCommand);
                m_agentChatDock->appendTranscript(QString("Planner> %1").arg(stepNote));
                appendTerminalMessage(QString("> %1").arg(stepNote));
                sendToolCall(plannedCommand);
            }
            return;
        }
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

void MainWindow::closeCenterTab(int index)
{
    if(index < 0 || index >= m_centerTabs->count()) {
        return;
    }

    QWidget* widget = m_centerTabs->widget(index);
    m_centerTabs->removeTab(index);
    delete widget;
}

QString MainWindow::planAgentSteps(const QString& commandText, QStringList& plannedCommands, bool& planned) const
{
    planned = false;
    plannedCommands.clear();

    const QString trimmed = commandText.trimmed();
    if(trimmed.isEmpty()) {
        return QString();
    }

    const QRegularExpression stepSeparator("\\s*(?:,\\s*then\\s+|;\\s*|\\s+and then\\s+|\\s+then\\s+)\\s*",
                                           QRegularExpression::CaseInsensitiveOption);
    const QStringList rawSteps = trimmed.split(stepSeparator, Qt::SkipEmptyParts);
    if(rawSteps.size() <= 1) {
        return QString();
    }

    QStringList mappedDescriptions;
    for(const QString& rawStep : rawSteps) {
        const QString stepLower = rawStep.trimmed().toLower();
        const bool asksForGoto = stepLower.contains("go to") || stepLower.contains("goto")
                                 || stepLower.contains("jump to") || stepLower.contains("move to");
        const bool asksForPeak = stepLower.contains("strongest") || stepLower.contains("peak")
                                 || stepLower.contains("burst");
        if(asksForGoto && asksForPeak) {
            const QRegularExpression windowPattern("(\\d+)\\s*(?:samples|sample)");
            const QRegularExpressionMatch windowMatch = windowPattern.match(stepLower);
            const int windowSamples = windowMatch.hasMatch() ? windowMatch.captured(1).toInt() : 4000;

            QStringList jsonParts;
            jsonParts << QString("\"window_samples\":%1").arg(windowSamples);
            if(stepLower.contains("eeg")) {
                jsonParts << "\"match\":\"EEG\"";
            } else if(stepLower.contains("meg")) {
                jsonParts << "\"match\":\"MEG\"";
            } else if(stepLower.contains("eog")) {
                jsonParts << "\"match\":\"EOG\"";
            }

            plannedCommands << QString("tools.call neurokernel.find_peak_window {%1}").arg(jsonParts.join(","));
            plannedCommands << "tools.call view.raw.goto {\"sample\":${last_peak_sample}}";
            mappedDescriptions << "Mapped strongest-burst search to `neurokernel.find_peak_window` and `view.raw.goto`.";
            continue;
        }

        bool stepPlanned = false;
        QString plannedCommand;
        const QString stepNote = planAgentIntent(rawStep.trimmed(), plannedCommand, stepPlanned);
        if(!stepPlanned || plannedCommand.isEmpty()) {
            plannedCommands.clear();
            return QString();
        }

        plannedCommands << plannedCommand;
        mappedDescriptions << (stepNote.isEmpty() ? plannedCommand : stepNote);
    }

    planned = true;
    return QString("Mapped your request into %1 steps: %2")
        .arg(plannedCommands.size())
        .arg(mappedDescriptions.join(" | "));
}

QString MainWindow::resolvePlannerReferences(const QString& commandText) const
{
    QString resolved = commandText;

    if(resolved.contains("${last_peak_sample}")) {
        const int peakSample = m_lastToolResult.value("peak_sample").toInt(-1);
        if(peakSample >= 0) {
            resolved.replace("${last_peak_sample}", QString::number(peakSample));
        }
    }

    if(resolved.contains("${last_cursor_sample}")) {
        RawDataBrowserWidget* rawBrowser = activeRawBrowser();
        const int cursorSample = rawBrowser ? rawBrowser->cursorSample() : -1;
        if(cursorSample >= 0) {
            resolved.replace("${last_cursor_sample}", QString::number(cursorSample));
        }
    }

    return resolved;
}

void MainWindow::rememberToolResult(const QString& toolName, const QJsonObject& result)
{
    if(toolName.isEmpty()) {
        return;
    }

    m_lastToolName = toolName;
    m_lastToolResult = result;
    updateStructuredResultView(toolName, result);
}

void MainWindow::updateStructuredResultView(const QString& toolName, const QJsonObject& result)
{
    if(!m_resultsTree || !m_resultsTitleLabel || !m_resultsStack || !m_resultsTable || !m_resultsActionButton) {
        return;
    }

    m_resultsTitleLabel->setText(QString("Latest structured result: %1").arg(toolName));
    m_resultsCurrentToolName = toolName;
    m_resultPrimaryActionCommand.clear();
    m_resultsActionButton->setVisible(false);
    m_resultsActionButton->setEnabled(false);
    m_resultsTree->clear();
    m_resultsTable->setRowCount(0);
    m_resultsStack->setCurrentWidget(m_resultsTree);

    if(result.isEmpty()) {
        m_resultsTree->addTopLevelItem(new QTreeWidgetItem(QStringList() << "status" << "No result payload."));
        return;
    }

    if(toolName == "neurokernel.channel_stats" && result.value("channels").isArray()) {
        const QJsonArray channels = result.value("channels").toArray();
        m_resultsTitleLabel->setText(result.value("message").toString(QString("Latest structured result: %1").arg(toolName)));
        m_resultsTable->setRowCount(channels.size());
        for(int row = 0; row < channels.size(); ++row) {
            const QJsonObject channel = channels.at(row).toObject();
            m_resultsTable->setItem(row, 0, new QTableWidgetItem(channel.value("name").toString()));
            m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(channel.value("rms").toDouble(), 'g', 8)));
            m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(channel.value("mean_abs").toDouble(), 'g', 8)));
            m_resultsTable->setItem(row, 3, new QTableWidgetItem(QString::number(channel.value("peak_abs").toDouble(), 'g', 8)));
        }
        m_resultsStack->setCurrentWidget(m_resultsTable);
        m_resultsActionButton->setText("Find Peak For Selected Channel");
        m_resultsActionButton->setVisible(true);
        updateResultPrimaryActionForSelection();
        m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
        return;
    }

    if(toolName == "neurokernel.raw_stats" && result.value("top_channels").isArray()) {
        const QJsonArray channels = result.value("top_channels").toArray();
        m_resultsTitleLabel->setText(result.value("message").toString(QString("Latest structured result: %1").arg(toolName)));
        m_resultsTable->setRowCount(channels.size());
        for(int row = 0; row < channels.size(); ++row) {
            const QJsonObject channel = channels.at(row).toObject();
            m_resultsTable->setItem(row, 0, new QTableWidgetItem(channel.value("name").toString()));
            m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(channel.value("rms").toDouble(), 'g', 8)));
            m_resultsTable->setItem(row, 2, new QTableWidgetItem("-"));
            m_resultsTable->setItem(row, 3, new QTableWidgetItem("-"));
        }
        m_resultsStack->setCurrentWidget(m_resultsTable);
        m_resultsActionButton->setText("Find Peak For Selected Channel");
        m_resultsActionButton->setVisible(true);
        updateResultPrimaryActionForSelection();
        m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
        return;
    }

    if(toolName == "neurokernel.find_peak_window") {
        const int peakSample = result.value("peak_sample").toInt(-1);
        if(peakSample >= 0) {
            m_resultPrimaryActionCommand = QString("tools.call view.raw.goto {\"sample\":%1}").arg(peakSample);
            m_resultsActionButton->setText(QString("Jump To Peak Sample %1").arg(peakSample));
            m_resultsActionButton->setVisible(true);
            m_resultsActionButton->setEnabled(true);
        }
    }

    for(auto it = result.constBegin(); it != result.constEnd(); ++it) {
        m_resultsTree->addTopLevelItem(buildJsonTreeItem(it.key(), it.value()));
    }
    m_resultsTree->expandToDepth(1);
    for(int column = 0; column < m_resultsTree->columnCount(); ++column) {
        m_resultsTree->resizeColumnToContents(column);
    }
    m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
}

void MainWindow::updateResultPrimaryActionForSelection()
{
    if(!m_resultsActionButton || !m_resultsTable) {
        return;
    }

    if(m_resultsCurrentToolName != "neurokernel.channel_stats"
       && m_resultsCurrentToolName != "neurokernel.raw_stats") {
        return;
    }

    const QList<QTableWidgetItem*> items = m_resultsTable->selectedItems();
    if(items.isEmpty()) {
        m_resultPrimaryActionCommand.clear();
        m_resultsActionButton->setEnabled(false);
        return;
    }

    const int row = items.first()->row();
    QTableWidgetItem* nameItem = m_resultsTable->item(row, 0);
    if(!nameItem || nameItem->text().trimmed().isEmpty()) {
        m_resultPrimaryActionCommand.clear();
        m_resultsActionButton->setEnabled(false);
        return;
    }

    const QString channelName = nameItem->text().trimmed();
    m_resultPrimaryActionCommand = QString("tools.call neurokernel.find_peak_window {\"window_samples\":4000,\"match\":\"%1\"}")
                                       .arg(channelName);
    m_resultsActionButton->setText(QString("Find Peak For %1").arg(channelName));
    m_resultsActionButton->setVisible(true);
    m_resultsActionButton->setEnabled(true);
}

void MainWindow::runResultPrimaryAction()
{
    if(m_resultPrimaryActionCommand.isEmpty()) {
        return;
    }

    appendTerminalMessage(QString("$ %1").arg(m_resultPrimaryActionCommand));
    sendToolCall(m_resultPrimaryActionCommand);
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

    if(containsAny({"strongest burst", "peak burst", "strongest eeg burst", "peak eeg burst"})) {
        planned = true;
        QStringList jsonParts;
        jsonParts << QString("\"window_samples\":%1").arg(windowSamples > 0 ? windowSamples : 4000);
        if(lower.contains("eeg")) {
            jsonParts << "\"match\":\"EEG\"";
        } else if(lower.contains("meg")) {
            jsonParts << "\"match\":\"MEG\"";
        } else if(lower.contains("eog")) {
            jsonParts << "\"match\":\"EOG\"";
        }
        plannedCommand = QString("tools.call neurokernel.find_peak_window {%1}").arg(jsonParts.join(","));
        return "Mapped your request to `neurokernel.find_peak_window`.";
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
        requestKernelToolDefinitions();
        return QString("Studio tools: %1 | Requested kernel tools list.")
            .arg(formatToolDefinitions(localToolDefinitions()));
    }

    if(trimmed == "kernel.tools.list") {
        requestKernelToolDefinitions();
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
        rememberToolResult(toolName, QJsonObject{
            {"tool_name", toolName},
            {"views", QJsonArray::fromStringList(views)}
        });
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    RawDataBrowserWidget* rawBrowser = activeRawBrowser();
    if(toolName == "view.raw.summary") {
        rememberToolResult(toolName, QJsonObject{
            {"tool_name", toolName},
            {"summary", rawBrowser ? rawBrowser->summaryText() : QString("No active raw browser.")}
        });
        return rawBrowser ? rawBrowser->summaryText() : "No active raw browser.";
    }

    if(toolName == "view.raw.state") {
        rememberToolResult(toolName, QJsonObject{
            {"tool_name", toolName},
            {"state", rawBrowser ? rawBrowser->stateText() : QString("No active raw browser.")}
        });
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
        rememberToolResult(toolName, QJsonObject{
            {"tool_name", toolName},
            {"sample", rawBrowser->cursorSample()},
            {"cursor_sample", rawBrowser->cursorSample()}
        });
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
        rememberToolResult(toolName, QJsonObject{
            {"tool_name", toolName},
            {"pixels_per_sample", rawBrowser->pixelsPerSample()}
        });
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

    if(toolName == "neurokernel.find_peak_window") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        QJsonObject kernelArguments = buildRawWindowArguments(arguments.value("window_samples").toInt(4000));
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
            {"description", "List open center views in the workbench."},
            {"input_schema", objectSchema(QJsonObject())},
            {"result_schema", objectSchema(QJsonObject{
                 {"views", arraySchema("Open Views", resultStringSchema("View Description"))}
             }, QJsonArray{"views"})}
        },
        QJsonObject{
            {"name", "view.raw.summary"},
            {"description", "Return summary metadata for the active raw browser."},
            {"input_schema", objectSchema(QJsonObject())},
            {"result_schema", objectSchema(QJsonObject{
                 {"summary", resultStringSchema("Summary", "Human-readable summary of the active raw browser dataset.")}
             }, QJsonArray{"summary"})}
        },
        QJsonObject{
            {"name", "view.raw.state"},
            {"description", "Return visible range, cursor, and zoom for the active raw browser."},
            {"input_schema", objectSchema(QJsonObject())},
            {"result_schema", objectSchema(QJsonObject{
                 {"state", resultStringSchema("State", "Human-readable state of the active raw browser, including cursor and zoom.")}
             }, QJsonArray{"state"})}
        },
        QJsonObject{
            {"name", "view.raw.goto"},
            {"description", "Move the active raw browser to a given sample with {\"sample\": <int>}."},
            {"input_schema", objectSchema(QJsonObject{
                 {"sample", integerSchema("Sample",
                                          0,
                                          std::numeric_limits<int>::max(),
                                          0,
                                          "Absolute sample index to move the active raw browser cursor to.")}
             }, QJsonArray{"sample"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"sample", integerSchema("Sample", 0, std::numeric_limits<int>::max(), 0)},
                 {"cursor_sample", integerSchema("Cursor Sample", 0, std::numeric_limits<int>::max(), 0)}
             }, QJsonArray{"sample", "cursor_sample"})}
        },
        QJsonObject{
            {"name", "view.raw.zoom"},
            {"description", "Set raw browser zoom with {\"pixels_per_sample\": <number>}."},
            {"input_schema", objectSchema(QJsonObject{
                 {"pixels_per_sample", numberSchema("Pixels / Sample",
                                                    0.01,
                                                    100.0,
                                                    1.0,
                                                    "Horizontal zoom density of the raw browser in pixels per sample.")}
             }, QJsonArray{"pixels_per_sample"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"pixels_per_sample", numberSchema("Pixels / Sample", 0.01, 100.0, 1.0)}
             }, QJsonArray{"pixels_per_sample"})}
        }
    };
}

QJsonArray MainWindow::kernelToolDefinitions() const
{
    if(!m_cachedKernelToolDefinitions.isEmpty()) {
        return m_cachedKernelToolDefinitions;
    }

    return QJsonArray{
        QJsonObject{
            {"name", "neurokernel.raw_stats"},
            {"description", "Compute RMS, mean absolute value, peak absolute value, and top channels for a raw sample window."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  1,
                                                  1000000,
                                                  600,
                                                  "Number of samples around the current cursor to include in the analysis window.")}
             }, QJsonArray{"window_samples"})}
        },
        QJsonObject{
            {"name", "neurokernel.channel_stats"},
            {"description", "Compute per-channel RMS, mean absolute value, and peak absolute value for a raw sample window."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  1,
                                                  1000000,
                                                  600,
                                                  "Number of samples around the current cursor to analyze.")},
                 {"limit", integerSchema("Channel Limit",
                                         1,
                                         512,
                                         5,
                                         "Maximum number of channels to return in the result.")},
                 {"match", stringSchema("Channel Match",
                                        QJsonArray{"", "EEG", "MEG", "EOG"},
                                        "EEG",
                                        "Optional channel-name filter applied before ranking channels.")}
             }, QJsonArray{"window_samples"})}
        },
        QJsonObject{
            {"name", "neurokernel.find_peak_window"},
            {"description", "Find the strongest absolute-amplitude sample inside a raw window, optionally filtered by channel name match."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  1,
                                                  1000000,
                                                  4000,
                                                  "Number of samples to search for the strongest absolute-amplitude event.")},
                 {"match", stringSchema("Channel Match",
                                        QJsonArray{"", "EEG", "MEG", "EOG"},
                                        "EEG",
                                        "Optional channel-name filter used while searching for the peak window.")}
             }, QJsonArray{"window_samples"})}
        }
    };
}

void MainWindow::requestKernelToolDefinitions()
{
    if(!m_kernelSocket || m_kernelSocket->state() != QLocalSocket::ConnectedState) {
        appendProblemMessage("Kernel tool discovery requested before the Neuro-Kernel socket was connected.");
        return;
    }

    const QJsonObject message = JsonRpcMessage::createRequest("workbench-tools-list", "tools/list");
    m_kernelSocket->write(JsonRpcMessage::serialize(message));
    m_kernelSocket->flush();
}

QJsonArray MainWindow::availableToolDefinitions() const
{
    QJsonArray tools = localToolDefinitions();
    const QJsonArray kernelTools = kernelToolDefinitions();
    for(const QJsonValue& tool : kernelTools) {
        tools.append(tool);
    }
    return tools;
}

QJsonObject MainWindow::toolDefinition(const QString& toolName) const
{
    const QJsonArray tools = availableToolDefinitions();
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = value.toObject();
        if(tool.value("name").toString() == toolName) {
            return tool;
        }
    }

    return QJsonObject();
}

QJsonObject MainWindow::llmPlanningContext(const QString& commandText) const
{
    Q_UNUSED(commandText)

    QJsonObject context{
        {"last_tool_name", m_lastToolName},
        {"last_tool_result", m_lastToolResult},
        {"tool_schema_summary", availableToolDefinitions()}
    };

    if(RawDataBrowserWidget* rawBrowser = activeRawBrowser()) {
        context.insert("active_view", "raw_browser");
        context.insert("active_file", rawBrowser->filePath());
        context.insert("raw_summary", rawBrowser->summaryText());
        context.insert("raw_state", rawBrowser->stateText());
        context.insert("cursor_sample", rawBrowser->cursorSample());
        context.insert("pixels_per_sample", rawBrowser->pixelsPerSample());
    } else {
        context.insert("active_view", "none");
    }

    return context;
}

QJsonObject MainWindow::defaultArgumentsForTool(const QString& toolName) const
{
    if(toolName == "studio.views.list" || toolName == "view.raw.summary" || toolName == "view.raw.state") {
        return QJsonObject();
    }

    if(toolName == "view.raw.goto" || toolName == "view.raw.cursor") {
        RawDataBrowserWidget* rawBrowser = activeRawBrowser();
        return QJsonObject{{"sample", rawBrowser ? rawBrowser->cursorSample() : 0}};
    }

    if(toolName == "view.raw.zoom") {
        RawDataBrowserWidget* rawBrowser = activeRawBrowser();
        return QJsonObject{{"pixels_per_sample", rawBrowser ? rawBrowser->pixelsPerSample() : 1.0}};
    }

    if(toolName == "neurokernel.raw_stats") {
        return QJsonObject{{"window_samples", 600}};
    }

    if(toolName == "neurokernel.channel_stats") {
        return QJsonObject{{"window_samples", 600}, {"limit", 5}, {"match", "EEG"}};
    }

    if(toolName == "neurokernel.find_peak_window") {
        return QJsonObject{{"window_samples", 4000}, {"match", "EEG"}};
    }

    if(toolName == "neurokernel.execute") {
        return QJsonObject{{"command", "help"}};
    }

    return QJsonObject();
}

bool MainWindow::editArgumentsForTool(const QString& toolName, QJsonObject& arguments)
{
    const QJsonObject definition = toolDefinition(toolName);
    const QJsonObject schema = definition.value("input_schema").toObject();
    const QJsonObject properties = schema.value("properties").toObject();
    const QJsonArray required = schema.value("required").toArray();

    if(properties.isEmpty()) {
        arguments = QJsonObject();
        return true;
    }

    struct FieldBinding {
        QString name;
        QString type;
        bool required = false;
        QWidget* widget = nullptr;
        QJsonObject schema;
    };

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Run %1").arg(toolName));
    QFormLayout* layout = new QFormLayout(&dialog);
    QList<FieldBinding> bindings;

    for(auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        const QString propertyName = it.key();
        const QJsonObject propertySchema = it.value().toObject();
        const QString type = propertySchema.value("type").toString();
        const QString title = propertySchema.value("title").toString(propertyName);
        const QString description = propertySchema.value("description").toString().trimmed();

        bool isRequired = false;
        for(const QJsonValue& requiredValue : required) {
            if(requiredValue.toString() == propertyName) {
                isRequired = true;
                break;
            }
        }

        QWidget* fieldWidget = nullptr;
        if(type == "integer") {
            QSpinBox* spinBox = new QSpinBox(&dialog);
            spinBox->setRange(propertySchema.value("minimum").toInt(0),
                              propertySchema.value("maximum").toInt(std::numeric_limits<int>::max()));
            spinBox->setToolTip(description);
            spinBox->setValue(arguments.contains(propertyName)
                                  ? arguments.value(propertyName).toInt()
                                  : propertySchema.value("default").toInt(propertySchema.value("minimum").toInt(0)));
            fieldWidget = spinBox;
        } else if(type == "number") {
            QDoubleSpinBox* spinBox = new QDoubleSpinBox(&dialog);
            spinBox->setRange(propertySchema.value("minimum").toDouble(0.0),
                              propertySchema.value("maximum").toDouble(1000000.0));
            spinBox->setDecimals(2);
            spinBox->setSingleStep(0.25);
            spinBox->setToolTip(description);
            spinBox->setValue(arguments.contains(propertyName)
                                  ? arguments.value(propertyName).toDouble()
                                  : propertySchema.value("default").toDouble(propertySchema.value("minimum").toDouble(0.0)));
            fieldWidget = spinBox;
        } else if(type == "boolean") {
            QComboBox* comboBox = new QComboBox(&dialog);
            comboBox->addItem("false", false);
            comboBox->addItem("true", true);
            comboBox->setToolTip(description);
            comboBox->setCurrentIndex(arguments.value(propertyName).toBool(false) ? 1 : 0);
            fieldWidget = comboBox;
        } else {
            const QJsonArray enumValues = propertySchema.value("enum").toArray();
            if(!enumValues.isEmpty()) {
                QComboBox* comboBox = new QComboBox(&dialog);
                for(const QJsonValue& enumValue : enumValues) {
                    comboBox->addItem(enumValue.toString(), enumValue.toString());
                }
                comboBox->setToolTip(description);
                const QString currentValue = arguments.contains(propertyName)
                    ? arguments.value(propertyName).toString()
                    : propertySchema.value("default").toString();
                const int currentIndex = comboBox->findData(currentValue);
                if(currentIndex >= 0) {
                    comboBox->setCurrentIndex(currentIndex);
                }
                fieldWidget = comboBox;
            } else {
                QLineEdit* lineEdit = new QLineEdit(&dialog);
                lineEdit->setToolTip(description);
                lineEdit->setPlaceholderText(description);
                lineEdit->setText(arguments.contains(propertyName)
                                      ? arguments.value(propertyName).toString()
                                      : propertySchema.value("default").toString());
                fieldWidget = lineEdit;
            }
        }

        if(!fieldWidget) {
            continue;
        }

        const QString labelText = isRequired
            ? QString("%1 *").arg(title)
            : (description.isEmpty() ? title : QString("%1 (%2)").arg(title, description));
        layout->addRow(labelText, fieldWidget);
        bindings.append(FieldBinding{propertyName, type, isRequired, fieldWidget, propertySchema});
    }

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    QJsonObject editedArguments;
    for(const FieldBinding& binding : std::as_const(bindings)) {
        if(binding.type == "integer") {
            editedArguments.insert(binding.name, qobject_cast<QSpinBox*>(binding.widget)->value());
            continue;
        }

        if(binding.type == "number") {
            editedArguments.insert(binding.name, qobject_cast<QDoubleSpinBox*>(binding.widget)->value());
            continue;
        }

        if(binding.type == "boolean") {
            editedArguments.insert(binding.name, qobject_cast<QComboBox*>(binding.widget)->currentData().toBool());
            continue;
        }

        if(QComboBox* comboBox = qobject_cast<QComboBox*>(binding.widget)) {
            const QString value = comboBox->currentData().toString();
            if(binding.required || !value.isEmpty()) {
                editedArguments.insert(binding.name, value);
            }
            continue;
        }

        if(QLineEdit* lineEdit = qobject_cast<QLineEdit*>(binding.widget)) {
            const QString value = lineEdit->text().trimmed();
            if(binding.required || !value.isEmpty()) {
                editedArguments.insert(binding.name, value);
            }
        }
    }

    arguments = editedArguments;
    return true;
}

void MainWindow::rebuildSkillsExplorer()
{
    if(!m_skillsExplorer) {
        return;
    }

    m_skillsExplorer->clear();

    QTreeWidgetItem* localRoot = new QTreeWidgetItem(QStringList() << "Workbench Tools");
    const QJsonArray localTools = localToolDefinitions();
    for(const QJsonValue& value : localTools) {
        localRoot->addChild(makeToolTreeItem(value.toObject()));
    }
    localRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(localRoot);

    QTreeWidgetItem* kernelRoot = new QTreeWidgetItem(QStringList() << "Neuro-Kernel Tools");
    const QJsonArray kernelTools = kernelToolDefinitions();
    if(kernelTools.isEmpty()) {
        kernelRoot->addChild(new QTreeWidgetItem(QStringList() << "No tools discovered yet."));
    } else {
        for(const QJsonValue& value : kernelTools) {
            kernelRoot->addChild(makeToolTreeItem(value.toObject()));
        }
    }
    kernelRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(kernelRoot);

    QTreeWidgetItem* statusRoot = new QTreeWidgetItem(QStringList() << "Planner Context");
    statusRoot->addChild(new QTreeWidgetItem(QStringList() << m_llmPlanner.statusSummary()));
    statusRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(statusRoot);
}

void MainWindow::updateSelectedSkillTool(QTreeWidgetItem* item)
{
    m_selectedSkillToolName.clear();
    m_skillRunButton->setEnabled(false);

    if(!item) {
        m_skillDetailsView->setPlainText("Select a workbench or Neuro-Kernel tool to inspect it here.");
        return;
    }

    const QString toolName = item->data(0, Qt::UserRole).toString().trimmed();
    const QJsonObject toolDefinition = item->data(0, Qt::UserRole + 1).toJsonObject();
    if(toolName.isEmpty() || toolDefinition.isEmpty()) {
        m_skillDetailsView->setPlainText(item->text(0));
        return;
    }

    m_selectedSkillToolName = toolName;
    m_skillRunButton->setEnabled(true);

    const QJsonObject preview{
        {"tool", toolName},
        {"definition", toolDefinition},
        {"suggested_arguments", defaultArgumentsForTool(toolName)}
    };
    m_skillDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(preview).toJson(QJsonDocument::Indented)));
}

void MainWindow::runSelectedSkillTool()
{
    if(m_selectedSkillToolName.isEmpty()) {
        return;
    }

    QJsonObject selectedArguments = defaultArgumentsForTool(m_selectedSkillToolName);
    if(editArgumentsForTool(m_selectedSkillToolName, selectedArguments)) {
        const QString commandText = QString("tools.call %1 %2")
            .arg(m_selectedSkillToolName,
                 QString::fromUtf8(QJsonDocument(selectedArguments).toJson(QJsonDocument::Compact)));
        appendTerminalMessage(QString("$ %1").arg(commandText));
        sendToolCall(commandText);
        return;
    }

    const QString initialArguments = QString::fromUtf8(QJsonDocument(selectedArguments)
                                                           .toJson(QJsonDocument::Indented)).trimmed();
    bool accepted = false;
    const QString argumentsText = QInputDialog::getMultiLineText(this,
                                                                 "Run Tool",
                                                                 QString("JSON arguments for %1").arg(m_selectedSkillToolName),
                                                                 initialArguments,
                                                                 &accepted);
    if(!accepted) {
        return;
    }

    const QString trimmedArguments = argumentsText.trimmed();
    const QString commandText = trimmedArguments.isEmpty()
        ? QString("tools.call %1 {}").arg(m_selectedSkillToolName)
        : QString("tools.call %1 %2").arg(m_selectedSkillToolName, trimmedArguments);

    appendTerminalMessage(QString("$ %1").arg(commandText));
    sendToolCall(commandText);
}

void MainWindow::openAgentSettings()
{
    LlmSettingsDialog dialog(m_llmPlanner.configuration(), this);
    dialog.setTestScenario("go to the strongest EEG burst and then show me the top EEG channels there",
                           availableToolDefinitions(),
                           llmPlanningContext("planner settings test"));
    if(dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_llmPlanner.setConfiguration(dialog.configuration());
    persistAgentSettings();
    refreshAgentPlannerStatus();
    appendOutputMessage("Updated agent planner settings.");
    appendTerminalMessage("> Updated agent planner settings.");
}

void MainWindow::loadAgentSettings()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    LlmPlannerConfig config;
    config.mode = settings.value("agent/mode").toString();
    config.providerName = settings.value("agent/provider").toString();
    config.endpoint = settings.value("agent/endpoint").toString();
    config.apiKey = settings.value("agent/api_key").toString();
    config.model = settings.value("agent/model").toString();
    m_llmPlanner.setConfiguration(config);
}

void MainWindow::persistAgentSettings() const
{
    const LlmPlannerConfig config = m_llmPlanner.configuration();
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    settings.setValue("agent/mode", config.mode);
    settings.setValue("agent/provider", config.providerName);
    settings.setValue("agent/endpoint", config.endpoint);
    settings.setValue("agent/api_key", config.apiKey);
    settings.setValue("agent/model", config.model);
}

void MainWindow::refreshAgentPlannerStatus()
{
    m_agentChatDock->setPlannerStatus(m_llmPlanner.statusSummary());
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
