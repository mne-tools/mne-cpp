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

#include "analysisresultwidget.h"
#include "agentchatdockwidget.h"
#include "editortabbar.h"
#include "editortabwidget.h"
#include "extensionhostedviewwidget.h"
#include "llmsettingsdialog.h"
#include "spectrumplotwidget.h"

#include <extensionviewfactoryregistry.h>
#include <iextensionviewfactory.h>
#include <irawdataview.h>
#include <jsonrpcmessage.h>
#include <viewproviderregistry.h>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QAction>
#include <QDir>
#include <QDirIterator>
#include <QFile>
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
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStringList>

using namespace MNEANALYZESTUDIO;

namespace
{

const char* kKernelSocketName = "mne_analyze_studio.neuro_kernel";
const char* kExtensionSocketName = "mne_analyze_studio.extension_host";
const char* kWorkspaceArtifactRoot = "__mne_analyze_studio_analysis_root__";
const char* kWorkspaceArtifactEntry = "__mne_analyze_studio_analysis_entry__";

QString resolveStudioExtensionsDirectory()
{
    const QString relativePath = "src/applications/mne_analyze_studio/extensions";
    const QStringList seedDirectories{
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };

    for(const QString& seedDirectory : seedDirectories) {
        QDir searchDir(seedDirectory);
        for(int depth = 0; depth < 8; ++depth) {
            const QString candidate = searchDir.filePath(relativePath);
            QFileInfo candidateInfo(candidate);
            if(candidateInfo.exists() && candidateInfo.isDir()) {
                return candidateInfo.absoluteFilePath();
            }

            if(!searchDir.cdUp()) {
                break;
            }
        }
    }

    return QDir::current().filePath(relativePath);
}

bool copyDirectoryRecursively(const QString& sourcePath, const QString& targetPath)
{
    QDir sourceDir(sourcePath);
    if(!sourceDir.exists()) {
        return false;
    }

    QDir targetDir(targetPath);
    if(!targetDir.exists() && !QDir().mkpath(targetPath)) {
        return false;
    }

    const QFileInfoList entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for(const QFileInfo& entry : entries) {
        const QString sourceEntryPath = entry.absoluteFilePath();
        const QString targetEntryPath = QDir(targetPath).filePath(entry.fileName());
        if(entry.isDir()) {
            if(!copyDirectoryRecursively(sourceEntryPath, targetEntryPath)) {
                return false;
            }
        } else {
            QFile::remove(targetEntryPath);
            if(!QFile::copy(sourceEntryPath, targetEntryPath)) {
                return false;
            }
        }
    }

    return true;
}

QString artifactTypeLabel(const QString& toolName)
{
    if(toolName == "neurokernel.psd_summary") {
        return "PSD";
    }
    if(toolName == "neurokernel.channel_stats") {
        return "Channel Stats";
    }
    if(toolName == "neurokernel.raw_stats") {
        return "Raw Stats";
    }
    if(toolName == "neurokernel.find_peak_window") {
        return "Peak";
    }
    if(toolName.startsWith("view.raw.")) {
        return "View";
    }

    return "Result";
}

QStyle::StandardPixmap artifactIconType(const QString& toolName)
{
    if(toolName == "neurokernel.psd_summary") {
        return QStyle::SP_ComputerIcon;
    }
    if(toolName == "neurokernel.find_peak_window") {
        return QStyle::SP_ArrowRight;
    }
    if(toolName == "neurokernel.channel_stats" || toolName == "neurokernel.raw_stats") {
        return QStyle::SP_FileDialogDetailedView;
    }

    return QStyle::SP_FileDialogInfoView;
}

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

bool hasQtSignal(const QObject* object, const char* normalizedSignal)
{
    if(!object || !normalizedSignal) {
        return false;
    }

    return object->metaObject()->indexOfSignal(normalizedSignal) >= 0;
}

bool hasQtMethod(const QObject* object, const char* normalizedMethod)
{
    if(!object || !normalizedMethod) {
        return false;
    }

    return object->metaObject()->indexOfMethod(normalizedMethod) >= 0;
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
, m_extensionsButton(new QToolButton)
, m_workspaceExplorer(new QTreeWidget)
, m_skillsExplorer(new QTreeWidget)
, m_skillsPage(new QWidget(this))
, m_skillDetailsView(new QPlainTextEdit(this))
, m_skillRunButton(new QPushButton("Run Tool...", this))
, m_extensionsPage(new QWidget(this))
, m_extensionsExplorer(new QTreeWidget)
, m_extensionDetailsView(new QPlainTextEdit(this))
, m_extensionInstallButton(new QPushButton("Install...", this))
, m_extensionToggleButton(new QPushButton("Disable", this))
, m_extensionSettingsButton(new QPushButton("Open Settings Tab", this))
, m_leftSidebarStack(new QStackedWidget)
, m_mainSplitter(new QSplitter(Qt::Horizontal))
, m_centerSplitter(new QSplitter(Qt::Vertical))
, m_centerTabs(new EditorTabWidget)
, m_centerTabBar(new EditorTabBar(this))
, m_bottomPanelTabs(new QTabWidget)
, m_outputPanel(new QListWidget)
, m_problemPanel(new QListWidget)
, m_resultsTab(new QWidget(this))
, m_resultsTitleLabel(new QLabel("Run a tool or open an Analysis artifact to populate Recent Results."))
, m_resultsHistoryCombo(new QComboBox(this))
, m_resultsPsdControlsWidget(new QWidget(this))
, m_resultsPsdMatchCombo(new QComboBox(this))
, m_resultsPsdNfftSpin(new QSpinBox(this))
, m_resultsPsdCompareCombo(new QComboBox(this))
, m_resultsPsdRunButton(new QPushButton("Update PSD", this))
, m_resultsActionButton(new QPushButton("Run Action", this))
, m_resultsStack(new QStackedWidget(this))
, m_resultsTree(new QTreeWidget(this))
, m_resultsTable(new QTableWidget(this))
, m_resultsSpectrumPlot(new SpectrumPlotWidget(this))
, m_terminalTab(new QWidget(this))
, m_terminalStatusLabel(new QLabel("Active view state: idle"))
, m_terminalPanel(new QTextEdit)
, m_terminalInput(new QLineEdit)
, m_terminalRunButton(new QPushButton("Run"))
, m_activeStateItem(new QListWidgetItem("Active view state: idle"))
, m_kernelSocket(new QLocalSocket(this))
, m_extensionSocket(new QLocalSocket(this))
, m_llmPlanner(this)
, m_sceneRegistry(this)
, m_viewProviderRegistry(new ViewProviderRegistry(this))
, m_viewManager(&m_sceneRegistry, m_viewProviderRegistry, this)
{
    setWindowTitle("MNE Analyze Studio");
    resize(1440, 900);

    reloadExtensionRegistry();

    createLayout();
    createConnections();
    loadAgentSettings();
    restoreWorkspace();
    applyWorkbenchStyle();
    refreshAgentPlannerStatus();

    connect(m_kernelSocket, &QLocalSocket::connected, this, &MainWindow::requestKernelToolDefinitions);
    connect(m_extensionSocket, &QLocalSocket::connected, this, &MainWindow::requestExtensionHostState);
    m_kernelSocket->connectToServer(kKernelSocketName);
    m_extensionSocket->connectToServer(kExtensionSocketName);
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
    m_extensionsButton->setText("Ext");
    m_explorerButton->setToolTip("Explorer");
    m_skillsButton->setToolTip("Tools and skills");
    m_extensionsButton->setToolTip("Extensions");
    m_explorerButton->setCheckable(true);
    m_skillsButton->setCheckable(true);
    m_extensionsButton->setCheckable(true);
    m_explorerButton->setChecked(true);
    activityLayout->addWidget(m_explorerButton);
    activityLayout->addWidget(m_skillsButton);
    activityLayout->addWidget(m_extensionsButton);
    activityLayout->addStretch(1);

    m_workspaceExplorer->setHeaderLabel("Workspace");
    m_workspaceExplorer->setAlternatingRowColors(true);
    m_workspaceExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_skillsExplorer->setHeaderLabel("Tools");
    m_skillsExplorer->setAlternatingRowColors(true);
    m_skillsExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_skillDetailsView->setReadOnly(true);
    m_skillDetailsView->setPlaceholderText("Select a workbench or Neuro-Kernel tool to inspect it here.");
    m_skillRunButton->setEnabled(false);
    m_extensionsExplorer->setHeaderLabel("Extensions");
    m_extensionsExplorer->setAlternatingRowColors(true);
    m_extensionDetailsView->setReadOnly(true);
    m_extensionDetailsView->setPlaceholderText("Select an extension to inspect its manifest, tools, view providers, and settings tabs.");
    m_extensionToggleButton->setEnabled(false);
    m_extensionSettingsButton->setEnabled(false);

    QVBoxLayout* skillsPageLayout = new QVBoxLayout(m_skillsPage);
    skillsPageLayout->setContentsMargins(0, 0, 0, 0);
    skillsPageLayout->setSpacing(8);
    skillsPageLayout->addWidget(m_skillsExplorer, 2);
    skillsPageLayout->addWidget(new QLabel("Tool Inspector", m_skillsPage));
    skillsPageLayout->addWidget(m_skillDetailsView, 1);
    skillsPageLayout->addWidget(m_skillRunButton);
    rebuildSkillsExplorer();

    QVBoxLayout* extensionsPageLayout = new QVBoxLayout(m_extensionsPage);
    extensionsPageLayout->setContentsMargins(0, 0, 0, 0);
    extensionsPageLayout->setSpacing(8);
    extensionsPageLayout->addWidget(m_extensionsExplorer, 2);
    extensionsPageLayout->addWidget(new QLabel("Extension Inspector", m_extensionsPage));
    extensionsPageLayout->addWidget(m_extensionDetailsView, 1);
    QHBoxLayout* extensionButtonsLayout = new QHBoxLayout;
    extensionButtonsLayout->setContentsMargins(0, 0, 0, 0);
    extensionButtonsLayout->setSpacing(8);
    extensionButtonsLayout->addWidget(m_extensionInstallButton);
    extensionButtonsLayout->addWidget(m_extensionToggleButton);
    extensionButtonsLayout->addWidget(m_extensionSettingsButton);
    extensionsPageLayout->addLayout(extensionButtonsLayout);
    refreshExtensionManagerUi();

    m_leftSidebarStack->addWidget(m_workspaceExplorer);
    m_leftSidebarStack->addWidget(m_skillsPage);
    m_leftSidebarStack->addWidget(m_extensionsPage);

    leftSidebarLayout->addWidget(m_activityBar);
    leftSidebarLayout->addWidget(m_leftSidebarStack, 1);

    QWidget* leftPanel = createSidebarSection("Primary Sidebar", leftSidebar);

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
    m_resultsHistoryCombo->addItem("Latest Result");
    m_resultsHistoryCombo->setToolTip("Recent Results: reopen a recently computed structured result.");
    m_resultsHistoryCombo->setMinimumWidth(320);
    m_resultsPsdControlsWidget->setVisible(false);
    m_resultsPsdMatchCombo->addItems(QStringList() << "EEG" << "MEG" << "EOG" << "All");
    m_resultsPsdMatchCombo->setToolTip("Channel family used for the PSD summary.");
    m_resultsPsdNfftSpin->setRange(32, 8192);
    m_resultsPsdNfftSpin->setSingleStep(32);
    m_resultsPsdNfftSpin->setValue(256);
    m_resultsPsdNfftSpin->setToolTip("FFT length used for the Welch PSD estimate.");
    m_resultsPsdCompareCombo->addItem("No comparison");
    m_resultsPsdCompareCombo->setToolTip("Overlay one previous PSD result on top of the current run.");
    QHBoxLayout* resultsPsdControlsLayout = new QHBoxLayout(m_resultsPsdControlsWidget);
    resultsPsdControlsLayout->setContentsMargins(0, 0, 0, 0);
    resultsPsdControlsLayout->setSpacing(8);
    resultsPsdControlsLayout->addWidget(new QLabel("PSD Match", m_resultsPsdControlsWidget));
    resultsPsdControlsLayout->addWidget(m_resultsPsdMatchCombo);
    resultsPsdControlsLayout->addWidget(new QLabel("FFT", m_resultsPsdControlsWidget));
    resultsPsdControlsLayout->addWidget(m_resultsPsdNfftSpin);
    resultsPsdControlsLayout->addWidget(new QLabel("Compare", m_resultsPsdControlsWidget));
    resultsPsdControlsLayout->addWidget(m_resultsPsdCompareCombo, 1);
    resultsPsdControlsLayout->addWidget(m_resultsPsdRunButton);
    resultsPsdControlsLayout->addStretch(1);
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
    m_resultsStack->addWidget(m_resultsSpectrumPlot);

    QVBoxLayout* resultsLayout = new QVBoxLayout(m_resultsTab);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    resultsLayout->setSpacing(8);
    QHBoxLayout* resultsHistoryLayout = new QHBoxLayout;
    resultsHistoryLayout->setContentsMargins(0, 0, 0, 0);
    resultsHistoryLayout->setSpacing(8);
    QLabel* resultsHistoryLabel = new QLabel("Recent Results", m_resultsTab);
    resultsHistoryLabel->setObjectName("terminalStatusLabel");
    resultsHistoryLayout->addWidget(resultsHistoryLabel);
    resultsHistoryLayout->addWidget(m_resultsHistoryCombo, 1);
    resultsLayout->addLayout(resultsHistoryLayout);
    resultsLayout->addWidget(m_resultsTitleLabel);
    resultsLayout->addWidget(m_resultsPsdControlsWidget);
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
    connect(m_extensionsButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("extensions");
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
        connect(refreshToolsAction, &QAction::triggered, this, [this]() {
            requestKernelToolDefinitions();
            requestExtensionHostState();
        });

        if(!m_selectedSkillToolName.isEmpty()) {
            QAction* runToolAction = contextMenu.addAction("Run Tool...");
            connect(runToolAction, &QAction::triggered, this, &MainWindow::runSelectedSkillTool);
        }

        contextMenu.exec(m_skillsExplorer->viewport()->mapToGlobal(position));
    });
    connect(m_skillRunButton, &QPushButton::clicked, this, &MainWindow::runSelectedSkillTool);
    connect(m_extensionsExplorer, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) {
        updateSelectedExtension(current);
    });
    connect(m_extensionsExplorer, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*) {
        openSelectedExtensionSettingsTab();
    });
    connect(m_extensionInstallButton, &QPushButton::clicked, this, &MainWindow::installExtensionFromDirectory);
    connect(m_extensionToggleButton, &QPushButton::clicked, this, &MainWindow::toggleSelectedExtensionEnabled);
    connect(m_extensionSettingsButton, &QPushButton::clicked, this, &MainWindow::openSelectedExtensionSettingsTab);
    connect(m_resultsActionButton, &QPushButton::clicked, this, &MainWindow::runResultPrimaryAction);
    connect(m_resultsPsdRunButton, &QPushButton::clicked, this, &MainWindow::runResultPsdAction);
    connect(m_resultsHistoryCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if(index <= 0 || index - 1 >= m_structuredResultHistory.size()) {
                    return;
                }

                const QJsonObject entry = m_structuredResultHistory.at(index - 1);
                updateStructuredResultView(entry.value("tool_name").toString(), entry.value("result").toObject());
            });
    connect(m_resultsPsdCompareCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) { updateResultPsdComparison(); });
    connect(m_resultsTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateResultPrimaryActionForSelection);
    connect(m_workspaceExplorer, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        openWorkspaceItem(item);
    });
    connect(m_workspaceExplorer, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        QTreeWidgetItem* item = m_workspaceExplorer->itemAt(position);
        if(!item) {
            return;
        }

        m_workspaceExplorer->setCurrentItem(item);

        QMenu contextMenu(this);
        const QString specialType = item->data(0, Qt::UserRole).toString();
        if(specialType == QLatin1String(kWorkspaceArtifactEntry)) {
            const QJsonObject entry = item->data(0, Qt::UserRole + 1).toJsonObject();
            const QString toolName = entry.value("tool_name").toString();
            const QJsonObject result = entry.value("result").toObject();

            QAction* reopenAction = contextMenu.addAction("Open Result");
            connect(reopenAction, &QAction::triggered, this, [this, item]() {
                openWorkspaceItem(item);
            });

            if(toolName == "neurokernel.find_peak_window" && result.value("peak_sample").toInt(-1) >= 0) {
                QAction* jumpToPeakAction = contextMenu.addAction("Jump To Peak In Active View");
                connect(jumpToPeakAction, &QAction::triggered, this, [this, result]() {
                    const int peakSample = result.value("peak_sample").toInt(-1);
                    if(peakSample < 0) {
                        return;
                    }
                    const QString commandText = QString("tools.call view.raw.goto {\"sample\":%1}").arg(peakSample);
                    appendTerminalMessage(QString("$ %1").arg(commandText));
                    sendToolCall(commandText);
                });
            }

            if(toolName == "neurokernel.psd_summary") {
                QAction* overlayAction = contextMenu.addAction("Open Result And Enable PSD Compare");
                connect(overlayAction, &QAction::triggered, this, [this, item, entry]() {
                    openAnalysisArtifact(entry, true);
                    const int compareIndex = m_resultsPsdCompareCombo->findData(entry);
                    if(compareIndex >= 0) {
                        m_resultsPsdCompareCombo->setCurrentIndex(compareIndex);
                    }
                });
            }
        } else {
            const QString filePath = item->data(0, Qt::UserRole).toString();
            if(!QFileInfo(filePath).isFile()) {
                return;
            }

            QAction* openInEditorAction = contextMenu.addAction("Open in Editor");
            openInEditorAction->setEnabled(isSupportedWorkspaceFile(filePath));
            connect(openInEditorAction, &QAction::triggered, this, [this, item]() {
                openWorkspaceItem(item);
            });
        }
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
    connect(m_extensionSocket, &QLocalSocket::readyRead, this, [this]() {
        while(m_extensionSocket->canReadLine()) {
            const QByteArray payload = m_extensionSocket->readLine();
            QJsonObject response;
            QString errorString;
            if(JsonRpcMessage::deserialize(payload, response, errorString)) {
                if(response.contains("error")) {
                    const QString message = response.value("error").toObject().value("message").toString();
                    m_agentChatDock->appendTranscript(QString("Extension Host> %1").arg(message));
                    appendProblemMessage(QString("Extension host error: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                } else {
                    const QString requestId = response.value("id").toString();
                    const QJsonObject result = response.value("result").toObject();
                    const QString toolName = result.value("tool_name").toString();
                    if(toolName == "views/open" && m_pendingExtensionViewOpens.contains(requestId)) {
                        const QJsonObject dispatch = m_pendingExtensionViewOpens.take(requestId);
                        const QString filePath = m_pendingExtensionViewFiles.take(requestId);
                        QJsonObject descriptor = result;
                        for(auto it = dispatch.constBegin(); it != dispatch.constEnd(); ++it) {
                            if(!descriptor.contains(it.key())) {
                                descriptor.insert(it.key(), it.value());
                            }
                        }
                        finalizeExtensionViewOpen(filePath, descriptor);
                        requestExtensionHostState();
                        continue;
                    }
                    if(toolName == "tools/list" && result.value("tools").isArray()) {
                        QJsonArray filteredTools;
                        for(const QJsonValue& value : result.value("tools").toArray()) {
                            const QJsonObject tool = value.toObject();
                            if(m_viewProviderRegistry->isExtensionEnabled(tool.value("extension_id").toString())) {
                                filteredTools.append(tool);
                            }
                        }
                        m_cachedExtensionToolDefinitions = filteredTools;
                        rebuildSkillsExplorer();
                        statusBar()->showMessage(QString("Discovered %1 extension-host tools.")
                                                     .arg(m_cachedExtensionToolDefinitions.size()),
                                                 4000);
                        refreshExtensionManagerUi();
                        continue;
                    }
                    if(toolName == "resources/list" && result.value("resources").isArray()) {
                        QJsonArray filteredResources;
                        for(const QJsonValue& value : result.value("resources").toArray()) {
                            const QJsonObject resource = value.toObject();
                            if(m_viewProviderRegistry->isExtensionEnabled(resource.value("id").toString())) {
                                filteredResources.append(resource);
                            }
                        }
                        m_cachedExtensionResources = filteredResources;
                        rebuildSkillsExplorer();
                        statusBar()->showMessage(QString("Discovered %1 extension manifests.")
                                                     .arg(m_cachedExtensionResources.size()),
                                                 4000);
                        refreshExtensionManagerUi();
                        continue;
                    }
                    if(toolName == "extensions/reload") {
                        m_lastExtensionReloadResult = result;
                        appendOutputMessage(QString("Extension Host: %1").arg(result.value("message").toString()));
                        appendTerminalMessage(QString("> %1").arg(result.value("message").toString()));
                        statusBar()->showMessage(result.value("message").toString(), 5000);
                        refreshExtensionManagerUi();
                        requestExtensionHostState();
                        continue;
                    }
                    if(toolName == "views/list" && result.value("sessions").isArray()) {
                        QJsonArray filteredSessions;
                        for(const QJsonValue& value : result.value("sessions").toArray()) {
                            const QJsonObject session = value.toObject();
                            if(m_viewProviderRegistry->isExtensionEnabled(session.value("extension_id").toString())) {
                                filteredSessions.append(session);
                            }
                        }
                        m_cachedExtensionViewSessions = filteredSessions;
                        rebuildSkillsExplorer();
                        statusBar()->showMessage(QString("Extension host sessions: %1")
                                                     .arg(m_cachedExtensionViewSessions.size()),
                                                 4000);
                        refreshExtensionManagerUi();
                        continue;
                    }
                    if(toolName == "views/command") {
                        const QString sessionId = result.value("session_id").toString();
                        if(QWidget* widget = m_extensionViewWidgetsBySessionId.value(sessionId, nullptr)) {
                            widget->setProperty("mne_session_descriptor", result);
                            QMetaObject::invokeMethod(widget,
                                                      "applySessionUpdate",
                                                      Qt::DirectConnection,
                                                      Q_ARG(QJsonObject, result));
                        }
                        const QString commandName = result.value("command").toString().trimmed();
                        if(!commandName.isEmpty()) {
                            rememberToolResult(QString("view.hosted.%1").arg(commandName), result);
                        }
                        requestExtensionHostState();
                    }
                    const QString message = result.value("message").toString(QJsonDocument(result).toJson(QJsonDocument::Compact));
                    m_agentChatDock->appendTranscript(QString("Extension Host> %1").arg(message));
                    appendOutputMessage(QString("Extension Host: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                }
            } else {
                const QString message = QString::fromUtf8(payload).trimmed();
                m_agentChatDock->appendTranscript(QString("Extension Host> %1").arg(message));
                appendOutputMessage(QString("Extension Host: %1").arg(message));
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
    rootItem->setData(0, Qt::UserRole + 1, "directory");
    rootItem->setExpanded(true);

    auto ensureChildItem = [](QTreeWidgetItem* parentItem,
                              const QString& name,
                              const QString& absolutePath,
                              bool isDirectory) -> QTreeWidgetItem* {
        if(!parentItem) {
            return nullptr;
        }

        for(int childIndex = 0; childIndex < parentItem->childCount(); ++childIndex) {
            QTreeWidgetItem* candidate = parentItem->child(childIndex);
            if(candidate->text(0) == name
               && candidate->data(0, Qt::UserRole).toString() == absolutePath) {
                return candidate;
            }
        }

        QTreeWidgetItem* childItem = new QTreeWidgetItem(QStringList() << name);
        childItem->setData(0, Qt::UserRole, absolutePath);
        childItem->setData(0, Qt::UserRole + 1, isDirectory ? "directory" : "file");
        parentItem->addChild(childItem);
        return childItem;
    };

    QDirIterator it(directoryPath,
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while(it.hasNext()) {
        const QString path = it.next();
        if(!isSupportedWorkspaceFile(path)) {
            continue;
        }

        const QString relativePath = QDir(directoryPath).relativeFilePath(path);
        const QStringList segments = relativePath.split('/', Qt::SkipEmptyParts);
        QTreeWidgetItem* parentItem = rootItem;
        QString accumulatedPath = directoryPath;

        for(int segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex) {
            const QString& segment = segments.at(segmentIndex);
            accumulatedPath = QDir(accumulatedPath).filePath(segment);
            const bool isLeafFile = segmentIndex == segments.size() - 1;
            parentItem = ensureChildItem(parentItem, segment, accumulatedPath, !isLeafFile);
            if(!parentItem) {
                break;
            }
        }

        if(!parentItem) {
            continue;
        }

        parentItem->setToolTip(0, path);
    }

    m_workspaceExplorer->addTopLevelItem(rootItem);
    appendOutputMessage(QString("Workspace added: %1").arg(directoryPath));
}

void MainWindow::refreshWorkspaceArtifacts()
{
    if(!m_workspaceExplorer) {
        return;
    }

    for(int i = m_workspaceExplorer->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem* item = m_workspaceExplorer->topLevelItem(i);
        if(item && item->data(0, Qt::UserRole).toString() == QLatin1String(kWorkspaceArtifactRoot)) {
            delete m_workspaceExplorer->takeTopLevelItem(i);
        }
    }

    if(m_structuredResultHistory.isEmpty()) {
        return;
    }

    QTreeWidgetItem* analysisRoot = new QTreeWidgetItem(QStringList() << "Analysis");
    analysisRoot->setData(0, Qt::UserRole, QString::fromLatin1(kWorkspaceArtifactRoot));
    analysisRoot->setExpanded(true);

    for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject entry = m_structuredResultHistory.at(i);
        const QString toolName = entry.value("tool_name").toString().trimmed();
        const QJsonObject result = entry.value("result").toObject();
        const QString fileName = QFileInfo(result.value("file").toString()).fileName();
        const QString message = result.value("message").toString().trimmed();

        QString label = artifactTypeLabel(toolName);
        if(!fileName.isEmpty()) {
            label += QString(" | %1").arg(fileName);
        }
        if(!message.isEmpty()) {
            label += QString(" | %1").arg(message.left(80));
        }

        QTreeWidgetItem* artifactItem = new QTreeWidgetItem(QStringList() << label);
        artifactItem->setToolTip(0, message);
        artifactItem->setIcon(0, style()->standardIcon(artifactIconType(toolName)));
        artifactItem->setData(0, Qt::UserRole, QString::fromLatin1(kWorkspaceArtifactEntry));
        artifactItem->setData(0, Qt::UserRole + 1, entry);
        analysisRoot->addChild(artifactItem);
    }

    m_workspaceExplorer->insertTopLevelItem(0, analysisRoot);
}

void MainWindow::openAnalysisArtifact(const QJsonObject& entry, bool focusBottomResults)
{
    const QString toolName = entry.value("tool_name").toString();
    const QJsonObject result = entry.value("result").toObject();
    if(toolName.isEmpty() || result.isEmpty()) {
        return;
    }

    const QString artifactKey = QString("analysis:%1")
                                    .arg(QString::fromUtf8(QJsonDocument(entry).toJson(QJsonDocument::Compact)));
    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(m_centerTabs->tabToolTip(i) == artifactKey) {
            m_centerTabs->setCurrentIndex(i);
            if(focusBottomResults) {
                m_lastToolName = toolName;
                m_lastToolResult = result;
                updateStructuredResultView(toolName, result);
                m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
            }
            return;
        }
    }

    AnalysisResultWidget* resultWidget = new AnalysisResultWidget;
    resultWidget->setResult(toolName, result);
    const QString fileName = QFileInfo(result.value("file").toString()).fileName();
    QString tabLabel = artifactTypeLabel(toolName);
    if(!fileName.isEmpty()) {
        tabLabel += QString(": %1").arg(fileName);
    }

    const int tabIndex = m_centerTabs->addTab(resultWidget, tabLabel);
    m_centerTabs->setTabToolTip(tabIndex, artifactKey);
    m_centerTabs->setCurrentIndex(tabIndex);

    if(focusBottomResults) {
        m_lastToolName = toolName;
        m_lastToolResult = result;
        updateStructuredResultView(toolName, result);
        m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
    }
}

void MainWindow::openWorkspaceItem(QTreeWidgetItem* item)
{
    if(!item) {
        return;
    }

    const QString specialType = item->data(0, Qt::UserRole).toString();
    if(specialType == QLatin1String(kWorkspaceArtifactEntry)) {
        const QJsonObject entry = item->data(0, Qt::UserRole + 1).toJsonObject();
        openAnalysisArtifact(entry, true);
        return;
    }

    const QString filePath = item->data(0, Qt::UserRole).toString();
    if(QFileInfo(filePath).isFile()) {
        if(!isSupportedWorkspaceFile(filePath)) {
            appendProblemMessage(QString("No registered view provider is currently available for %1.").arg(filePath));
            statusBar()->showMessage(QString("Unsupported file type: %1").arg(QFileInfo(filePath).fileName()), 4000);
            return;
        }
        openFileInView(filePath);
    }
}

void MainWindow::openFileInView(const QString& filePath)
{
    const QJsonObject dispatch = m_viewManager.dispatchFileSelection(filePath);
    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(IRawDataView* rawDataView = dynamic_cast<IRawDataView*>(m_centerTabs->widget(i))) {
            if(rawDataView->filePath() == filePath) {
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
    if(dispatch.value("view").toString() == "ExtensionView") {
        requestExtensionViewOpen(filePath, dispatch);
        statusBar()->showMessage(QString("Opening hosted extension view for %1...")
                                     .arg(QFileInfo(filePath).fileName()),
                                 4000);
        appendOutputMessage(QString("Requested hosted extension view for %1 via %2")
                                .arg(filePath, dispatch.value("provider_id").toString()));
        appendTerminalMessage(QString("$ extension view open %1").arg(filePath));
        return;
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
    const bool skillsSelected = sectionName == "skills";
    const bool extensionsSelected = sectionName == "extensions";
    m_explorerButton->setChecked(workspaceSelected);
    m_skillsButton->setChecked(skillsSelected);
    m_extensionsButton->setChecked(extensionsSelected);

    if(workspaceSelected) {
        m_leftSidebarStack->setCurrentWidget(m_workspaceExplorer);
    } else if(skillsSelected) {
        m_leftSidebarStack->setCurrentWidget(m_skillsPage);
    } else {
        m_leftSidebarStack->setCurrentWidget(m_extensionsPage);
    }

    if(workspaceSelected) {
        statusBar()->showMessage("Workspace explorer active");
    } else if(skillsSelected) {
        statusBar()->showMessage("Skill explorer active");
    } else {
        statusBar()->showMessage("Extension manager active");
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
    if(ExtensionHostedViewWidget* hostedView = qobject_cast<ExtensionHostedViewWidget*>(widget)) {
        m_extensionViewWidgetsBySessionId.remove(hostedView->sessionId());
    }
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
        IRawDataView* rawBrowser = activeRawDataView();
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

    if(toolName == "neurokernel.psd_summary" && !result.isEmpty()) {
        bool alreadyTracked = false;
        for(int i = m_psdResultHistory.size() - 1; i >= 0; --i) {
            if(m_psdResultHistory.at(i) == result) {
                alreadyTracked = true;
                break;
            }
        }

        if(!alreadyTracked) {
            m_psdResultHistory.append(result);
            while(m_psdResultHistory.size() > 8) {
                m_psdResultHistory.removeFirst();
            }
        }
    }

    if(!result.isEmpty()) {
        const QJsonObject historyEntry{
            {"tool_name", toolName},
            {"result", result}
        };

        bool alreadyTracked = false;
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            if(m_structuredResultHistory.at(i) == historyEntry) {
                alreadyTracked = true;
                break;
            }
        }

        if(!alreadyTracked) {
            m_structuredResultHistory.append(historyEntry);
            while(m_structuredResultHistory.size() > 20) {
                m_structuredResultHistory.removeFirst();
            }
        }
    }

    refreshStructuredResultHistoryUi();
    updateStructuredResultView(toolName, result);
}

void MainWindow::refreshStructuredResultHistoryUi()
{
    if(!m_resultsHistoryCombo) {
        return;
    }

    const QSignalBlocker blocker(m_resultsHistoryCombo);
    m_resultsHistoryCombo->clear();
    m_resultsHistoryCombo->addItem("Latest Result");

    for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject entry = m_structuredResultHistory.at(i);
        const QString toolName = entry.value("tool_name").toString();
        const QJsonObject result = entry.value("result").toObject();
        const QString message = result.value("message").toString().trimmed();
        const QString fileName = QFileInfo(result.value("file").toString()).fileName();

        QString label = toolName;
        if(!fileName.isEmpty()) {
            label += QString(" | %1").arg(fileName);
        }
        if(!message.isEmpty()) {
            label += QString(" | %1").arg(message);
        }

        m_resultsHistoryCombo->addItem(label.left(180), entry);
    }

    m_resultsHistoryCombo->setCurrentIndex(0);
    refreshWorkspaceArtifacts();
}

void MainWindow::updateStructuredResultView(const QString& toolName, const QJsonObject& result)
{
    if(!m_resultsTree || !m_resultsTitleLabel || !m_resultsStack || !m_resultsTable || !m_resultsActionButton) {
        return;
    }

    m_resultsTitleLabel->setText(QString("Latest structured result: %1").arg(toolName));
    m_resultsCurrentToolName = toolName;
    m_resultPrimaryActionCommand.clear();
    m_resultsPsdControlsWidget->setVisible(false);
    m_resultsActionButton->setVisible(false);
    m_resultsActionButton->setEnabled(false);
    m_resultsTree->clear();
    m_resultsTable->setRowCount(0);
    m_resultsSpectrumPlot->clear();
    m_resultsStack->setCurrentWidget(m_resultsTree);

    if(result.isEmpty()) {
        m_resultsTree->addTopLevelItem(new QTreeWidgetItem(QStringList()
                                                           << "hint"
                                                           << "Run a tool or open an Analysis artifact to populate Recent Results."));
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

    if(toolName == "neurokernel.psd_summary"
       && result.value("frequencies").isArray()
       && result.value("psd").isArray()) {
        updateResultPsdControls(result);
        QVector<double> frequencies;
        QVector<double> values;
        const QJsonArray freqArray = result.value("frequencies").toArray();
        const QJsonArray valueArray = result.value("psd").toArray();
        const int pointCount = std::min(freqArray.size(), valueArray.size());
        frequencies.reserve(pointCount);
        values.reserve(pointCount);
        for(int i = 0; i < pointCount; ++i) {
            frequencies.append(freqArray.at(i).toDouble());
            values.append(valueArray.at(i).toDouble());
        }

        m_resultsSpectrumPlot->setSpectrum(frequencies,
                                           values,
                                           result.value("message").toString(QString("Spectrum summary for %1").arg(toolName)));
        updateResultPsdComparison();
        m_resultsStack->setCurrentWidget(m_resultsSpectrumPlot);
        m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
        return;
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

void MainWindow::updateResultPsdControls(const QJsonObject& result)
{
    if(!m_resultsPsdControlsWidget || !m_resultsPsdMatchCombo || !m_resultsPsdNfftSpin || !m_resultsPsdRunButton) {
        return;
    }

    const QString match = result.value("match").toString().trimmed().toUpper();
    const int matchIndex = m_resultsPsdMatchCombo->findText(match.isEmpty() ? "All" : match, Qt::MatchFixedString);
    m_resultsPsdMatchCombo->setCurrentIndex(matchIndex >= 0 ? matchIndex : m_resultsPsdMatchCombo->findText("All"));

    const int nfft = result.value("nfft").toInt(256);
    m_resultsPsdNfftSpin->setValue(std::max(m_resultsPsdNfftSpin->minimum(),
                                            std::min(m_resultsPsdNfftSpin->maximum(), nfft)));
    const QSignalBlocker blocker(m_resultsPsdCompareCombo);
    m_resultsPsdCompareCombo->clear();
    m_resultsPsdCompareCombo->addItem("No comparison");
    for(int i = m_psdResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject entry = m_psdResultHistory.at(i);
        if(entry == result) {
            continue;
        }

        const QString label = QString("%1 | %2 | nfft %3 | %4 ch")
                                  .arg(entry.value("match").toString().trimmed().isEmpty() ? QString("All") : entry.value("match").toString().trimmed(),
                                       QFileInfo(entry.value("file").toString()).fileName())
                                  .arg(entry.value("nfft").toInt(0))
                                  .arg(entry.value("channel_count").toInt(0));
        m_resultsPsdCompareCombo->addItem(label, entry);
    }
    m_resultsPsdControlsWidget->setVisible(true);
    m_resultsPsdRunButton->setEnabled(activeRawDataView() != nullptr);
}

void MainWindow::updateResultPsdComparison()
{
    if(!m_resultsSpectrumPlot || m_resultsCurrentToolName != "neurokernel.psd_summary") {
        return;
    }

    if(!m_resultsPsdCompareCombo || m_resultsPsdCompareCombo->currentIndex() <= 0) {
        m_resultsSpectrumPlot->clearComparisonSpectrum();
        return;
    }

    const QJsonObject comparison = m_resultsPsdCompareCombo->currentData().toJsonObject();
    if(!comparison.value("frequencies").isArray() || !comparison.value("psd").isArray()) {
        m_resultsSpectrumPlot->clearComparisonSpectrum();
        return;
    }

    QVector<double> frequencies;
    QVector<double> values;
    const QJsonArray freqArray = comparison.value("frequencies").toArray();
    const QJsonArray valueArray = comparison.value("psd").toArray();
    const int pointCount = std::min(freqArray.size(), valueArray.size());
    frequencies.reserve(pointCount);
    values.reserve(pointCount);
    for(int i = 0; i < pointCount; ++i) {
        frequencies.append(freqArray.at(i).toDouble());
        values.append(valueArray.at(i).toDouble());
    }

    const QString label = QString("%1 | nfft %2")
                              .arg(comparison.value("match").toString().trimmed().isEmpty()
                                       ? QString("All")
                                       : comparison.value("match").toString().trimmed())
                              .arg(comparison.value("nfft").toInt(0));
    m_resultsSpectrumPlot->setComparisonSpectrum(frequencies, values, label);
}

void MainWindow::runResultPrimaryAction()
{
    if(m_resultPrimaryActionCommand.isEmpty()) {
        return;
    }

    appendTerminalMessage(QString("$ %1").arg(m_resultPrimaryActionCommand));
    sendToolCall(m_resultPrimaryActionCommand);
}

void MainWindow::runResultPsdAction()
{
    if(m_resultsCurrentToolName != "neurokernel.psd_summary") {
        return;
    }

    IRawDataView* rawBrowser = activeRawDataView();
    if(!rawBrowser) {
        appendProblemMessage("PSD rerun requested without an active raw browser.");
        return;
    }

    const int fromSample = m_lastToolResult.value("from_sample").toInt(rawBrowser->cursorSample());
    const int toSample = m_lastToolResult.value("to_sample").toInt(fromSample);
    const int windowSamples = std::max(32, toSample - fromSample);
    QJsonObject arguments = buildRawWindowArguments(windowSamples);
    arguments.insert("nfft", m_resultsPsdNfftSpin->value());

    const QString selectedMatch = m_resultsPsdMatchCombo->currentText().trimmed();
    arguments.insert("match", selectedMatch.compare("All", Qt::CaseInsensitive) == 0 ? QString() : selectedMatch);

    appendTerminalMessage(QString("$ tools.call neurokernel.psd_summary {\"window_samples\":%1,\"nfft\":%2,\"match\":\"%3\"}")
                              .arg(windowSamples)
                              .arg(m_resultsPsdNfftSpin->value())
                              .arg(arguments.value("match").toString()));
    sendKernelToolCall("neurokernel.psd_summary", arguments);
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

    if(containsAny({"left hemisphere", "focus left", "rotate left"})) {
        planned = true;
        plannedCommand = "tools.call view.hosted.focus_left {}";
        return "Mapped your request to `view.hosted.focus_left`.";
    }

    if(containsAny({"right hemisphere", "focus right", "rotate right"})) {
        planned = true;
        plannedCommand = "tools.call view.hosted.focus_right {}";
        return "Mapped your request to `view.hosted.focus_right`.";
    }

    if(containsAny({"reset 3d", "reset view", "reset scene", "home view"})) {
        planned = true;
        plannedCommand = "tools.call view.hosted.reset_view {}";
        return "Mapped your request to `view.hosted.reset_view`.";
    }

    QRegularExpression opacityPattern("(?:opacity|set opacity to)\\s+(\\d+(?:\\.\\d+)?)");
    QRegularExpressionMatch opacityMatch = opacityPattern.match(lower);
    if(opacityMatch.hasMatch()) {
        planned = true;
        plannedCommand = QString("tools.call view.hosted.set_opacity {\"opacity\":%1}").arg(opacityMatch.captured(1));
        return QString("Mapped your request to `view.hosted.set_opacity` with opacity %1.").arg(opacityMatch.captured(1));
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

    if(containsAny({"psd", "power spectrum", "spectrum", "spectral density"})) {
        planned = true;
        QStringList jsonParts;
        jsonParts << QString("\"window_samples\":%1").arg(std::max(windowSamples, 1200));
        jsonParts << "\"nfft\":256";
        if(lower.contains("eeg")) {
            jsonParts << "\"match\":\"EEG\"";
        } else if(lower.contains("meg")) {
            jsonParts << "\"match\":\"MEG\"";
        } else if(lower.contains("eog")) {
            jsonParts << "\"match\":\"EOG\"";
        } else {
            jsonParts << "\"match\":\"EEG\"";
        }
        plannedCommand = QString("tools.call neurokernel.psd_summary {%1}").arg(jsonParts.join(","));
        return "Mapped your request to `neurokernel.psd_summary`.";
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
        QJsonArray studioTools = localToolDefinitions();
        const QJsonArray hostedTools = activeHostedViewToolDefinitions();
        for(const QJsonValue& value : hostedTools) {
            studioTools.append(value);
        }
        return QString("Studio tools: %1 | Requested kernel tools list.")
            .arg(formatToolDefinitions(studioTools));
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

    IRawDataView* rawBrowser = activeRawDataView();
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

    if(toolName.startsWith("view.hosted.")) {
        const QJsonObject session = activeHostedViewSession();
        const QString sessionId = session.value("session_id").toString();
        if(sessionId.isEmpty()) {
            return "No active hosted extension view.";
        }

        const QString commandName = toolName.mid(QString("view.hosted.").size());
        if(commandName.isEmpty()) {
            return "Hosted view tool requires a command name.";
        }

        sendExtensionViewCommand(sessionId, commandName, arguments);
        rememberToolResult(toolName, QJsonObject{
            {"tool_name", toolName},
            {"session_id", sessionId},
            {"command", commandName},
            {"arguments", arguments}
        });
        return QString("Requested hosted view command `%1` for session %2.").arg(commandName, sessionId);
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

    if(toolName == "neurokernel.psd_summary") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }
        QJsonObject kernelArguments = buildRawWindowArguments(arguments.value("window_samples").toInt(1200));
        if(arguments.contains("nfft")) {
            kernelArguments.insert("nfft", arguments.value("nfft").toInt(256));
        }
        if(arguments.contains("match")) {
            kernelArguments.insert("match", arguments.value("match").toString());
        }
        sendKernelToolCall(toolName, kernelArguments);
        return QString("Requested %1 for %2.")
            .arg(toolName, QFileInfo(rawBrowser->filePath()).fileName());
    }

    if(toolName.startsWith("dummy3d.") || toolName.startsWith("extension.")) {
        sendExtensionToolCall(toolName, arguments);
        return QString("Forwarded extension tool call: %1").arg(toolName);
    }

    sendKernelToolCall(toolName, arguments);
    return QString("Forwarded tool call: %1").arg(toolName);
}

IRawDataView* MainWindow::activeRawDataView() const
{
    return dynamic_cast<IRawDataView*>(m_centerTabs->currentWidget());
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
        return "Commands: help, tools.list, tools.call <tool> {json}, views.list, raw.summary, raw.state, raw.goto_sample <n>, raw.cursor <n>, raw.zoom <px_per_sample>, kernel.raw_stats [window_samples], kernel.channel_stats [window_samples], kernel.psd [window_samples] [match].";
    }

    if(command == "views.list") {
        QStringList views;
        for(int i = 0; i < m_centerTabs->count(); ++i) {
            views << QString("%1: %2").arg(i + 1).arg(m_centerTabs->tabText(i));
        }
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    IRawDataView* rawBrowser = activeRawDataView();

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

    if(command == "kernel.psd") {
        if(!rawBrowser) {
            return "No active raw browser.";
        }

        int windowSamples = 1200;
        if(parts.size() >= 2) {
            bool ok = false;
            const int parsedWindow = parts.at(1).toInt(&ok);
            if(!ok || parsedWindow <= 0) {
                return QString("Invalid window size: %1").arg(parts.at(1));
            }
            windowSamples = parsedWindow;
        }

        QString match = "EEG";
        if(parts.size() >= 3) {
            match = parts.at(2).trimmed().toUpper();
        }

        QJsonObject arguments = buildRawWindowArguments(windowSamples);
        arguments.insert("nfft", 256);
        arguments.insert("match", match);

        const int cursorSample = rawBrowser->cursorSample() >= 0 ? rawBrowser->cursorSample() : 0;
        sendKernelToolCall("neurokernel.psd_summary", arguments);
        return QString("Requested neuro-kernel PSD summary for %1 around sample %2 with match %3.")
            .arg(QFileInfo(rawBrowser->filePath()).fileName())
            .arg(cursorSample)
            .arg(match.isEmpty() ? "all channels" : match);
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
        },
        QJsonObject{
            {"name", "neurokernel.psd_summary"},
            {"description", "Compute a Welch PSD summary for the active raw sample window and optional channel match."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  32,
                                                  1000000,
                                                  1200,
                                                  "Number of samples around the current cursor to include in the PSD window.")},
                 {"nfft", integerSchema("FFT Size",
                                        32,
                                        8192,
                                        256,
                                        "FFT length used for the Welch PSD estimate.")},
                 {"match", stringSchema("Channel Match",
                                        QJsonArray{"", "EEG", "MEG", "EOG"},
                                        "EEG",
                                        "Optional channel-name filter applied before averaging PSDs.")}
             }, QJsonArray{"window_samples"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"ok", "error"})},
                 {"tool_name", stringSchema("Tool Name")},
                 {"message", stringSchema("Message")},
                 {"file", stringSchema("File")},
                 {"channel_count", integerSchema("Channel Count", 0, 1000000, 0)},
                 {"channels", arraySchema("Channels", stringSchema("Channel Name"))},
                 {"frequencies", arraySchema("Frequencies",
                                             numberSchema("Frequency",
                                                          0.0,
                                                          1000000.0,
                                                          0.0,
                                                          "Frequency bins in hertz."))},
                 {"psd", arraySchema("PSD",
                                     numberSchema("Power Spectral Density",
                                                  0.0,
                                                  1000000000.0,
                                                  0.0,
                                                  "Average PSD values for the selected channel set."))}
             }, QJsonArray{"status", "tool_name", "message", "frequencies", "psd"})}
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

void MainWindow::requestExtensionHostState()
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        appendProblemMessage("Extension host discovery requested before the extension socket was connected.");
        return;
    }

    const QJsonObject toolsMessage = JsonRpcMessage::createRequest("workbench-extension-tools-list", "tools/list");
    m_extensionSocket->write(JsonRpcMessage::serialize(toolsMessage));
    const QJsonObject resourcesMessage = JsonRpcMessage::createRequest("workbench-extension-resources-list", "resources/list");
    m_extensionSocket->write(JsonRpcMessage::serialize(resourcesMessage));
    const QJsonObject viewsMessage = JsonRpcMessage::createRequest("workbench-extension-views-list", "views/list");
    m_extensionSocket->write(JsonRpcMessage::serialize(viewsMessage));
    m_extensionSocket->flush();
}

void MainWindow::requestExtensionHostReload()
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        appendProblemMessage("Extension host reload requested before the extension socket was connected.");
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QStringList disabledExtensions = settings.value("extensions/disabled_ids").toStringList();
    QJsonArray disabledArray;
    for(const QString& extensionId : disabledExtensions) {
        disabledArray.append(extensionId);
    }

    const QJsonObject params{
        {"extensions_directory", resolveStudioExtensionsDirectory()},
        {"disabled_extension_ids", disabledArray}
    };
    const QJsonObject message = JsonRpcMessage::createRequest("workbench-extension-reload", "extensions.reload", params);
    m_extensionSocket->write(JsonRpcMessage::serialize(message));
    m_extensionSocket->flush();
}

QJsonArray MainWindow::availableToolDefinitions() const
{
    QJsonArray tools = localToolDefinitions();
    const QJsonArray hostedTools = activeHostedViewToolDefinitions();
    for(const QJsonValue& tool : hostedTools) {
        tools.append(tool);
    }
    const QJsonArray kernelTools = kernelToolDefinitions();
    for(const QJsonValue& tool : kernelTools) {
        tools.append(tool);
    }
    const QJsonArray extensionTools = !m_cachedExtensionToolDefinitions.isEmpty()
        ? m_cachedExtensionToolDefinitions
        : (m_viewProviderRegistry ? m_viewProviderRegistry->toolDefinitions() : QJsonArray());
    for(const QJsonValue& tool : extensionTools) {
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

    if(IRawDataView* rawBrowser = activeRawDataView()) {
        context.insert("active_view", "raw_browser");
        context.insert("active_file", rawBrowser->filePath());
        context.insert("raw_summary", rawBrowser->summaryText());
        context.insert("raw_state", rawBrowser->stateText());
        context.insert("cursor_sample", rawBrowser->cursorSample());
        context.insert("pixels_per_sample", rawBrowser->pixelsPerSample());
    } else {
        context.insert("active_view", "none");
    }

    const QJsonObject hostedSession = activeHostedViewSession();
    if(!hostedSession.isEmpty()) {
        context.insert("active_hosted_view", hostedSession);
    }

    return context;
}

QJsonObject MainWindow::defaultArgumentsForTool(const QString& toolName) const
{
    if(toolName == "studio.views.list" || toolName == "view.raw.summary" || toolName == "view.raw.state") {
        return QJsonObject();
    }

    if(toolName == "view.raw.goto" || toolName == "view.raw.cursor") {
        IRawDataView* rawBrowser = activeRawDataView();
        return QJsonObject{{"sample", rawBrowser ? rawBrowser->cursorSample() : 0}};
    }

    if(toolName == "view.raw.zoom") {
        IRawDataView* rawBrowser = activeRawDataView();
        return QJsonObject{{"pixels_per_sample", rawBrowser ? rawBrowser->pixelsPerSample() : 1.0}};
    }

    if(toolName == "neurokernel.raw_stats") {
        QJsonObject arguments = buildRawWindowArguments(600);
        arguments.insert("window_samples", 600);
        return arguments;
    }

    if(toolName == "neurokernel.channel_stats") {
        QJsonObject arguments = buildRawWindowArguments(600);
        arguments.insert("window_samples", 600);
        arguments.insert("limit", 5);
        arguments.insert("match", "EEG");
        return arguments;
    }

    if(toolName == "neurokernel.find_peak_window") {
        QJsonObject arguments = buildRawWindowArguments(4000);
        arguments.insert("window_samples", 4000);
        arguments.insert("match", "EEG");
        return arguments;
    }

    if(toolName == "neurokernel.psd_summary") {
        QJsonObject arguments = buildRawWindowArguments(1200);
        arguments.insert("window_samples", 1200);
        arguments.insert("nfft", 256);
        arguments.insert("match", "EEG");
        return arguments;
    }

    if(toolName == "neurokernel.execute") {
        return QJsonObject{{"command", "help"}};
    }

    if(toolName == "dummy3d.set_opacity") {
        return QJsonObject{{"opacity", 0.8}};
    }

    if(toolName == "view.hosted.set_opacity") {
        const QJsonObject session = activeHostedViewSession();
        const double opacity = session.value("state").toObject().value("opacity").toDouble(0.8);
        return QJsonObject{{"opacity", opacity}};
    }

    if(toolName.startsWith("view.hosted.")) {
        return QJsonObject();
    }

    return QJsonObject();
}

QJsonObject MainWindow::activeHostedViewSession() const
{
    QWidget* currentWidget = m_centerTabs ? m_centerTabs->currentWidget() : nullptr;
    if(!currentWidget) {
        return QJsonObject();
    }

    const QString sessionId = currentWidget->property("mne_session_id").toString().trimmed();
    if(!sessionId.isEmpty()) {
        for(const QJsonValue& value : m_cachedExtensionViewSessions) {
            const QJsonObject session = value.toObject();
            if(session.value("session_id").toString() == sessionId) {
                return session;
            }
        }

        const QJsonObject descriptor = currentWidget->property("mne_session_descriptor").toJsonObject();
        if(!descriptor.isEmpty()) {
            return descriptor;
        }
    }

    return QJsonObject();
}

QJsonArray MainWindow::activeHostedViewToolDefinitions() const
{
    const QJsonObject session = activeHostedViewSession();
    if(session.isEmpty()) {
        return QJsonArray();
    }

    const QString providerName = session.value("provider_display_name").toString("Hosted View");
    const QString extensionName = session.value("extension_display_name").toString("Extension Host");
    const QJsonObject state = session.value("state").toObject();
    const QJsonObject stateSchema = session.value("state_schema").toObject();
    const QJsonObject controls = session.value("controls").toObject();
    const QJsonArray actions = session.value("actions").toArray();

    QJsonArray tools;
    for(const QJsonValue& value : actions) {
        const QJsonObject action = value.toObject();
        const QString commandName = action.value("command").toString().trimmed();
        if(commandName.isEmpty()) {
            continue;
        }

        QJsonObject resultSchema = objectSchema(QJsonObject{
            {"session_id", resultStringSchema("Session ID")},
            {"command", resultStringSchema("Command")},
            {"state_before", stateSchema.isEmpty() ? objectSchema(QJsonObject()) : stateSchema},
            {"state_after", stateSchema.isEmpty() ? objectSchema(QJsonObject()) : stateSchema}
        }, QJsonArray{"session_id", "command"});

        const QJsonObject actionResultSchema = action.value("result_schema").toObject();
        if(!actionResultSchema.isEmpty()) {
            resultSchema = actionResultSchema;
        }

        tools.append(QJsonObject{
            {"name", QString("view.hosted.%1").arg(commandName)},
            {"description", QString("Trigger the active hosted view action `%1` for %2 from %3.")
                                .arg(commandName, providerName, extensionName)},
            {"input_schema", objectSchema(QJsonObject())},
            {"result_schema", resultSchema}
        });
    }

    if(controls.contains("opacity") || state.contains("opacity")) {
        tools.append(QJsonObject{
            {"name", "view.hosted.set_opacity"},
            {"description", QString("Set the opacity of the active hosted view for %1 from %2.")
                                .arg(providerName, extensionName)},
            {"input_schema", objectSchema(QJsonObject{
                 {"opacity", numberSchema("Opacity",
                                          0.0,
                                          1.0,
                                          state.value("opacity").toDouble(0.8),
                                          "Opacity for the active hosted extension view.")}
             }, QJsonArray{"opacity"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"session_id", resultStringSchema("Session ID")},
                 {"command", resultStringSchema("Command")},
                 {"state_before", stateSchema.isEmpty() ? objectSchema(QJsonObject()) : stateSchema},
                 {"state_after", stateSchema.isEmpty() ? objectSchema(QJsonObject()) : stateSchema},
                 {"arguments", objectSchema(QJsonObject{
                      {"opacity", numberSchema("Opacity", 0.0, 1.0, 0.8)}
                  })}
             }, QJsonArray{"session_id", "command"})}
        });
    }

    return tools;
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

    QTreeWidgetItem* extensionRoot = new QTreeWidgetItem(QStringList() << "Extension Host Tools");
    const QJsonArray extensionTools = !m_cachedExtensionToolDefinitions.isEmpty()
        ? m_cachedExtensionToolDefinitions
        : (m_viewProviderRegistry ? m_viewProviderRegistry->toolDefinitions() : QJsonArray());
    if(extensionTools.isEmpty()) {
        extensionRoot->addChild(new QTreeWidgetItem(QStringList() << "No extension tools discovered yet."));
    } else {
        for(const QJsonValue& value : extensionTools) {
            extensionRoot->addChild(makeToolTreeItem(value.toObject()));
        }
    }
    extensionRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(extensionRoot);

    QTreeWidgetItem* hostedToolsRoot = new QTreeWidgetItem(QStringList() << "Active Hosted View Tools");
    const QJsonArray hostedTools = activeHostedViewToolDefinitions();
    if(hostedTools.isEmpty()) {
        hostedToolsRoot->addChild(new QTreeWidgetItem(QStringList() << "No active hosted view actions exposed as tools."));
    } else {
        for(const QJsonValue& value : hostedTools) {
            hostedToolsRoot->addChild(makeToolTreeItem(value.toObject()));
        }
    }
    hostedToolsRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(hostedToolsRoot);

    QTreeWidgetItem* resourceRoot = new QTreeWidgetItem(QStringList() << "Extension Resources");
    if(m_cachedExtensionResources.isEmpty()) {
        resourceRoot->addChild(new QTreeWidgetItem(QStringList() << "No extension manifests discovered yet."));
    } else {
        for(const QJsonValue& value : m_cachedExtensionResources) {
            const QJsonObject resource = value.toObject();
            const QString label = QString("%1 (%2)")
                                      .arg(resource.value("display_name").toString(),
                                           resource.value("version").toString());
            QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << label);
            item->setToolTip(0, resource.value("root_path").toString());
            item->setData(0, Qt::UserRole + 1, resource);
            resourceRoot->addChild(item);
        }
    }
    resourceRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(resourceRoot);

    QTreeWidgetItem* sessionsRoot = new QTreeWidgetItem(QStringList() << "Hosted Views");
    if(m_cachedExtensionViewSessions.isEmpty()) {
        sessionsRoot->addChild(new QTreeWidgetItem(QStringList() << "No hosted extension view sessions yet."));
    } else {
        for(const QJsonValue& value : m_cachedExtensionViewSessions) {
            const QJsonObject session = value.toObject();
            const QJsonObject state = session.value("state").toObject();
            QString label = session.value("title").toString(session.value("provider_display_name").toString("Hosted View"));
            const QString fileName = QFileInfo(session.value("file").toString()).fileName();
            if(!fileName.isEmpty()) {
                label += QString(" | %1").arg(fileName);
            }
            if(state.contains("hemisphere")) {
                label += QString(" | %1").arg(state.value("hemisphere").toString());
            }

            QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << label);
            item->setToolTip(0, session.value("session_id").toString());
            item->setData(0, Qt::UserRole + 1, session);
            sessionsRoot->addChild(item);
        }
    }
    sessionsRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(sessionsRoot);

    QTreeWidgetItem* statusRoot = new QTreeWidgetItem(QStringList() << "Planner Context");
    statusRoot->addChild(new QTreeWidgetItem(QStringList() << m_llmPlanner.statusSummary()));
    statusRoot->addChild(new QTreeWidgetItem(QStringList() << QString("View Providers: %1")
                                                    .arg(m_viewProviderRegistry ? m_viewProviderRegistry->manifests().size() : 0)));
    statusRoot->addChild(new QTreeWidgetItem(QStringList() << QString("Extension Resources: %1").arg(m_cachedExtensionResources.size())));
    statusRoot->addChild(new QTreeWidgetItem(QStringList() << QString("Hosted Sessions: %1").arg(m_cachedExtensionViewSessions.size())));
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
        const QJsonObject payload = item->data(0, Qt::UserRole + 1).toJsonObject();
        if(!payload.isEmpty()) {
            m_skillDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Indented)));
        } else {
            m_skillDetailsView->setPlainText(item->text(0));
        }
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

void MainWindow::reloadExtensionRegistry()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QStringList disabledExtensions = settings.value("extensions/disabled_ids").toStringList();
    const QString extensionsDirectory = resolveStudioExtensionsDirectory();
    QString errorMessage;
    m_viewProviderRegistry->loadFromDirectory(extensionsDirectory, &errorMessage);
    m_viewProviderRegistry->setDisabledExtensionIds(disabledExtensions);

    if(!errorMessage.trimmed().isEmpty()) {
        appendProblemMessage(QString("Extension registry: %1").arg(errorMessage));
    }
}

void MainWindow::refreshExtensionManagerUi()
{
    if(!m_extensionsExplorer) {
        return;
    }

    m_extensionsExplorer->clear();
    QTreeWidgetItem* hostStateItem = new QTreeWidgetItem(QStringList() << "Extension Host State");
    hostStateItem->setData(0, Qt::UserRole + 10, "host_state");
    hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                << QString("Live Tools: %1").arg(m_cachedExtensionToolDefinitions.size())));
    hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                << QString("Live Resources: %1").arg(m_cachedExtensionResources.size())));
    hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                << QString("Hosted Sessions: %1").arg(m_cachedExtensionViewSessions.size())));

    if(!m_lastExtensionReloadResult.isEmpty()) {
        hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                    << QString("Last Reload: %1")
                                                           .arg(m_lastExtensionReloadResult.value("message").toString())));
        hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                    << QString("Active Extensions: %1")
                                                           .arg(m_lastExtensionReloadResult.value("active_extension_count").toInt())));
        hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                    << QString("Disabled Extensions: %1")
                                                           .arg(m_lastExtensionReloadResult.value("disabled_extension_count").toInt())));
        hostStateItem->addChild(new QTreeWidgetItem(QStringList()
                                                    << QString("Invalidated Sessions: %1")
                                                           .arg(m_lastExtensionReloadResult.value("invalidated_session_count").toInt())));

        const QJsonArray invalidatedSessionIds = m_lastExtensionReloadResult.value("invalidated_session_ids").toArray();
        if(!invalidatedSessionIds.isEmpty()) {
            QTreeWidgetItem* invalidatedRoot = new QTreeWidgetItem(QStringList() << "Invalidated Session IDs");
            for(const QJsonValue& value : invalidatedSessionIds) {
                invalidatedRoot->addChild(new QTreeWidgetItem(QStringList() << value.toString()));
            }
            hostStateItem->addChild(invalidatedRoot);
        }

        const QJsonArray invalidatedSessions = m_lastExtensionReloadResult.value("invalidated_sessions").toArray();
        if(!invalidatedSessions.isEmpty()) {
            QTreeWidgetItem* invalidatedSessionsRoot = new QTreeWidgetItem(QStringList() << "Invalidated Sessions");
            for(const QJsonValue& value : invalidatedSessions) {
                const QJsonObject session = value.toObject();
                const QString label = QString("%1 | %2")
                                          .arg(session.value("title").toString(),
                                               session.value("session_id").toString());
                QTreeWidgetItem* sessionItem = new QTreeWidgetItem(QStringList() << label);
                sessionItem->setToolTip(0, session.value("file").toString());
                invalidatedSessionsRoot->addChild(sessionItem);
            }
            hostStateItem->addChild(invalidatedSessionsRoot);
        }
    } else {
        hostStateItem->addChild(new QTreeWidgetItem(QStringList() << "Last Reload: Not requested yet"));
    }
    hostStateItem->setExpanded(true);
    m_extensionsExplorer->addTopLevelItem(hostStateItem);

    const QVector<ExtensionManifest> manifests = m_viewProviderRegistry->allManifests();
    for(const ExtensionManifest& manifest : manifests) {
        const bool enabled = m_viewProviderRegistry->isExtensionEnabled(manifest.id);
        const QString label = QString("%1 %2")
                                  .arg(enabled ? "[Enabled]" : "[Disabled]",
                                       manifest.displayName);
        QTreeWidgetItem* extensionItem = new QTreeWidgetItem(QStringList() << label);
        extensionItem->setToolTip(0, manifest.rootPath);
        extensionItem->setData(0, Qt::UserRole, manifest.id);
        extensionItem->setData(0, Qt::UserRole + 1, manifest.rawManifest);
        extensionItem->addChild(new QTreeWidgetItem(QStringList() << QString("Version: %1").arg(manifest.version)));
        extensionItem->addChild(new QTreeWidgetItem(QStringList() << QString("Entry Point: %1").arg(manifest.entryPoint)));
        extensionItem->addChild(new QTreeWidgetItem(QStringList() << QString("View Providers: %1").arg(manifest.viewProviders.size())));
        extensionItem->addChild(new QTreeWidgetItem(QStringList() << QString("Tools: %1").arg(manifest.tools.size())));

        if(!manifest.ui.settingsTabs.isEmpty()) {
            QTreeWidgetItem* settingsRoot = new QTreeWidgetItem(QStringList() << "Settings Tabs");
            for(const UiContribution::SettingsTabContribution& tab : manifest.ui.settingsTabs) {
                QTreeWidgetItem* tabItem = new QTreeWidgetItem(QStringList() << tab.title);
                tabItem->setToolTip(0, tab.description);
                tabItem->setData(0, Qt::UserRole, manifest.id);
                tabItem->setData(0, Qt::UserRole + 2, tab.id);
                settingsRoot->addChild(tabItem);
            }
            extensionItem->addChild(settingsRoot);
        }

        extensionItem->setExpanded(true);
        m_extensionsExplorer->addTopLevelItem(extensionItem);
    }

    updateSelectedExtension(m_extensionsExplorer->currentItem());
}

