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
#include "extensionsettingswidget.h"
#include "extensionhostedviewwidget.h"
#include "llmsettingsdialog.h"

#include <extensionviewfactoryregistry.h>
#include <iextensionviewfactory.h>
#include <irawdataview.h>
#include <iresultrendererfactory.h>
#include <iresultrendererwidget.h>
#include <jsonrpcmessage.h>
#include <resultrendererfactoryregistry.h>
#include <viewproviderregistry.h>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
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
const char* kWorkspaceArtifactStepEntry = "__mne_analyze_studio_analysis_step_entry__";

QString extensionSettingsToolName(const QString& extensionId,
                                  const QString& tabId,
                                  const QString& fieldId,
                                  const QString& verb)
{
    return QString("settings.%1.%2.%3.%4")
        .arg(extensionId.trimmed(),
             tabId.trimmed(),
             fieldId.trimmed(),
             verb.trimmed());
}

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
    if(toolName == "studio.pipeline.run") {
        return "Pipeline Run";
    }
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

QString pipelineArtifactStatusBadge(const QString& status)
{
    if(status == QLatin1String("completed")) {
        return "[done]";
    }
    if(status == QLatin1String("failed")) {
        return "[failed]";
    }
    if(status == QLatin1String("running")) {
        return "[running]";
    }
    if(status == QLatin1String("resumed")) {
        return "[resumed]";
    }
    if(status == QLatin1String("queued")) {
        return "[queued]";
    }

    return "[saved]";
}