void MainWindow::updateSelectedExtension(QTreeWidgetItem* item)
{
    m_selectedExtensionId.clear();
    m_extensionToggleButton->setEnabled(false);
    m_extensionSettingsButton->setEnabled(false);

    if(!item) {
        m_extensionDetailsView->setPlainText("Select an extension to inspect it here.");
        return;
    }

    if(item->data(0, Qt::UserRole + 10).toString() == "host_state") {
        QJsonObject details{
            {"last_reload", m_lastExtensionReloadResult},
            {"live_tool_count", static_cast<int>(m_cachedExtensionToolDefinitions.size())},
            {"live_resource_count", static_cast<int>(m_cachedExtensionResources.size())},
            {"live_session_count", static_cast<int>(m_cachedExtensionViewSessions.size())},
            {"live_resources", m_cachedExtensionResources},
            {"live_sessions", m_cachedExtensionViewSessions}
        };
        m_extensionDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(details).toJson(QJsonDocument::Indented)));
        return;
    }

    QString extensionId = item->data(0, Qt::UserRole).toString().trimmed();
    if(extensionId.isEmpty() && item->parent()) {
        extensionId = item->parent()->data(0, Qt::UserRole).toString().trimmed();
    }

    if(extensionId.isEmpty()) {
        m_extensionDetailsView->setPlainText(item->text(0));
        return;
    }

    const QVector<ExtensionManifest> manifests = m_viewProviderRegistry->allManifests();
    for(const ExtensionManifest& manifest : manifests) {
        if(manifest.id != extensionId) {
            continue;
        }

        m_selectedExtensionId = extensionId;
        const bool enabled = m_viewProviderRegistry->isExtensionEnabled(extensionId);
        m_extensionToggleButton->setEnabled(true);
        m_extensionToggleButton->setText(enabled ? "Disable" : "Enable");
        m_extensionSettingsButton->setEnabled(!manifest.ui.settingsTabs.isEmpty());

        QJsonObject details{
            {"id", manifest.id},
            {"display_name", manifest.displayName},
            {"version", manifest.version},
            {"enabled", enabled},
            {"root_path", manifest.rootPath},
            {"entry_point", manifest.entryPoint},
            {"view_provider_count", static_cast<int>(manifest.viewProviders.size())},
            {"tool_count", static_cast<int>(manifest.tools.size())},
            {"settings_tabs", static_cast<int>(manifest.ui.settingsTabs.size())},
            {"manifest", manifest.rawManifest}
        };
        m_extensionDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(details).toJson(QJsonDocument::Indented)));
        return;
    }

    m_extensionDetailsView->setPlainText(item->text(0));
}