QStyle::StandardPixmap artifactIconType(const QString& toolName)
{
    if(toolName == "studio.pipeline.run") {
        return QStyle::SP_FileDialogContentsView;
    }
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

QStyle::StandardPixmap pipelineArtifactIconType(const QString& status)
{
    if(status == QLatin1String("completed")) {
        return QStyle::SP_DialogApplyButton;
    }
    if(status == QLatin1String("failed")) {
        return QStyle::SP_MessageBoxCritical;
    }
    if(status == QLatin1String("running") || status == QLatin1String("resumed")) {
        return QStyle::SP_BrowserReload;
    }
    if(status == QLatin1String("queued")) {
        return QStyle::SP_MediaPlay;
    }

    return QStyle::SP_FileDialogContentsView;
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

QJsonObject fieldSchemaForSettingsTool(const QJsonObject& field)
{
    const QString type = field.value("type").toString("string").trimmed().toLower();
    const QString label = field.value("label").toString(field.value("id").toString());
    const QString description = field.value("description").toString();

    if(type == "boolean" || type == "bool") {
        return QJsonObject{
            {"type", "boolean"},
            {"title", label},
            {"default", field.value("default").toBool(false)},
            {"description", description}
        };
    }

    if(type == "integer" || type == "int") {
        return integerSchema(label,
                             field.value("minimum").toInt(std::numeric_limits<int>::min()),
                             field.value("maximum").toInt(std::numeric_limits<int>::max()),
                             field.value("default").toInt(0),
                             description);
    }

    if(type == "number" || type == "double") {
        return numberSchema(label,
                            field.value("minimum").toDouble(-1e12),
                            field.value("maximum").toDouble(1e12),
                            field.value("default").toDouble(0.0),
                            description);
    }

    if(type == "enum") {
        return stringSchema(label,
                            field.value("options").toArray(),
                            field.value("default").toString(),
                            description);
    }

    return stringSchema(label,
                        QJsonArray(),
                        field.value("default").toString(),
                        description);
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
, m_resultsStack(new QStackedWidget(this))
, m_resultsTree(new QTreeWidget(this))
, m_resultsTable(new QTableWidget(this))
, m_resultsExtensionRenderer(nullptr)
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
, m_activePipelineTotalSteps(0)
, m_isAdvancingPipeline(false)
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
    QHBoxLayout* resultsHistoryLayout = new QHBoxLayout;
    resultsHistoryLayout->setContentsMargins(0, 0, 0, 0);
    resultsHistoryLayout->setSpacing(8);
    QLabel* resultsHistoryLabel = new QLabel("Recent Results", m_resultsTab);
    resultsHistoryLabel->setObjectName("terminalStatusLabel");
    resultsHistoryLayout->addWidget(resultsHistoryLabel);
    resultsHistoryLayout->addWidget(m_resultsHistoryCombo, 1);
    resultsLayout->addLayout(resultsHistoryLayout);
    resultsLayout->addWidget(m_resultsTitleLabel);
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
    connect(m_agentChatDock, &AgentChatDockWidget::confirmationRequested, this, &MainWindow::approvePlannerConfirmation);
    connect(m_agentChatDock, &AgentChatDockWidget::confirmationDismissed, this, &MainWindow::dismissPlannerConfirmation);
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

            if(toolName == "studio.pipeline.run") {
                const QString runId = result.value("run_id").toString().trimmed();
                const QString pipelineId = result.value("pipeline_id").toString().trimmed();
                const int pendingSteps = result.value("pending_steps").toInt();

                if(pendingSteps > 0 && !runId.isEmpty()) {
                    QAction* resumeAction = contextMenu.addAction("Resume Pipeline");
                    connect(resumeAction, &QAction::triggered, this, [this, runId]() {
                        const QString commandText = QString("tools.call studio.pipeline.resume {\"run_id\":\"%1\"}")
                                                        .arg(runId);
                        appendTerminalMessage(QString("$ %1").arg(commandText));
                        sendToolCall(commandText);
                    });
                }

                if(!pipelineId.isEmpty()) {
                    QAction* rerunAction = contextMenu.addAction("Rerun Pipeline");
                    connect(rerunAction, &QAction::triggered, this, [this, pipelineId]() {
                        const QString commandText = QString("tools.call studio.pipeline.run {\"pipeline_id\":\"%1\"}")
                                                        .arg(pipelineId);
                        appendTerminalMessage(QString("$ %1").arg(commandText));
                        sendToolCall(commandText);
                    });
                }
            }

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
                });
            }
        } else if(specialType == QLatin1String(kWorkspaceArtifactStepEntry)) {
            const QJsonObject entry = item->data(0, Qt::UserRole + 1).toJsonObject();
            const QJsonObject step = item->data(0, Qt::UserRole + 2).toJsonObject();

            QAction* openPipelineAction = contextMenu.addAction("Open Pipeline Session");
            connect(openPipelineAction, &QAction::triggered, this, [this, entry]() {
                openAnalysisArtifact(entry, true);
            });

            const QString commandText = step.value("command").toString().trimmed();
            const QString runId = step.value("run_id").toString().trimmed();
            const int stepNumber = step.value("step_number").toInt(-1);
            if(!runId.isEmpty() && stepNumber > 0) {
                QAction* rehydrateStepAction = contextMenu.addAction("Run Step With Current Defaults");
                connect(rehydrateStepAction, &QAction::triggered, this, [this, runId, stepNumber]() {
                    const QString rerunText = QString("tools.call studio.pipeline.rerun_step {\"run_id\":\"%1\",\"step_number\":%2,\"mode\":\"rehydrate\"}")
                                                  .arg(runId)
                                                  .arg(stepNumber);
                    appendTerminalMessage(QString("$ %1").arg(rerunText));
                    sendToolCall(rerunText);
                });
            }
            if(!commandText.isEmpty()) {
                QAction* runStepAction = contextMenu.addAction("Replay Exact Step");
                connect(runStepAction, &QAction::triggered, this, [this, commandText]() {
                    appendTerminalMessage(QString("$ %1").arg(commandText));
                    sendToolCall(commandText);
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
                    rememberToolResult("kernel.error",
                                     normalizedToolErrorEnvelope("kernel.error",
                                                                 message,
                                                                 "kernel",
                                                                 "rpc_error",
                                                                 "retry"));
                    if(!m_activePipelineId.isEmpty()) {
                        failActivePipeline(QString("Kernel error: %1").arg(message));
                    }
                    m_agentChatDock->appendTranscript(QString("Kernel> %1").arg(message));
                    appendProblemMessage(QString("Kernel error: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                } else {
                    const QJsonObject result = normalizedToolResultEnvelope(response.value("result").toObject().value("tool_name").toString(),
                                                                           response.value("result").toObject(),
                                                                           "kernel");
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
                    rememberToolResult("extension_host.error",
                                     normalizedToolErrorEnvelope("extension_host.error",
                                                                 message,
                                                                 "extension_host",
                                                                 "rpc_error",
                                                                 "retry"));
                    if(!m_activePipelineId.isEmpty()) {
                        failActivePipeline(QString("Extension host error: %1").arg(message));
                    }
                    m_agentChatDock->appendTranscript(QString("Extension Host> %1").arg(message));
                    appendProblemMessage(QString("Extension host error: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                } else {
                    const QString requestId = response.value("id").toString();
                    const QJsonObject result = normalizedToolResultEnvelope(response.value("result").toObject().value("tool_name").toString(),
                                                                           response.value("result").toObject(),
                                                                           "extension_host");
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
    QTreeWidgetItem* pipelineRoot = nullptr;
    QTreeWidgetItem* resultRoot = nullptr;
    int pipelineCount = 0;
    int runningPipelineCount = 0;
    int queuedPipelineCount = 0;
    int failedPipelineCount = 0;
    int completedPipelineCount = 0;
    int resultCount = 0;

    for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject entry = m_structuredResultHistory.at(i);
        const QString toolName = entry.value("tool_name").toString().trimmed();
        const QJsonObject result = entry.value("result").toObject();
        const QString fileName = QFileInfo(result.value("file").toString()).fileName();
        const QString message = result.value("message").toString().trimmed();
        const QString displayName = result.value("display_name").toString().trimmed();
        const QString status = result.value("status").toString().trimmed();
        const QString currentStep = result.value("current_step").toString().trimmed();
        const int completedSteps = result.value("completed_steps").toInt();
        const int totalSteps = result.value("total_steps").toInt();
        const int pendingSteps = result.value("pending_steps").toInt();

        QString label = artifactTypeLabel(toolName);
        if(toolName == "studio.pipeline.run" && !displayName.isEmpty()) {
            label += QString(" | %1").arg(displayName);
        }
        if(!fileName.isEmpty()) {
            label += QString(" | %1").arg(fileName);
        }
        if(toolName == "studio.pipeline.run" && !status.isEmpty()) {
            label += QString(" %1").arg(pipelineArtifactStatusBadge(status));
            if(totalSteps > 0) {
                label += QString(" %1/%2").arg(completedSteps).arg(totalSteps);
            }
            if(pendingSteps > 0) {
                label += QString(" | %1 pending").arg(pendingSteps);
            }
            if((status == QLatin1String("running") || status == QLatin1String("resumed"))
               && !currentStep.isEmpty()) {
                label += QString(" | active: %1").arg(currentStep.left(72));
            }
        }
        if(!message.isEmpty()) {
            label += QString(" | %1").arg(message.left(80));
        }

        QTreeWidgetItem* artifactItem = new QTreeWidgetItem(QStringList() << label);
        artifactItem->setToolTip(0, message);
        if(toolName == "studio.pipeline.run") {
            artifactItem->setIcon(0, style()->standardIcon(pipelineArtifactIconType(status)));
        } else {
            artifactItem->setIcon(0, style()->standardIcon(artifactIconType(toolName)));
        }
        artifactItem->setData(0, Qt::UserRole, QString::fromLatin1(kWorkspaceArtifactEntry));
        artifactItem->setData(0, Qt::UserRole + 1, entry);

        if(toolName == "studio.pipeline.run") {
            ++pipelineCount;
            if(status == QLatin1String("running") || status == QLatin1String("resumed")) {
                ++runningPipelineCount;
            } else if(status == QLatin1String("queued")) {
                ++queuedPipelineCount;
            } else if(status == QLatin1String("failed")) {
                ++failedPipelineCount;
            } else if(status == QLatin1String("completed")) {
                ++completedPipelineCount;
            }
            if(!pipelineRoot) {
                pipelineRoot = new QTreeWidgetItem(QStringList() << "Pipeline Sessions");
                pipelineRoot->setExpanded(true);
                analysisRoot->addChild(pipelineRoot);
            }

            const QJsonArray steps = result.value("steps").toArray();
            for(int stepIndex = 0; stepIndex < steps.size(); ++stepIndex) {
                QJsonObject step = steps.at(stepIndex).toObject();
                if(!result.value("run_id").toString().trimmed().isEmpty() && !step.contains("run_id")) {
                    step.insert("run_id", result.value("run_id").toString());
                }
                if(!result.value("pipeline_id").toString().trimmed().isEmpty() && !step.contains("pipeline_id")) {
                    step.insert("pipeline_id", result.value("pipeline_id").toString());
                }
                const QString stepStatus = step.value("status").toString().trimmed();
                const QString stepTool = step.value("tool_name").toString().trimmed();
                const QString stepCommand = step.value("command").toString().trimmed();
                const bool isActiveStep = stepStatus == QLatin1String("running");
                QString stepLabel = QString("Step %1").arg(step.value("step_number").toInt(stepIndex + 1));
                if(!stepStatus.isEmpty()) {
                    stepLabel += QString(" %1").arg(pipelineArtifactStatusBadge(stepStatus));
                }
                if(!stepTool.isEmpty()) {
                    stepLabel += QString(" | %1").arg(stepTool);
                }
                if(!stepCommand.isEmpty()) {
                    stepLabel += QString(" | %1").arg(stepCommand.left(96));
                }

                QTreeWidgetItem* stepItem = new QTreeWidgetItem(QStringList() << stepLabel);
                stepItem->setData(0, Qt::UserRole, QString::fromLatin1(kWorkspaceArtifactStepEntry));
                stepItem->setData(0, Qt::UserRole + 1, entry);
                stepItem->setData(0, Qt::UserRole + 2, step);
                stepItem->setToolTip(0, QString::fromUtf8(QJsonDocument(step).toJson(QJsonDocument::Indented)));
                stepItem->setIcon(0, style()->standardIcon(pipelineArtifactIconType(stepStatus)));
                if(isActiveStep) {
                    QFont stepFont = stepItem->font(0);
                    stepFont.setBold(true);
                    stepItem->setFont(0, stepFont);
                    stepItem->setForeground(0, m_workspaceExplorer->palette().brush(QPalette::Link));
                }
                artifactItem->addChild(stepItem);
            }

            artifactItem->setExpanded(status == QLatin1String("running")
                                      || status == QLatin1String("resumed")
                                      || status == QLatin1String("queued"));
            if(status == QLatin1String("running") || status == QLatin1String("resumed")) {
                QFont artifactFont = artifactItem->font(0);
                artifactFont.setBold(true);
                artifactItem->setFont(0, artifactFont);
                artifactItem->setForeground(0, m_workspaceExplorer->palette().brush(QPalette::Highlight));
            }
            pipelineRoot->addChild(artifactItem);
        } else {
            ++resultCount;
            if(!resultRoot) {
                resultRoot = new QTreeWidgetItem(QStringList() << "Tool Results");
                resultRoot->setExpanded(true);
                analysisRoot->addChild(resultRoot);
            }
            resultRoot->addChild(artifactItem);
        }
    }

    if(pipelineRoot) {
        QString pipelineLabel = QString("Pipeline Sessions (%1)").arg(pipelineCount);
        QStringList summaryParts;
        if(runningPipelineCount > 0) {
            summaryParts << QString("%1 running").arg(runningPipelineCount);
        }
        if(queuedPipelineCount > 0) {
            summaryParts << QString("%1 queued").arg(queuedPipelineCount);
        }
        if(failedPipelineCount > 0) {
            summaryParts << QString("%1 failed").arg(failedPipelineCount);
        }
        if(completedPipelineCount > 0) {
            summaryParts << QString("%1 completed").arg(completedPipelineCount);
        }
        if(!summaryParts.isEmpty()) {
            pipelineLabel += QString(" | %1").arg(summaryParts.join(", "));
        }
        pipelineRoot->setText(0, pipelineLabel);
    }

    if(resultRoot) {
        resultRoot->setText(0, QString("Tool Results (%1)").arg(resultCount));
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

    const QString runId = result.value("run_id").toString().trimmed();
    const QString artifactKey = !runId.isEmpty()
        ? QString("analysis:%1:%2").arg(toolName, runId)
        : QString("analysis:%1")
              .arg(QString::fromUtf8(QJsonDocument(entry).toJson(QJsonDocument::Compact)));
    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(m_centerTabs->tabToolTip(i) == artifactKey) {
            if(AnalysisResultWidget* resultWidget = qobject_cast<AnalysisResultWidget*>(m_centerTabs->widget(i))) {
                resultWidget->setResultHistory(resultHistoryForTool(toolName));
                resultWidget->setRuntimeContext(resultRendererRuntimeContext());
                resultWidget->setResult(toolName, result);
            }
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
    connect(resultWidget, &AnalysisResultWidget::toolCommandRequested, this, &MainWindow::handleResultRendererToolCommand);
    connect(resultWidget, &AnalysisResultWidget::selectionContextChanged, this, &MainWindow::handleResultRendererSelectionContext);
    resultWidget->setResultHistory(resultHistoryForTool(toolName));
    resultWidget->setRuntimeContext(resultRendererRuntimeContext());
    resultWidget->setResult(toolName, result);
    const QString fileName = QFileInfo(result.value("file").toString()).fileName();
    const QString displayName = result.value("display_name").toString().trimmed();
    QString tabLabel = artifactTypeLabel(toolName);
    if(toolName == "studio.pipeline.run" && !displayName.isEmpty()) {
        const QString status = result.value("status").toString().trimmed();
        tabLabel += QString(": %1").arg(displayName);
        if(!status.isEmpty()) {
            tabLabel += QString(" [%1]").arg(status);
        }
    }
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

void MainWindow::handleResultRendererToolCommand(const QString& commandText)
{
    const QString trimmed = commandText.trimmed();
    if(trimmed.isEmpty()) {
        return;
    }

    appendTerminalMessage(QString("$ %1").arg(trimmed));
    sendToolCall(trimmed);
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
    if(specialType == QLatin1String(kWorkspaceArtifactStepEntry)) {
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
                                                       plannerReadyToolDefinitions(),
                                                       llmPlanningContext(commandText));
        if(llmPlan.success) {
            const QString plannerMessage = llmPlan.summary.isEmpty()
                ? QString("LLM planner (%1) proposed %2 steps.").arg(llmPlan.providerName).arg(llmPlan.plannedCommands.size())
                : QString("LLM planner (%1): %2").arg(llmPlan.providerName, llmPlan.summary);
            m_agentChatDock->appendTranscript(QString("Planner> %1").arg(plannerMessage));
            appendTerminalMessage(QString("> %1").arg(plannerMessage));

            for(int i = 0; i < llmPlan.plannedCommands.size(); ++i) {
                const QString plannedCommand = resolvePlannerReferences(llmPlan.plannedCommands.at(i));
                const QString plannedToolName = toolNameFromCommand(plannedCommand);
                const QJsonObject execution = plannerExecutionMetadata(plannedToolName);
                const QString executionMode = execution.value("execution_mode").toString("suggestion_only");
                if(executionMode == QLatin1String("auto_run")) {
                    const QString stepNote = QString("Running LLM step %1/%2: %3")
                                                 .arg(i + 1)
                                                 .arg(llmPlan.plannedCommands.size())
                                                 .arg(plannedCommand);
                    m_agentChatDock->appendTranscript(QString("Planner> %1").arg(stepNote));
                    appendTerminalMessage(QString("> %1").arg(stepNote));
                    sendToolCall(plannedCommand);
                    continue;
                }

                const QString policyNote = executionMode == QLatin1String("confirm_required")
                    ? QString("LLM step %1/%2 requires confirmation and was not auto-run: %3 | %4")
                          .arg(i + 1)
                          .arg(llmPlan.plannedCommands.size())
                          .arg(plannedCommand,
                               execution.value("execution_reason").toString())
                    : QString("LLM step %1/%2 is suggestion-only and was not auto-run: %3 | %4")
                          .arg(i + 1)
                          .arg(llmPlan.plannedCommands.size())
                          .arg(plannedCommand,
                               execution.value("execution_reason").toString());
                m_agentChatDock->appendTranscript(QString("Planner> %1").arg(policyNote));
                appendTerminalMessage(QString("> %1").arg(policyNote));
                if(executionMode == QLatin1String("confirm_required")) {
                    const QJsonObject presentation = plannerConfirmationPresentation(plannedCommand,
                                                                                     i + 1,
                                                                                     llmPlan.plannedCommands.size(),
                                                                                     policyNote,
                                                                                     llmPlan.summary,
                                                                                     i > 0 ? resolvePlannerReferences(llmPlan.plannedCommands.at(i - 1))
                                                                                           : QString());
                    queuePlannerConfirmation(plannedCommand,
                                             presentation.value("title").toString(),
                                             presentation.value("details").toString(),
                                             presentation.value("reason").toString());
                }
            }
            return;
        }

        const QString errorSummary = llmPlan.errorMessage.isEmpty()
            ? QString("unknown planner error")
            : llmPlan.errorMessage;
        QString plannerFailureNote = QString("LLM planner (%1) failed: %2")
                                         .arg(llmPlan.providerName, errorSummary);
        if(llmPlan.httpStatusCode > 0) {
            plannerFailureNote += QString(" | HTTP %1").arg(llmPlan.httpStatusCode);
        }
        if(!llmPlan.providerErrorType.isEmpty()) {
            plannerFailureNote += QString(" | type=%1").arg(llmPlan.providerErrorType);
        }
        m_agentChatDock->appendTranscript(QString("Planner> %1").arg(plannerFailureNote));
        appendProblemMessage(plannerFailureNote);
        appendTerminalMessage(QString("> %1").arg(plannerFailureNote));
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

void MainWindow::refreshPlannerConfirmationsUi()
{
    if(m_agentChatDock) {
        QJsonArray decoratedConfirmations;
        for(const QJsonValue& value : m_pendingPlannerConfirmations) {
            QJsonObject confirmation = value.toObject();
            const QJsonObject staleness = plannerConfirmationStaleness(confirmation);
            for(auto it = staleness.constBegin(); it != staleness.constEnd(); ++it) {
                confirmation.insert(it.key(), it.value());
            }
            decoratedConfirmations.append(confirmation);
        }
        m_agentChatDock->setPendingConfirmations(decoratedConfirmations);
    }
}

void MainWindow::queuePlannerConfirmation(const QString& commandText,
                                          const QString& summary,
                                          const QString& details,
                                          const QString& reason)
{
    const QString trimmedCommand = commandText.trimmed();
    if(trimmedCommand.isEmpty()) {
        return;
    }

    for(int i = 0; i < m_pendingPlannerConfirmations.size(); ++i) {
        const QJsonObject existing = m_pendingPlannerConfirmations.at(i).toObject();
        if(existing.value("command").toString().trimmed() == trimmedCommand) {
            m_pendingPlannerConfirmations[i] = QJsonObject{
                {"command", trimmedCommand},
                {"title", summary},
                {"details", details},
                {"reason", reason},
                {"context_snapshot", plannerConfirmationSnapshot(trimmedCommand)},
                {"created_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
            };
            refreshPlannerConfirmationsUi();
            return;
        }
    }

    m_pendingPlannerConfirmations.append(QJsonObject{
        {"command", trimmedCommand},
        {"title", summary},
        {"details", details},
        {"reason", reason},
        {"context_snapshot", plannerConfirmationSnapshot(trimmedCommand)},
        {"created_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    });
    refreshPlannerConfirmationsUi();
}

bool MainWindow::removePlannerConfirmation(const QString& commandText)
{
    const QString trimmedCommand = commandText.trimmed();
    for(int i = 0; i < m_pendingPlannerConfirmations.size(); ++i) {
        const QJsonObject existing = m_pendingPlannerConfirmations.at(i).toObject();
        if(existing.value("command").toString().trimmed() == trimmedCommand) {
            m_pendingPlannerConfirmations.removeAt(i);
            refreshPlannerConfirmationsUi();
            return true;
        }
    }

    return false;
}

void MainWindow::approvePlannerConfirmation(const QString& commandText)
{
    const QString trimmedCommand = commandText.trimmed();
    if(trimmedCommand.isEmpty()) {
        return;
    }

    QJsonObject confirmation;
    for(const QJsonValue& value : m_pendingPlannerConfirmations) {
        const QJsonObject candidate = value.toObject();
        if(candidate.value("command").toString().trimmed() == trimmedCommand) {
            confirmation = candidate;
            break;
        }
    }
    const QJsonObject staleness = plannerConfirmationStaleness(confirmation);
    removePlannerConfirmation(trimmedCommand);
    const QString note = QString("Approved planner action: %1").arg(trimmedCommand);
    m_agentChatDock->appendTranscript(QString("Planner> %1").arg(note));
    appendTerminalMessage(QString("> %1").arg(note));
    if(staleness.value("stale").toBool(false)) {
        const QString staleNote = QString("Planner confirmation was stale when approved: %1")
                                      .arg(staleness.value("stale_reason").toString());
        m_agentChatDock->appendTranscript(QString("Planner> %1").arg(staleNote));
        appendTerminalMessage(QString("> %1").arg(staleNote));
    }
    appendTerminalMessage(QString("$ %1").arg(trimmedCommand));
    sendToolCall(trimmedCommand);
}

void MainWindow::dismissPlannerConfirmation(const QString& commandText)
{
    const QString trimmedCommand = commandText.trimmed();
    if(trimmedCommand.isEmpty()) {
        return;
    }

    if(removePlannerConfirmation(trimmedCommand)) {
        const QString note = QString("Dismissed planner action: %1").arg(trimmedCommand);
        m_agentChatDock->appendTranscript(QString("Planner> %1").arg(note));
        appendTerminalMessage(QString("> %1").arg(note));
    }
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

QJsonObject MainWindow::normalizedToolResultEnvelope(const QString& toolName,
                                                     const QJsonObject& result,
                                                     const QString& source) const
{
    QJsonObject envelope = result;
    const QString normalizedToolName = toolName.trimmed().isEmpty()
        ? result.value("tool_name").toString().trimmed()
        : toolName.trimmed();
    const QString normalizedSource = source.trimmed().isEmpty() ? QString("workbench") : source.trimmed();

    QString status = result.value("status").toString().trimmed().toLower();
    if(status.isEmpty()) {
        status = "ok";
    }

    QString severity = "info";
    if(status == QLatin1String("error") || status == QLatin1String("failed")) {
        severity = "error";
    } else if(status == QLatin1String("ignored")) {
        severity = "warning";
    }

    QString recoverability = "none";
    if(status == QLatin1String("error") || status == QLatin1String("failed")) {
        recoverability = "retry";
    } else if(status == QLatin1String("ignored")) {
        recoverability = "alternative";
    }

    QJsonArray suggestedTools;
    if((normalizedToolName == QLatin1String("neurokernel.find_peak_window")
        || normalizedToolName == QLatin1String("view.raw.goto"))
       && activeRawDataView()) {
        suggestedTools.append("view.raw.state");
    }
    if(normalizedToolName == QLatin1String("neurokernel.channel_stats")) {
        suggestedTools.append("neurokernel.find_peak_window");
    }
    if(normalizedToolName == QLatin1String("neurokernel.psd_summary")) {
        suggestedTools.append("studio.pipeline.run");
    }
    if(normalizedToolName == QLatin1String("studio.pipeline.run")
       && result.value("pending_steps").toInt() > 0) {
        suggestedTools.append("studio.pipeline.resume");
    }

    envelope.insert("tool_name", normalizedToolName);
    envelope.insert("status", status);
    envelope.insert("severity", severity);
    envelope.insert("recoverability", recoverability);
    envelope.insert("source", normalizedSource);
    envelope.insert("normalized", true);
    envelope.insert("next_suggested_tools", suggestedTools);
    return envelope;
}

QJsonObject MainWindow::normalizedToolErrorEnvelope(const QString& toolName,
                                                    const QString& message,
                                                    const QString& source,
                                                    const QString& failureKind,
                                                    const QString& recoverability,
                                                    const QJsonObject& details) const
{
    QJsonObject envelope{
        {"tool_name", toolName.trimmed().isEmpty() ? QString("unknown") : toolName.trimmed()},
        {"status", "error"},
        {"severity", "error"},
        {"recoverability", recoverability.trimmed().isEmpty() ? QString("retry") : recoverability.trimmed()},
        {"source", source.trimmed().isEmpty() ? QString("workbench") : source.trimmed()},
        {"failure_kind", failureKind.trimmed().isEmpty() ? QString("unknown") : failureKind.trimmed()},
        {"message", message.trimmed()},
        {"normalized", true},
        {"next_suggested_tools", QJsonArray()}
    };

    if(!details.isEmpty()) {
        envelope.insert("details", details);
    }

    if(envelope.value("recoverability").toString() == QLatin1String("retry")) {
        envelope.insert("next_suggested_tools", QJsonArray{"studio.views.list", "view.raw.state"});
    }

    return envelope;
}

void MainWindow::rememberToolResult(const QString& toolName, const QJsonObject& result)
{
    const QString effectiveToolName = toolName.trimmed().isEmpty()
        ? result.value("tool_name").toString().trimmed()
        : toolName.trimmed();
    if(effectiveToolName.isEmpty()) {
        return;
    }

    const QJsonObject normalizedResult = normalizedToolResultEnvelope(effectiveToolName, result);

    m_lastToolName = effectiveToolName;
    m_lastToolResult = normalizedResult;

    if(effectiveToolName != QLatin1String("studio.pipeline.run")
       && !m_activePipelineId.isEmpty()
       && !m_activePipelineStepHistory.isEmpty()) {
        QJsonObject step = m_activePipelineStepHistory.last().toObject();
        if(step.value("status").toString() == QLatin1String("running")) {
            step.insert("status", "completed");
            step.insert("tool_name", effectiveToolName);
            step.insert("result", normalizedResult);
            step.insert("message", normalizedResult.value("message").toString());
            step.insert("completed_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            m_activePipelineStepHistory.removeAt(m_activePipelineStepHistory.size() - 1);
            m_activePipelineStepHistory.append(step);
            updateActivePipelineArtifact("running", step.value("command").toString());
        }
    }

    if(effectiveToolName == "neurokernel.psd_summary" && !normalizedResult.isEmpty()) {
        bool alreadyTracked = false;
        for(int i = m_psdResultHistory.size() - 1; i >= 0; --i) {
            if(m_psdResultHistory.at(i) == normalizedResult) {
                alreadyTracked = true;
                break;
            }
        }

        if(!alreadyTracked) {
            m_psdResultHistory.append(normalizedResult);
            while(m_psdResultHistory.size() > 8) {
                m_psdResultHistory.removeFirst();
            }
        }
    }

    if(!normalizedResult.isEmpty()) {
        const QJsonObject historyEntry{
            {"tool_name", effectiveToolName},
            {"result", normalizedResult}
        };

        bool alreadyTracked = false;
        const QString runId = normalizedResult.value("run_id").toString().trimmed();
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            const QJsonObject existingEntry = m_structuredResultHistory.at(i);
            if(!runId.isEmpty()
               && existingEntry.value("tool_name").toString() == effectiveToolName
               && existingEntry.value("result").toObject().value("run_id").toString() == runId) {
                m_structuredResultHistory[i] = historyEntry;
                alreadyTracked = true;
                break;
            }

            if(existingEntry == historyEntry) {
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

    if(toolName == "studio.pipeline.run") {
        return;
    }

    if(!m_activePipelineId.isEmpty() && !m_pendingPipelineCommands.isEmpty()) {
        continuePendingPipelineExecution();
    } else if(!m_activePipelineId.isEmpty() && m_pendingPipelineCommands.isEmpty()) {
        continuePendingPipelineExecution();
    }
}

void MainWindow::updateActivePipelineArtifact(const QString& status, const QString& currentStep)
{
    if(m_activePipelineId.isEmpty() || m_activePipelineRunId.isEmpty()) {
        return;
    }

    const int pendingSteps = static_cast<int>(m_pendingPipelineCommands.size());
    const int completedSteps = std::max(0, m_activePipelineTotalSteps - pendingSteps);
    QJsonObject artifactResult{
        {"tool_name", QString("studio.pipeline.run")},
        {"run_id", m_activePipelineRunId},
        {"pipeline_id", m_activePipelineId},
        {"display_name", m_activePipelineDisplayName},
        {"status", status},
        {"current_step", currentStep},
        {"completed_steps", completedSteps},
        {"total_steps", m_activePipelineTotalSteps},
        {"pending_steps", pendingSteps},
        {"pending_commands", QJsonArray::fromStringList(m_pendingPipelineCommands)},
        {"inputs", m_activePipelineInputs},
        {"input_overrides", m_activePipelineInputOverrides},
        {"steps", m_activePipelineStepHistory},
        {"updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"message", QString("%1 | %2/%3 steps")
                        .arg(status,
                             QString::number(completedSteps),
                             QString::number(m_activePipelineTotalSteps))}
    };

    if(!m_activePipelineStepHistory.isEmpty()) {
        artifactResult.insert("started_at",
                              m_activePipelineStepHistory.first().toObject().value("started_at").toString());
    }
    if(status == QLatin1String("completed")) {
        artifactResult.insert("completed_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    if(status == QLatin1String("failed")) {
        artifactResult.insert("failed_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }

    const QJsonObject historyEntry{
        {"tool_name", QString("studio.pipeline.run")},
        {"result", artifactResult}
    };

    bool alreadyTracked = false;
    for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject existingEntry = m_structuredResultHistory.at(i);
        if(existingEntry.value("tool_name").toString() == QLatin1String("studio.pipeline.run")
           && existingEntry.value("result").toObject().value("run_id").toString() == m_activePipelineRunId) {
            m_structuredResultHistory[i] = historyEntry;
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

    refreshStructuredResultHistoryUi();
    if(m_resultsCurrentToolName == QLatin1String("studio.pipeline.run")) {
        updateStructuredResultView("studio.pipeline.run", artifactResult);
    }
}

void MainWindow::failActivePipeline(const QString& failureMessage)
{
    if(m_activePipelineId.isEmpty()) {
        return;
    }

    if(!m_activePipelineStepHistory.isEmpty()) {
        QJsonObject step = m_activePipelineStepHistory.last().toObject();
        if(step.value("status").toString() == QLatin1String("running")) {
            step.insert("status", "failed");
            step.insert("error", failureMessage);
            step.insert("failed_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            m_activePipelineStepHistory.removeAt(m_activePipelineStepHistory.size() - 1);
            m_activePipelineStepHistory.append(step);
        }
    }

    m_pendingPipelineCommands.clear();
    m_activePipelineLastStatus = failureMessage;
    updateActivePipelineArtifact("failed", failureMessage);
    rebuildSkillsExplorer();
    m_activePipelineId.clear();
    m_activePipelineRunId.clear();
    m_activePipelineDisplayName.clear();
    m_activePipelineInputs = QJsonObject();
    m_activePipelineInputOverrides = QJsonObject();
    m_activePipelineStepHistory = QJsonArray();
    m_activePipelineTotalSteps = 0;
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
        const QString displayName = result.value("display_name").toString().trimmed();
        const QString status = result.value("status").toString().trimmed();
        const int completedSteps = result.value("completed_steps").toInt();
        const int totalSteps = result.value("total_steps").toInt();

        QString label = toolName;
        if(toolName == "studio.pipeline.run" && !displayName.isEmpty()) {
            label = QString("studio.pipeline.run | %1").arg(displayName);
        }
        if(!fileName.isEmpty()) {
            label += QString(" | %1").arg(fileName);
        }
        if(toolName == "studio.pipeline.run" && !status.isEmpty()) {
            label += QString(" | %1").arg(status);
            if(totalSteps > 0) {
                label += QString(" %1/%2").arg(completedSteps).arg(totalSteps);
            }
        }
        if(!message.isEmpty()) {
            label += QString(" | %1").arg(message);
        }

        m_resultsHistoryCombo->addItem(label.left(180), entry);
    }

    m_resultsHistoryCombo->setCurrentIndex(0);
    refreshWorkspaceArtifacts();
}

QJsonArray MainWindow::resultHistoryForTool(const QString& toolName) const
{
    QJsonArray history;

    if(toolName == "neurokernel.psd_summary") {
        for(const QJsonObject& result : m_psdResultHistory) {
            history.append(result);
        }
        return history;
    }

    for(const QJsonObject& entry : m_structuredResultHistory) {
        if(entry.value("tool_name").toString() == toolName) {
            history.append(entry.value("result").toObject());
        }
    }

    return history;
}

QJsonObject MainWindow::resultRendererRuntimeContext() const
{
    QJsonObject context{
        {"has_active_raw_view", activeRawDataView() != nullptr},
        {"current_result_selection", m_resultSelectionContext}
    };

    if(IRawDataView* rawBrowser = activeRawDataView()) {
        context.insert("active_raw_file", rawBrowser->filePath());
        context.insert("cursor_sample", rawBrowser->cursorSample());
        context.insert("pixels_per_sample", rawBrowser->pixelsPerSample());
    }

    context.insert("tool_defaults", QJsonObject{
        {"neurokernel.psd_summary", defaultArgumentsForTool("neurokernel.psd_summary")},
        {"neurokernel.find_peak_window", defaultArgumentsForTool("neurokernel.find_peak_window")},
        {"studio.pipeline.run", defaultArgumentsForTool("studio.pipeline.run")}
    });

    return context;
}

void MainWindow::handleResultRendererSelectionContext(const QJsonObject& context)
{
    m_resultSelectionContext = context;
}

QJsonArray MainWindow::resultRendererContracts() const
{
    if(!m_viewProviderRegistry) {
        return QJsonArray();
    }

    return m_viewProviderRegistry->resultRendererDefinitions();
}

QJsonArray MainWindow::extensionSettingsContracts() const
{
    QJsonArray contracts;
    if(!m_viewProviderRegistry) {
        return contracts;
    }

    for(const ExtensionManifest& manifest : m_viewProviderRegistry->manifests()) {
        for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
            contracts.append(QJsonObject{
                {"extension_id", manifest.id},
                {"extension_display_name", manifest.displayName},
                {"id", settingsTab.id},
                {"title", settingsTab.title},
                {"description", settingsTab.description},
                {"fields", settingsTab.fields},
                {"actions", settingsTab.actions}
            });
        }
    }

    return contracts;
}

QJsonArray MainWindow::analysisPipelineContracts() const
{
    if(!m_viewProviderRegistry) {
        return QJsonArray();
    }

    return m_viewProviderRegistry->analysisPipelineDefinitions();
}

QJsonArray MainWindow::extensionSettingsState() const
{
    QJsonArray state;

    if(!m_viewProviderRegistry) {
        return state;
    }

    for(const ExtensionManifest& manifest : m_viewProviderRegistry->manifests()) {
        for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
            QJsonArray fields;
            for(const QJsonValue& value : settingsTab.fields) {
                const QJsonObject field = value.toObject();
                const QString fieldId = field.value("id").toString().trimmed();
                if(fieldId.isEmpty()) {
                    continue;
                }

                fields.append(QJsonObject{
                    {"id", fieldId},
                    {"value", QJsonValue::fromVariant(extensionSettingValue(manifest.id, settingsTab.id, field))},
                    {"default", field.value("default")}
                });
            }

            state.append(QJsonObject{
                {"extension_id", manifest.id},
                {"tab_id", settingsTab.id},
                {"fields", fields}
            });
        }
    }

    return state;
}

QJsonArray MainWindow::extensionSettingsToolDefinitions() const
{
    QJsonArray tools;

    tools.append(QJsonObject{
        {"name", "studio.settings.list"},
        {"description", "List manifest-declared extension settings tabs, fields, and current values."},
        {"input_schema", objectSchema(QJsonObject())},
        {"result_schema", objectSchema(QJsonObject{
             {"settings", arraySchema("Extension Settings Contracts", objectSchema(QJsonObject()))}
         }, QJsonArray{"settings"})}
    });

    tools.append(QJsonObject{
        {"name", "studio.pipelines.list"},
        {"description", "List extension-contributed analysis pipelines, including steps and recommended follow-up actions."},
        {"input_schema", objectSchema(QJsonObject())},
        {"result_schema", objectSchema(QJsonObject{
             {"pipelines", arraySchema("Analysis Pipelines", objectSchema(QJsonObject()))}
         }, QJsonArray{"pipelines"})}
    });

    tools.append(QJsonObject{
        {"name", "studio.pipeline.run"},
        {"description", "Run an extension-contributed analysis pipeline by id, with optional input overrides."},
        {"input_schema", objectSchema(QJsonObject{
             {"pipeline_id", stringSchema("Pipeline ID", QJsonArray(), QString(), "Identifier of the analysis pipeline to execute.")},
             {"inputs", objectSchema(QJsonObject())}
         }, QJsonArray{"pipeline_id"})},
        {"result_schema", objectSchema(QJsonObject{
             {"status", stringSchema("Status", QJsonArray{"queued", "error"})},
             {"pipeline_id", stringSchema("Pipeline ID")},
             {"queued_steps", integerSchema("Queued Steps", 0, 1000, 0)}
         }, QJsonArray{"status", "pipeline_id", "queued_steps"})}
    });

    tools.append(QJsonObject{
        {"name", "studio.pipeline.resume"},
        {"description", "Resume a saved analysis pipeline run from its remaining pending steps."},
        {"input_schema", objectSchema(QJsonObject{
             {"run_id", stringSchema("Run ID", QJsonArray(), QString(), "Identifier of the saved pipeline run to resume.")}
         }, QJsonArray{"run_id"})},
        {"result_schema", objectSchema(QJsonObject{
             {"status", stringSchema("Status", QJsonArray{"queued", "error"})},
             {"run_id", stringSchema("Run ID")},
             {"pipeline_id", stringSchema("Pipeline ID")},
             {"pending_steps", integerSchema("Pending Steps", 0, 1000, 0)}
         }, QJsonArray{"status", "run_id", "pipeline_id", "pending_steps"})}
    });

    tools.append(QJsonObject{
        {"name", "studio.pipeline.rerun_step"},
        {"description", "Rerun a saved pipeline step either exactly as recorded or rehydrated through current defaults."},
        {"input_schema", objectSchema(QJsonObject{
             {"run_id", stringSchema("Run ID", QJsonArray(), QString(), "Identifier of the saved pipeline run.")},
             {"step_number", integerSchema("Step Number", 1, 1000, 1, "1-based pipeline step index.")},
             {"mode", stringSchema("Rerun Mode", QJsonArray{"rehydrate", "exact"}, "rehydrate",
                                    "Use `rehydrate` to rebuild the step from the current pipeline/tool defaults, or `exact` to replay the recorded command.")}
         }, QJsonArray{"run_id", "step_number"})},
        {"result_schema", objectSchema(QJsonObject{
             {"status", stringSchema("Status", QJsonArray{"queued", "error"})},
             {"run_id", stringSchema("Run ID")},
             {"step_number", integerSchema("Step Number", 1, 1000, 1)},
             {"mode", stringSchema("Rerun Mode", QJsonArray{"rehydrate", "exact"})},
             {"command", stringSchema("Command")}
         }, QJsonArray{"status", "run_id", "step_number", "mode", "command"})}
    });

    if(!m_viewProviderRegistry) {
        return tools;
    }

    for(const ExtensionManifest& manifest : m_viewProviderRegistry->manifests()) {
        for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
            for(const QJsonValue& value : settingsTab.fields) {
                const QJsonObject field = value.toObject();
                const QString fieldId = field.value("id").toString().trimmed();
                if(fieldId.isEmpty()) {
                    continue;
                }

                const QString getName = extensionSettingsToolName(manifest.id, settingsTab.id, fieldId, "get");
                const QString setName = extensionSettingsToolName(manifest.id, settingsTab.id, fieldId, "set");
                const QString fieldLabel = field.value("label").toString(fieldId);
                const QString baseDescription = QString("%1 / %2 / %3")
                                                    .arg(manifest.displayName,
                                                         settingsTab.title,
                                                         fieldLabel);

                tools.append(QJsonObject{
                    {"name", getName},
                    {"description", QString("Read extension setting %1.").arg(baseDescription)},
                    {"input_schema", objectSchema(QJsonObject())},
                    {"result_schema", objectSchema(QJsonObject{
                         {"extension_id", stringSchema("Extension ID")},
                         {"tab_id", stringSchema("Settings Tab ID")},
                         {"field_id", stringSchema("Field ID")},
                         {"value", fieldSchemaForSettingsTool(field)}
                     }, QJsonArray{"extension_id", "tab_id", "field_id", "value"})}
                });

                tools.append(QJsonObject{
                    {"name", setName},
                    {"description", QString("Update extension setting %1.").arg(baseDescription)},
                    {"input_schema", objectSchema(QJsonObject{
                         {"value", fieldSchemaForSettingsTool(field)}
                     }, QJsonArray{"value"})},
                    {"result_schema", objectSchema(QJsonObject{
                         {"status", stringSchema("Status", QJsonArray{"ok"})},
                         {"extension_id", stringSchema("Extension ID")},
                         {"tab_id", stringSchema("Settings Tab ID")},
                         {"field_id", stringSchema("Field ID")},
                         {"value", fieldSchemaForSettingsTool(field)}
                     }, QJsonArray{"status", "extension_id", "tab_id", "field_id", "value"})}
                });
            }
        }
    }

    return tools;
}

QJsonObject MainWindow::resolveExtensionSettingsTool(const QString& toolName) const
{
    if(toolName == "studio.settings.list" || !toolName.startsWith("settings.")) {
        return QJsonObject();
    }

    const bool isGet = toolName.endsWith(".get");
    const bool isSet = toolName.endsWith(".set");
    if(!isGet && !isSet) {
        return QJsonObject();
    }

    if(!m_viewProviderRegistry) {
        return QJsonObject();
    }

    for(const ExtensionManifest& manifest : m_viewProviderRegistry->manifests()) {
        for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
            for(const QJsonValue& value : settingsTab.fields) {
                const QJsonObject field = value.toObject();
                const QString fieldId = field.value("id").toString().trimmed();
                if(fieldId.isEmpty()) {
                    continue;
                }

                if(toolName == extensionSettingsToolName(manifest.id, settingsTab.id, fieldId, "get")
                   || toolName == extensionSettingsToolName(manifest.id, settingsTab.id, fieldId, "set")) {
                    return QJsonObject{
                        {"extension_id", manifest.id},
                        {"extension_display_name", manifest.displayName},
                        {"tab_id", settingsTab.id},
                        {"tab_title", settingsTab.title},
                        {"field", field},
                        {"mode", isGet ? "get" : "set"}
                    };
                }
            }
        }
    }

    return QJsonObject();
}

QVariant MainWindow::extensionSettingValue(const QString& extensionId,
                                          const QString& tabId,
                                          const QJsonObject& field) const
{
    const QString fieldId = field.value("id").toString().trimmed();
    if(fieldId.isEmpty()) {
        return QVariant();
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    return settings.value(QString("extensions/settings/%1/%2/%3").arg(extensionId, tabId, fieldId),
                          field.value("default").toVariant());
}

QStringList MainWindow::candidateSettingFieldIds(const QString& inputName) const
{
    const QString trimmedName = inputName.trimmed().toLower();
    if(trimmedName.isEmpty()) {
        return QStringList();
    }

    QStringList candidates{
        trimmedName,
        QString("default_%1").arg(trimmedName),
        QString("preferred_%1").arg(trimmedName)
    };

    if(trimmedName == QLatin1String("match")) {
        candidates << "preferred_channel_family";
    }

    if(trimmedName == QLatin1String("window_samples")) {
        candidates << "default_peak_window_samples";
    }

    return candidates;
}

QJsonObject MainWindow::applyExtensionSettingDefaults(const QString& extensionId,
                                                      const QJsonObject& schemaProperties,
                                                      const QJsonObject& currentValues) const
{
    if(extensionId.trimmed().isEmpty() || !m_viewProviderRegistry) {
        return currentValues;
    }

    QJsonObject mergedValues = currentValues;

    for(const ExtensionManifest& manifest : m_viewProviderRegistry->manifests()) {
        if(manifest.id != extensionId) {
            continue;
        }

        for(auto propertyIt = schemaProperties.constBegin(); propertyIt != schemaProperties.constEnd(); ++propertyIt) {
            const QString inputName = propertyIt.key().trimmed();
            if(inputName.isEmpty()) {
                continue;
            }

            const QStringList candidateIds = candidateSettingFieldIds(inputName);
            for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
                bool matched = false;
                for(const QJsonValue& value : settingsTab.fields) {
                    const QJsonObject field = value.toObject();
                    const QString fieldId = field.value("id").toString().trimmed().toLower();
                    if(fieldId.isEmpty()) {
                        continue;
                    }

                    if(candidateIds.contains(fieldId)
                       || fieldId.endsWith(QString("_%1").arg(inputName.toLower()))) {
                        mergedValues.insert(inputName,
                                            QJsonValue::fromVariant(extensionSettingValue(manifest.id, settingsTab.id, field)));
                        matched = true;
                        break;
                    }
                }

                if(matched) {
                    break;
                }
            }
        }

        break;
    }

    return mergedValues;
}

QStringList MainWindow::extensionIdsForTool(const QString& toolName) const
{
    QStringList extensionIds;
    if(toolName.trimmed().isEmpty() || !m_viewProviderRegistry) {
        return extensionIds;
    }

    for(const ExtensionManifest& manifest : m_viewProviderRegistry->manifests()) {
        bool matchesTool = false;

        for(const ResultRendererContribution& renderer : manifest.resultRenderers) {
            if(renderer.toolNames.contains(toolName)) {
                matchesTool = true;
                break;
            }
        }

        if(!matchesTool) {
            for(const AnalysisPipelineContribution& pipeline : manifest.analysisPipelines) {
                for(const QJsonValue& stepValue : pipeline.steps) {
                    if(stepValue.toObject().value("tool_name").toString().trimmed() == toolName) {
                        matchesTool = true;
                        break;
                    }
                }

                if(matchesTool) {
                    break;
                }
            }
        }

        if(matchesTool) {
            extensionIds.append(manifest.id);
        }
    }

    return extensionIds;
}

QJsonObject MainWindow::defaultInputsForPipeline(const QJsonObject& pipeline) const
{
    QJsonObject defaults;
    const QJsonObject properties = pipeline.value("input_schema").toObject().value("properties").toObject();
    for(auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        const QJsonObject property = it.value().toObject();
        if(property.contains("default")) {
            defaults.insert(it.key(), property.value("default"));
        }
    }

    return applyExtensionSettingDefaults(pipeline.value("extension_id").toString(), properties, defaults);
}

QJsonValue MainWindow::pipelineTemplateValueToJson(const QString& resolvedText) const
{
    const QString trimmed = resolvedText.trimmed();
    if(trimmed.isEmpty()) {
        return QJsonValue(QString());
    }

    if(trimmed == QLatin1String("true")) {
        return QJsonValue(true);
    }
    if(trimmed == QLatin1String("false")) {
        return QJsonValue(false);
    }
    if(trimmed == QLatin1String("null")) {
        return QJsonValue(QJsonValue::Null);
    }

    if(QRegularExpression("^-?\\d+$").match(trimmed).hasMatch()) {
        return QJsonValue(trimmed.toDouble());
    }
    if(QRegularExpression("^-?\\d+(?:\\.\\d+)?$").match(trimmed).hasMatch()) {
        return QJsonValue(trimmed.toDouble());
    }

    if(trimmed.startsWith('{') || trimmed.startsWith('[') || trimmed.startsWith('"')) {
        QJsonParseError error;
        const QByteArray payload = trimmed.startsWith('"')
            ? QByteArray("{\"value\":") + trimmed.toUtf8() + QByteArray("}")
            : trimmed.toUtf8();
        const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
        if(error.error == QJsonParseError::NoError) {
            if(trimmed.startsWith('"')) {
                return document.object().value("value");
            }
            if(document.isObject()) {
                return document.object();
            }
            if(document.isArray()) {
                return document.array();
            }
        }
    }

    return QJsonValue(trimmed);
}

QJsonObject MainWindow::resolvePipelineStepArguments(const QJsonObject& step,
                                                    const QJsonObject& pipelineInputs) const
{
    QJsonObject arguments;
    const QJsonObject argumentsTemplate = step.value("arguments_template").toObject();
    for(auto it = argumentsTemplate.constBegin(); it != argumentsTemplate.constEnd(); ++it) {
        if(it.value().isString()) {
            arguments.insert(it.key(), pipelineTemplateValueToJson(resolvePipelineCommandTemplate(it.value().toString(),
                                                                                                pipelineInputs)));
        } else {
            arguments.insert(it.key(), it.value());
        }
    }

    return arguments;
}

QString MainWindow::buildToolCallCommand(const QString& toolName, const QJsonObject& arguments) const
{
    return QString("tools.call %1 %2")
        .arg(toolName,
             QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact)));
}

QJsonObject MainWindow::pipelineRunArtifact(const QString& runId) const
{
    const QString trimmedRunId = runId.trimmed();
    if(trimmedRunId.isEmpty()) {
        return QJsonObject();
    }

    for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject entry = m_structuredResultHistory.at(i);
        if(entry.value("tool_name").toString() == QLatin1String("studio.pipeline.run")
           && entry.value("result").toObject().value("run_id").toString() == trimmedRunId) {
            return entry.value("result").toObject();
        }
    }

    return QJsonObject();
}

QString MainWindow::rerunPipelineStepCommand(const QString& runId,
                                             int stepNumber,
                                             const QString& mode,
                                             QString* errorMessage) const
{
    const QJsonObject artifact = pipelineRunArtifact(runId);
    if(artifact.isEmpty()) {
        if(errorMessage) {
            *errorMessage = QString("Unknown pipeline run id: %1").arg(runId);
        }
        return QString();
    }

    const QJsonArray executedSteps = artifact.value("steps").toArray();
    QJsonObject executedStep;
    for(const QJsonValue& value : executedSteps) {
        const QJsonObject candidate = value.toObject();
        if(candidate.value("step_number").toInt() == stepNumber) {
            executedStep = candidate;
            break;
        }
    }

    if(executedStep.isEmpty()) {
        if(errorMessage) {
            *errorMessage = QString("Pipeline run %1 has no step %2.").arg(runId).arg(stepNumber);
        }
        return QString();
    }

    if(mode.trimmed().compare("exact", Qt::CaseInsensitive) == 0) {
        const QString exactCommand = executedStep.value("command").toString().trimmed();
        if(exactCommand.isEmpty() && errorMessage) {
            *errorMessage = QString("Pipeline step %1 has no exact replay command.").arg(stepNumber);
        }
        return exactCommand;
    }

    const QString pipelineId = artifact.value("pipeline_id").toString().trimmed();
    const QJsonObject pipeline = analysisPipelineContract(pipelineId);
    if(pipeline.isEmpty()) {
        if(errorMessage) {
            *errorMessage = QString("Unknown pipeline contract for %1.").arg(pipelineId);
        }
        return QString();
    }

    const QJsonArray stepContracts = pipeline.value("steps").toArray();
    if(stepNumber <= 0 || stepNumber > stepContracts.size()) {
        if(errorMessage) {
            *errorMessage = QString("Pipeline contract %1 has no step %2.").arg(pipelineId).arg(stepNumber);
        }
        return QString();
    }

    const QJsonObject stepContract = stepContracts.at(stepNumber - 1).toObject();
    const QString toolName = stepContract.value("tool_name").toString().trimmed();
    if(toolName.isEmpty()) {
        if(errorMessage) {
            *errorMessage = QString("Pipeline step %1 is missing a tool name.").arg(stepNumber);
        }
        return QString();
    }

    QJsonObject pipelineInputs = defaultInputsForPipeline(pipeline);
    const QJsonObject inputOverrides = artifact.value("input_overrides").toObject();
    for(auto it = inputOverrides.constBegin(); it != inputOverrides.constEnd(); ++it) {
        pipelineInputs.insert(it.key(), it.value());
    }
    const QJsonObject rehydratedArguments = resolvePipelineStepArguments(stepContract, pipelineInputs);
    const QString command = buildToolCallCommand(toolName, rehydratedArguments);
    if(command.contains("${")) {
        if(errorMessage) {
            *errorMessage = QString("Pipeline step %1 still contains unresolved placeholders.").arg(stepNumber);
        }
        return QString();
    }

    return command;
}

QJsonObject MainWindow::analysisPipelineContract(const QString& pipelineId) const
{
    const QJsonArray pipelines = analysisPipelineContracts();
    for(const QJsonValue& value : pipelines) {
        const QJsonObject pipeline = value.toObject();
        if(pipeline.value("id").toString() == pipelineId) {
            return pipeline;
        }
    }

    return QJsonObject();
}

QString MainWindow::resolvePipelineCommandTemplate(const QString& commandTemplate,
                                                   const QJsonObject& pipelineInputs) const
{
    QString resolved = commandTemplate;
    resolved = resolvePlannerReferences(resolved);

    for(auto it = pipelineInputs.constBegin(); it != pipelineInputs.constEnd(); ++it) {
        const QString placeholder = QString("${%1}").arg(it.key());
        if(resolved.contains(placeholder)) {
            QString replacement;
            if(it.value().isString()) {
                replacement = it.value().toString();
            } else if(it.value().isDouble()) {
                replacement = QString::number(it.value().toDouble(), 'g', 12);
            } else if(it.value().isBool()) {
                replacement = it.value().toBool() ? "true" : "false";
            } else {
                replacement = QString::fromUtf8(QJsonDocument::fromVariant(it.value().toVariant()).toJson(QJsonDocument::Compact));
            }
            resolved.replace(placeholder, replacement);
        }
    }

    if(resolved.contains("${last_peak_channel}")) {
        const QString peakChannel = m_lastToolResult.value("peak_channel").toString().trimmed();
        if(!peakChannel.isEmpty()) {
            resolved.replace("${last_peak_channel}", peakChannel);
        }
    }

    return resolved;
}

bool MainWindow::resumeAnalysisPipeline(const QJsonObject& artifactResult)
{
    const QString runId = artifactResult.value("run_id").toString().trimmed();
    const QString pipelineId = artifactResult.value("pipeline_id").toString().trimmed();
    const QString displayName = artifactResult.value("display_name").toString(pipelineId);
    const QStringList pendingCommands = qvariant_cast<QStringList>(artifactResult.value("pending_commands").toVariant());
    if(runId.isEmpty() || pipelineId.isEmpty() || pendingCommands.isEmpty()) {
        appendProblemMessage(QString("Cannot resume pipeline %1: missing run id or pending commands.").arg(displayName));
        return false;
    }

    if(!m_activePipelineId.isEmpty()) {
        appendProblemMessage(QString("Cannot resume %1 while pipeline %2 is still active.")
                                 .arg(displayName, m_activePipelineDisplayName));
        return false;
    }

    m_activePipelineId = pipelineId;
    m_activePipelineRunId = runId;
    m_activePipelineDisplayName = displayName;
    m_activePipelineInputs = artifactResult.value("inputs").toObject();
    m_activePipelineInputOverrides = artifactResult.value("input_overrides").toObject();
    m_pendingPipelineCommands = pendingCommands;
    m_activePipelineStepHistory = artifactResult.value("steps").toArray();
    m_activePipelineTotalSteps = artifactResult.value("total_steps").toInt(m_activePipelineStepHistory.size() + m_pendingPipelineCommands.size());
    m_activePipelineLastStatus = QString("Resumed with %1 pending steps.").arg(m_pendingPipelineCommands.size());

    appendOutputMessage(QString("Resuming analysis pipeline %1 with %2 pending steps.")
                            .arg(m_activePipelineDisplayName)
                            .arg(m_pendingPipelineCommands.size()));
    appendTerminalMessage(QString("> Pipeline %1 resumed with %2 pending steps.")
                              .arg(m_activePipelineDisplayName)
                              .arg(m_pendingPipelineCommands.size()));
    updateActivePipelineArtifact("resumed");
    continuePendingPipelineExecution();
    return true;
}

bool MainWindow::executeAnalysisPipeline(const QString& pipelineId,
                                         const QJsonObject& pipelineInputs,
                                         const QJsonObject& inputOverrides)
{
    const QJsonObject pipeline = analysisPipelineContract(pipelineId);
    if(pipeline.isEmpty()) {
        appendProblemMessage(QString("Unknown analysis pipeline: %1").arg(pipelineId));
        return false;
    }

    QJsonArray preparedSteps;
    const QJsonArray steps = pipeline.value("steps").toArray();
    for(int index = 0; index < steps.size(); ++index) {
        const QJsonObject step = steps.at(index).toObject();
        const QString toolName = step.value("tool_name").toString().trimmed();
        if(toolName.isEmpty()) {
            continue;
        }

        const QJsonObject resolvedArguments = resolvePipelineStepArguments(step, pipelineInputs);
        const QString command = buildToolCallCommand(toolName, resolvedArguments);
        preparedSteps.append(QJsonObject{
            {"step_number", index + 1},
            {"tool_name", toolName},
            {"arguments", resolvedArguments},
            {"arguments_template", step.value("arguments_template").toObject()},
            {"replay_command", command}
        });
    }

    if(preparedSteps.isEmpty()) {
        appendProblemMessage(QString("Pipeline %1 did not define any executable steps.").arg(pipelineId));
        return false;
    }

    m_activePipelineId = pipelineId;
    m_activePipelineRunId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_activePipelineDisplayName = pipeline.value("display_name").toString(pipelineId);
    m_activePipelineInputs = pipelineInputs;
    m_activePipelineInputOverrides = inputOverrides;
    m_pendingPipelineCommands.clear();
    for(const QJsonValue& value : preparedSteps) {
        m_pendingPipelineCommands.append(value.toObject().value("replay_command").toString());
    }
    m_activePipelineStepHistory = QJsonArray();
    m_activePipelineTotalSteps = preparedSteps.size();
    m_activePipelineLastStatus = QString("Queued %1 steps.").arg(preparedSteps.size());

    appendOutputMessage(QString("Starting analysis pipeline %1 with %2 steps.")
                            .arg(m_activePipelineDisplayName)
                            .arg(preparedSteps.size()));
    appendTerminalMessage(QString("> Pipeline %1 queued with %2 steps.")
                              .arg(m_activePipelineDisplayName)
                              .arg(preparedSteps.size()));
    updateActivePipelineArtifact("queued");
    continuePendingPipelineExecution();
    return true;
}

void MainWindow::continuePendingPipelineExecution()
{
    if(m_isAdvancingPipeline) {
        return;
    }

    if(m_pendingPipelineCommands.isEmpty()) {
        if(!m_activePipelineId.isEmpty()) {
            appendOutputMessage(QString("Analysis pipeline %1 completed.").arg(m_activePipelineDisplayName));
            appendTerminalMessage(QString("> Pipeline %1 completed.").arg(m_activePipelineDisplayName));
            m_activePipelineLastStatus = "Completed.";
            updateActivePipelineArtifact("completed");
            rebuildSkillsExplorer();
        }
        m_activePipelineId.clear();
        m_activePipelineRunId.clear();
        m_activePipelineDisplayName.clear();
        m_activePipelineInputs = QJsonObject();
        m_activePipelineInputOverrides = QJsonObject();
        m_activePipelineStepHistory = QJsonArray();
        m_activePipelineTotalSteps = 0;
        return;
    }

    m_isAdvancingPipeline = true;
    const int remainingBefore = m_pendingPipelineCommands.size();
    const QString commandTemplate = m_pendingPipelineCommands.takeFirst();
    const QString resolvedCommand = resolvePipelineCommandTemplate(commandTemplate, m_activePipelineInputs);

    if(resolvedCommand.contains("${")) {
        appendProblemMessage(QString("Pipeline %1 could not resolve step placeholder(s): %2")
                                 .arg(m_activePipelineDisplayName, resolvedCommand));
        failActivePipeline(QString("Failed to resolve placeholders in %1").arg(resolvedCommand));
        m_isAdvancingPipeline = false;
        return;
    }

    const int stepNumber = m_activePipelineTotalSteps - remainingBefore + 1;
    m_activePipelineStepHistory.append(QJsonObject{
        {"step_number", stepNumber},
        {"pipeline_id", m_activePipelineId},
        {"run_id", m_activePipelineRunId},
        {"command", resolvedCommand},
        {"replay_command", commandTemplate},
        {"status", "running"},
        {"started_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    });
    const QString note = QString("Pipeline %1 | Step %2: %3")
                             .arg(m_activePipelineDisplayName)
                             .arg(stepNumber)
                             .arg(resolvedCommand);
    m_activePipelineLastStatus = note;
    updateActivePipelineArtifact("running", resolvedCommand);
    rebuildSkillsExplorer();
    appendTerminalMessage(QString("> %1").arg(note));
    m_agentChatDock->appendTranscript(QString("Planner> %1").arg(note));

    sendToolCall(resolvedCommand);
    m_isAdvancingPipeline = false;
}

QWidget* MainWindow::ensureBottomResultRenderer(const QString& toolName)
{
    const IResultRendererFactory* factory = ResultRendererFactoryRegistry::instance().factoryForToolName(toolName);
    if(!factory) {
        if(m_resultsExtensionRenderer) {
            m_resultsStack->removeWidget(m_resultsExtensionRenderer);
            m_resultsExtensionRenderer->deleteLater();
            m_resultsExtensionRenderer = nullptr;
        }
        return nullptr;
    }

    if(m_resultsExtensionRenderer
       && m_resultsExtensionRenderer->property("mne_result_renderer_tool").toString() == toolName) {
        return m_resultsExtensionRenderer;
    }

    if(m_resultsExtensionRenderer) {
        m_resultsStack->removeWidget(m_resultsExtensionRenderer);
        m_resultsExtensionRenderer->deleteLater();
        m_resultsExtensionRenderer = nullptr;
    }

    m_resultsExtensionRenderer = factory->createRenderer(m_resultsTab);
    if(!m_resultsExtensionRenderer) {
        return nullptr;
    }

    m_resultsExtensionRenderer->setProperty("mne_result_renderer_tool", toolName);
    if(hasQtSignal(m_resultsExtensionRenderer, "toolCommandRequested(QString)")) {
        QObject::connect(m_resultsExtensionRenderer,
                         SIGNAL(toolCommandRequested(QString)),
                         this,
                         SLOT(handleResultRendererToolCommand(QString)));
    }
    if(hasQtSignal(m_resultsExtensionRenderer, "selectionContextChanged(QJsonObject)")) {
        QObject::connect(m_resultsExtensionRenderer,
                         SIGNAL(selectionContextChanged(QJsonObject)),
                         this,
                         SLOT(handleResultRendererSelectionContext(QJsonObject)));
    }
    m_resultsStack->addWidget(m_resultsExtensionRenderer);
    return m_resultsExtensionRenderer;
}

void MainWindow::updateStructuredResultView(const QString& toolName, const QJsonObject& result)
{
    if(!m_resultsTree || !m_resultsTitleLabel || !m_resultsStack || !m_resultsTable) {
        return;
    }

    m_resultsTitleLabel->setText(QString("Latest structured result: %1").arg(toolName));
    m_resultsCurrentToolName = toolName;
    m_resultSelectionContext = QJsonObject{
        {"tool_name", toolName}
    };
    m_resultsTree->clear();
    m_resultsTable->setRowCount(0);
    m_resultsStack->setCurrentWidget(m_resultsTree);

    if(result.isEmpty()) {
        m_resultSelectionContext = QJsonObject();
        m_resultsTree->addTopLevelItem(new QTreeWidgetItem(QStringList()
                                                           << "hint"
                                                           << "Run a tool or open an Analysis artifact to populate Recent Results."));
        return;
    }

    if(QWidget* renderer = ensureBottomResultRenderer(toolName)) {
        if(IResultRendererWidget* resultRenderer = dynamic_cast<IResultRendererWidget*>(renderer)) {
            resultRenderer->setResultHistory(resultHistoryForTool(toolName));
            resultRenderer->setRuntimeContext(resultRendererRuntimeContext());
            resultRenderer->setResult(toolName, result);
            m_resultsStack->setCurrentWidget(renderer);
            m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
            return;
        }
    }

    m_resultSelectionContext = QJsonObject{
        {"tool_name", toolName},
        {"selection_kind", "result"},
        {"has_primary_action", false},
        {"has_secondary_action", false}
    };

    for(auto it = result.constBegin(); it != result.constEnd(); ++it) {
        m_resultsTree->addTopLevelItem(buildJsonTreeItem(it.key(), it.value()));
    }
    m_resultsTree->expandToDepth(1);
    for(int column = 0; column < m_resultsTree->columnCount(); ++column) {
        m_resultsTree->resizeColumnToContents(column);
    }
    m_bottomPanelTabs->setCurrentWidget(m_resultsTab);
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

    if(containsAny({"list settings", "show settings", "available settings", "which settings"})) {
        planned = true;
        plannedCommand = "tools.call studio.settings.list {}";
        return "Mapped your request to `studio.settings.list`.";
    }

    if(containsAny({"list pipelines", "show pipelines", "available pipelines", "which pipelines", "list workflows", "available workflows"})) {
        planned = true;
        plannedCommand = "tools.call studio.pipelines.list {}";
        return "Mapped your request to `studio.pipelines.list`.";
    }

    if(containsAny({"run pipeline", "start pipeline", "execute pipeline", "run workflow", "start workflow"})) {
        const QJsonArray pipelines = analysisPipelineContracts();
        for(const QJsonValue& value : pipelines) {
            const QJsonObject pipeline = value.toObject();
            const QString pipelineId = pipeline.value("id").toString();
            const QString displayName = pipeline.value("display_name").toString().toLower();
            if(lower.contains(pipelineId.toLower()) || (!displayName.isEmpty() && lower.contains(displayName))) {
                planned = true;
                plannedCommand = QString("tools.call studio.pipeline.run {\"pipeline_id\":\"%1\"}").arg(pipelineId);
                return QString("Mapped your request to `studio.pipeline.run` for %1.").arg(pipelineId);
            }
        }
    }

    if(containsAny({"resume pipeline", "continue pipeline", "resume workflow", "continue workflow"})) {
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            const QJsonObject entry = m_structuredResultHistory.at(i);
            if(entry.value("tool_name").toString() != QLatin1String("studio.pipeline.run")) {
                continue;
            }
            const QJsonObject result = entry.value("result").toObject();
            if(result.value("pending_steps").toInt() > 0) {
                planned = true;
                plannedCommand = QString("tools.call studio.pipeline.resume {\"run_id\":\"%1\"}")
                                     .arg(result.value("run_id").toString());
                return QString("Mapped your request to `studio.pipeline.resume` for %1.")
                    .arg(result.value("display_name").toString(result.value("pipeline_id").toString()));
            }
        }
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
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"views", QJsonArray::fromStringList(views)}
        }, "workbench"));
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    if(toolName == "studio.settings.list") {
        const QJsonArray settingsContracts = extensionSettingsContracts();
        const QJsonArray settingsState = extensionSettingsState();
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"settings", settingsContracts},
            {"state", settingsState}
        }, "workbench"));
        return QString("Extension settings contracts: %1 tabs across %2 extensions.")
            .arg(settingsContracts.size())
            .arg(m_viewProviderRegistry ? m_viewProviderRegistry->manifests().size() : 0);
    }

    if(toolName == "studio.pipelines.list") {
        const QJsonArray pipelineContracts = analysisPipelineContracts();
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"pipelines", pipelineContracts}
        }, "workbench"));
        return QString("Analysis pipelines: %1").arg(pipelineContracts.size());
    }

    if(toolName == "studio.pipeline.run") {
        const QString pipelineId = arguments.value("pipeline_id").toString().trimmed();
        if(pipelineId.isEmpty()) {
            return "Tool studio.pipeline.run requires {\"pipeline_id\": \"...\"}.";
        }

        const QJsonObject pipeline = analysisPipelineContract(pipelineId);
        if(pipeline.isEmpty()) {
            return QString("Unknown analysis pipeline: %1").arg(pipelineId);
        }

        QJsonObject pipelineInputs = defaultInputsForPipeline(pipeline);
        const QJsonObject requestedInputs = arguments.value("inputs").toObject();
        for(auto it = requestedInputs.constBegin(); it != requestedInputs.constEnd(); ++it) {
            pipelineInputs.insert(it.key(), it.value());
        }

        const bool started = executeAnalysisPipeline(pipelineId, pipelineInputs, requestedInputs);
        if(!started) {
            rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
                {"tool_name", toolName},
                {"status", QString("error")},
                {"pipeline_id", pipelineId},
                {"display_name", pipeline.value("display_name").toString(pipelineId)},
                {"queued_steps", pipeline.value("steps").toArray().size()},
                {"inputs", pipelineInputs},
                {"message", QString("Failed to queue pipeline %1").arg(pipeline.value("display_name").toString(pipelineId))}
            }, "workbench"));
        }
        return started
            ? QString("Queued analysis pipeline %1.").arg(pipeline.value("display_name").toString(pipelineId))
            : QString("Failed to queue analysis pipeline %1.").arg(pipelineId);
    }

    if(toolName == "studio.pipeline.resume") {
        const QString runId = arguments.value("run_id").toString().trimmed();
        if(runId.isEmpty()) {
            return "Tool studio.pipeline.resume requires {\"run_id\": \"...\"}.";
        }

        QJsonObject artifactResult;
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            const QJsonObject entry = m_structuredResultHistory.at(i);
            if(entry.value("tool_name").toString() == QLatin1String("studio.pipeline.run")
               && entry.value("result").toObject().value("run_id").toString() == runId) {
                artifactResult = entry.value("result").toObject();
                break;
            }
        }

        if(artifactResult.isEmpty()) {
            return QString("Unknown pipeline run id: %1").arg(runId);
        }

        const bool started = resumeAnalysisPipeline(artifactResult);
        if(!started) {
            rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
                {"status", QString("error")},
                {"run_id", runId},
                {"pipeline_id", artifactResult.value("pipeline_id").toString()},
                {"pending_steps", artifactResult.value("pending_steps").toInt()},
                {"message", QString("Failed to resume pipeline run %1").arg(runId)}
            }, "workbench"));
        }
        return started
            ? QString("Resumed analysis pipeline %1.")
                  .arg(artifactResult.value("display_name").toString(artifactResult.value("pipeline_id").toString()))
            : QString("Failed to resume analysis pipeline %1.").arg(runId);
    }

    if(toolName == "studio.pipeline.rerun_step") {
        const QString runId = arguments.value("run_id").toString().trimmed();
        const int stepNumber = arguments.value("step_number").toInt(-1);
        const QString mode = arguments.value("mode").toString("rehydrate").trimmed().toLower();
        if(runId.isEmpty() || stepNumber <= 0) {
            return "Tool studio.pipeline.rerun_step requires {\"run_id\": \"...\", \"step_number\": <int>}.";
        }

        QString errorMessage;
        const QString rerunCommand = rerunPipelineStepCommand(runId, stepNumber, mode, &errorMessage);
        if(rerunCommand.isEmpty()) {
            rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
                {"tool_name", toolName},
                {"status", "error"},
                {"run_id", runId},
                {"step_number", stepNumber},
                {"mode", mode},
                {"message", errorMessage}
            }, "workbench"));
            return errorMessage.isEmpty() ? "Failed to rerun pipeline step." : errorMessage;
        }

        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"status", "queued"},
            {"run_id", runId},
            {"step_number", stepNumber},
            {"mode", mode},
            {"command", rerunCommand},
            {"message", QString("Queued pipeline step %1 in %2 mode.").arg(stepNumber).arg(mode)}
        }, "workbench"));
        appendTerminalMessage(QString("$ %1").arg(rerunCommand));
        sendToolCall(rerunCommand);
        return QString("Queued pipeline step %1 using %2 mode.").arg(stepNumber).arg(mode);
    }

    const QJsonObject resolvedSettingsTool = resolveExtensionSettingsTool(toolName);
    if(!resolvedSettingsTool.isEmpty()) {
        const QString extensionId = resolvedSettingsTool.value("extension_id").toString();
        const QString tabId = resolvedSettingsTool.value("tab_id").toString();
        const QString mode = resolvedSettingsTool.value("mode").toString();
        const QJsonObject field = resolvedSettingsTool.value("field").toObject();
        const QString fieldId = field.value("id").toString();
        const QVariant currentValue = extensionSettingValue(extensionId, tabId, field);

        if(mode == "get") {
            rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
                {"tool_name", toolName},
                {"extension_id", extensionId},
                {"tab_id", tabId},
                {"field_id", fieldId},
                {"value", QJsonValue::fromVariant(currentValue)}
            }, "workbench"));
            return QString("%1 / %2 / %3 = %4")
                .arg(resolvedSettingsTool.value("extension_display_name").toString(),
                     resolvedSettingsTool.value("tab_title").toString(),
                     field.value("label").toString(fieldId),
                     currentValue.toString());
        }

        if(mode == "set") {
            if(!arguments.contains("value")) {
                return QString("Tool %1 requires {\"value\": ...}.").arg(toolName);
            }

            QVariant nextValue = arguments.value("value").toVariant();
            const QString fieldType = field.value("type").toString("string").trimmed().toLower();
            if(fieldType == "boolean" || fieldType == "bool") {
                nextValue = arguments.value("value").toBool();
            } else if(fieldType == "integer" || fieldType == "int") {
                nextValue = arguments.value("value").toInt();
            } else if(fieldType == "number" || fieldType == "double") {
                nextValue = arguments.value("value").toDouble();
            } else {
                nextValue = arguments.value("value").toString();
            }

            QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
            settings.setValue(QString("extensions/settings/%1/%2/%3").arg(extensionId, tabId, fieldId), nextValue);

            rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
                {"tool_name", toolName},
                {"status", "ok"},
                {"extension_id", extensionId},
                {"tab_id", tabId},
                {"field_id", fieldId},
                {"value", QJsonValue::fromVariant(nextValue)}
            }, "workbench"));

            return QString("Updated %1 / %2 / %3 to %4.")
                .arg(resolvedSettingsTool.value("extension_display_name").toString(),
                     resolvedSettingsTool.value("tab_title").toString(),
                     field.value("label").toString(fieldId),
                     nextValue.toString());
        }
    }

    IRawDataView* rawBrowser = activeRawDataView();
    if(toolName == "view.raw.summary") {
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"summary", rawBrowser ? rawBrowser->summaryText() : QString("No active raw browser.")}
        }, "workbench"));
        return rawBrowser ? rawBrowser->summaryText() : "No active raw browser.";
    }

    if(toolName == "view.raw.state") {
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"state", rawBrowser ? rawBrowser->stateText() : QString("No active raw browser.")}
        }, "workbench"));
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
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"sample", rawBrowser->cursorSample()},
            {"cursor_sample", rawBrowser->cursorSample()}
        }, "workbench"));
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
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"pixels_per_sample", rawBrowser->pixelsPerSample()}
        }, "workbench"));
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
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"session_id", sessionId},
            {"command", commandName},
            {"arguments", arguments}
        }, "workbench"));
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
        return "Commands: help, tools.list, tools.call <tool> {json}, views.list, settings.list, pipelines.list, pipeline.run <id>, pipeline.resume <run_id>, pipeline.rerun_step <run_id> <step>, raw.summary, raw.state, raw.goto_sample <n>, raw.cursor <n>, raw.zoom <px_per_sample>, kernel.raw_stats [window_samples], kernel.channel_stats [window_samples], kernel.psd [window_samples] [match].";
    }

    if(command == "views.list") {
        QStringList views;
        for(int i = 0; i < m_centerTabs->count(); ++i) {
            views << QString("%1: %2").arg(i + 1).arg(m_centerTabs->tabText(i));
        }
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    if(command == "settings.list") {
        return QString("Extension settings tabs available: %1").arg(extensionSettingsContracts().size());
    }

    if(command == "pipelines.list") {
        return QString("Analysis pipelines available: %1").arg(analysisPipelineContracts().size());
    }

    if(command == "pipeline.run") {
        if(parts.size() < 2) {
            return "Usage: pipeline.run <pipeline_id>";
        }

        const QString pipelineId = parts.at(1).trimmed();
        const QJsonObject pipeline = analysisPipelineContract(pipelineId);
        const bool started = executeAnalysisPipeline(pipelineId, defaultInputsForPipeline(pipeline));
        return started
            ? QString("Queued pipeline %1.").arg(pipelineId)
            : QString("Failed to queue pipeline %1.").arg(pipelineId);
    }

    if(command == "pipeline.resume") {
        if(parts.size() < 2) {
            return "Usage: pipeline.resume <run_id>";
        }

        QJsonObject artifactResult;
        const QString runId = parts.at(1).trimmed();
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            const QJsonObject entry = m_structuredResultHistory.at(i);
            if(entry.value("tool_name").toString() == QLatin1String("studio.pipeline.run")
               && entry.value("result").toObject().value("run_id").toString() == runId) {
                artifactResult = entry.value("result").toObject();
                break;
            }
        }
        const bool started = !artifactResult.isEmpty() && resumeAnalysisPipeline(artifactResult);
        return started
            ? QString("Resumed pipeline %1.").arg(artifactResult.value("display_name").toString(artifactResult.value("pipeline_id").toString()))
            : QString("Failed to resume pipeline %1.").arg(runId);
    }

    if(command == "pipeline.rerun_step") {
        if(parts.size() < 3) {
            return "Usage: pipeline.rerun_step <run_id> <step_number> [rehydrate|exact]";
        }

        const QString runId = parts.at(1).trimmed();
        const int stepNumber = parts.at(2).toInt();
        const QString mode = parts.size() >= 4 ? parts.at(3).trimmed().toLower() : QString("rehydrate");
        QString errorMessage;
        const QString rerunCommand = rerunPipelineStepCommand(runId, stepNumber, mode, &errorMessage);
        if(rerunCommand.isEmpty()) {
            return errorMessage.isEmpty() ? "Failed to rerun pipeline step." : errorMessage;
        }

        appendTerminalMessage(QString("$ %1").arg(rerunCommand));
        sendToolCall(rerunCommand);
        return QString("Queued pipeline step %1 using %2 mode.").arg(stepNumber).arg(mode);
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
    QJsonArray tools{
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

    const QJsonArray settingsTools = extensionSettingsToolDefinitions();
    for(const QJsonValue& value : settingsTools) {
        tools.append(value);
    }

    return tools;
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

QJsonObject MainWindow::plannerSafetyMetadata(const QString& toolName) const
{
    const QString trimmedName = toolName.trimmed();
    if(trimmedName.isEmpty()) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "blocked"},
            {"reason", "Unnamed tools are not exposed to the LLM planner."}
        };
    }

    if(trimmedName.endsWith(".set") && trimmedName.startsWith("settings.")) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "high"},
            {"reason", "Mutates persisted extension settings."}
        };
    }

    if(trimmedName.startsWith("view.hosted.")
       || trimmedName.startsWith("dummy3d.")
       || trimmedName.startsWith("extension.")) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "medium"},
            {"reason", "Requires active hosted extension sessions with side effects that are harder for the planner to validate."}
        };
    }

    if(trimmedName == QLatin1String("studio.pipeline.rerun_step")) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "medium"},
            {"reason", "Replays historical pipeline state and is better triggered from explicit user intent."}
        };
    }

    if(trimmedName == QLatin1String("studio.pipeline.resume")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "medium"},
            {"requires_active_context", false},
            {"reason", "Resumes an existing pipeline run and is safe when explicitly chosen from known saved runs."}
        };
    }

    if(trimmedName == QLatin1String("studio.pipeline.run")
       || trimmedName == QLatin1String("view.raw.goto")
       || trimmedName == QLatin1String("view.raw.zoom")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "medium"},
            {"requires_active_context", true},
            {"reason", "State-changing but reversible workbench action."}
        };
    }

    if(trimmedName == QLatin1String("view.raw.summary")
       || trimmedName == QLatin1String("view.raw.state")
       || trimmedName == QLatin1String("studio.views.list")
       || trimmedName == QLatin1String("studio.settings.list")
       || trimmedName == QLatin1String("studio.pipelines.list")
       || trimmedName.endsWith(".get")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "low"},
            {"requires_active_context", trimmedName.startsWith("view.raw.")},
            {"reason", "Read-only inspection tool."}
        };
    }

    if(trimmedName.startsWith("neurokernel.")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "low"},
            {"requires_active_context", true},
            {"reason", "Analytical computation on the active dataset without mutating persistent workspace state."}
        };
    }

    return QJsonObject{
        {"planner_safe", false},
        {"risk_level", "blocked"},
        {"reason", "Tool is outside the curated planner-safe profile."}
    };
}