void MainWindow::installExtensionFromDirectory()
{
    const QString sourceDirectory = QFileDialog::getExistingDirectory(this, "Install extension from folder");
    if(sourceDirectory.isEmpty()) {
        return;
    }

    const QString manifestPath = QDir(sourceDirectory).filePath("manifest.json");
    if(!QFileInfo::exists(manifestPath)) {
        appendProblemMessage(QString("Selected folder does not contain a manifest.json: %1").arg(sourceDirectory));
        return;
    }

    const QString extensionsRoot = resolveStudioExtensionsDirectory();
    const QString targetDirectory = QDir(extensionsRoot).filePath(QFileInfo(sourceDirectory).fileName());
    if(QFileInfo::exists(targetDirectory)) {
        appendProblemMessage(QString("An extension folder already exists at %1").arg(targetDirectory));
        return;
    }

    if(!copyDirectoryRecursively(sourceDirectory, targetDirectory)) {
        appendProblemMessage(QString("Failed to install extension from %1").arg(sourceDirectory));
        return;
    }

    reloadExtensionRegistry();
    refreshExtensionManagerUi();
    rebuildSkillsExplorer();
    appendOutputMessage(QString("Installed extension into %1").arg(targetDirectory));
    appendTerminalMessage("> Extension installed on disk. Reloading extension host...");
    requestExtensionHostReload();
}