QJsonObject MainWindow::plannerReadinessMetadata(const QString& toolName) const
{
    const QString trimmedName = toolName.trimmed();
    const bool hasRawBrowser = activeRawDataView() != nullptr;
    const QJsonObject hostedSession = activeHostedViewSession();
    const QJsonArray pipelines = analysisPipelineContracts();
    const QString selectedChannelName = m_resultSelectionContext.value("selected_channel_name").toString().trimmed();
    const QJsonObject selectedPipelineStep = m_resultSelectionContext.value("selected_pipeline_step").toObject();
    const QString selectedPipelineRunId = m_resultSelectionContext.value("selected_pipeline_run_id").toString().trimmed();
    const QString selectedPipelineId = m_resultSelectionContext.value("selected_pipeline_id").toString().trimmed();

    int resumablePipelineRuns = 0;
    int rerunnablePipelineRuns = 0;
    for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
        const QJsonObject entry = m_structuredResultHistory.at(i);
        if(entry.value("tool_name").toString() != QLatin1String("studio.pipeline.run")) {
            continue;
        }

        const QJsonObject result = entry.value("result").toObject();
        if(result.value("pending_steps").toInt() > 0) {
            ++resumablePipelineRuns;
        }
        if(!result.value("steps").toArray().isEmpty()) {
            ++rerunnablePipelineRuns;
        }
    }

    auto readyEnvelope = [](bool ready,
                            const QString& reason,
                            const QJsonArray& requiredContext = QJsonArray(),
                            const QJsonObject& details = QJsonObject()) {
        QJsonObject readiness{
            {"planner_ready", ready},
            {"readiness_reason", reason},
            {"required_context", requiredContext}
        };

        if(!details.isEmpty()) {
            readiness.insert("readiness_details", details);
        }

        return readiness;
    };

    if(trimmedName.isEmpty()) {
        return readyEnvelope(false, "Tool has no stable name and cannot be reasoned about.");
    }

    if(trimmedName == QLatin1String("studio.views.list")
       || trimmedName == QLatin1String("studio.settings.list")
       || trimmedName == QLatin1String("studio.pipelines.list")
       || trimmedName.endsWith(".get")) {
        return readyEnvelope(true,
                             "Available without additional workspace context.",
                             QJsonArray(),
                             QJsonObject{{"has_raw_browser", hasRawBrowser},
                                         {"analysis_pipeline_count", pipelines.size()}});
    }

    if(trimmedName.startsWith("view.raw.") || trimmedName.startsWith("neurokernel.")) {
        QJsonObject details{{"has_raw_browser", hasRawBrowser}};
        QString reason = hasRawBrowser
            ? "Active raw browser is available."
            : "Requires an active raw browser with a loaded dataset.";
        if(trimmedName == QLatin1String("neurokernel.find_peak_window") && hasRawBrowser && !selectedChannelName.isEmpty()) {
            details.insert("selected_channel_name", selectedChannelName);
            details.insert("selection_grounded", true);
            reason = QString("Active raw browser is available and the selected channel `%1` grounds a peak follow-up.")
                         .arg(selectedChannelName);
        }

        return readyEnvelope(hasRawBrowser,
                             reason,
                             QJsonArray{"active_raw_browser"},
                             details);
    }

    if(trimmedName == QLatin1String("studio.pipeline.run")) {
        const bool hasPipelines = !pipelines.isEmpty();
        const bool ready = hasPipelines && hasRawBrowser;
        QString reason = "Ready to run manifest-declared pipelines against the active dataset.";
        if(!hasPipelines) {
            reason = "No manifest-declared analysis pipelines are currently available.";
        } else if(!hasRawBrowser) {
            reason = "Requires an active raw browser before dataset-bound pipelines can run.";
        }

        return readyEnvelope(ready,
                             reason,
                             QJsonArray{"analysis_pipeline_contract", "active_raw_browser"},
                             QJsonObject{{"analysis_pipeline_count", pipelines.size()},
                                         {"has_raw_browser", hasRawBrowser}});
    }

    if(trimmedName == QLatin1String("studio.pipeline.resume")) {
        const bool hasSelectedPendingRun = !selectedPipelineRunId.isEmpty()
            && pipelineRunArtifact(selectedPipelineRunId).value("pending_steps").toInt() > 0;
        const bool ready = resumablePipelineRuns > 0;
        QJsonObject details{{"resumable_run_count", resumablePipelineRuns}};
        QString reason = ready
            ? "Saved pipeline runs with pending steps are available."
            : "No saved pipeline runs currently have pending steps to resume.";
        if(hasSelectedPendingRun) {
            details.insert("selected_pipeline_run_id", selectedPipelineRunId);
            details.insert("selected_pipeline_id", selectedPipelineId);
            details.insert("selection_grounded", true);
            reason = QString("The selected pipeline run `%1` still has pending steps and can be resumed.")
                         .arg(selectedPipelineRunId);
        }
        return readyEnvelope(ready,
                             reason,
                             QJsonArray{"resumable_pipeline_run"},
                             details);
    }

    if(trimmedName == QLatin1String("studio.pipeline.rerun_step")) {
        const bool ready = rerunnablePipelineRuns > 0;
        QJsonObject details{{"rerunnable_run_count", rerunnablePipelineRuns}};
        QString reason = ready
            ? "Saved pipeline history contains executable steps."
            : "No saved pipeline steps are available to rerun.";
        if(!selectedPipelineStep.isEmpty()) {
            details.insert("selected_pipeline_step", selectedPipelineStep);
            details.insert("selection_grounded", true);
            reason = QString("The selected pipeline step #%1 is available for rerun.")
                         .arg(selectedPipelineStep.value("step_number").toInt());
        }
        return readyEnvelope(ready,
                             reason,
                             QJsonArray{"saved_pipeline_step"},
                             details);
    }

    if(trimmedName.startsWith("view.hosted.")) {
        const bool ready = !hostedSession.isEmpty();
        return readyEnvelope(ready,
                             ready
                                 ? "Active hosted extension session is available."
                                 : "Requires an active hosted extension view session.",
                             QJsonArray{"active_hosted_view"},
                             QJsonObject{{"has_hosted_session", ready},
                                         {"session_id", hostedSession.value("session_id").toString()}});
    }

    if(trimmedName.startsWith("settings.")) {
        const bool ready = !resolveExtensionSettingsTool(trimmedName).isEmpty();
        return readyEnvelope(ready,
                             ready
                                 ? "Resolved to a manifest-declared extension settings field."
                                 : "Referenced settings field is not available in the loaded extension manifests.",
                             QJsonArray{"extension_settings_contract"},
                             QJsonObject{{"resolved", ready}});
    }

    return readyEnvelope(true, "No additional runtime context required.");
}

QJsonObject MainWindow::plannerExecutionMetadata(const QString& toolName) const
{
    const QString trimmedName = toolName.trimmed();
    if(trimmedName.isEmpty()) {
        return QJsonObject{
            {"execution_mode", "suggestion_only"},
            {"execution_reason", "Unnamed tools cannot be safely executed by the planner."}
        };
    }

    if(trimmedName == QLatin1String("studio.views.list")
       || trimmedName == QLatin1String("studio.settings.list")
       || trimmedName == QLatin1String("studio.pipelines.list")
       || trimmedName == QLatin1String("view.raw.summary")
       || trimmedName == QLatin1String("view.raw.state")
       || trimmedName.endsWith(".get")
       || trimmedName.startsWith("neurokernel.")) {
        return QJsonObject{
            {"execution_mode", "auto_run"},
            {"execution_reason", "Read-only inspection or dataset analysis is safe to execute automatically."}
        };
    }

    if(trimmedName == QLatin1String("view.raw.goto")
       || trimmedName == QLatin1String("view.raw.zoom")
       || trimmedName == QLatin1String("studio.pipeline.run")
       || trimmedName == QLatin1String("studio.pipeline.resume")) {
        return QJsonObject{
            {"execution_mode", "confirm_required"},
            {"execution_reason", "This action changes visible workspace state and should be surfaced before auto-execution."}
        };
    }

    return QJsonObject{
        {"execution_mode", "suggestion_only"},
        {"execution_reason", "This tool should be suggested rather than auto-executed by the planner."}
    };
}