void MainWindow::toggleSelectedExtensionEnabled()
{
    if(m_selectedExtensionId.isEmpty()) {
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    QStringList disabledExtensions = settings.value("extensions/disabled_ids").toStringList();
    if(disabledExtensions.contains(m_selectedExtensionId)) {
        disabledExtensions.removeAll(m_selectedExtensionId);
        appendOutputMessage(QString("Enabled extension %1").arg(m_selectedExtensionId));
    } else {
        disabledExtensions.append(m_selectedExtensionId);
        appendOutputMessage(QString("Disabled extension %1").arg(m_selectedExtensionId));
    }

    settings.setValue("extensions/disabled_ids", disabledExtensions);
    reloadExtensionRegistry();
    refreshExtensionManagerUi();
    rebuildSkillsExplorer();
    requestExtensionHostReload();
}

void MainWindow::openSelectedExtensionSettingsTab()
{
    if(m_selectedExtensionId.isEmpty()) {
        return;
    }

    const QString settingsTabId = m_extensionsExplorer->currentItem()
        ? m_extensionsExplorer->currentItem()->data(0, Qt::UserRole + 2).toString().trimmed()
        : QString();

    const QVector<ExtensionManifest> manifests = m_viewProviderRegistry->allManifests();
    for(const ExtensionManifest& manifest : manifests) {
        if(manifest.id != m_selectedExtensionId || manifest.ui.settingsTabs.isEmpty()) {
            continue;
        }

        const QString chosenTabId = settingsTabId.isEmpty() ? manifest.ui.settingsTabs.first().id : settingsTabId;
        openExtensionSettingsTab(m_selectedExtensionId, chosenTabId);
        return;
    }
}

void MainWindow::openExtensionSettingsTab(const QString& extensionId, const QString& settingsTabId)
{
    const QVector<ExtensionManifest> manifests = m_viewProviderRegistry->allManifests();
    for(const ExtensionManifest& manifest : manifests) {
        if(manifest.id != extensionId) {
            continue;
        }

        for(const UiContribution::SettingsTabContribution& tab : manifest.ui.settingsTabs) {
            if(tab.id != settingsTabId) {
                continue;
            }

            const QString tabKey = QString("extension-settings:%1:%2").arg(extensionId, settingsTabId);
            for(int i = 0; i < m_centerTabs->count(); ++i) {
                if(m_centerTabs->tabToolTip(i) == tabKey) {
                    m_centerTabs->setCurrentIndex(i);
                    return;
                }
            }

            QWidget* container = new QWidget;
            QVBoxLayout* layout = new QVBoxLayout(container);
            layout->setContentsMargins(16, 16, 16, 16);
            layout->setSpacing(12);

            QLabel* titleLabel = new QLabel(QString("%1 Settings").arg(manifest.displayName), container);
            titleLabel->setObjectName("terminalStatusLabel");
            QLabel* subtitleLabel = new QLabel(QString("%1 | %2").arg(tab.title, tab.description), container);
            subtitleLabel->setWordWrap(true);
            QTextEdit* detailsView = new QTextEdit(container);
            detailsView->setReadOnly(true);

            QJsonArray settingsTabs;
            for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
                settingsTabs.append(QJsonObject{
                    {"id", settingsTab.id},
                    {"title", settingsTab.title},
                    {"description", settingsTab.description}
                });
            }

            const QJsonObject payload{
                {"extension_id", manifest.id},
                {"display_name", manifest.displayName},
                {"active_settings_tab", settingsTabId},
                {"settings_tabs", settingsTabs},
                {"message", "This extension contributes a manifest-defined settings tab. The next step is wiring provider-owned settings forms into this slot."}
            };
            detailsView->setPlainText(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Indented)));

            layout->addWidget(titleLabel);
            layout->addWidget(subtitleLabel);
            layout->addWidget(detailsView, 1);

            const int tabIndex = m_centerTabs->addTab(container, QString("%1: %2").arg(manifest.displayName, tab.title));
            m_centerTabs->setTabToolTip(tabIndex, tabKey);
            m_centerTabs->setCurrentIndex(tabIndex);
            appendOutputMessage(QString("Opened extension settings tab %1 for %2").arg(tab.title, manifest.displayName));
            return;
        }
    }
}