QJsonArray MainWindow::plannerSafeToolDefinitions() const
{
    QJsonArray safeTools;
    const QJsonArray tools = availableToolDefinitions();
    for(const QJsonValue& value : tools) {
        QJsonObject tool = value.toObject();
        if(tool.isEmpty()) {
            continue;
        }

        const QJsonObject safety = plannerSafetyMetadata(tool.value("name").toString());
        for(auto it = safety.constBegin(); it != safety.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }
        const QJsonObject readiness = plannerReadinessMetadata(tool.value("name").toString());
        for(auto it = readiness.constBegin(); it != readiness.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }
        const QJsonObject execution = plannerExecutionMetadata(tool.value("name").toString());
        for(auto it = execution.constBegin(); it != execution.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }

        if(safety.value("planner_safe").toBool(false)) {
            safeTools.append(tool);
        }
    }

    return safeTools;
}

QJsonArray MainWindow::plannerReadyToolDefinitions() const
{
    QJsonArray readyTools;
    const QJsonArray tools = plannerSafeToolDefinitions();
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = value.toObject();
        if(tool.value("planner_ready").toBool(true)) {
            readyTools.append(tool);
        }
    }

    return readyTools;
}

QJsonArray MainWindow::plannerBlockedToolDefinitions() const
{
    QJsonArray blockedTools;
    const QJsonArray tools = availableToolDefinitions();
    for(const QJsonValue& value : tools) {
        QJsonObject tool = value.toObject();
        if(tool.isEmpty()) {
            continue;
        }

        const QJsonObject safety = plannerSafetyMetadata(tool.value("name").toString());
        for(auto it = safety.constBegin(); it != safety.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }

        const QJsonObject readiness = plannerReadinessMetadata(tool.value("name").toString());
        for(auto it = readiness.constBegin(); it != readiness.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }
        const QJsonObject execution = plannerExecutionMetadata(tool.value("name").toString());
        for(auto it = execution.constBegin(); it != execution.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }

        if(!tool.value("planner_safe").toBool(false) || !tool.value("planner_ready").toBool(true)) {
            blockedTools.append(tool);
        }
    }

    return blockedTools;
}