QJsonObject MainWindow::buildRawWindowArguments(int windowSamples) const
{
    IRawDataView* rawBrowser = activeRawDataView();
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

void MainWindow::requestExtensionViewOpen(const QString& filePath, const QJsonObject& dispatch)
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        finalizeExtensionViewOpen(filePath, dispatch);
        return;
    }

    const QString requestId = QString("workbench-extension-view-open-%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_pendingExtensionViewOpens.insert(requestId, dispatch);
    m_pendingExtensionViewFiles.insert(requestId, filePath);

    QJsonObject params{
        {"file", filePath},
        {"provider_id", dispatch.value("provider_id").toString()},
        {"sceneId", dispatch.value("sceneId").toString()},
        {"slot", dispatch.value("slot").toString("center")}
    };

    const QJsonObject message = JsonRpcMessage::createRequest(requestId, "views/open", params);
    m_extensionSocket->write(JsonRpcMessage::serialize(message));
    m_extensionSocket->flush();
}

void MainWindow::finalizeExtensionViewOpen(const QString& filePath, const QJsonObject& sessionDescriptor)
{
    const QString sessionId = sessionDescriptor.value("session_id").toString();
    const QString widgetType = sessionDescriptor.value("widget_type").toString().trimmed();
    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(m_centerTabs->tabToolTip(i) == filePath) {
            m_centerTabs->setCurrentIndex(i);
            if(!sessionId.isEmpty()) {
                if(QWidget* widget = m_centerTabs->widget(i)) {
                    widget->setProperty("mne_session_id", sessionId);
                    widget->setProperty("mne_session_descriptor", sessionDescriptor);
                    QMetaObject::invokeMethod(widget,
                                              "setSessionDescriptor",
                                              Qt::DirectConnection,
                                              Q_ARG(QJsonObject, sessionDescriptor));
                    m_extensionViewWidgetsBySessionId.insert(sessionId, widget);
                }
            }
            return;
        }
    }

    QWidget* viewWidget = nullptr;
    if(const IExtensionViewFactory* viewFactory = ExtensionViewFactoryRegistry::instance().factoryForWidgetType(widgetType)) {
        viewWidget = viewFactory->createView(sessionDescriptor, nullptr);
        if(!viewWidget) {
            appendProblemMessage(QString("Failed to create hosted extension view for %1 with widget type %2.")
                                     .arg(filePath, widgetType));
            return;
        }

        viewWidget->setProperty("mne_session_id", sessionId);
        viewWidget->setProperty("mne_session_descriptor", sessionDescriptor);

        if(hasQtSignal(viewWidget, "outputMessage(QString)")) {
            QObject::connect(viewWidget, SIGNAL(outputMessage(QString)), this, SLOT(handleHostedExtensionViewOutput(QString)));
        }
        if(hasQtSignal(viewWidget, "statusMessage(QString)")) {
            QObject::connect(viewWidget, SIGNAL(statusMessage(QString)), this, SLOT(handleHostedExtensionViewStatus(QString)));
        }
        if(hasQtSignal(viewWidget, "viewCommandRequested(QString,QString,QJsonObject)")) {
            QObject::connect(viewWidget,
                             SIGNAL(viewCommandRequested(QString,QString,QJsonObject)),
                             this,
                             SLOT(handleHostedExtensionViewCommand(QString,QString,QJsonObject)));
        }
        if(!sessionId.isEmpty()) {
            m_extensionViewWidgetsBySessionId.insert(sessionId, viewWidget);
        }
    } else {
        ExtensionHostedViewWidget* hostedView = new ExtensionHostedViewWidget;
        hostedView->setSessionDescriptor(sessionDescriptor);
        hostedView->setProperty("mne_session_id", sessionId);
        hostedView->setProperty("mne_session_descriptor", sessionDescriptor);
        QObject::connect(hostedView,
                         SIGNAL(viewCommandRequested(QString,QString,QJsonObject)),
                         this,
                         SLOT(handleHostedExtensionViewCommand(QString,QString,QJsonObject)));
        viewWidget = hostedView;
        if(!sessionId.isEmpty()) {
            m_extensionViewWidgetsBySessionId.insert(sessionId, hostedView);
        }
    }

    const int tabIndex = m_centerTabs->addTab(viewWidget, QFileInfo(filePath).fileName());
    m_centerTabs->setTabToolTip(tabIndex, filePath);
    m_centerTabs->setCurrentIndex(tabIndex);

    appendOutputMessage(QString("Opened hosted extension view session for %1").arg(filePath));
    appendTerminalMessage(QString("> Hosted extension view ready for %1").arg(QFileInfo(filePath).fileName()));
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

void MainWindow::handleHostedExtensionViewOutput(const QString& message)
{
    appendOutputMessage(message);
    appendTerminalMessage(QString("> %1").arg(message));
}

void MainWindow::handleHostedExtensionViewStatus(const QString& message)
{
    statusBar()->showMessage(message);
    setActivePanelState(message);
}

void MainWindow::handleHostedExtensionViewCommand(const QString& sessionId,
                                                  const QString& commandName,
                                                  const QJsonObject& arguments)
{
    sendExtensionViewCommand(sessionId, commandName, arguments);
}

void MainWindow::sendExtensionToolCall(const QString& toolName, const QJsonObject& arguments)
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        appendProblemMessage(QString("Extension host is not connected for tool %1.").arg(toolName));
        return;
    }

    QJsonObject params{
        {"name", toolName},
        {"arguments", arguments}
    };

    const QJsonObject message = JsonRpcMessage::createRequest("workbench-extension-1", "tools/call", params);
    m_extensionSocket->write(JsonRpcMessage::serialize(message));
    m_extensionSocket->flush();
}

void MainWindow::sendExtensionViewCommand(const QString& sessionId,
                                          const QString& commandName,
                                          const QJsonObject& arguments)
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        appendProblemMessage(QString("Extension host is not connected for view session %1.").arg(sessionId));
        return;
    }

    QJsonObject params{
        {"session_id", sessionId},
        {"command", commandName},
        {"arguments", arguments}
    };

    const QString requestId = QString("workbench-extension-view-command-%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QJsonObject message = JsonRpcMessage::createRequest(requestId, "views/command", params);
    m_extensionSocket->write(JsonRpcMessage::serialize(message));
    m_extensionSocket->flush();
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

    m_psdResultHistory.clear();
    m_structuredResultHistory.clear();
    const QByteArray psdHistoryPayload = settings.value("workspace/psd_history").toByteArray();
    if(!psdHistoryPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(psdHistoryPayload);
        if(document.isArray()) {
            const QJsonArray historyArray = document.array();
            for(const QJsonValue& value : historyArray) {
                if(value.isObject()) {
                    m_psdResultHistory.append(value.toObject());
                }
            }
            while(m_psdResultHistory.size() > 8) {
                m_psdResultHistory.removeFirst();
            }
        }
    }

    const QByteArray resultHistoryPayload = settings.value("workspace/result_history").toByteArray();
    if(!resultHistoryPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(resultHistoryPayload);
        if(document.isArray()) {
            const QJsonArray historyArray = document.array();
            for(const QJsonValue& value : historyArray) {
                if(value.isObject()) {
                    m_structuredResultHistory.append(value.toObject());
                }
            }
            while(m_structuredResultHistory.size() > 20) {
                m_structuredResultHistory.removeFirst();
            }
        }
    }
    refreshStructuredResultHistoryUi();

    const QStringList workspaceFiles = settings.value("workspace/files").toStringList();
    const QStringList workspaceDirectories = settings.value("workspace/directories").toStringList();
    for(const QString& directoryPath : workspaceDirectories) {
        addWorkspaceDirectory(directoryPath);
    }
    for(const QString& filePath : workspaceFiles) {
        openFileInView(filePath);
    }

    m_lastToolName = settings.value("workspace/latest_tool_name").toString();
    m_lastToolResult = QJsonObject();
    const QByteArray latestToolPayload = settings.value("workspace/latest_tool_result").toByteArray();
    if(!latestToolPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(latestToolPayload);
        if(document.isObject()) {
            m_lastToolResult = document.object();
        }
    }

    if(!m_lastToolName.isEmpty() && !m_lastToolResult.isEmpty()) {
        updateStructuredResultView(m_lastToolName, m_lastToolResult);
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
    QJsonArray psdHistoryArray;
    for(const QJsonObject& result : m_psdResultHistory) {
        psdHistoryArray.append(result);
    }
    QJsonArray resultHistoryArray;
    for(const QJsonObject& result : m_structuredResultHistory) {
        resultHistoryArray.append(result);
    }
    settings.setValue("workspace/psd_history", QJsonDocument(psdHistoryArray).toJson(QJsonDocument::Compact));
    settings.setValue("workspace/result_history", QJsonDocument(resultHistoryArray).toJson(QJsonDocument::Compact));
    settings.setValue("workspace/latest_tool_name", m_lastToolName);
    settings.setValue("workspace/latest_tool_result", QJsonDocument(m_lastToolResult).toJson(QJsonDocument::Compact));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    persistWorkspace();
    QMainWindow::closeEvent(event);
}

bool MainWindow::isSupportedWorkspaceFile(const QString& filePath) const
{
    const QString extension = QString(".%1").arg(QFileInfo(filePath).suffix().toLower());
    if(extension == ".fif" || extension == ".ave" || extension == ".edf") {
        return true;
    }

    if(m_viewProviderRegistry && !m_viewProviderRegistry->providerForFile(filePath).isEmpty()) {
        return true;
    }

    const auto kind = ViewManager::viewKindForFile(filePath);
    return kind != ViewManager::ViewKind::Unsupported;
}