QJsonObject MainWindow::toolDefinition(const QString& toolName) const
{
    const QJsonArray tools = availableToolDefinitions();
    for(const QJsonValue& value : tools) {
        QJsonObject tool = value.toObject();
        if(tool.isEmpty()) {
            continue;
        }

        const QJsonObject safety = plannerSafetyMetadata(tool.value("name").toString());
        for(auto it = safety.constBegin(); it != safety.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }
        const QJsonObject readiness = plannerReadinessMetadata(tool.value("name").toString());
        for(auto it = readiness.constBegin(); it != readiness.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }
        const QJsonObject execution = plannerExecutionMetadata(tool.value("name").toString());
        for(auto it = execution.constBegin(); it != execution.constEnd(); ++it) {
            tool.insert(it.key(), it.value());
        }

        if(tool.value("name").toString() == toolName) {
            return tool;
        }
    }

    return QJsonObject();
}

QString MainWindow::toolNameFromCommand(const QString& commandText) const
{
    const QRegularExpression commandPattern("^\\s*tools\\.call\\s+([^\\s]+)");
    const QRegularExpressionMatch match = commandPattern.match(commandText.trimmed());
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QJsonObject MainWindow::toolArgumentsFromCommand(const QString& commandText) const
{
    const QString trimmedCommand = commandText.trimmed();
    const QString toolName = toolNameFromCommand(trimmedCommand);
    if(toolName.isEmpty()) {
        return QJsonObject();
    }

    const int toolStart = trimmedCommand.indexOf(toolName);
    if(toolStart < 0) {
        return QJsonObject();
    }

    const QString argumentsText = trimmedCommand.mid(toolStart + toolName.size()).trimmed();
    if(argumentsText.isEmpty()) {
        return QJsonObject();
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(argumentsText.toUtf8(), &error);
    if(error.error == QJsonParseError::NoError && document.isObject()) {
        return document.object();
    }

    return QJsonObject();
}

QJsonObject MainWindow::plannerConfirmationPresentation(const QString& commandText,
                                                        int stepIndex,
                                                        int totalSteps,
                                                        const QString& fallbackDetails,
                                                        const QString& plannerSummary,
                                                        const QString& previousPlannedCommand) const
{
    const QString toolName = toolNameFromCommand(commandText);
    const QJsonObject arguments = toolArgumentsFromCommand(commandText);
    QString title = QString("Approve LLM Step %1/%2").arg(stepIndex).arg(totalSteps);
    QString details = fallbackDetails;
    QString reason = plannerSummary.trimmed();

    if(toolName == QLatin1String("view.raw.goto")) {
        const int sample = arguments.value("sample").toInt(-1);
        title = sample >= 0
            ? QString("Jump Raw Browser To Sample %1").arg(sample)
            : QString("Jump Raw Browser");
        details = sample >= 0
            ? QString("Move the active raw browser cursor to sample %1.").arg(sample)
            : QString("Move the active raw browser cursor to the requested sample.");
    } else if(toolName == QLatin1String("view.raw.zoom")) {
        const double pixelsPerSample = arguments.value("pixels_per_sample").toDouble(-1.0);
        title = pixelsPerSample > 0.0
            ? QString("Set Raw Browser Zoom To %1 px/sample").arg(QString::number(pixelsPerSample, 'f', 2))
            : QString("Adjust Raw Browser Zoom");
        details = pixelsPerSample > 0.0
            ? QString("Change the raw browser zoom to %1 pixels per sample.")
                  .arg(QString::number(pixelsPerSample, 'f', 2))
            : QString("Change the zoom level of the active raw browser.");
    } else if(toolName == QLatin1String("studio.pipeline.run")) {
        const QString pipelineId = arguments.value("pipeline_id").toString().trimmed();
        const QJsonObject pipeline = analysisPipelineContract(pipelineId);
        const QString displayName = pipeline.value("display_name").toString(pipelineId);
        title = displayName.isEmpty()
            ? QString("Run Analysis Pipeline")
            : QString("Run Pipeline %1").arg(displayName);
        details = displayName.isEmpty()
            ? QString("Start the requested analysis pipeline.")
            : QString("Start the analysis pipeline `%1`.").arg(displayName);
        if(arguments.contains("inputs") && arguments.value("inputs").isObject()) {
            details += QString(" Inputs: %1")
                .arg(QString::fromUtf8(QJsonDocument(arguments.value("inputs").toObject())
                                           .toJson(QJsonDocument::Compact)));
        }
    } else if(toolName == QLatin1String("studio.pipeline.resume")) {
        const QString runId = arguments.value("run_id").toString().trimmed();
        const QJsonObject artifact = pipelineRunArtifact(runId);
        const QString displayName = artifact.value("display_name").toString(artifact.value("pipeline_id").toString());
        title = displayName.isEmpty()
            ? QString("Resume Pipeline Run")
            : QString("Resume Pipeline %1").arg(displayName);
        details = displayName.isEmpty()
            ? QString("Resume the saved pipeline run `%1`.").arg(runId)
            : QString("Resume the saved pipeline `%1` with remaining steps.").arg(displayName);
    }

    if(reason.isEmpty()) {
        reason = QString("The planner marked step %1 of %2 as the next state-changing action.")
                     .arg(stepIndex)
                     .arg(totalSteps);
    } else if(stepIndex > 1) {
        reason = QString("%1 This is step %2 of %3.").arg(reason).arg(stepIndex).arg(totalSteps);
    }

    const QString previousToolName = toolNameFromCommand(previousPlannedCommand);
    if(!previousToolName.isEmpty()) {
        if(toolName == QLatin1String("view.raw.goto")
           && previousToolName == QLatin1String("neurokernel.find_peak_window")) {
            reason += " It follows peak detection and would move the raw view to the detected sample.";
        } else if(toolName == QLatin1String("studio.pipeline.resume")) {
            reason += " It follows the planner's decision to continue an existing saved workflow.";
        } else if(toolName == QLatin1String("studio.pipeline.run")) {
            reason += " It follows the planner's decision that a structured pipeline is the best next workflow step.";
        }
    }

    if(details.trimmed().isEmpty()) {
        details = fallbackDetails;
    }

    return QJsonObject{
        {"title", title},
        {"details", details},
        {"reason", reason}
    };
}

QJsonObject MainWindow::plannerConfirmationSnapshot(const QString& commandText) const
{
    const QString toolName = toolNameFromCommand(commandText);
    const QJsonObject arguments = toolArgumentsFromCommand(commandText);
    QJsonObject snapshot{
        {"tool_name", toolName},
        {"result_selection_context", m_resultSelectionContext}
    };

    if(IRawDataView* rawBrowser = activeRawDataView()) {
        snapshot.insert("raw_file", rawBrowser->filePath());
        snapshot.insert("cursor_sample", rawBrowser->cursorSample());
    }

    if(toolName == QLatin1String("view.raw.goto")) {
        snapshot.insert("target_sample", arguments.value("sample").toInt(-1));
    } else if(toolName == QLatin1String("studio.pipeline.run")) {
        const QString pipelineId = arguments.value("pipeline_id").toString().trimmed();
        snapshot.insert("pipeline_id", pipelineId);
        snapshot.insert("inputs", arguments.value("inputs").toObject());
    } else if(toolName == QLatin1String("studio.pipeline.resume")) {
        const QString runId = arguments.value("run_id").toString().trimmed();
        snapshot.insert("run_id", runId);
        const QJsonObject artifact = pipelineRunArtifact(runId);
        snapshot.insert("pipeline_id", artifact.value("pipeline_id").toString());
        snapshot.insert("pending_steps", artifact.value("pending_steps").toInt());
    }

    return snapshot;
}

QJsonObject MainWindow::plannerConfirmationStaleness(const QJsonObject& confirmation) const
{
    const QJsonObject snapshot = confirmation.value("context_snapshot").toObject();
    const QString toolName = confirmation.value("command").toString().trimmed().isEmpty()
        ? snapshot.value("tool_name").toString().trimmed()
        : toolNameFromCommand(confirmation.value("command").toString());

    QStringList reasons;

    const QString snapshotRawFile = snapshot.value("raw_file").toString().trimmed();
    const QString currentRawFile = activeRawDataView() ? activeRawDataView()->filePath().trimmed() : QString();
    if(!snapshotRawFile.isEmpty() && snapshotRawFile != currentRawFile) {
        reasons << QString("raw file changed from `%1` to `%2`.")
                       .arg(QFileInfo(snapshotRawFile).fileName(),
                            currentRawFile.isEmpty() ? QString("none") : QFileInfo(currentRawFile).fileName());
    }

    const QJsonObject snapshotSelection = snapshot.value("result_selection_context").toObject();
    const QString snapshotChannel = snapshotSelection.value("selected_channel_name").toString().trimmed();
    const QString currentChannel = m_resultSelectionContext.value("selected_channel_name").toString().trimmed();
    if(!snapshotChannel.isEmpty() && snapshotChannel != currentChannel) {
        reasons << QString("selected channel changed from `%1` to `%2`.")
                       .arg(snapshotChannel,
                            currentChannel.isEmpty() ? QString("none") : currentChannel);
    }

    const QString snapshotRunId = snapshot.value("run_id").toString().trimmed();
    if(toolName == QLatin1String("studio.pipeline.resume") && !snapshotRunId.isEmpty()) {
        const QJsonObject artifact = pipelineRunArtifact(snapshotRunId);
        if(artifact.isEmpty()) {
            reasons << QString("saved pipeline run `%1` is no longer available.").arg(snapshotRunId);
        } else if(artifact.value("pending_steps").toInt() <= 0) {
            reasons << QString("pipeline run `%1` no longer has pending steps.").arg(snapshotRunId);
        }
    }

    const int snapshotStepNumber = snapshotSelection.value("selected_step_number").toInt(-1);
    const int currentStepNumber = m_resultSelectionContext.value("selected_step_number").toInt(-1);
    if(snapshotStepNumber > 0 && snapshotStepNumber != currentStepNumber) {
        reasons << QString("selected pipeline step changed from %1 to %2.")
                       .arg(snapshotStepNumber)
                       .arg(currentStepNumber > 0 ? QString::number(currentStepNumber) : QString("none"));
    }

    const bool stale = !reasons.isEmpty();
    return QJsonObject{
        {"stale", stale},
        {"stale_reason", reasons.join(" ")},
        {"stale_severity", stale ? QString("warning") : QString("ok")}
    };
}

QJsonObject MainWindow::llmPlanningContext(const QString& commandText) const
{
    Q_UNUSED(commandText)

    const QJsonArray plannerSafeTools = plannerSafeToolDefinitions();
    const QJsonArray plannerReadyTools = plannerReadyToolDefinitions();
    const QJsonArray plannerBlockedTools = plannerBlockedToolDefinitions();
    QJsonArray normalizedRecentResults;
    const int recentResultStart = std::max(0, static_cast<int>(m_structuredResultHistory.size()) - 8);
    for(int i = recentResultStart; i < m_structuredResultHistory.size(); ++i) {
        const QJsonObject entry = m_structuredResultHistory.at(i);
        normalizedRecentResults.append(normalizedToolResultEnvelope(entry.value("tool_name").toString(),
                                                                   entry.value("result").toObject()));
    }
    QJsonObject context{
        {"last_tool_name", m_lastToolName},
        {"last_tool_result", normalizedToolResultEnvelope(m_lastToolName, m_lastToolResult)},
        {"recent_normalized_results", normalizedRecentResults},
        {"tool_schema_summary", availableToolDefinitions()},
        {"planner_safe_tools", plannerSafeTools},
        {"planner_ready_tools", plannerReadyTools},
        {"planner_blocked_tools", plannerBlockedTools},
        {"planner_tool_count", plannerReadyTools.size()},
        {"planner_policy", QJsonObject{
             {"allowed_tool_classes", QJsonArray{"read_only_inspection", "dataset_analysis", "reversible_navigation", "pipeline_run"}},
             {"blocked_tool_classes", QJsonArray{"settings_mutation", "hosted_extension_actions", "historical_step_replay"}},
             {"execution_modes", QJsonArray{"auto_run", "confirm_required", "suggestion_only"}},
             {"notes", QJsonArray{
                  "Prefer read-only or analytical tools first.",
                  "Only use state-changing workbench tools when they are reversible and clearly advance the user request.",
                  "Do not mutate extension settings.",
                  "Do not use hosted extension session tools unless the user explicitly asks for them.",
                  "Treat planner_ready false as unavailable in the current workspace state.",
                  "Only auto-run tools whose execution_mode is auto_run.",
                  "Treat confirm_required tools as proposals that need user confirmation before execution.",
                  "Treat suggestion_only tools as ideas, not executable steps."
              }}
         }},
        {"planner_context_readiness", QJsonObject{
             {"has_raw_browser", activeRawDataView() != nullptr},
             {"has_hosted_session", !activeHostedViewSession().isEmpty()},
             {"analysis_pipeline_count", analysisPipelineContracts().size()},
             {"planner_safe_tool_count", plannerSafeTools.size()},
             {"planner_ready_tool_count", plannerReadyTools.size()},
             {"planner_blocked_tool_count", plannerBlockedTools.size()}
         }},
        {"pending_planner_confirmations", m_pendingPlannerConfirmations},
        {"current_result_selection", m_resultSelectionContext},
        {"result_renderer_contracts", resultRendererContracts()},
        {"analysis_pipeline_contracts", analysisPipelineContracts()},
        {"extension_settings_contracts", extensionSettingsContracts()},
        {"extension_settings_state", extensionSettingsState()}
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

    if(toolName == "studio.pipeline.run") {
        const QJsonArray pipelines = analysisPipelineContracts();
        if(!pipelines.isEmpty()) {
            const QJsonObject firstPipeline = pipelines.first().toObject();
            return QJsonObject{
                {"pipeline_id", firstPipeline.value("id").toString()},
                {"inputs", defaultInputsForPipeline(firstPipeline)}
            };
        }
        return QJsonObject{
            {"pipeline_id", QString()},
            {"inputs", QJsonObject()}
        };
    }

    if(toolName == "studio.pipeline.resume") {
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            const QJsonObject entry = m_structuredResultHistory.at(i);
            if(entry.value("tool_name").toString() == QLatin1String("studio.pipeline.run")) {
                const QJsonObject result = entry.value("result").toObject();
                if(result.value("pending_steps").toInt() > 0) {
                    return QJsonObject{{"run_id", result.value("run_id").toString()}};
                }
            }
        }
        return QJsonObject{{"run_id", QString()}};
    }

    if(toolName == "studio.pipeline.rerun_step") {
        for(int i = m_structuredResultHistory.size() - 1; i >= 0; --i) {
            const QJsonObject entry = m_structuredResultHistory.at(i);
            if(entry.value("tool_name").toString() != QLatin1String("studio.pipeline.run")) {
                continue;
            }

            const QJsonObject result = entry.value("result").toObject();
            const QJsonArray steps = result.value("steps").toArray();
            if(!steps.isEmpty()) {
                return QJsonObject{
                    {"run_id", result.value("run_id").toString()},
                    {"step_number", steps.last().toObject().value("step_number").toInt(1)},
                    {"mode", "rehydrate"}
                };
            }
        }

        return QJsonObject{
            {"run_id", QString()},
            {"step_number", 1},
            {"mode", "rehydrate"}
        };
    }

    const QJsonObject resolvedSettingsTool = resolveExtensionSettingsTool(toolName);
    if(!resolvedSettingsTool.isEmpty() && resolvedSettingsTool.value("mode").toString() == "set") {
        const QString extensionId = resolvedSettingsTool.value("extension_id").toString();
        const QString tabId = resolvedSettingsTool.value("tab_id").toString();
        const QJsonObject field = resolvedSettingsTool.value("field").toObject();
        return QJsonObject{
            {"value", QJsonValue::fromVariant(extensionSettingValue(extensionId, tabId, field))}
        };
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
        const QJsonObject properties = toolDefinition(toolName).value("input_schema").toObject().value("properties").toObject();
        for(const QString& extensionId : extensionIdsForTool(toolName)) {
            arguments = applyExtensionSettingDefaults(extensionId, properties, arguments);
        }
        return arguments;
    }

    if(toolName == "neurokernel.find_peak_window") {
        QJsonObject arguments = buildRawWindowArguments(4000);
        arguments.insert("window_samples", 4000);
        arguments.insert("match", "EEG");
        const QJsonObject properties = toolDefinition(toolName).value("input_schema").toObject().value("properties").toObject();
        for(const QString& extensionId : extensionIdsForTool(toolName)) {
            arguments = applyExtensionSettingDefaults(extensionId, properties, arguments);
        }
        return arguments;
    }

    if(toolName == "neurokernel.psd_summary") {
        QJsonObject arguments = buildRawWindowArguments(1200);
        arguments.insert("window_samples", 1200);
        arguments.insert("nfft", 256);
        arguments.insert("match", "EEG");
        const QJsonObject properties = toolDefinition(toolName).value("input_schema").toObject().value("properties").toObject();
        for(const QString& extensionId : extensionIdsForTool(toolName)) {
            arguments = applyExtensionSettingDefaults(extensionId, properties, arguments);
        }
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

    for(auto it = controls.constBegin(); it != controls.constEnd(); ++it) {
        const QString controlId = it.key();
        const QJsonObject control = it.value().toObject();
        const QString commandName = control.value("command").toString().trimmed();
        if(commandName.isEmpty()) {
            continue;
        }

        QJsonObject inputSchema = objectSchema(QJsonObject());
        const QString controlType = control.value("type").toString().trimmed().toLower();
        const QString label = control.value("label").toString(controlId);
        const QString description = control.value("description").toString(
            QString("Set %1 on the active hosted view for %2 from %3.").arg(label, providerName, extensionName));

        if(controlType == "number") {
            inputSchema = objectSchema(QJsonObject{
                 {control.value("target_argument").toString(controlId),
                  numberSchema(label,
                               control.value("minimum").toDouble(0.0),
                               control.value("maximum").toDouble(1.0),
                               state.value(control.value("state_key").toString(control.value("target_argument").toString(controlId)))
                                   .toDouble(control.value("value").toDouble(0.0)),
                               description)}
             }, QJsonArray{control.value("target_argument").toString(controlId)});
        } else if(controlType == "integer") {
            inputSchema = objectSchema(QJsonObject{
                 {control.value("target_argument").toString(controlId),
                  integerSchema(label,
                                control.value("minimum").toInt(0),
                                control.value("maximum").toInt(100),
                                state.value(control.value("state_key").toString(control.value("target_argument").toString(controlId)))
                                    .toInt(control.value("value").toInt(0)),
                                description)}
             }, QJsonArray{control.value("target_argument").toString(controlId)});
        }

        QJsonObject resultSchema = objectSchema(QJsonObject{
            {"session_id", resultStringSchema("Session ID")},
            {"command", resultStringSchema("Command")},
            {"state_before", stateSchema.isEmpty() ? objectSchema(QJsonObject()) : stateSchema},
            {"state_after", stateSchema.isEmpty() ? objectSchema(QJsonObject()) : stateSchema}
        }, QJsonArray{"session_id", "command"});
        const QJsonObject controlResultSchema = control.value("result_schema").toObject();
        if(!controlResultSchema.isEmpty()) {
            resultSchema = controlResultSchema;
        }

        tools.append(QJsonObject{
            {"name", QString("view.hosted.%1").arg(commandName)},
            {"description", description},
            {"input_schema", inputSchema},
            {"result_schema", resultSchema}
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

    QTreeWidgetItem* pipelinesRoot = new QTreeWidgetItem(QStringList() << "Analysis Pipelines");
    const QJsonArray pipelines = analysisPipelineContracts();
    if(pipelines.isEmpty()) {
        pipelinesRoot->addChild(new QTreeWidgetItem(QStringList() << "No analysis pipelines discovered yet."));
    } else {
        for(const QJsonValue& value : pipelines) {
            const QJsonObject pipeline = value.toObject();
            const QString label = QString("%1 (%2)")
                                      .arg(pipeline.value("display_name").toString(pipeline.value("id").toString()),
                                           pipeline.value("extension_display_name").toString());
            QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << label);
            item->setToolTip(0, pipeline.value("description").toString());
            item->setData(0, Qt::UserRole, "studio.pipeline.run");
            item->setData(0, Qt::UserRole + 1, pipeline);
            item->setData(0, Qt::UserRole + 2, pipeline.value("id").toString());
            pipelinesRoot->addChild(item);
        }
    }
    pipelinesRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(pipelinesRoot);

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

    QTreeWidgetItem* activePipelinesRoot = new QTreeWidgetItem(QStringList() << "Active Pipelines");
    if(m_activePipelineId.isEmpty()) {
        activePipelinesRoot->addChild(new QTreeWidgetItem(QStringList() << "No pipeline currently running."));
    } else {
        const int pendingSteps = static_cast<int>(m_pendingPipelineCommands.size());
        const int completedSteps = std::max(0, m_activePipelineTotalSteps - pendingSteps);
        activePipelinesRoot->addChild(new QTreeWidgetItem(QStringList()
                                                          << QString("%1 | %2/%3 steps")
                                                                 .arg(m_activePipelineDisplayName)
                                                                 .arg(completedSteps)
                                                                 .arg(m_activePipelineTotalSteps)));
        if(!m_activePipelineLastStatus.isEmpty()) {
            activePipelinesRoot->addChild(new QTreeWidgetItem(QStringList() << m_activePipelineLastStatus));
        }
    }
    activePipelinesRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(activePipelinesRoot);

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
    m_selectedPipelineId.clear();
    m_skillRunButton->setEnabled(false);

    if(!item) {
        m_skillDetailsView->setPlainText("Select a workbench or Neuro-Kernel tool to inspect it here.");
        return;
    }

    const QString toolName = item->data(0, Qt::UserRole).toString().trimmed();
    const QJsonObject toolDefinition = item->data(0, Qt::UserRole + 1).toJsonObject();
    const QString pipelineId = item->data(0, Qt::UserRole + 2).toString().trimmed();
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
    m_selectedPipelineId = pipelineId;
    m_skillRunButton->setEnabled(true);

    QJsonObject preview;
    if(!pipelineId.isEmpty()) {
        preview = QJsonObject{
            {"pipeline_id", pipelineId},
            {"contract", toolDefinition},
            {"run_tool", "studio.pipeline.run"},
            {"suggested_arguments", QJsonObject{
                 {"pipeline_id", pipelineId},
                 {"inputs", defaultInputsForPipeline(toolDefinition)}
             }}
        };
    } else {
        preview = QJsonObject{
            {"tool", toolName},
            {"definition", toolDefinition},
            {"suggested_arguments", defaultArgumentsForTool(toolName)}
        };
    }
    m_skillDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(preview).toJson(QJsonDocument::Indented)));
}

void MainWindow::runSelectedSkillTool()
{
    if(m_selectedSkillToolName.isEmpty() && m_selectedPipelineId.isEmpty()) {
        return;
    }

    if(!m_selectedPipelineId.isEmpty()) {
        const QJsonObject pipeline = analysisPipelineContract(m_selectedPipelineId);
        QJsonObject arguments{
            {"pipeline_id", m_selectedPipelineId},
            {"inputs", defaultInputsForPipeline(pipeline)}
        };

        const QString initialArguments = QString::fromUtf8(QJsonDocument(arguments)
                                                               .toJson(QJsonDocument::Indented)).trimmed();
        bool accepted = false;
        const QString argumentsText = QInputDialog::getMultiLineText(this,
                                                                     "Run Pipeline",
                                                                     QString("JSON arguments for pipeline %1").arg(m_selectedPipelineId),
                                                                     initialArguments,
                                                                     &accepted);
        if(!accepted) {
            return;
        }

        const QString trimmedArguments = argumentsText.trimmed();
        const QString commandText = trimmedArguments.isEmpty()
            ? QString("tools.call studio.pipeline.run {}")
            : QString("tools.call studio.pipeline.run %1").arg(trimmedArguments);

        appendTerminalMessage(QString("$ %1").arg(commandText));
        sendToolCall(commandText);
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
                           plannerReadyToolDefinitions(),
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
        extensionItem->addChild(new QTreeWidgetItem(QStringList() << QString("Result Renderers: %1").arg(manifest.resultRenderers.size())));
        extensionItem->addChild(new QTreeWidgetItem(QStringList() << QString("Analysis Pipelines: %1").arg(manifest.analysisPipelines.size())));
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
            {"result_renderer_count", static_cast<int>(manifest.resultRenderers.size())},
            {"tool_count", static_cast<int>(manifest.tools.size())},
            {"settings_tabs", static_cast<int>(manifest.ui.settingsTabs.size())},
            {"view_providers", QJsonArray()},
            {"result_renderers", QJsonArray()},
            {"manifest", manifest.rawManifest}
        };

        QJsonArray providerContracts;
        for(const ViewProviderContribution& provider : manifest.viewProviders) {
            providerContracts.append(QJsonObject{
                {"id", provider.id},
                {"display_name", provider.displayName},
                {"widget_type", provider.widgetType},
                {"slot", provider.slot},
                {"file_extensions", QJsonArray::fromStringList(provider.fileExtensions)},
                {"supports_scene_merging", provider.supportsSceneMerging},
                {"controls", provider.controls},
                {"actions", provider.actions},
                {"state_schema", provider.stateSchema},
                {"initial_state", provider.initialState}
            });
        }
        details.insert("view_providers", providerContracts);

        QJsonArray rendererContracts;
        for(const ResultRendererContribution& renderer : manifest.resultRenderers) {
            rendererContracts.append(QJsonObject{
                {"id", renderer.id},
                {"display_name", renderer.displayName},
                {"widget_type", renderer.widgetType},
                {"tool_names", QJsonArray::fromStringList(renderer.toolNames)},
                {"controls", renderer.controls},
                {"actions", renderer.actions},
                {"runtime_context_schema", renderer.runtimeContextSchema},
                {"history_schema", renderer.historySchema}
            });
        }
        details.insert("result_renderers", rendererContracts);

        QJsonArray settingsContracts;
        for(const UiContribution::SettingsTabContribution& settingsTab : manifest.ui.settingsTabs) {
            settingsContracts.append(QJsonObject{
                {"id", settingsTab.id},
                {"title", settingsTab.title},
                {"description", settingsTab.description},
                {"fields", settingsTab.fields},
                {"actions", settingsTab.actions}
            });
        }
        details.insert("settings_tab_contracts", settingsContracts);

        QJsonArray pipelineContracts;
        for(const AnalysisPipelineContribution& pipeline : manifest.analysisPipelines) {
            pipelineContracts.append(QJsonObject{
                {"id", pipeline.id},
                {"display_name", pipeline.displayName},
                {"description", pipeline.description},
                {"input_schema", pipeline.inputSchema},
                {"output_schema", pipeline.outputSchema},
                {"steps", pipeline.steps},
                {"follow_up_actions", pipeline.followUpActions}
            });
        }
        details.insert("analysis_pipelines", pipelineContracts);
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
            ExtensionSettingsWidget* settingsWidget = new ExtensionSettingsWidget(manifest, tab, nullptr);
            connect(settingsWidget, &ExtensionSettingsWidget::outputMessage, this, &MainWindow::appendOutputMessage);
            connect(settingsWidget, &ExtensionSettingsWidget::statusMessage, this, &MainWindow::setActivePanelState);
            container = settingsWidget;

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

    m_pendingPlannerConfirmations = QJsonArray();
    const QByteArray pendingConfirmationsPayload = settings.value("workspace/pending_planner_confirmations").toByteArray();
    if(!pendingConfirmationsPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(pendingConfirmationsPayload);
        if(document.isArray()) {
            const QJsonArray confirmationsArray = document.array();
            for(const QJsonValue& value : confirmationsArray) {
                if(value.isObject()) {
                    m_pendingPlannerConfirmations.append(value.toObject());
                }
            }
        }
    }
    refreshPlannerConfirmationsUi();

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
    m_resultSelectionContext = QJsonObject();
    const QByteArray resultSelectionPayload = settings.value("workspace/result_selection_context").toByteArray();
    if(!resultSelectionPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(resultSelectionPayload);
        if(document.isObject()) {
            m_resultSelectionContext = document.object();
        }
    }
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
    settings.setValue("workspace/pending_planner_confirmations",
                      QJsonDocument(m_pendingPlannerConfirmations).toJson(QJsonDocument::Compact));
    settings.setValue("workspace/result_selection_context",
                      QJsonDocument(m_resultSelectionContext).toJson(QJsonDocument::Compact));
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
