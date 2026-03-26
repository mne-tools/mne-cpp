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
#include <dummy3dhostedviewwidget.h>
#include "extensionhostedviewwidget.h"
#include "llmsettingsdialog.h"
#include "workflowminimapwidget.h"

#include <capabilitycatalog.h>
#include <capabilityutils.h>
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
#include <QBrush>
#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLibraryInfo>
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
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSettings>
#include <QSet>
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
const char* kActiveWorkflowGraphUri = "mne://workspace/active_graph";
const char* kWorkflowWorkspaceUriPrefix = "mne://workspace/";
const char* kWorkflowCenterTabKey = "__mne_analyze_studio_workflow_graph__";
constexpr int kWorkflowItemKindRole = Qt::UserRole + 20;
constexpr int kWorkflowPayloadRole = Qt::UserRole + 21;
constexpr int kWorkflowOpenPathRole = Qt::UserRole + 22;
constexpr int kWorkflowStableIdRole = Qt::UserRole + 23;

enum class ThreeDOpenChoice {
    Cancel,
    OpenNew,
    AddToExisting
};

struct ThreeDViewSelection {
    ThreeDOpenChoice choice = ThreeDOpenChoice::Cancel;
    int tabIndex = -1;
};

bool isWorkflowAnalysisFile(const QString& filePath)
{
    return QFileInfo(filePath).suffix().compare(QStringLiteral("mna"), Qt::CaseInsensitive) == 0;
}

QString loadTextFileForDisplay(const QString& filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("Could not open %1 for display.").arg(filePath);
    }

    return QString::fromUtf8(file.readAll());
}

QString qtPluginsPath()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QLibraryInfo::path(QLibraryInfo::PluginsPath);
#else
    return QLibraryInfo::location(QLibraryInfo::PluginsPath);
#endif
}

QString qtLibrariesPath()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QLibraryInfo::path(QLibraryInfo::LibrariesPath);
#else
    return QLibraryInfo::location(QLibraryInfo::LibrariesPath);
#endif
}

QProcessEnvironment studioBackendProcessEnvironment(const QString& executablePath)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.remove(QStringLiteral("QT_PLUGIN_PATH"));
    environment.remove(QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH"));

#ifdef Q_OS_MACOS
    environment.remove(QStringLiteral("DYLD_FRAMEWORK_PATH"));
    environment.remove(QStringLiteral("DYLD_LIBRARY_PATH"));
#endif

    const QString pluginsPath = qtPluginsPath();
    if(QDir(pluginsPath).exists()) {
        environment.insert(QStringLiteral("QT_PLUGIN_PATH"), pluginsPath);

        const QString platformPluginsPath = QDir(pluginsPath).filePath(QStringLiteral("platforms"));
        if(QDir(platformPluginsPath).exists()) {
            environment.insert(QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH"), platformPluginsPath);
        }
    }

#ifdef Q_OS_MACOS
    QStringList dyldLibraryPaths;

    const QString librariesPath = qtLibrariesPath();
    if(QDir(librariesPath).exists()) {
        environment.insert(QStringLiteral("DYLD_FRAMEWORK_PATH"), librariesPath);
        dyldLibraryPaths << QDir::cleanPath(librariesPath);
    }

    const QFileInfo executableInfo(executablePath);
    if(executableInfo.exists()) {
        const QString studioLibraryPath = QFileInfo(QDir(executableInfo.absolutePath()).filePath(QStringLiteral("../lib")))
                                              .absoluteFilePath();
        if(QDir(studioLibraryPath).exists() && !dyldLibraryPaths.contains(studioLibraryPath)) {
            dyldLibraryPaths.prepend(studioLibraryPath);
        }
    }

    if(!dyldLibraryPaths.isEmpty()) {
        environment.insert(QStringLiteral("DYLD_LIBRARY_PATH"), dyldLibraryPaths.join(QLatin1Char(':')));
    }
#else
    Q_UNUSED(executablePath)
#endif

    return environment;
}

void insertSortedUnique(QStringList& values, const QString& value)
{
    if(value.isEmpty() || values.contains(value)) {
        return;
    }

    const auto insertPosition = std::lower_bound(values.begin(), values.end(), value);
    values.insert(std::distance(values.begin(), insertPosition), value);
}

QString summarizeJsonValue(const QJsonValue& value)
{
    if(value.isString()) {
        return value.toString();
    }
    if(value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if(value.isDouble()) {
        return QString::number(value.toDouble());
    }
    if(value.isNull() || value.isUndefined()) {
        return QStringLiteral("<null>");
    }
    if(value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if(value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }

    return QStringLiteral("<value>");
}

QString resolveWorkflowUriToLocalPath(const QString& uri, const QString& sourceFilePath)
{
    const QString trimmedUri = uri.trimmed();
    if(trimmedUri.isEmpty()) {
        return QString();
    }

    const QFileInfo directFileInfo(trimmedUri);
    if(directFileInfo.isFile()) {
        return directFileInfo.absoluteFilePath();
    }

    if(sourceFilePath.trimmed().isEmpty()) {
        return QString();
    }

    const QString workspacePrefix = QString::fromLatin1(kWorkflowWorkspaceUriPrefix);
    if(!trimmedUri.startsWith(workspacePrefix)) {
        return QString();
    }

    QString relativePath = trimmedUri.mid(workspacePrefix.size());
    while(relativePath.startsWith(QLatin1Char('/'))) {
        relativePath.remove(0, 1);
    }

    if(relativePath.isEmpty()) {
        return QString();
    }

    const QString candidatePath = QDir(QFileInfo(sourceFilePath).absolutePath()).filePath(relativePath);
    const QFileInfo candidateInfo(candidatePath);
    return candidateInfo.isFile() ? candidateInfo.absoluteFilePath() : QString();
}

QString rawToolNameFromCommandText(const QString& commandText)
{
    const QRegularExpression commandPattern("^\\s*tools\\.call\\s+([^\\s]+)");
    const QRegularExpressionMatch match = commandPattern.match(commandText.trimmed());
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QHash<QString, QJsonObject> workflowResourceMap(const QJsonObject& graph)
{
    QHash<QString, QJsonObject> resourceByUid;
    const QJsonArray resources = graph.value(QStringLiteral("resources")).toArray();
    for(const QJsonValue& value : resources) {
        const QJsonObject resource = value.toObject();
        const QString uid = resource.value(QStringLiteral("uid")).toString().trimmed();
        if(!uid.isEmpty()) {
            resourceByUid.insert(uid, resource);
        }
    }

    return resourceByUid;
}

QHash<QString, QString> workflowOutputProducerMap(const QJsonObject& graph)
{
    QHash<QString, QString> producerByOutputUid;
    const QJsonArray pipeline = graph.value(QStringLiteral("pipeline")).toArray();
    for(const QJsonValue& value : pipeline) {
        const QJsonObject node = value.toObject();
        const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
        const QJsonObject outputs = node.value(QStringLiteral("outputs")).toObject();
        for(auto it = outputs.constBegin(); it != outputs.constEnd(); ++it) {
            const QString outputUid = it.value().toString().trimmed();
            if(!nodeUid.isEmpty() && !outputUid.isEmpty()) {
                producerByOutputUid.insert(outputUid, nodeUid);
            }
        }
    }

    return producerByOutputUid;
}

QStringList workflowDependencyNodeUids(const QJsonObject& node, const QHash<QString, QString>& outputProducerByUid)
{
    QStringList dependencyNodeUids;
    const QJsonObject inputs = node.value(QStringLiteral("inputs")).toObject();
    for(auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
        const QString inputUid = it.value().toString().trimmed();
        const QString producerNodeUid = outputProducerByUid.value(inputUid);
        if(!producerNodeUid.isEmpty()) {
            insertSortedUnique(dependencyNodeUids, producerNodeUid);
        }
    }

    return dependencyNodeUids;
}

QVector<QJsonObject> workflowNodesTopologicallyOrdered(const QJsonObject& graph)
{
    const QJsonArray pipeline = graph.value(QStringLiteral("pipeline")).toArray();
    QHash<QString, QJsonObject> nodeByUid;
    QStringList sortedNodeUids;
    for(const QJsonValue& value : pipeline) {
        const QJsonObject node = value.toObject();
        const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
        if(nodeUid.isEmpty()) {
            continue;
        }

        nodeByUid.insert(nodeUid, node);
        insertSortedUnique(sortedNodeUids, nodeUid);
    }

    const QHash<QString, QString> outputProducerByUid = workflowOutputProducerMap(graph);
    QHash<QString, QStringList> dependentNodeUidsByNodeUid;
    QHash<QString, int> indegreeByNodeUid;
    for(const QString& nodeUid : sortedNodeUids) {
        indegreeByNodeUid.insert(nodeUid, 0);
    }

    for(const QString& nodeUid : sortedNodeUids) {
        const QStringList dependencies = workflowDependencyNodeUids(nodeByUid.value(nodeUid), outputProducerByUid);
        indegreeByNodeUid[nodeUid] = dependencies.size();
        for(const QString& dependencyNodeUid : dependencies) {
            insertSortedUnique(dependentNodeUidsByNodeUid[dependencyNodeUid], nodeUid);
        }
    }

    QStringList readyNodeUids;
    for(const QString& nodeUid : sortedNodeUids) {
        if(indegreeByNodeUid.value(nodeUid) == 0) {
            insertSortedUnique(readyNodeUids, nodeUid);
        }
    }

    QVector<QJsonObject> orderedNodes;
    orderedNodes.reserve(sortedNodeUids.size());
    while(!readyNodeUids.isEmpty()) {
        const QString nodeUid = readyNodeUids.takeFirst();
        orderedNodes.append(nodeByUid.value(nodeUid));

        const QStringList dependents = dependentNodeUidsByNodeUid.value(nodeUid);
        for(const QString& dependentNodeUid : dependents) {
            const int remainingDependencies = indegreeByNodeUid.value(dependentNodeUid) - 1;
            indegreeByNodeUid[dependentNodeUid] = remainingDependencies;
            if(remainingDependencies == 0) {
                insertSortedUnique(readyNodeUids, dependentNodeUid);
            }
        }
    }

    if(orderedNodes.size() == sortedNodeUids.size()) {
        return orderedNodes;
    }

    QVector<QJsonObject> fallbackNodes;
    fallbackNodes.reserve(pipeline.size());
    for(const QJsonValue& value : pipeline) {
        fallbackNodes.append(value.toObject());
    }
    return fallbackNodes;
}

QString workflowNodeStatus(const QJsonObject& node)
{
    const QString status = node.value(QStringLiteral("runtime")).toObject().value(QStringLiteral("status")).toString().trimmed();
    return status.isEmpty() ? QStringLiteral("pending") : status;
}

QString workflowNodeDisplayLabel(const QJsonObject& node)
{
    const QString label = node.value(QStringLiteral("label")).toString().trimmed();
    return label.isEmpty() ? node.value(QStringLiteral("uid")).toString().trimmed() : label;
}

QHash<QString, QJsonObject> workflowNodeMap(const QJsonObject& graph)
{
    QHash<QString, QJsonObject> nodeByUid;
    const QJsonArray pipeline = graph.value(QStringLiteral("pipeline")).toArray();
    for(const QJsonValue& value : pipeline) {
        const QJsonObject node = value.toObject();
        const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
        if(!nodeUid.isEmpty()) {
            nodeByUid.insert(nodeUid, node);
        }
    }

    return nodeByUid;
}

QHash<QString, QStringList> workflowDependencyNodeMap(const QJsonObject& graph)
{
    QHash<QString, QStringList> dependenciesByNodeUid;
    const QHash<QString, QJsonObject> nodeByUid = workflowNodeMap(graph);
    const QHash<QString, QString> outputProducerByUid = workflowOutputProducerMap(graph);

    for(auto it = nodeByUid.constBegin(); it != nodeByUid.constEnd(); ++it) {
        dependenciesByNodeUid.insert(it.key(), workflowDependencyNodeUids(it.value(), outputProducerByUid));
    }

    return dependenciesByNodeUid;
}

QHash<QString, QStringList> workflowDependentNodeMap(const QJsonObject& graph)
{
    QHash<QString, QStringList> dependentsByNodeUid;
    const QHash<QString, QStringList> dependenciesByNodeUid = workflowDependencyNodeMap(graph);
    for(auto it = dependenciesByNodeUid.constBegin(); it != dependenciesByNodeUid.constEnd(); ++it) {
        if(!dependentsByNodeUid.contains(it.key())) {
            dependentsByNodeUid.insert(it.key(), QStringList());
        }

        for(const QString& dependencyNodeUid : it.value()) {
            insertSortedUnique(dependentsByNodeUid[dependencyNodeUid], it.key());
        }
    }

    return dependentsByNodeUid;
}

QStringList workflowReachableNodeUids(const QString& startNodeUid, const QHash<QString, QStringList>& adjacencyByNodeUid)
{
    QStringList visitedNodeUids;
    QStringList pendingNodeUids = adjacencyByNodeUid.value(startNodeUid);
    QSet<QString> seenNodeUids;

    while(!pendingNodeUids.isEmpty()) {
        const QString nodeUid = pendingNodeUids.takeFirst();
        if(nodeUid.isEmpty() || seenNodeUids.contains(nodeUid)) {
            continue;
        }

        seenNodeUids.insert(nodeUid);
        insertSortedUnique(visitedNodeUids, nodeUid);
        for(const QString& adjacentNodeUid : adjacencyByNodeUid.value(nodeUid)) {
            if(!adjacentNodeUid.isEmpty() && !seenNodeUids.contains(adjacentNodeUid)) {
                insertSortedUnique(pendingNodeUids, adjacentNodeUid);
            }
        }
    }

    return visitedNodeUids;
}

QString workflowFocusNodeUid(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("node_uid")).toString().trimmed();
}

bool workflowItemRepresentsPipelineNode(const QTreeWidgetItem* item, QString* nodeUid = nullptr)
{
    if(!item) {
        return false;
    }

    const QJsonObject payload = item->data(0, kWorkflowPayloadRole).toJsonObject();
    if(payload.value(QStringLiteral("kind")).toString() != QLatin1String("node")) {
        return false;
    }

    const QString resolvedNodeUid = workflowFocusNodeUid(payload);
    if(resolvedNodeUid.isEmpty()) {
        return false;
    }

    if(nodeUid) {
        *nodeUid = resolvedNodeUid;
    }

    return true;
}

QTreeWidgetItem* findWorkflowItemByStableId(QTreeWidget* tree, const QString& stableId)
{
    if(!tree || stableId.trimmed().isEmpty()) {
        return nullptr;
    }

    QList<QTreeWidgetItem*> pendingItems;
    for(int i = 0; i < tree->topLevelItemCount(); ++i) {
        pendingItems.append(tree->topLevelItem(i));
    }

    while(!pendingItems.isEmpty()) {
        QTreeWidgetItem* item = pendingItems.takeFirst();
        if(item->data(0, kWorkflowStableIdRole).toString() == stableId) {
            return item;
        }

        for(int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
            pendingItems.append(item->child(childIndex));
        }
    }

    return nullptr;
}

ThreeDViewSelection promptForThreeDTargetView(QWidget* parent,
                                              const QString& filePath,
                                              const QList<QPair<int, QString>>& viewOptions)
{
    ThreeDViewSelection selection;

    QDialog dialog(parent);
    dialog.setWindowTitle("Open 3D Surface");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* description = new QLabel(
        QString("Choose where to add %1.").arg(QFileInfo(filePath).fileName()),
        &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    QListWidget* viewList = new QListWidget(&dialog);
    for(const QPair<int, QString>& option : viewOptions) {
        QListWidgetItem* item = new QListWidgetItem(option.second, viewList);
        item->setData(Qt::UserRole, option.first);
    }
    if(viewList->count() > 0) {
        viewList->setCurrentRow(0);
    }
    layout->addWidget(viewList, 1);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(&dialog);
    QPushButton* addToSelectedButton = buttonBox->addButton("Add To Selected View", QDialogButtonBox::AcceptRole);
    QPushButton* openNewButton = buttonBox->addButton("Open New View", QDialogButtonBox::ActionRole);
    QPushButton* cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    addToSelectedButton->setEnabled(viewList->count() > 0);
    layout->addWidget(buttonBox);

    QObject::connect(addToSelectedButton, &QPushButton::clicked, &dialog, [&dialog, &selection, viewList]() {
        if(QListWidgetItem* item = viewList->currentItem()) {
            selection.choice = ThreeDOpenChoice::AddToExisting;
            selection.tabIndex = item->data(Qt::UserRole).toInt();
            dialog.accept();
        }
    });
    QObject::connect(openNewButton, &QPushButton::clicked, &dialog, [&dialog, &selection]() {
        selection.choice = ThreeDOpenChoice::OpenNew;
        dialog.accept();
    });
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&dialog, &selection]() {
        selection.choice = ThreeDOpenChoice::Cancel;
        dialog.reject();
    });
    QObject::connect(viewList, &QListWidget::itemDoubleClicked, &dialog, [&dialog, &selection](QListWidgetItem* item) {
        if(!item) {
            return;
        }

        selection.choice = ThreeDOpenChoice::AddToExisting;
        selection.tabIndex = item->data(Qt::UserRole).toInt();
        dialog.accept();
    });

    dialog.exec();
    return selection;
}

#ifdef Q_OS_MACOS
QString llmKeychainService()
{
    return QString("org.mnecpp.mne_analyze_studio.llm");
}

QString readSecretFromKeychain(const QString& accountName)
{
    if(accountName.trimmed().isEmpty()) {
        return QString();
    }

    QProcess process;
    process.start("/usr/bin/security",
                  QStringList() << "find-generic-password"
                                << "-s" << llmKeychainService()
                                << "-a" << accountName.trimmed()
                                << "-w");
    process.waitForFinished();
    if(process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return QString();
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool writeSecretToKeychain(const QString& accountName, const QString& secret)
{
    if(accountName.trimmed().isEmpty()) {
        return false;
    }

    QProcess process;
    process.start("/usr/bin/security",
                  QStringList() << "add-generic-password"
                                << "-U"
                                << "-s" << llmKeychainService()
                                << "-a" << accountName.trimmed()
                                << "-w" << secret);
    process.waitForFinished();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void deleteSecretFromKeychain(const QString& accountName)
{
    if(accountName.trimmed().isEmpty()) {
        return;
    }

    QProcess process;
    process.start("/usr/bin/security",
                  QStringList() << "delete-generic-password"
                                << "-s" << llmKeychainService()
                                << "-a" << accountName.trimmed());
    process.waitForFinished();
}
#else
QString readSecretFromKeychain(const QString&)
{
    return QString();
}

bool writeSecretToKeychain(const QString&, const QString&)
{
    return false;
}

void deleteSecretFromKeychain(const QString&)
{
}
#endif

QString activeAgentSecretAccountName()
{
    return QString("active");
}

QString providerSecretAccountName(const QString& profileName)
{
    return QString("profile:%1").arg(profileName.trimmed());
}

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

bool isAnalysisPipelineCapability(const QJsonObject& tool)
{
    return tool.value("capability_kind").toString().trimmed() == QLatin1String("analysis_pipeline")
           || !tool.value("pipeline_id").toString().trimmed().isEmpty();
}

bool isWorkflowSkillCapability(const QJsonObject& tool)
{
    return tool.value("workflow_operator").toBool(false)
           || tool.value("capability_kind").toString().trimmed() == QLatin1String("workflow_skill");
}

bool isWorkflowIoCapability(const QJsonObject& tool)
{
    const QString capabilityKind = tool.value("capability_kind").toString().trimmed();
    const QString toolName = tool.value("name").toString().trimmed();
    return capabilityKind == QLatin1String("workflow_io")
           || toolName == QLatin1String("studio.workflow.load")
           || toolName == QLatin1String("studio.workflow.save");
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

// Compose a human-readable Studio narration for a completed tool call.
// Returns an empty string when there is nothing interesting to add beyond the
// existing "message" bubble already shown by the Kernel/Extension Host line.
QString narrateToolResult(const QString& toolName, const QJsonObject& result)
{
    const QString status = result.value(QStringLiteral("status")).toString();

    // -----------------------------------------------------------------------
    // neurokernel.raw_stats
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("neurokernel.raw_stats")) {
        const QJsonObject stats = result.value(QStringLiteral("stats")).toObject();
        if(stats.isEmpty()) {
            return QString();
        }
        const int nChannels    = stats.value(QStringLiteral("n_channels")).toInt();
        const int nSamples     = stats.value(QStringLiteral("n_samples")).toInt();
        const double sfreq     = stats.value(QStringLiteral("sfreq")).toDouble();
        const double durationS = stats.value(QStringLiteral("duration_s")).toDouble();
        const double globalRms = stats.value(QStringLiteral("global_rms")).toDouble();
        const QJsonArray topCh = result.value(QStringLiteral("top_rms_channels")).toArray();

        QString narration = QString("Raw data stats computed: %1 channels × %2 samples at %3 Hz "
                                    "(%4 s). Global RMS = %5.")
                                .arg(nChannels)
                                .arg(nSamples)
                                .arg(sfreq, 0, 'f', 1)
                                .arg(durationS, 0, 'f', 2)
                                .arg(globalRms, 0, 'g', 4);

        if(!topCh.isEmpty()) {
            QStringList chNames;
            for(const QJsonValue& v : topCh) {
                const QJsonObject ch = v.toObject();
                const QString name = ch.value(QStringLiteral("channel")).toString();
                const double rms   = ch.value(QStringLiteral("rms")).toDouble();
                if(!name.isEmpty()) {
                    chNames << QString("%1 (%2)").arg(name).arg(rms, 0, 'g', 3);
                }
            }
            if(!chNames.isEmpty()) {
                narration += QString(" Top channels by RMS: %1.").arg(chNames.join(QStringLiteral(", ")));
            }
        }
        return narration;
    }

    // -----------------------------------------------------------------------
    // neurokernel.channel_stats
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("neurokernel.channel_stats")) {
        const QJsonArray channels = result.value(QStringLiteral("channels")).toArray();
        if(channels.isEmpty()) {
            return QString();
        }
        QStringList summaries;
        for(const QJsonValue& v : channels) {
            const QJsonObject ch = v.toObject();
            const QString name = ch.value(QStringLiteral("channel")).toString();
            const double rms   = ch.value(QStringLiteral("rms")).toDouble();
            const double peak  = ch.value(QStringLiteral("peak")).toDouble();
            if(!name.isEmpty()) {
                summaries << QString("%1: RMS=%2, peak=%3").arg(name)
                                 .arg(rms, 0, 'g', 3)
                                 .arg(peak, 0, 'g', 3);
            }
        }
        return QString("Channel stats for %1 channel(s): %2.")
            .arg(channels.size())
            .arg(summaries.join(QStringLiteral("; ")));
    }

    // -----------------------------------------------------------------------
    // neurokernel.find_peak_window
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("neurokernel.find_peak_window")) {
        const double tStart = result.value(QStringLiteral("window_start_s")).toDouble(-1.0);
        const double tEnd   = result.value(QStringLiteral("window_end_s")).toDouble(-1.0);
        const QString ch    = result.value(QStringLiteral("channel")).toString();
        const double peak   = result.value(QStringLiteral("peak_value")).toDouble();
        if(tStart < 0.0 && tEnd < 0.0) {
            return QString();
        }
        return QString("Peak window found on channel %1: [%2 s, %3 s], peak amplitude = %4.")
            .arg(ch.isEmpty() ? QStringLiteral("(unknown)") : ch)
            .arg(tStart, 0, 'f', 4)
            .arg(tEnd, 0, 'f', 4)
            .arg(peak, 0, 'g', 4);
    }

    // -----------------------------------------------------------------------
    // neurokernel.psd_summary
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("neurokernel.psd_summary")) {
        const double dominantFreq   = result.value(QStringLiteral("dominant_frequency_hz")).toDouble(-1.0);
        const double dominantPower  = result.value(QStringLiteral("dominant_power")).toDouble();
        const int nChannels         = result.value(QStringLiteral("n_channels")).toInt();
        const double freqRes        = result.value(QStringLiteral("frequency_resolution_hz")).toDouble(-1.0);
        if(dominantFreq < 0.0) {
            return QString();
        }
        QString narration = QString("PSD computed for %1 channel(s). Dominant frequency: %2 Hz "
                                    "(power = %3).")
                                .arg(nChannels)
                                .arg(dominantFreq, 0, 'f', 2)
                                .arg(dominantPower, 0, 'g', 4);
        if(freqRes > 0.0) {
            narration += QString(" Frequency resolution: %1 Hz.").arg(freqRes, 0, 'f', 3);
        }
        return narration;
    }

    // -----------------------------------------------------------------------
    // neurokernel.execute  (help / status / version)
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("neurokernel.execute")) {
        const QString command   = result.value(QStringLiteral("command")).toString();
        const int toolCount     = result.value(QStringLiteral("tool_count")).toInt(-1);
        const QString version   = result.value(QStringLiteral("kernel_version")).toString();
        if(command.compare(QLatin1String("status"), Qt::CaseInsensitive) == 0) {
            return QString("Neuro-Kernel status: running. %1 tools available. Version %2.")
                .arg(toolCount < 0 ? QStringLiteral("?") : QString::number(toolCount))
                .arg(version.isEmpty() ? QStringLiteral("unknown") : version);
        }
        if(command.compare(QLatin1String("version"), Qt::CaseInsensitive) == 0) {
            return QString("Neuro-Kernel version: %1.").arg(version.isEmpty() ? QStringLiteral("unknown") : version);
        }
        if(command.compare(QLatin1String("help"), Qt::CaseInsensitive) == 0 || command.isEmpty()) {
            const QJsonArray tools = result.value(QStringLiteral("available_tools")).toArray();
            if(!tools.isEmpty()) {
                QStringList names;
                for(const QJsonValue& v : tools) {
                    names << v.toString();
                }
                return QString("Neuro-Kernel exposes %1 tool(s): %2.")
                    .arg(tools.size())
                    .arg(names.join(QStringLiteral(", ")));
            }
        }
        return QString();
    }

    // -----------------------------------------------------------------------
    // apply_filter  (temporal filter skill)
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("apply_filter")) {
        if(status != QLatin1String("completed")) {
            return QString();
        }
        const QJsonObject params = result.value(QStringLiteral("parameters_used")).toObject();
        const QString filterType  = params.value(QStringLiteral("filter_type")).toString();
        const double highpass     = params.value(QStringLiteral("highpass")).toDouble(-1.0);
        const double lowpass      = params.value(QStringLiteral("lowpass")).toDouble(-1.0);
        const int filterOrder     = params.value(QStringLiteral("filter_order")).toInt(-1);
        const double sfreq        = params.value(QStringLiteral("sampling_frequency")).toDouble(-1.0);
        const QString outputPath  = result.value(QStringLiteral("output_path")).toString();
        const QJsonObject outputs = result.value(QStringLiteral("outputs")).toObject();
        const QString outputUri   = outputs.value(QStringLiteral("filtered_data")).toString();

        QString narration = QString("Temporal filter applied successfully. Type: %1").arg(filterType.isEmpty() ? QStringLiteral("unknown") : filterType);
        if(highpass >= 0.0) {
            narration += QString(", highpass = %1 Hz").arg(highpass, 0, 'f', 2);
        }
        if(lowpass >= 0.0) {
            narration += QString(", lowpass = %1 Hz").arg(lowpass, 0, 'f', 2);
        }
        if(filterOrder > 0) {
            narration += QString(", order = %1").arg(filterOrder);
        }
        if(sfreq > 0.0) {
            narration += QString(", sfreq = %1 Hz").arg(sfreq, 0, 'f', 1);
        }
        narration += QStringLiteral(".");
        if(!outputPath.isEmpty()) {
            narration += QString(" Output written to: %1.").arg(QFileInfo(outputPath).fileName());
        }
        if(!outputUri.isEmpty()) {
            narration += QString(" Output URI: %1.").arg(outputUri);
        }
        return narration;
    }

    // -----------------------------------------------------------------------
    // fiffbrowser.reveal_active_state
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("fiffbrowser.reveal_active_state")) {
        const int sessionCount = result.value(QStringLiteral("session_count")).toInt(0);
        if(sessionCount == 0) {
            return QStringLiteral("No FIFF browser session is currently open.");
        }
        const QJsonArray sessions = result.value(QStringLiteral("sessions")).toArray();
        QStringList fileNames;
        for(const QJsonValue& v : sessions) {
            const QString file = v.toObject().value(QStringLiteral("file")).toString();
            if(!file.isEmpty()) {
                fileNames << QFileInfo(file).fileName();
            }
        }
        return QString("FIFF browser has %1 active session(s). Open file(s): %2.")
            .arg(sessionCount)
            .arg(fileNames.isEmpty() ? QStringLiteral("(unknown)") : fileNames.join(QStringLiteral(", ")));
    }

    // -----------------------------------------------------------------------
    // dummy3d.set_opacity
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("dummy3d.set_opacity")) {
        const double opacity = result.value(QStringLiteral("opacity")).toDouble(-1.0);
        const QJsonArray updated = result.value(QStringLiteral("updated_sessions")).toArray();
        if(opacity < 0.0) {
            return QString();
        }
        return QString("3D surface opacity set to %1 across %2 session(s).")
            .arg(opacity, 0, 'f', 2)
            .arg(updated.size());
    }

    // -----------------------------------------------------------------------
    // studio.workflow.load / studio.workflow.save
    // -----------------------------------------------------------------------
    if(toolName == QLatin1String("studio.workflow.load")) {
        const QJsonObject graph = result.value(QStringLiteral("graph")).toObject();
        const QString workflowName = graph.value(QStringLiteral("name")).toString();
        const QJsonArray pipeline  = graph.value(QStringLiteral("pipeline")).toArray();
        const QJsonArray resources = graph.value(QStringLiteral("resources")).toArray();
        if(workflowName.isEmpty() && pipeline.isEmpty()) {
            return QString();
        }
        return QString("Workflow loaded: \"%1\" — %2 node(s), %3 resource(s).")
            .arg(workflowName.isEmpty() ? QStringLiteral("(unnamed)") : workflowName)
            .arg(pipeline.size())
            .arg(resources.size());
    }

    if(toolName == QLatin1String("studio.workflow.save")) {
        const QString savedPath = result.value(QStringLiteral("path")).toString();
        if(savedPath.isEmpty()) {
            return QString();
        }
        return QString("Workflow saved to: %1.").arg(QFileInfo(savedPath).fileName());
    }

    return QString();
}

}

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
, m_rootWidget(new QWidget(this))
, m_agentChatDock(new AgentChatDockWidget(this))
, m_activityBar(new QWidget(this))
, m_explorerButton(new QToolButton)
, m_workflowButton(new QToolButton)
, m_skillsButton(new QToolButton)
, m_extensionsButton(new QToolButton)
, m_workspaceExplorer(new QTreeWidget)
, m_workflowExplorer(new QTreeWidget)
, m_workflowPage(new QWidget(this))
, m_workflowStatusBanner(new QLabel(this))
, m_workflowMiniMapDockStateLabel(new QLabel(this))
, m_workflowOpenGraphButton(new QPushButton("Open In Center", this))
, m_workflowCenterView(nullptr)
, m_workflowCenterSummaryLabel(nullptr)
, m_workflowCenterMiniMap(nullptr)
, m_workflowCenterDetailsView(nullptr)
, m_workflowCenterOpenSourceButton(nullptr)
, m_workflowCenterDockButton(nullptr)
, m_workflowCenterAddStepButton(nullptr)
, m_workflowCenterSaveButton(nullptr)
, m_workflowMiniMap(new WorkflowMiniMapWidget(this))
, m_workflowDetailsView(new QPlainTextEdit(this))
, m_workflowAddStepButton(new QPushButton("Add Step...", this))
, m_workflowSaveButton(new QPushButton("Save Workflow", this))
, m_workflowOpenFileButton(new QPushButton("Open Selected File", this))
, m_workflowRefreshButton(new QPushButton("Refresh Graph", this))
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
, m_kernelProcess(new QProcess(this))
, m_extensionHostProcess(new QProcess(this))
, m_llmPlanner(this)
, m_sceneRegistry(this)
, m_viewProviderRegistry(new ViewProviderRegistry(this))
, m_viewManager(&m_sceneRegistry, m_viewProviderRegistry, this)
, m_activePipelineTotalSteps(0)
, m_isAdvancingPipeline(false)
, m_isShuttingDown(false)
{
    setWindowTitle("MNE Analyze Studio");
    resize(1440, 900);
    m_activeWorkflowHasUnsavedChanges = false;

    reloadExtensionRegistry();

    createLayout();
    createConnections();
    loadAgentSettings();
    restoreWorkspace();
    applyWorkbenchStyle();
    refreshAgentPlannerStatus();

    connect(m_kernelSocket, &QLocalSocket::connected, this, &MainWindow::requestKernelToolDefinitions);
    connect(m_extensionSocket, &QLocalSocket::connected, this, &MainWindow::requestExtensionHostState);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::shutdownManagedBackends);

    const auto configureBackendProcess = [this](QProcess* process, const QString& displayName) {
        if(!process) {
            return;
        }

        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process, &QProcess::readyReadStandardOutput, this, [this, process, displayName]() {
            const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
            if(!output.isEmpty()) {
                appendOutputMessage(QString("%1: %2").arg(displayName, output));
            }
        });
        connect(process,
                qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                this,
                [this, displayName](int exitCode, QProcess::ExitStatus exitStatus) {
                    if(m_isShuttingDown) {
                        return;
                    }

                    const QString message = QString("%1 exited (%2, code %3).")
                                                .arg(displayName,
                                                     exitStatus == QProcess::NormalExit ? QString("normal") : QString("crashed"))
                                                .arg(exitCode);
                    appendProblemMessage(message);
                    statusBar()->showMessage(message, 6000);
        });
        connect(process, &QProcess::errorOccurred, this, [this, displayName](QProcess::ProcessError error) {
            if(m_isShuttingDown) {
                return;
            }

            if(error == QProcess::FailedToStart || error == QProcess::Crashed) {
                const QString message = QString("%1 backend error: %2")
                                            .arg(displayName)
                                            .arg(static_cast<int>(error));
                appendProblemMessage(message);
                statusBar()->showMessage(message, 6000);
            }
        });
    };

    configureBackendProcess(m_kernelProcess, "Neuro-Kernel");
    configureBackendProcess(m_extensionHostProcess, "Extension Host");

    ensureBackendConnection(m_kernelSocket,
                            QString::fromLatin1(kKernelSocketName),
                            QStringLiteral("mne_analyze_studio_neuro_kernel"),
                            m_kernelProcess,
                            QStringLiteral("Neuro-Kernel"));
    ensureBackendConnection(m_extensionSocket,
                            QString::fromLatin1(kExtensionSocketName),
                            QStringLiteral("mne_analyze_studio_skill_host"),
                            m_extensionHostProcess,
                            QStringLiteral("Extension Host"));
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
    container->setSizePolicy(contentWidget->sizePolicy());
    container->setMaximumWidth(QWIDGETSIZE_MAX);
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
    m_workflowButton->setText("Flow");
    m_skillsButton->setText("Skills");
    m_extensionsButton->setText("Ext");
    m_explorerButton->setToolTip("Explorer");
    m_workflowButton->setToolTip("Workflow graph navigator");
    m_skillsButton->setToolTip("Tools and skills");
    m_extensionsButton->setToolTip("Extensions");
    m_explorerButton->setCheckable(true);
    m_workflowButton->setCheckable(true);
    m_skillsButton->setCheckable(true);
    m_extensionsButton->setCheckable(true);
    m_explorerButton->setChecked(true);
    activityLayout->addWidget(m_explorerButton);
    activityLayout->addWidget(m_workflowButton);
    activityLayout->addWidget(m_skillsButton);
    activityLayout->addWidget(m_extensionsButton);
    activityLayout->addStretch(1);

    m_workspaceExplorer->setHeaderLabel("Workspace");
    m_workspaceExplorer->setAlternatingRowColors(true);
    m_workspaceExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_workflowExplorer->setHeaderLabel("Workflow");
    m_workflowExplorer->setAlternatingRowColors(true);
    m_workflowExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_workflowStatusBanner->setWordWrap(true);
    m_workflowStatusBanner->setVisible(false);
    m_workflowMiniMapDockStateLabel->setWordWrap(true);
    m_workflowMiniMapDockStateLabel->setVisible(false);
    m_workflowMiniMap->setToolTip("Compact dependency map for the active workflow DAG. Click a node to focus it.");
    m_workflowDetailsView->setReadOnly(true);
    m_workflowDetailsView->setPlaceholderText("Select a workflow resource or pipeline node to inspect it here.");
    m_workflowOpenGraphButton->setToolTip("Open the workflow dependency map in a dedicated center tab.");
    m_workflowAddStepButton->setToolTip("Append a new skill operator node to the active workflow graph.");
    m_workflowSaveButton->setToolTip("Persist the current active workflow DAG back to a .mne file.");
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

    QVBoxLayout* workflowPageLayout = new QVBoxLayout(m_workflowPage);
    workflowPageLayout->setContentsMargins(0, 0, 0, 0);
    workflowPageLayout->setSpacing(8);
    workflowPageLayout->addWidget(m_workflowStatusBanner);
    workflowPageLayout->addWidget(m_workflowExplorer, 2);
    QHBoxLayout* workflowGraphHeaderLayout = new QHBoxLayout;
    workflowGraphHeaderLayout->setContentsMargins(0, 0, 0, 0);
    workflowGraphHeaderLayout->setSpacing(8);
    QLabel* workflowGraphLabel = new QLabel("Dependency Map", m_workflowPage);
    workflowGraphHeaderLayout->addWidget(workflowGraphLabel);
    workflowGraphHeaderLayout->addStretch(1);
    workflowGraphHeaderLayout->addWidget(m_workflowOpenGraphButton);
    workflowPageLayout->addLayout(workflowGraphHeaderLayout);
    workflowPageLayout->addWidget(m_workflowMiniMapDockStateLabel);
    workflowPageLayout->addWidget(m_workflowMiniMap, 1);
    workflowPageLayout->addWidget(new QLabel("Workflow Inspector", m_workflowPage));
    workflowPageLayout->addWidget(m_workflowDetailsView, 1);
    QHBoxLayout* workflowButtonsLayout = new QHBoxLayout;
    workflowButtonsLayout->setContentsMargins(0, 0, 0, 0);
    workflowButtonsLayout->setSpacing(8);
    workflowButtonsLayout->addWidget(m_workflowAddStepButton);
    workflowButtonsLayout->addWidget(m_workflowSaveButton);
    workflowButtonsLayout->addWidget(m_workflowOpenFileButton);
    workflowButtonsLayout->addWidget(m_workflowRefreshButton);
    workflowPageLayout->addLayout(workflowButtonsLayout);
    rebuildWorkflowNavigatorUi();
    refreshWorkflowGraphDockingUi();

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
    m_leftSidebarStack->addWidget(m_workflowPage);
    m_leftSidebarStack->addWidget(m_skillsPage);
    m_leftSidebarStack->addWidget(m_extensionsPage);

    leftSidebarLayout->addWidget(m_activityBar);
    leftSidebarLayout->addWidget(m_leftSidebarStack, 1);

    QWidget* leftPanel = createSidebarSection("Primary Sidebar", leftSidebar);
    leftPanel->setMinimumWidth(220);

    m_centerTabs->setDocumentMode(true);
    m_centerTabs->setTabsClosable(true);
    m_centerTabs->setMovable(true);
    m_centerTabs->setTabBar(m_centerTabBar);
    m_centerTabs->tabBar()->setExpanding(false);
    m_centerTabs->setMinimumWidth(0);

    m_bottomPanelTabs->setDocumentMode(true);
    m_bottomPanelTabs->setMovable(false);
    m_bottomPanelTabs->setMinimumWidth(0);
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
    m_resultsTitleLabel->setObjectName("resultsMetaLabel");
    m_resultsStack->addWidget(m_resultsTree);
    m_resultsStack->addWidget(m_resultsTable);

    QVBoxLayout* resultsLayout = new QVBoxLayout(m_resultsTab);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    resultsLayout->setSpacing(8);
    QHBoxLayout* resultsHistoryLayout = new QHBoxLayout;
    resultsHistoryLayout->setContentsMargins(0, 0, 0, 0);
    resultsHistoryLayout->setSpacing(8);
    QLabel* resultsHistoryLabel = new QLabel("Recent Results", m_resultsTab);
    resultsHistoryLabel->setObjectName("resultsSectionLabel");
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
    activitySection->setMinimumWidth(0);
    chatSection->setMinimumWidth(380);
    chatSection->setMaximumWidth(QWIDGETSIZE_MAX);
    chatSection->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

    m_centerSplitter->addWidget(m_centerTabs);
    m_centerSplitter->addWidget(activitySection);
    m_centerSplitter->setMinimumWidth(0);
    m_centerSplitter->setStretchFactor(0, 1);
    m_centerSplitter->setStretchFactor(1, 0);
    m_centerSplitter->setSizes({700, 180});

    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(m_centerSplitter);
    m_mainSplitter->addWidget(chatSection);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setHandleWidth(14);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 2);
    m_mainSplitter->setSizes({220, 560, 640});

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
        const QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open neuroscience asset",
            QString(),
            "Workflow Files (*.mne);;Signal Files (*.fif *.ave *.edf);;Source Files (*.cpp *.h *.json *.py *.tex *.md);;All Files (*)");
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
    connect(m_agentChatDock, &AgentChatDockWidget::connectionProfileSelected, this, &MainWindow::handleAgentConnectionProfileSelected);
    connect(m_agentChatDock, &AgentChatDockWidget::connectionModeSelected, this, &MainWindow::handleAgentConnectionModeSelected);
    connect(m_agentChatDock, &AgentChatDockWidget::connectionModelSelected, this, &MainWindow::handleAgentConnectionModelSelected);
    connect(m_agentChatDock, &AgentChatDockWidget::openConnectionSettingsRequested, this, &MainWindow::openAgentSettings);
    connect(m_agentChatDock, &AgentChatDockWidget::plannerSafetyLevelSelected, this, &MainWindow::handleAgentPlannerSafetyLevelSelected);
    connect(m_explorerButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("workspace");
    });
    connect(m_workflowButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("workflow");
    });
    connect(m_skillsButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("skills");
    });
    connect(m_extensionsButton, &QToolButton::clicked, this, [this]() {
        switchPrimarySidebar("extensions");
    });
    connect(m_workflowExplorer, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) {
        updateSelectedWorkflowItem(current);
    });
    connect(m_workflowExplorer, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        updateSelectedWorkflowItem(item);
        if(!openableWorkflowPathForItem(item).isEmpty()) {
            openSelectedWorkflowFile();
        }
    });
    connect(m_workflowExplorer, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        QMenu contextMenu(this);
        QTreeWidgetItem* item = m_workflowExplorer->itemAt(position);
        if(item) {
            m_workflowExplorer->setCurrentItem(item);
            updateSelectedWorkflowItem(item);
        }

        QAction* refreshGraphAction = contextMenu.addAction("Refresh Workflow Graph");
        connect(refreshGraphAction, &QAction::triggered, this, [this]() {
            if(!m_activeWorkflowFilePath.trimmed().isEmpty() && QFileInfo::exists(m_activeWorkflowFilePath)) {
                requestWorkflowLoad(m_activeWorkflowFilePath);
                statusBar()->showMessage(QString("Refreshing workflow graph from %1...")
                                             .arg(QFileInfo(m_activeWorkflowFilePath).fileName()),
                                         4000);
            } else {
                requestActiveWorkflowGraph();
                statusBar()->showMessage("Refreshing active workflow graph...", 4000);
            }
        });

        QAction* openCenterGraphAction = contextMenu.addAction("Open Workflow Graph");
        connect(openCenterGraphAction, &QAction::triggered, this, [this]() {
            openWorkflowCenterView(true);
        });

        QAction* appendStepAction = contextMenu.addAction("Append Workflow Step...");
        appendStepAction->setEnabled(m_workflowAddStepButton->isEnabled());
        connect(appendStepAction, &QAction::triggered, this, &MainWindow::appendWorkflowStep);

        QAction* saveWorkflowAction = contextMenu.addAction("Save Workflow");
        saveWorkflowAction->setEnabled(m_workflowSaveButton->isEnabled());
        connect(saveWorkflowAction, &QAction::triggered, this, &MainWindow::saveActiveWorkflowGraph);

        if(item && !openableWorkflowPathForItem(item).isEmpty()) {
            QAction* openFileAction = contextMenu.addAction("Open Selected File");
            connect(openFileAction, &QAction::triggered, this, &MainWindow::openSelectedWorkflowFile);
        }

        contextMenu.exec(m_workflowExplorer->viewport()->mapToGlobal(position));
    });
    connect(m_workflowMiniMap, &WorkflowMiniMapWidget::nodeActivated, this, [this](const QString& nodeUid) {
        if(QTreeWidgetItem* nodeItem = findWorkflowItemByStableId(m_workflowExplorer, QStringLiteral("node:%1").arg(nodeUid))) {
            m_workflowExplorer->setCurrentItem(nodeItem);
            m_workflowExplorer->scrollToItem(nodeItem);
            updateSelectedWorkflowItem(nodeItem);
        }
    });
    connect(m_workflowAddStepButton, &QPushButton::clicked, this, &MainWindow::appendWorkflowStep);
    connect(m_workflowOpenGraphButton, &QPushButton::clicked, this, [this]() {
        if(isWorkflowCenterViewOpen()) {
            openWorkflowCenterView(true);
            return;
        }

        openWorkflowCenterView(true);
    });
    connect(m_workflowSaveButton, &QPushButton::clicked, this, &MainWindow::saveActiveWorkflowGraph);
    connect(m_workflowOpenFileButton, &QPushButton::clicked, this, &MainWindow::openSelectedWorkflowFile);
    connect(m_workflowRefreshButton, &QPushButton::clicked, this, [this]() {
        if(!m_activeWorkflowFilePath.trimmed().isEmpty() && QFileInfo::exists(m_activeWorkflowFilePath)) {
            requestWorkflowLoad(m_activeWorkflowFilePath);
            appendOutputMessage(QString("Refreshing workflow graph from %1").arg(m_activeWorkflowFilePath));
            statusBar()->showMessage(QString("Refreshing workflow graph from %1...")
                                         .arg(QFileInfo(m_activeWorkflowFilePath).fileName()),
                                     4000);
        } else {
            requestActiveWorkflowGraph();
            appendOutputMessage("Refreshing active workflow graph");
            statusBar()->showMessage("Refreshing active workflow graph...", 4000);
        }
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
                    const QString kernelNarration = narrateToolResult(toolName, result);
                    if(!kernelNarration.isEmpty()) {
                        m_agentChatDock->appendTranscript(QString("Studio> %1").arg(kernelNarration));
                    }
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
                    const QString requestId = response.value("id").toString();
                    const QString workflowFilePath = m_pendingWorkflowLoads.take(requestId);
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
                    if(!workflowFilePath.isEmpty()) {
                        const QString workflowLabel = QFileInfo(workflowFilePath).fileName().isEmpty()
                            ? workflowFilePath
                            : QFileInfo(workflowFilePath).fileName();
                        setWorkflowStatusBanner(QString("Failed to activate %1: %2").arg(workflowLabel, message),
                                               QStringLiteral("error"));
                        if(m_activeWorkflowGraph.isEmpty()) {
                            rebuildWorkflowNavigatorUi();
                        }
                    }
                } else {
                    const QString requestId = response.value("id").toString();
                    const QJsonObject result = normalizedToolResultEnvelope(response.value("result").toObject().value("tool_name").toString(),
                                                                           response.value("result").toObject(),
                                                                           "extension_host");
                    const QString toolName = result.value("tool_name").toString();
                    const QString workflowFilePath = m_pendingWorkflowLoads.take(requestId);
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
                            const QString extensionId = tool.value("extension_id").toString().trimmed();
                            if(!m_viewProviderRegistry
                               || extensionId.isEmpty()
                               || m_viewProviderRegistry->isExtensionEnabled(extensionId)) {
                                filteredTools.append(tool);
                            }
                        }
                        m_cachedExtensionToolDefinitions = filteredTools;
                        rebuildSkillsExplorer();
                        statusBar()->showMessage(QString("Discovered %1 extension-host tools.")
                                                     .arg(m_cachedExtensionToolDefinitions.size()),
                                                 4000);
                        refreshExtensionManagerUi();
                        rebuildWorkflowNavigatorUi();
                        continue;
                    }
                    if(toolName == "resources/list" && result.value("resources").isArray()) {
                        QJsonArray filteredResources;
                        for(const QJsonValue& value : result.value("resources").toArray()) {
                            const QJsonObject resource = value.toObject();
                            const QString extensionId = resource.value("extension_id").toString().trimmed();
                            const QString resourceId = resource.value("id").toString().trimmed();
                            const QString resourceUri = resource.value("uri").toString().trimmed();
                            const bool isWorkflowGraphResource = resourceUri == QLatin1String(kActiveWorkflowGraphUri)
                                || resource.value("kind").toString().trimmed() == QLatin1String("workflow_graph");
                            const QString visibilityId = extensionId.isEmpty() ? resourceId : extensionId;
                            if(isWorkflowGraphResource
                               || !m_viewProviderRegistry
                               || visibilityId.isEmpty()
                               || m_viewProviderRegistry->isExtensionEnabled(visibilityId)) {
                                filteredResources.append(resource);
                            }
                        }
                        m_cachedExtensionResources = filteredResources;
                        rebuildSkillsExplorer();
                        statusBar()->showMessage(QString("Discovered %1 live extension/workflow resources.")
                                                     .arg(m_cachedExtensionResources.size()),
                                                 4000);
                        refreshExtensionManagerUi();
                        continue;
                    }
                    if(toolName == "resources/read"
                       && result.value("uri").toString().trimmed() == QLatin1String(kActiveWorkflowGraphUri)) {
                        adoptWorkflowGraph(result);
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
                    const QString resultStatus = result.value("status").toString().trimmed().toLower();
                    const QJsonObject workflowOperatorDefinition = workflowOperatorToolDefinition(toolName);
                    if(toolName == "studio.workflow.load") {
                        const QString effectiveWorkflowFilePath = !workflowFilePath.isEmpty()
                            ? workflowFilePath
                            : result.value("source_file").toString().trimmed();
                        const QString workflowLabel = QFileInfo(effectiveWorkflowFilePath).fileName().isEmpty()
                            ? effectiveWorkflowFilePath
                            : QFileInfo(effectiveWorkflowFilePath).fileName();
                        const QString resultMessage = result.value("message").toString().trimmed();

                        if(resultStatus == QLatin1String("error") || resultStatus == QLatin1String("failed")) {
                            const QString bannerMessage = workflowLabel.isEmpty()
                                ? (resultMessage.isEmpty()
                                       ? QStringLiteral("Failed to activate workflow graph.")
                                       : resultMessage)
                                : QString("Failed to activate %1: %2")
                                      .arg(workflowLabel,
                                           resultMessage.isEmpty()
                                               ? QStringLiteral("Unknown workflow load error.")
                                               : resultMessage);
                            setWorkflowStatusBanner(bannerMessage, QStringLiteral("error"));
                            if(m_activeWorkflowGraph.isEmpty()) {
                                rebuildWorkflowNavigatorUi();
                            }
                        }
                    }
                    if(toolName == "studio.workflow.save"
                       && (resultStatus == QLatin1String("error") || resultStatus == QLatin1String("failed"))) {
                        const QString targetPath = result.value("source_file").toString().trimmed();
                        const QString targetLabel = QFileInfo(targetPath).fileName().isEmpty()
                            ? targetPath
                            : QFileInfo(targetPath).fileName();
                        const QString bannerMessage = targetLabel.isEmpty()
                            ? result.value("message").toString(QStringLiteral("Failed to save the active workflow graph."))
                            : QString("Failed to save %1: %2")
                                  .arg(targetLabel,
                                       result.value("message").toString(QStringLiteral("Unknown workflow save error.")));
                        setWorkflowStatusBanner(bannerMessage, QStringLiteral("error"));
                    }
                    if(!workflowOperatorDefinition.isEmpty()
                       && (resultStatus == QLatin1String("error") || resultStatus == QLatin1String("failed"))) {
                        const QString operatorLabel = workflowOperatorDefinition.value("display_name")
                            .toString(workflowOperatorDefinition.value("name").toString(toolName));
                        setWorkflowStatusBanner(QString("Failed to append workflow step via %1: %2")
                                                    .arg(operatorLabel,
                                                         result.value("message").toString(QStringLiteral("Unknown operator error."))),
                                               QStringLiteral("error"));
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
                    const bool isHousekeepingMethod = toolName == QLatin1String("tools/list")
                        || toolName == QLatin1String("resources/list")
                        || toolName == QLatin1String("resources/read")
                        || toolName == QLatin1String("views/list");
                    if(toolName != QLatin1String("views/command") && !isHousekeepingMethod) {
                        rememberToolResult(toolName, result);
                    }
                    if(!workflowFilePath.isEmpty() || result.value("graph").isObject()) {
                        adoptWorkflowGraph(result, workflowFilePath);
                    }
                    const QString message = result.value("message").toString(QJsonDocument(result).toJson(QJsonDocument::Compact));
                    m_agentChatDock->appendTranscript(QString("Extension Host> %1").arg(message));
                    appendOutputMessage(QString("Extension Host: %1").arg(message));
                    appendTerminalMessage(QString("> %1").arg(message));
                    const QString extNarration = narrateToolResult(toolName, result);
                    if(!extNarration.isEmpty()) {
                        m_agentChatDock->appendTranscript(QString("Studio> %1").arg(extNarration));
                    }
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
    if(isWorkflowAnalysisFile(filePath)) {
        requestWorkflowLoad(filePath);
    }

    const QJsonObject dispatch = m_viewManager.dispatchFileSelection(filePath);
    if(Dummy3DHostedViewWidget* existingThreeDView = threeDViewContainingFile(filePath)) {
        const int existingTabIndex = centerTabIndexForWidget(existingThreeDView);
        if(existingTabIndex >= 0) {
            m_centerTabs->setCurrentIndex(existingTabIndex);
            statusBar()->showMessage(QString("%1 is already loaded in the selected 3D scene.")
                                         .arg(QFileInfo(filePath).fileName()),
                                     4000);
            return;
        }
    }

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
        if(dispatch.value("supports_scene_merging").toBool(false) && handleThreeDFileOpen(filePath, dispatch)) {
            return;
        }

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
        if(dispatch.value("view").toString() == QLatin1String("CodeEditorView")
           || isWorkflowAnalysisFile(filePath)) {
            editor->setPlainText(loadTextFileForDisplay(filePath));
        } else {
            editor->setPlainText(QJsonDocument(dispatch).toJson(QJsonDocument::Indented));
        }
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

bool MainWindow::handleThreeDFileOpen(const QString& filePath, const QJsonObject& dispatch)
{
    if(Dummy3DHostedViewWidget* activeView = currentThreeDView()) {
        return addFileToThreeDView(activeView, filePath);
    }

    const QList<Dummy3DHostedViewWidget*> availableViews = openThreeDViews();
    if(availableViews.isEmpty()) {
        const QJsonObject sceneDispatch = dispatchForNewThreeDScene(filePath, dispatch);
        requestExtensionViewOpen(filePath, sceneDispatch);
        statusBar()->showMessage(QString("Opening new 3D view for %1...")
                                     .arg(QFileInfo(filePath).fileName()),
                                 4000);
        appendOutputMessage(QString("Opening new 3D scene for %1").arg(filePath));
        appendTerminalMessage(QString("$ extension view open %1").arg(filePath));
        return true;
    }

    QList<QPair<int, QString>> viewOptions;
    for(Dummy3DHostedViewWidget* view : availableViews) {
        const int tabIndex = centerTabIndexForWidget(view);
        if(tabIndex < 0) {
            continue;
        }

        const QString label = QString("%1 | %2 layers")
                                  .arg(m_centerTabs->tabText(tabIndex),
                                       QString::number(view->loadedFiles().size()));
        viewOptions.append(qMakePair(tabIndex, label));
    }

    const ThreeDViewSelection selection = promptForThreeDTargetView(this, filePath, viewOptions);
    if(selection.choice == ThreeDOpenChoice::Cancel) {
        statusBar()->showMessage(QString("Canceled opening %1").arg(QFileInfo(filePath).fileName()), 3000);
        return true;
    }

    if(selection.choice == ThreeDOpenChoice::OpenNew) {
        const QJsonObject sceneDispatch = dispatchForNewThreeDScene(filePath, dispatch);
        requestExtensionViewOpen(filePath, sceneDispatch);
        statusBar()->showMessage(QString("Opening new 3D view for %1...")
                                     .arg(QFileInfo(filePath).fileName()),
                                 4000);
        appendOutputMessage(QString("Opening new 3D scene for %1").arg(filePath));
        appendTerminalMessage(QString("$ extension view open %1").arg(filePath));
        return true;
    }

    if(selection.tabIndex >= 0 && selection.tabIndex < m_centerTabs->count()) {
        if(Dummy3DHostedViewWidget* targetView = dynamic_cast<Dummy3DHostedViewWidget*>(m_centerTabs->widget(selection.tabIndex))) {
            return addFileToThreeDView(targetView, filePath);
        }
    }

    appendProblemMessage(QString("The selected 3D target view for %1 is no longer available.").arg(filePath));
    return true;
}

bool MainWindow::addFileToThreeDView(Dummy3DHostedViewWidget* targetView, const QString& filePath)
{
    if(!targetView) {
        return false;
    }

    const int tabIndex = centerTabIndexForWidget(targetView);
    if(tabIndex >= 0) {
        m_centerTabs->setCurrentIndex(tabIndex);
    }

    if(!targetView->addFileToScene(filePath)) {
        appendProblemMessage(QString("Failed to add %1 to the target 3D view.").arg(filePath));
        statusBar()->showMessage(QString("Could not add %1 to the 3D view.")
                                     .arg(QFileInfo(filePath).fileName()),
                                 5000);
        return true;
    }

    if(!targetView->sceneId().isEmpty()) {
        m_sceneRegistry.addLayerToScene(targetView->sceneId(), filePath);
    }

    statusBar()->showMessage(QString("Added %1 to %2.")
                                 .arg(QFileInfo(filePath).fileName(), targetView->displayTitle()),
                             4000);
    appendOutputMessage(QString("Added %1 to 3D scene %2").arg(filePath, targetView->displayTitle()));
    appendTerminalMessage(QString("$ add-to-3d %1").arg(filePath));
    return true;
}

QJsonObject MainWindow::dispatchForNewThreeDScene(const QString& filePath, const QJsonObject& dispatch)
{
    QJsonObject updatedDispatch = dispatch;

    QString sceneId = updatedDispatch.value("sceneId").toString().trimmed();
    if(sceneId.isEmpty()) {
        const QString subjectId = updatedDispatch.value("subjectId").toString();
        sceneId = m_sceneRegistry.createScene(subjectId, QFileInfo(filePath).fileName());
        updatedDispatch.insert("sceneId", sceneId);
    }

    if(!sceneId.isEmpty()) {
        m_sceneRegistry.addLayerToScene(sceneId, filePath);
        updatedDispatch.insert("sceneLayers", QJsonArray::fromStringList(m_sceneRegistry.layersForScene(sceneId)));
    }

    return updatedDispatch;
}

Dummy3DHostedViewWidget* MainWindow::currentThreeDView() const
{
    return dynamic_cast<Dummy3DHostedViewWidget*>(m_centerTabs ? m_centerTabs->currentWidget() : nullptr);
}

Dummy3DHostedViewWidget* MainWindow::threeDViewContainingFile(const QString& filePath) const
{
    const QString normalizedPath = filePath.trimmed();
    if(normalizedPath.isEmpty() || !m_centerTabs) {
        return nullptr;
    }

    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(Dummy3DHostedViewWidget* threeDView = dynamic_cast<Dummy3DHostedViewWidget*>(m_centerTabs->widget(i))) {
            if(threeDView->hasLoadedFile(normalizedPath)) {
                return threeDView;
            }
        }
    }

    return nullptr;
}

QList<Dummy3DHostedViewWidget*> MainWindow::openThreeDViews() const
{
    QList<Dummy3DHostedViewWidget*> views;
    if(!m_centerTabs) {
        return views;
    }

    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(Dummy3DHostedViewWidget* threeDView = dynamic_cast<Dummy3DHostedViewWidget*>(m_centerTabs->widget(i))) {
            views.append(threeDView);
        }
    }

    return views;
}

int MainWindow::centerTabIndexForWidget(QWidget* widget) const
{
    if(!m_centerTabs || !widget) {
        return -1;
    }

    for(int i = 0; i < m_centerTabs->count(); ++i) {
        if(m_centerTabs->widget(i) == widget) {
            return i;
        }
    }

    return -1;
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
        "#resultsSectionLabel { color: #c9d1d9; font-weight: 600; padding: 0; background: transparent; border: none; }"
        "#resultsMetaLabel { color: #8b949e; padding: 2px 0 6px 0; background: transparent; border: none; }"
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
    const bool workflowSelected = sectionName == "workflow";
    const bool skillsSelected = sectionName == "skills";
    const bool extensionsSelected = sectionName == "extensions";
    m_explorerButton->setChecked(workspaceSelected);
    m_workflowButton->setChecked(workflowSelected);
    m_skillsButton->setChecked(skillsSelected);
    m_extensionsButton->setChecked(extensionsSelected);

    if(workspaceSelected) {
        m_leftSidebarStack->setCurrentWidget(m_workspaceExplorer);
    } else if(workflowSelected) {
        m_leftSidebarStack->setCurrentWidget(m_workflowPage);
    } else if(skillsSelected) {
        m_leftSidebarStack->setCurrentWidget(m_skillsPage);
    } else {
        m_leftSidebarStack->setCurrentWidget(m_extensionsPage);
    }

    if(workspaceSelected) {
        statusBar()->showMessage("Workspace explorer active");
    } else if(workflowSelected) {
        statusBar()->showMessage("Workflow navigator active");
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
                const QString stepDescription = i < llmPlan.plannedStepDescriptions.size()
                    ? llmPlan.plannedStepDescriptions.at(i).trimmed()
                    : QString();
                if(!stepDescription.isEmpty()) {
                    m_agentChatDock->appendTranscript(QString("Planner> Step %1/%2 — %3")
                                                          .arg(i + 1)
                                                          .arg(llmPlan.plannedCommands.size())
                                                          .arg(stepDescription));
                }
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
    if(widget == m_workflowCenterView) {
        m_workflowCenterView = nullptr;
        m_workflowCenterSummaryLabel = nullptr;
        m_workflowCenterMiniMap = nullptr;
        m_workflowCenterDetailsView = nullptr;
        m_workflowCenterOpenSourceButton = nullptr;
        m_workflowCenterDockButton = nullptr;
        m_workflowCenterAddStepButton = nullptr;
        m_workflowCenterSaveButton = nullptr;
    }
    if(ExtensionHostedViewWidget* hostedView = qobject_cast<ExtensionHostedViewWidget*>(widget)) {
        m_extensionViewWidgetsBySessionId.remove(hostedView->sessionId());
    }
    m_centerTabs->removeTab(index);
    delete widget;
    refreshWorkflowGraphDockingUi();
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
    const QString normalizedToolName = normalizedPlannerToolName(toolName.trimmed().isEmpty()
        ? result.value("tool_name").toString().trimmed()
        : toolName.trimmed());
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
    if(normalizedToolName == QLatin1String("studio.workflow.load")) {
        suggestedTools.append("studio.workflow.active_graph");
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
    const QString effectiveToolName = normalizedPlannerToolName(toolName.trimmed().isEmpty()
        ? result.value("tool_name").toString().trimmed()
        : toolName.trimmed());
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

QJsonArray MainWindow::analysisPipelineToolDefinitions() const
{
    if(!m_cachedExtensionToolDefinitions.isEmpty()) {
        QJsonArray tools;
        for(const QJsonValue& value : m_cachedExtensionToolDefinitions) {
            const QJsonObject tool = value.toObject();
            if(isAnalysisPipelineCapability(tool)) {
                tools.append(tool);
            }
        }
        if(!tools.isEmpty()) {
            return tools;
        }
    }

    if(!m_viewProviderRegistry) {
        return QJsonArray();
    }

    return m_viewProviderRegistry->analysisPipelineToolDefinitions();
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

QString MainWindow::validateAnalysisPipelineContract(const QJsonObject& pipeline,
                                                    const QJsonObject& pipelineInputs) const
{
    const QString pipelineId = pipeline.value("id").toString().trimmed();
    const QJsonArray requiredInputs = pipeline.value("input_schema").toObject().value("required").toArray();
    for(const QJsonValue& requiredValue : requiredInputs) {
        const QString inputName = requiredValue.toString().trimmed();
        if(inputName.isEmpty()) {
            continue;
        }
        if(!pipelineInputs.contains(inputName) || pipelineInputs.value(inputName).isUndefined() || pipelineInputs.value(inputName).isNull()) {
            return QString("Pipeline `%1` is missing required input `%2`.").arg(pipelineId, inputName);
        }
    }

    const QJsonArray steps = pipeline.value("steps").toArray();
    if(steps.isEmpty()) {
        return pipelineId.isEmpty()
            ? QString("Pipeline defines no executable steps.")
            : QString("Pipeline `%1` defines no executable steps.").arg(pipelineId);
    }

    const QStringList resolvablePlaceholders = QStringList(pipelineInputs.keys())
        << QStringLiteral("last_peak_sample")
        << QStringLiteral("last_cursor_sample");

    for(int index = 0; index < steps.size(); ++index) {
        const QJsonObject step = steps.at(index).toObject();
        const QString toolName = step.value("tool_name").toString().trimmed();
        if(toolName.isEmpty()) {
            return QString("Step %1 in pipeline `%2` is missing `tool_name`.")
                .arg(index + 1)
                .arg(pipelineId);
        }

        if(normalizedPlannerToolName(toolName) == QLatin1String("studio.pipeline.run")) {
            return QString("Step %1 in pipeline `%2` recursively invokes another pipeline run, which is not supported.")
                .arg(index + 1)
                .arg(pipelineId);
        }

        if(toolDefinition(toolName).isEmpty()) {
            return QString("Step %1 in pipeline `%2` references unknown tool `%3`.")
                .arg(index + 1)
                .arg(pipelineId, toolName);
        }

        const QJsonObject argumentsTemplate = step.value("arguments_template").toObject();
        for(auto it = argumentsTemplate.constBegin(); it != argumentsTemplate.constEnd(); ++it) {
            if(!it.value().isString()) {
                continue;
            }

            const QStringList placeholders = extractTemplatePlaceholders(it.value().toString());
            for(const QString& placeholder : placeholders) {
                if(!resolvablePlaceholders.contains(placeholder)) {
                    return QString("Step %1 in pipeline `%2` uses unresolved placeholder `${%3}`.")
                        .arg(index + 1)
                        .arg(pipelineId, placeholder);
                }
            }
        }
    }

    return QString();
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

    const QString validationError = validateAnalysisPipelineContract(pipeline, pipelineInputs);
    if(!validationError.isEmpty()) {
        appendProblemMessage(validationError);
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
                plannedCommand = QString("tools.call %1 {}").arg(pipelineRunAliasToolName(pipelineId));
                return QString("Mapped your request to pipeline capability `%1`.").arg(pipelineId);
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
        requestExtensionHostState();
        return QString("Studio tools: %1 | Requested kernel and extension-host tool lists.")
            .arg(formatToolDefinitions(availableToolDefinitions()));
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
    const QString rawToolName = separatorIndex >= 0 ? invocation.left(separatorIndex).trimmed()
                                                    : invocation;
    const QString argumentsText = separatorIndex >= 0 ? invocation.mid(separatorIndex + 1).trimmed()
                                                      : QString();

    if(rawToolName.isEmpty()) {
        return "Usage: tools.call <tool_name> {json_arguments}";
    }

    const QString toolName = normalizedPlannerToolName(rawToolName);
    const QString pipelineAliasId = pipelineIdFromPipelineRunAliasToolName(rawToolName);

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

    if(toolName == "studio.workflow.active_graph") {
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"source_file", m_activeWorkflowFilePath},
            {"graph_resource_uri", kActiveWorkflowGraphUri},
            {"graph", m_activeWorkflowGraph}
        }, "workbench"));

        if(m_activeWorkflowGraph.isEmpty()) {
            return "No active workflow graph.";
        }

        return QString("Active workflow graph: %1 resources, %2 nodes.")
            .arg(m_activeWorkflowGraph.value("resources").toArray().size())
            .arg(m_activeWorkflowGraph.value("pipeline").toArray().size());
    }

    if(toolName == "studio.workflow.load") {
        const QString filePath = arguments.value("file").toString().trimmed();
        if(filePath.isEmpty()) {
            return "Tool studio.workflow.load requires {\"file\": \"/absolute/path/to/workflow.mne\"}.";
        }

        if(!QFileInfo::exists(filePath)) {
            return QString("Workflow file does not exist: %1").arg(filePath);
        }

        requestWorkflowLoad(filePath);
        rememberToolResult(toolName, normalizedToolResultEnvelope(toolName, QJsonObject{
            {"tool_name", toolName},
            {"status", "queued"},
            {"source_file", filePath},
            {"message", QString("Requested workflow activation for %1.").arg(QFileInfo(filePath).fileName())}
        }, "workbench"));
        return QString("Requested workflow activation for %1.").arg(QFileInfo(filePath).fileName());
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
        const QString pipelineId = pipelineAliasId.isEmpty()
            ? arguments.value("pipeline_id").toString().trimmed()
            : pipelineAliasId;
        if(pipelineId.isEmpty()) {
            return "Tool studio.pipeline.run requires {\"pipeline_id\": \"...\"}.";
        }

        const QJsonObject pipeline = analysisPipelineContract(pipelineId);
        if(pipeline.isEmpty()) {
            return QString("Unknown analysis pipeline: %1").arg(pipelineId);
        }

        QJsonObject pipelineInputs = defaultInputsForPipeline(pipeline);
        QJsonObject requestedInputs = arguments.value("inputs").toObject();
        if(!pipelineAliasId.isEmpty()) {
            requestedInputs = arguments.value("inputs").isObject() ? arguments.value("inputs").toObject()
                                                                   : arguments;
            requestedInputs.remove(QStringLiteral("pipeline_id"));
        }
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

    if(toolName.startsWith("dummy3d.") || toolName.startsWith("extension.") || isExtensionHostTool(toolName)) {
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
        return "Commands: help, tools.list, tools.call <tool> {json}, views.list, workflow.graph, settings.list, pipelines.list, pipeline.run <id>, pipeline.resume <run_id>, pipeline.rerun_step <run_id> <step>, raw.summary, raw.state, raw.goto_sample <n>, raw.cursor <n>, raw.zoom <px_per_sample>, kernel.raw_stats [window_samples], kernel.channel_stats [window_samples], kernel.psd [window_samples] [match].";
    }

    if(command == "views.list") {
        QStringList views;
        for(int i = 0; i < m_centerTabs->count(); ++i) {
            views << QString("%1: %2").arg(i + 1).arg(m_centerTabs->tabText(i));
        }
        return views.isEmpty() ? "No open views." : QString("Open views: %1").arg(views.join(" | "));
    }

    if(command == "workflow.graph") {
        if(m_activeWorkflowGraph.isEmpty()) {
            return "No active workflow graph.";
        }

        return QString("Active workflow graph: %1 resources, %2 nodes.")
            .arg(m_activeWorkflowGraph.value("resources").toArray().size())
            .arg(m_activeWorkflowGraph.value("pipeline").toArray().size());
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
            {"name", "studio.workflow.active_graph"},
            {"description", "Return the active declarative workflow graph currently loaded in the studio."},
            {"input_schema", objectSchema(QJsonObject())},
            {"result_schema", objectSchema(QJsonObject{
                 {"source_file", stringSchema("Workflow File")},
                 {"graph_resource_uri", stringSchema("Graph Resource URI")},
                 {"graph", objectSchema(QJsonObject())}
             }, QJsonArray{"graph_resource_uri", "graph"})}
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
    requestActiveWorkflowGraph();
    if(m_activeWorkflowGraph.isEmpty() && !m_activeWorkflowFilePath.trimmed().isEmpty() && QFileInfo::exists(m_activeWorkflowFilePath)) {
        requestWorkflowLoad(m_activeWorkflowFilePath);
    }
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
    QVector<CapabilityCatalogSource> sources;
    sources.append(CapabilityCatalogSource{
        QStringLiteral("workbench_local"),
        QStringLiteral("Workbench Tools"),
        localToolDefinitions()
    });
    sources.append(CapabilityCatalogSource{
        QStringLiteral("hosted_view"),
        QStringLiteral("Active Hosted View Tools"),
        activeHostedViewToolDefinitions()
    });
    sources.append(CapabilityCatalogSource{
        QStringLiteral("neurokernel"),
        QStringLiteral("Neuro-Kernel Tools"),
        kernelToolDefinitions()
    });

    QJsonArray workflowSkillTools;
    QJsonArray workflowIoTools;
    QJsonArray extensionTools;
    if(!m_cachedExtensionToolDefinitions.isEmpty()) {
        for(const QJsonValue& value : m_cachedExtensionToolDefinitions) {
            const QJsonObject tool = value.toObject();
            if(isAnalysisPipelineCapability(tool)) {
                continue;
            }

            if(isWorkflowSkillCapability(tool)) {
                workflowSkillTools.append(tool);
                continue;
            }

            if(isWorkflowIoCapability(tool)) {
                workflowIoTools.append(tool);
                continue;
            }

            extensionTools.append(tool);
        }
    } else if(m_viewProviderRegistry) {
        extensionTools = m_viewProviderRegistry->toolDefinitions();
    }
    sources.append(CapabilityCatalogSource{
        QStringLiteral("workflow_skill"),
        QStringLiteral("Workflow Skills"),
        workflowSkillTools
    });
    sources.append(CapabilityCatalogSource{
        QStringLiteral("workflow_io"),
        QStringLiteral("Workflow Actions"),
        workflowIoTools
    });
    sources.append(CapabilityCatalogSource{
        QStringLiteral("extension_host"),
        QStringLiteral("Extension Tools"),
        extensionTools
    });
    sources.append(CapabilityCatalogSource{
        QStringLiteral("analysis_pipeline"),
        QStringLiteral("Analysis Pipelines"),
        analysisPipelineToolDefinitions()
    });

    return buildCapabilityCatalog(sources);
}

QJsonObject MainWindow::plannerAnnotatedToolDefinition(const QJsonObject& rawTool) const
{
    QJsonObject tool = rawTool;
    if(tool.isEmpty()) {
        return tool;
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

    return tool;
}

QJsonArray MainWindow::plannerAnnotatedToolDefinitions() const
{
    QJsonArray annotatedTools;
    const QJsonArray tools = availableToolDefinitions();
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = plannerAnnotatedToolDefinition(value.toObject());
        if(!tool.isEmpty()) {
            annotatedTools.append(tool);
        }
    }

    return annotatedTools;
}

QJsonObject MainWindow::plannerSafetyMetadata(const QString& toolName) const
{
    const QString trimmedName = toolName.trimmed();
    const QString normalizedName = normalizedPlannerToolName(trimmedName);
    if(trimmedName.isEmpty()) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "blocked"},
            {"reason", "Unnamed tools are not exposed to the LLM planner."}
        };
    }

    if(normalizedName.endsWith(".set") && normalizedName.startsWith("settings.")) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "high"},
            {"reason", "Mutates persisted extension settings."}
        };
    }

    if(normalizedName.startsWith("view.hosted.")
       || normalizedName.startsWith("dummy3d.")
       || normalizedName.startsWith("extension.")) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "medium"},
            {"reason", "Requires active hosted extension sessions with side effects that are harder for the planner to validate."}
        };
    }

    if(normalizedName == QLatin1String("studio.pipeline.rerun_step")) {
        return QJsonObject{
            {"planner_safe", false},
            {"risk_level", "medium"},
            {"reason", "Replays historical pipeline state and is better triggered from explicit user intent."}
        };
    }

    if(normalizedName == QLatin1String("studio.pipeline.resume")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "medium"},
            {"requires_active_context", false},
            {"reason", "Resumes an existing pipeline run and is safe when explicitly chosen from known saved runs."}
        };
    }

    if(normalizedName == QLatin1String("studio.pipeline.run")
       || normalizedName == QLatin1String("view.raw.goto")
       || normalizedName == QLatin1String("view.raw.zoom")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "medium"},
            {"requires_active_context", true},
            {"reason", "State-changing but reversible workbench action."}
        };
    }

    if(normalizedName == QLatin1String("view.raw.summary")
       || normalizedName == QLatin1String("view.raw.state")
       || normalizedName == QLatin1String("studio.views.list")
       || normalizedName == QLatin1String("studio.workflow.active_graph")
       || normalizedName == QLatin1String("studio.settings.list")
       || normalizedName == QLatin1String("studio.pipelines.list")
       || normalizedName.endsWith(".get")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "low"},
            {"requires_active_context", normalizedName.startsWith("view.raw.")},
            {"reason", "Read-only inspection tool."}
        };
    }

    if(normalizedName == QLatin1String("studio.workflow.load")) {
        return QJsonObject{
            {"planner_safe", true},
            {"risk_level", "medium"},
            {"requires_active_context", false},
            {"reason", "Activates a declarative workflow file while preserving direct file browsing."}
        };
    }

    if(normalizedName.startsWith("neurokernel.")) {
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
    const QString normalizedName = normalizedPlannerToolName(trimmedName);
    const QString pipelineAliasId = pipelineIdFromPipelineRunAliasToolName(trimmedName);
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

    if(normalizedName == QLatin1String("studio.views.list")
       || normalizedName == QLatin1String("studio.workflow.active_graph")
       || normalizedName == QLatin1String("studio.settings.list")
       || normalizedName == QLatin1String("studio.pipelines.list")
       || normalizedName.endsWith(".get")) {
        return readyEnvelope(true,
                             "Available without additional workspace context.",
                             QJsonArray(),
                             QJsonObject{{"has_raw_browser", hasRawBrowser},
                                         {"analysis_pipeline_count", pipelines.size()}});
    }

    if(normalizedName.startsWith("view.raw.") || normalizedName.startsWith("neurokernel.")) {
        QJsonObject details{{"has_raw_browser", hasRawBrowser}};
        QString reason = hasRawBrowser
            ? "Active raw browser is available."
            : "Requires an active raw browser with a loaded dataset.";
        if(normalizedName == QLatin1String("neurokernel.find_peak_window") && hasRawBrowser && !selectedChannelName.isEmpty()) {
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

    if(normalizedName == QLatin1String("studio.pipeline.run")) {
        const QJsonObject pipeline = pipelineAliasId.isEmpty() ? QJsonObject() : analysisPipelineContract(pipelineAliasId);
        const bool hasPipelines = pipelineAliasId.isEmpty() ? !pipelines.isEmpty() : !pipeline.isEmpty();
        const bool ready = hasPipelines && hasRawBrowser;
        QString reason = pipelineAliasId.isEmpty()
            ? QString("Ready to run manifest-declared pipelines against the active dataset.")
            : QString("Ready to run pipeline `%1` against the active dataset.").arg(pipelineAliasId);
        if(!hasPipelines) {
            reason = pipelineAliasId.isEmpty()
                ? QString("No manifest-declared analysis pipelines are currently available.")
                : QString("Pipeline capability `%1` is not available in the loaded manifests.").arg(pipelineAliasId);
        } else if(!hasRawBrowser) {
            reason = "Requires an active raw browser before dataset-bound pipelines can run.";
        }

        QJsonObject details{
            {"analysis_pipeline_count", pipelines.size()},
            {"has_raw_browser", hasRawBrowser}
        };
        if(!pipelineAliasId.isEmpty()) {
            details.insert("pipeline_id", pipelineAliasId);
        }
        if(ready && !pipeline.isEmpty()) {
            const QString validationError = validateAnalysisPipelineContract(pipeline, defaultInputsForPipeline(pipeline));
            if(!validationError.isEmpty()) {
                details.insert("validation_error", validationError);
                return readyEnvelope(false,
                                     QString("Pipeline `%1` is currently invalid: %2").arg(pipelineAliasId, validationError),
                                     QJsonArray{"analysis_pipeline_contract", "active_raw_browser"},
                                     details);
            }
        }

        return readyEnvelope(ready,
                             reason,
                             QJsonArray{"analysis_pipeline_contract", "active_raw_browser"},
                             details);
    }

    if(normalizedName == QLatin1String("studio.pipeline.resume")) {
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

    if(normalizedName == QLatin1String("studio.pipeline.rerun_step")) {
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

    if(normalizedName.startsWith("view.hosted.")) {
        const bool ready = !hostedSession.isEmpty();
        return readyEnvelope(ready,
                             ready
                                 ? "Active hosted extension session is available."
                                 : "Requires an active hosted extension view session.",
                             QJsonArray{"active_hosted_view"},
                             QJsonObject{{"has_hosted_session", ready},
                                         {"session_id", hostedSession.value("session_id").toString()}});
    }

    if(normalizedName.startsWith("settings.")) {
        const bool ready = !resolveExtensionSettingsTool(normalizedName).isEmpty();
        return readyEnvelope(ready,
                             ready
                                 ? "Resolved to a manifest-declared extension settings field."
                                 : "Referenced settings field is not available in the loaded extension manifests.",
                             QJsonArray{"extension_settings_contract"},
                             QJsonObject{{"resolved", ready}});
    }

    if(normalizedName == QLatin1String("studio.workflow.load")) {
        const bool ready = !m_activeWorkflowFilePath.trimmed().isEmpty();
        return readyEnvelope(ready,
                             ready
                                 ? "A workflow file is already known and can be reactivated."
                                 : "Requires an explicit .mne file path.",
                             QJsonArray{"workflow_file_path"},
                             QJsonObject{{"active_workflow_file", m_activeWorkflowFilePath}});
    }

    return readyEnvelope(true, "No additional runtime context required.");
}

QJsonObject MainWindow::plannerExecutionMetadata(const QString& toolName) const
{
    const QString trimmedName = toolName.trimmed();
    const QString normalizedName = normalizedPlannerToolName(trimmedName);
    if(trimmedName.isEmpty()) {
        return QJsonObject{
            {"execution_mode", "suggestion_only"},
            {"execution_reason", "Unnamed tools cannot be safely executed by the planner."}
        };
    }

    // Global safety level override: "confirm" demotes auto_run → confirm_required;
    // "safe" demotes everything → suggestion_only.
    const QString safetyLevel = m_plannerSafetyLevel.trimmed().toLower();
    if(safetyLevel == QLatin1String("safe")) {
        return QJsonObject{
            {"execution_mode", "suggestion_only"},
            {"execution_reason", "Safety level is set to Safe — all steps are suggestions only."}
        };
    }

    if(normalizedName == QLatin1String("studio.views.list")
       || normalizedName == QLatin1String("studio.workflow.active_graph")
       || normalizedName == QLatin1String("studio.settings.list")
       || normalizedName == QLatin1String("studio.pipelines.list")
       || normalizedName == QLatin1String("view.raw.summary")
       || normalizedName == QLatin1String("view.raw.state")
       || normalizedName.endsWith(".get")
       || normalizedName.startsWith("neurokernel.")) {
        // "confirm" level demotes auto_run to confirm_required
        const QString mode = (safetyLevel == QLatin1String("confirm"))
            ? QStringLiteral("confirm_required")
            : QStringLiteral("auto_run");
        const QString reason = (safetyLevel == QLatin1String("confirm"))
            ? QStringLiteral("Safety level is set to Confirm — all steps require user approval.")
            : QStringLiteral("Read-only inspection or dataset analysis is safe to execute automatically.");
        return QJsonObject{{"execution_mode", mode}, {"execution_reason", reason}};
    }

    if(normalizedName == QLatin1String("view.raw.goto")
       || normalizedName == QLatin1String("view.raw.zoom")
       || normalizedName == QLatin1String("studio.workflow.load")
       || normalizedName == QLatin1String("studio.pipeline.run")
       || normalizedName == QLatin1String("studio.pipeline.resume")) {
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
    const QJsonArray tools = plannerAnnotatedToolDefinitions();
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = value.toObject();
        if(tool.value("planner_safe").toBool(false)) {
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
    const QJsonArray tools = plannerAnnotatedToolDefinitions();
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = value.toObject();
        if(!tool.value("planner_safe").toBool(false) || !tool.value("planner_ready").toBool(true)) {
            blockedTools.append(tool);
        }
    }

    return blockedTools;
}

QJsonObject MainWindow::toolDefinition(const QString& toolName) const
{
    return plannerAnnotatedToolDefinition(capabilityFromCatalog(availableToolDefinitions(), toolName));
}

QString MainWindow::toolNameFromCommand(const QString& commandText) const
{
    return normalizedPlannerToolName(rawToolNameFromCommandText(commandText));
}

QJsonObject MainWindow::toolArgumentsFromCommand(const QString& commandText) const
{
    const QString trimmedCommand = commandText.trimmed();
    const QString rawToolName = rawToolNameFromCommandText(trimmedCommand);
    if(rawToolName.isEmpty()) {
        return QJsonObject();
    }

    const int toolStart = trimmedCommand.indexOf(rawToolName);
    if(toolStart < 0) {
        return QJsonObject();
    }

    const QString argumentsText = trimmedCommand.mid(toolStart + rawToolName.size()).trimmed();
    if(argumentsText.isEmpty()) {
        const QString pipelineId = pipelineIdFromPipelineRunAliasToolName(rawToolName);
        if(!pipelineId.isEmpty()) {
            return QJsonObject{
                {"pipeline_id", pipelineId},
                {"inputs", QJsonObject()}
            };
        }
        return QJsonObject();
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(argumentsText.toUtf8(), &error);
    if(error.error == QJsonParseError::NoError && document.isObject()) {
        QJsonObject arguments = document.object();
        const QString pipelineId = pipelineIdFromPipelineRunAliasToolName(rawToolName);
        if(!pipelineId.isEmpty()) {
            QJsonObject pipelineInputs = arguments;
            if(arguments.value("inputs").isObject()) {
                pipelineInputs = arguments.value("inputs").toObject();
            }
            pipelineInputs.remove(QStringLiteral("pipeline_id"));
            return QJsonObject{
                {"pipeline_id", pipelineId},
                {"inputs", pipelineInputs}
            };
        }
        return arguments;
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
    } else if(toolName == QLatin1String("studio.workflow.load")) {
        const QString filePath = arguments.value("file").toString().trimmed();
        title = filePath.isEmpty()
            ? QString("Activate Workflow File")
            : QString("Activate Workflow %1").arg(QFileInfo(filePath).fileName());
        details = filePath.isEmpty()
            ? QString("Load the requested .mne workflow file.")
            : QString("Load `%1` as the active declarative workflow graph.").arg(filePath);
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
    } else if(toolName == QLatin1String("studio.workflow.load")) {
        snapshot.insert("workflow_file", arguments.value("file").toString().trimmed());
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

    const QString snapshotWorkflowFile = snapshot.value("workflow_file").toString().trimmed();
    if(toolName == QLatin1String("studio.workflow.load")
       && !snapshotWorkflowFile.isEmpty()
       && snapshotWorkflowFile != m_activeWorkflowFilePath.trimmed()
       && !m_activeWorkflowFilePath.trimmed().isEmpty()) {
        reasons << QString("active workflow changed from `%1` to `%2`.")
                       .arg(QFileInfo(snapshotWorkflowFile).fileName(),
                            QFileInfo(m_activeWorkflowFilePath).fileName());
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

    const QJsonArray capabilityCatalog = availableToolDefinitions();
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
        {"has_active_workflow_graph", !m_activeWorkflowGraph.isEmpty()},
        {"active_workflow_file", m_activeWorkflowFilePath},
        {"active_workflow_graph_resource_uri", kActiveWorkflowGraphUri},
        {"active_workflow_graph", m_activeWorkflowGraph},
        {"capability_catalog", capabilityCatalog},
        {"tool_schema_summary", capabilityCatalog},
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
    const QString pipelineAliasId = pipelineIdFromPipelineRunAliasToolName(toolName);
    if(!pipelineAliasId.isEmpty()) {
        const QJsonObject pipeline = analysisPipelineContract(pipelineAliasId);
        return pipeline.isEmpty() ? QJsonObject() : defaultInputsForPipeline(pipeline);
    }

    if(toolName == "studio.views.list"
       || toolName == "studio.workflow.active_graph"
       || toolName == "view.raw.summary"
       || toolName == "view.raw.state") {
        return QJsonObject();
    }

    if(toolName == "studio.workflow.load") {
        return QJsonObject{{"file", m_activeWorkflowFilePath}};
    }

    if(toolName == "studio.workflow.save") {
        return m_activeWorkflowFilePath.trimmed().isEmpty()
            ? QJsonObject()
            : QJsonObject{{"file", m_activeWorkflowFilePath}};
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

    const QJsonArray capabilityCatalog = availableToolDefinitions();

    QTreeWidgetItem* localRoot = new QTreeWidgetItem(QStringList() << "Workbench Tools");
    const QJsonArray localTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("workbench_local"));
    for(const QJsonValue& value : localTools) {
        localRoot->addChild(makeToolTreeItem(value.toObject()));
    }
    localRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(localRoot);

    QTreeWidgetItem* kernelRoot = new QTreeWidgetItem(QStringList() << "Neuro-Kernel Tools");
    const QJsonArray kernelTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("neurokernel"));
    if(kernelTools.isEmpty()) {
        kernelRoot->addChild(new QTreeWidgetItem(QStringList() << "No tools discovered yet."));
    } else {
        for(const QJsonValue& value : kernelTools) {
            kernelRoot->addChild(makeToolTreeItem(value.toObject()));
        }
    }
    kernelRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(kernelRoot);

    QTreeWidgetItem* workflowSkillsRoot = new QTreeWidgetItem(QStringList() << "Workflow Skills");
    const QJsonArray workflowSkillTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("workflow_skill"));
    if(workflowSkillTools.isEmpty()) {
        workflowSkillsRoot->addChild(new QTreeWidgetItem(QStringList() << "No workflow skills discovered yet."));
    } else {
        for(const QJsonValue& value : workflowSkillTools) {
            workflowSkillsRoot->addChild(makeToolTreeItem(value.toObject()));
        }
    }
    workflowSkillsRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(workflowSkillsRoot);

    QTreeWidgetItem* workflowActionsRoot = new QTreeWidgetItem(QStringList() << "Workflow Actions");
    const QJsonArray workflowIoTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("workflow_io"));
    if(workflowIoTools.isEmpty()) {
        workflowActionsRoot->addChild(new QTreeWidgetItem(QStringList() << "No workflow graph actions discovered yet."));
    } else {
        for(const QJsonValue& value : workflowIoTools) {
            workflowActionsRoot->addChild(makeToolTreeItem(value.toObject()));
        }
    }
    workflowActionsRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(workflowActionsRoot);

    QTreeWidgetItem* extensionRoot = new QTreeWidgetItem(QStringList() << "Extension Tools");
    const QJsonArray extensionTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("extension_host"));
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
    const QJsonArray hostedTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("hosted_view"));
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
    const QJsonArray pipelineTools = capabilitiesFromCatalogBySource(capabilityCatalog, QStringLiteral("analysis_pipeline"));
    if(pipelineTools.isEmpty()) {
        pipelinesRoot->addChild(new QTreeWidgetItem(QStringList() << "No analysis pipelines discovered yet."));
    } else {
        for(const QJsonValue& value : pipelineTools) {
            const QJsonObject pipelineTool = value.toObject();
            const QString label = QString("%1 (%2)")
                                      .arg(pipelineTool.value("display_name").toString(pipelineTool.value("pipeline_id").toString()),
                                           pipelineTool.value("extension_display_name").toString());
            QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << label);
            item->setToolTip(0, pipelineTool.value("description").toString());
            item->setData(0, Qt::UserRole, pipelineTool.value("name").toString());
            item->setData(0, Qt::UserRole + 1, pipelineTool);
            item->setData(0, Qt::UserRole + 2, pipelineTool.value("pipeline_id").toString());
            pipelinesRoot->addChild(item);
        }
    }
    pipelinesRoot->setExpanded(true);
    m_skillsExplorer->addTopLevelItem(pipelinesRoot);

    QTreeWidgetItem* resourceRoot = new QTreeWidgetItem(QStringList() << "Live Resources");
    if(m_cachedExtensionResources.isEmpty()) {
        resourceRoot->addChild(new QTreeWidgetItem(QStringList() << "No live extension or workflow resources discovered yet."));
    } else {
        for(const QJsonValue& value : m_cachedExtensionResources) {
            const QJsonObject resource = value.toObject();
            const QString detail = resource.value("version").toString().trimmed().isEmpty()
                ? resource.value("kind").toString(resource.value("uri").toString())
                : resource.value("version").toString();
            const QString label = QString("%1 (%2)")
                                      .arg(resource.value("display_name").toString(),
                                           detail);
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
    statusRoot->addChild(new QTreeWidgetItem(QStringList() << QString("Live Resources: %1").arg(m_cachedExtensionResources.size())));
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
            if(payload.value("uri").toString().trimmed() == QLatin1String(kActiveWorkflowGraphUri)
               && !m_activeWorkflowGraph.isEmpty()) {
                QJsonObject details = payload;
                details.insert("graph", m_activeWorkflowGraph);
                details.insert("source_file", m_activeWorkflowFilePath);
                m_skillDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(details).toJson(QJsonDocument::Indented)));
            } else {
                m_skillDetailsView->setPlainText(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Indented)));
            }
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
            {"run_tool", pipelineRunAliasToolName(pipelineId)},
            {"suggested_arguments", defaultInputsForPipeline(toolDefinition)}
        };
    } else {
        preview = QJsonObject{
            {"tool", toolName},
            {"definition", toolDefinition},
            {"suggested_arguments", workflowOperatorToolDefinition(toolName).isEmpty()
                                      ? defaultArgumentsForTool(toolName)
                                      : defaultArgumentsForWorkflowTool(toolName)}
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
        const QString pipelineToolName = pipelineRunAliasToolName(m_selectedPipelineId);
        QJsonObject arguments = defaultInputsForPipeline(pipeline);

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
            ? QString("tools.call %1 {}").arg(pipelineToolName)
            : QString("tools.call %1 %2").arg(pipelineToolName, trimmedArguments);

        appendTerminalMessage(QString("$ %1").arg(commandText));
        sendToolCall(commandText);
        return;
    }

    QJsonObject selectedArguments = workflowOperatorToolDefinition(m_selectedSkillToolName).isEmpty()
        ? defaultArgumentsForTool(m_selectedSkillToolName)
        : defaultArgumentsForWorkflowTool(m_selectedSkillToolName);
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
    const LlmPlannerConfig previousConfig = m_llmPlanner.configuration();
    LlmSettingsDialog dialog(previousConfig, this);
    dialog.setTestScenario("go to the strongest EEG burst and then show me the top EEG channels there",
                           plannerReadyToolDefinitions(),
                           llmPlanningContext("planner settings test"));
    if(dialog.exec() != QDialog::Accepted) {
        return;
    }

    const LlmPlannerConfig updatedConfig = dialog.configuration();
    m_llmPlanner.setConfiguration(updatedConfig);
    persistAgentSettings();
    if(dialog.hasValidationResult()) {
        persistAgentValidationState(true, dialog.lastValidationSucceeded(), dialog.lastValidationMessage());
    } else if(previousConfig.mode != updatedConfig.mode
              || previousConfig.apiKey != updatedConfig.apiKey
              || previousConfig.model != updatedConfig.model
              || previousConfig.endpoint != updatedConfig.endpoint
              || previousConfig.providerName != updatedConfig.providerName) {
        persistAgentValidationState(false, false, QString());
    }
    refreshAgentPlannerStatus();
    refreshAgentConnectionSelectors();
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
    const QString storedApiKey = readSecretFromKeychain(activeAgentSecretAccountName());
    config.apiKey = storedApiKey.isEmpty()
        ? settings.value("agent/api_key").toString()
        : storedApiKey;
    config.model = settings.value("agent/model").toString();
    m_llmPlanner.setConfiguration(config);
    refreshAgentConnectionSelectors();
}

void MainWindow::persistAgentSettings() const
{
    const LlmPlannerConfig config = m_llmPlanner.configuration();
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    settings.setValue("agent/mode", config.mode);
    settings.setValue("agent/provider", config.providerName);
    settings.setValue("agent/endpoint", config.endpoint);
    if(!config.apiKey.trimmed().isEmpty() && writeSecretToKeychain(activeAgentSecretAccountName(), config.apiKey.trimmed())) {
        settings.remove("agent/api_key");
    } else {
        if(config.apiKey.trimmed().isEmpty()) {
            deleteSecretFromKeychain(activeAgentSecretAccountName());
        }
        settings.setValue("agent/api_key", config.apiKey);
    }
    settings.setValue("agent/model", config.model);
}

void MainWindow::persistAgentValidationState(bool hasResult, bool succeeded, const QString& message) const
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    settings.setValue("agent/validation/has_result", hasResult);
    settings.setValue("agent/validation/succeeded", succeeded);
    settings.setValue("agent/validation/message", message.trimmed());
}

void MainWindow::refreshAgentPlannerStatus()
{
    m_agentChatDock->setPlannerStatus(m_llmPlanner.statusSummary());
}

void MainWindow::refreshAgentConnectionSelectors()
{
    if(!m_agentChatDock) {
        return;
    }

    QList<QPair<QString, QString>> modes;
    modes << qMakePair(QString("Rule-Based"), QString("disabled"))
          << qMakePair(QString("OpenAI"), QString("openai_responses"))
          << qMakePair(QString("Gemini"), QString("gemini_openai"));
    m_agentChatDock->setConnectionModes(modes, m_llmPlanner.configuration().mode);

    const LlmPlannerConfig config = m_llmPlanner.configuration();
    const QString currentMode = config.mode.trimmed();
    QStringList modelChoices;
    if(currentMode == QLatin1String("openai_responses")) {
        modelChoices << "gpt-5-mini" << "gpt-5" << "gpt-4.1-mini";
    } else if(currentMode == QLatin1String("gemini_openai")) {
        modelChoices << "gemini-2.5-flash" << "gemini-2.5-pro" << "gemini-2.0-flash";
    }
    const QString currentModel = config.model.trimmed();
    if(!currentModel.isEmpty() && !modelChoices.contains(currentModel)) {
        modelChoices.prepend(currentModel);
    }
    m_agentChatDock->setSuggestedModels(modelChoices, currentModel);

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    settings.beginGroup("agent/profiles");
    const QStringList profiles = settings.childGroups();
    settings.endGroup();
    m_agentChatDock->setConnectionProfiles(profiles, settings.value("agent/selected_profile").toString());

    // Restore persisted safety level
    const QString persistedSafetyLevel =
        settings.value("agent/planner_safety_level", "auto").toString().trimmed().toLower();
    if(m_plannerSafetyLevel.isEmpty()) {
        m_plannerSafetyLevel = persistedSafetyLevel;
    }
    m_agentChatDock->setPlannerSafetyLevel(m_plannerSafetyLevel.isEmpty()
                                                ? QStringLiteral("auto")
                                                : m_plannerSafetyLevel);

    const bool localMode = currentMode == QLatin1String("disabled")
                           || currentMode == QLatin1String("mock");
    const bool needsEndpoint = currentMode == QLatin1String("http");
    const bool missingKey = !localMode
                            && config.apiKey.trimmed().isEmpty();
    const bool missingModel = !localMode
                              && config.model.trimmed().isEmpty();
    const bool missingEndpoint = needsEndpoint
                                 && config.endpoint.trimmed().isEmpty();

    const bool hasValidationResult = settings.value("agent/validation/has_result", false).toBool();
    const bool validationSucceeded = settings.value("agent/validation/succeeded", false).toBool();
    const QString validationMessage = settings.value("agent/validation/message").toString().trimmed();

    QString stateText = QString("Connected");
    bool warning = false;
    QString detailMessage;
    if(localMode) {
        stateText = QString("Rule-based");
        warning = true;
    } else if(missingKey) {
        stateText = QString("Connect");
        warning = true;
        detailMessage = QString("Add an API key in Connect Model to use %1.").arg(currentMode == QLatin1String("gemini_openai") ? QString("Gemini") : QString("OpenAI"));
    } else if(missingModel) {
        stateText = QString("Choose model");
        warning = true;
        detailMessage = QString("Pick a model for the selected provider.");
    } else if(missingEndpoint) {
        stateText = QString("Configure");
        warning = true;
        detailMessage = QString("Complete the provider setup before validating the connection.");
    } else if(!hasValidationResult) {
        stateText = QString("Needs validation");
        warning = true;
        detailMessage = QString("Open Connect Model and run Validate Connection.");
    } else if(!validationSucceeded) {
        stateText = QString("Validation failed");
        warning = true;
        detailMessage = validationMessage;
    } else if(validationSucceeded) {
        detailMessage = validationMessage.isEmpty() ? QString("Connection validated successfully.") : validationMessage;
    }

    m_agentChatDock->setConnectionState(stateText, warning, detailMessage);
}

void MainWindow::handleAgentConnectionProfileSelected(const QString& profileName)
{
    const QString trimmedProfile = profileName.trimmed();
    if(trimmedProfile.isEmpty() || trimmedProfile == QLatin1String("No Profile")) {
        return;
    }

    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString baseKey = QString("agent/profiles/%1").arg(trimmedProfile);
    LlmPlannerConfig config = m_llmPlanner.configuration();
    config.mode = settings.value(QString("%1/mode").arg(baseKey), config.mode).toString();
    config.providerName = settings.value(QString("%1/provider").arg(baseKey), config.providerName).toString();
    config.endpoint = settings.value(QString("%1/endpoint").arg(baseKey), config.endpoint).toString();
    const QString storedApiKey = readSecretFromKeychain(providerSecretAccountName(trimmedProfile));
    config.apiKey = storedApiKey.isEmpty()
        ? settings.value(QString("%1/api_key").arg(baseKey), config.apiKey).toString()
        : storedApiKey;
    config.model = settings.value(QString("%1/model").arg(baseKey), config.model).toString();
    settings.setValue("agent/selected_profile", trimmedProfile);
    m_llmPlanner.setConfiguration(config);
    persistAgentSettings();
    persistAgentValidationState(false, false, QString());
    refreshAgentPlannerStatus();
    refreshAgentConnectionSelectors();
}

void MainWindow::handleAgentConnectionModeSelected(const QString& mode)
{
    if(mode.trimmed().isEmpty()) {
        return;
    }

    LlmPlannerConfig config = m_llmPlanner.configuration();
    if(config.mode == mode) {
        return;
    }

    config.mode = mode;
    config.model.clear();
    if(mode == QLatin1String("openai_responses")) {
        config.providerName = QString("OpenAI");
        config.endpoint = QString("https://api.openai.com/v1/responses");
    } else if(mode == QLatin1String("gemini_openai")) {
        config.providerName = QString("Google Gemini");
        config.endpoint = QString("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
    } else {
        config.providerName = QString("Rule-Based");
        config.endpoint.clear();
    }
    m_llmPlanner.setConfiguration(config);
    persistAgentSettings();
    persistAgentValidationState(false, false, QString());
    refreshAgentPlannerStatus();
    refreshAgentConnectionSelectors();
}

void MainWindow::handleAgentConnectionModelSelected(const QString& model)
{
    const QString trimmedModel = model.trimmed();
    if(trimmedModel.isEmpty()) {
        return;
    }

    LlmPlannerConfig config = m_llmPlanner.configuration();
    if(config.model == trimmedModel) {
        return;
    }

    config.model = trimmedModel;
    m_llmPlanner.setConfiguration(config);
    persistAgentSettings();
    persistAgentValidationState(false, false, QString());
    refreshAgentPlannerStatus();
    refreshAgentConnectionSelectors();
}

void MainWindow::handleAgentPlannerSafetyLevelSelected(const QString& level)
{
    const QString normalized = level.trimmed().toLower();
    if(normalized != QLatin1String("auto")
       && normalized != QLatin1String("confirm")
       && normalized != QLatin1String("safe")) {
        return;
    }
    if(m_plannerSafetyLevel == normalized) {
        return;
    }
    m_plannerSafetyLevel = normalized;
    if(m_agentChatDock) {
        m_agentChatDock->setPlannerSafetyLevel(normalized);
    }
    // Persist so the setting survives restarts.
    QSettings persistSettings("MNE-CPP", "MNEAnalyzeStudio");
    persistSettings.setValue("agent/planner_safety_level", normalized);
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
                    const QJsonObject effectiveDescriptor = widget->property("mne_session_descriptor").toJsonObject().isEmpty()
                        ? sessionDescriptor
                        : widget->property("mne_session_descriptor").toJsonObject();
                    widget->setProperty("mne_session_id", sessionId);
                    widget->setProperty("mne_session_descriptor", effectiveDescriptor);
                    QMetaObject::invokeMethod(widget,
                                              "setSessionDescriptor",
                                              Qt::DirectConnection,
                                              Q_ARG(QJsonObject, effectiveDescriptor));
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

        const QJsonObject effectiveDescriptor = viewWidget->property("mne_session_descriptor").toJsonObject().isEmpty()
            ? sessionDescriptor
            : viewWidget->property("mne_session_descriptor").toJsonObject();
        viewWidget->setProperty("mne_session_id", sessionId);
        viewWidget->setProperty("mne_session_descriptor", effectiveDescriptor);

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

bool MainWindow::isWorkflowCenterViewOpen() const
{
    return m_workflowCenterView
        && m_centerTabs
        && m_centerTabs->indexOf(m_workflowCenterView) >= 0;
}

void MainWindow::refreshWorkflowGraphDockingUi()
{
    const bool graphOpenInCenter = isWorkflowCenterViewOpen();
    const bool hasActiveGraph = !m_activeWorkflowGraph.isEmpty();

    if(m_workflowMiniMap) {
        m_workflowMiniMap->setVisible(!graphOpenInCenter);
        m_workflowMiniMap->setMinimumHeight(graphOpenInCenter ? 0 : 180);
    }

    if(m_workflowMiniMapDockStateLabel) {
        m_workflowMiniMapDockStateLabel->setVisible(graphOpenInCenter);
        m_workflowMiniMapDockStateLabel->setText(graphOpenInCenter
                                                     ? QStringLiteral("Dependency map is open in the center view. Close or dock that tab to bring it back here.")
                                                     : QString());
    }

    if(m_workflowOpenGraphButton) {
        m_workflowOpenGraphButton->setEnabled(hasActiveGraph || graphOpenInCenter);
        m_workflowOpenGraphButton->setText(graphOpenInCenter
                                               ? QStringLiteral("Focus Graph Tab")
                                               : QStringLiteral("Open In Center"));
        m_workflowOpenGraphButton->setToolTip(graphOpenInCenter
                                                  ? QStringLiteral("Focus the workflow graph tab in the center view.")
                                                  : QStringLiteral("Open the workflow dependency map in a dedicated center tab."));
    }
}

void MainWindow::openWorkflowCenterView(bool focusTab)
{
    if(!m_workflowCenterView) {
        m_workflowCenterView = new QWidget(this);
        QVBoxLayout* containerLayout = new QVBoxLayout(m_workflowCenterView);
        containerLayout->setContentsMargins(10, 10, 10, 10);
        containerLayout->setSpacing(10);

        m_workflowCenterSummaryLabel = new QLabel(m_workflowCenterView);
        m_workflowCenterSummaryLabel->setWordWrap(true);
        m_workflowCenterSummaryLabel->setObjectName("resultsSectionLabel");
        containerLayout->addWidget(m_workflowCenterSummaryLabel);

        QSplitter* centerSplitter = new QSplitter(Qt::Horizontal, m_workflowCenterView);
        m_workflowCenterMiniMap = new WorkflowMiniMapWidget(centerSplitter);
        m_workflowCenterMiniMap->setMinimumHeight(420);
        m_workflowCenterMiniMap->setToolTip("Large dependency map for the active workflow DAG. Click a node to focus it.");

        QWidget* inspectorPane = new QWidget(centerSplitter);
        QVBoxLayout* inspectorLayout = new QVBoxLayout(inspectorPane);
        inspectorLayout->setContentsMargins(0, 0, 0, 0);
        inspectorLayout->setSpacing(8);
        QLabel* inspectorTitle = new QLabel("Workflow Inspector", inspectorPane);
        inspectorTitle->setObjectName("resultsSectionLabel");
        inspectorLayout->addWidget(inspectorTitle);

        m_workflowCenterDetailsView = new QPlainTextEdit(inspectorPane);
        m_workflowCenterDetailsView->setReadOnly(true);
        m_workflowCenterDetailsView->setPlaceholderText("Select a workflow resource or pipeline node to inspect it here.");
        inspectorLayout->addWidget(m_workflowCenterDetailsView, 1);

        centerSplitter->setStretchFactor(0, 3);
        centerSplitter->setStretchFactor(1, 2);
        containerLayout->addWidget(centerSplitter, 1);

        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(8);
        m_workflowCenterOpenSourceButton = new QPushButton("Open Source File", m_workflowCenterView);
        m_workflowCenterDockButton = new QPushButton("Dock To Sidebar", m_workflowCenterView);
        m_workflowCenterAddStepButton = new QPushButton("Add Step...", m_workflowCenterView);
        m_workflowCenterSaveButton = new QPushButton("Save Workflow", m_workflowCenterView);
        buttonLayout->addWidget(m_workflowCenterOpenSourceButton);
        buttonLayout->addWidget(m_workflowCenterDockButton);
        buttonLayout->addWidget(m_workflowCenterAddStepButton);
        buttonLayout->addWidget(m_workflowCenterSaveButton);
        buttonLayout->addStretch(1);
        containerLayout->addLayout(buttonLayout);

        connect(m_workflowCenterMiniMap, &WorkflowMiniMapWidget::nodeActivated, this, [this](const QString& nodeUid) {
            if(QTreeWidgetItem* nodeItem = findWorkflowItemByStableId(m_workflowExplorer, QStringLiteral("node:%1").arg(nodeUid))) {
                m_workflowExplorer->setCurrentItem(nodeItem);
                m_workflowExplorer->scrollToItem(nodeItem);
                updateSelectedWorkflowItem(nodeItem);
            }
        });
        connect(m_workflowCenterOpenSourceButton, &QPushButton::clicked, this, [this]() {
            if(QFileInfo(m_activeWorkflowFilePath).isFile()) {
                openFileInView(m_activeWorkflowFilePath);
            }
        });
        connect(m_workflowCenterDockButton, &QPushButton::clicked, this, [this]() {
            QWidget* workflowCenterView = m_workflowCenterView;
            if(workflowCenterView) {
                QMetaObject::invokeMethod(this, [this, workflowCenterView]() {
                    const int tabIndex = m_centerTabs ? m_centerTabs->indexOf(workflowCenterView) : -1;
                    if(tabIndex >= 0) {
                        closeCenterTab(tabIndex);
                    }
                }, Qt::QueuedConnection);
            }
        });
        connect(m_workflowCenterAddStepButton, &QPushButton::clicked, this, &MainWindow::appendWorkflowStep);
        connect(m_workflowCenterSaveButton, &QPushButton::clicked, this, &MainWindow::saveActiveWorkflowGraph);
    }

    int tabIndex = m_centerTabs->indexOf(m_workflowCenterView);
    if(tabIndex < 0) {
        tabIndex = m_centerTabs->addTab(m_workflowCenterView, "Workflow Graph");
        m_centerTabs->setTabToolTip(tabIndex, QString::fromLatin1(kWorkflowCenterTabKey));
    }

    refreshWorkflowCenterView();
    refreshWorkflowGraphDockingUi();
    if(focusTab && tabIndex >= 0) {
        m_centerTabs->setCurrentIndex(tabIndex);
    }
}

void MainWindow::refreshWorkflowCenterView()
{
    if(!m_workflowCenterView) {
        return;
    }

    const bool hasActiveGraph = !m_activeWorkflowGraph.isEmpty();
    const QString sourceFileLabel = QFileInfo(m_activeWorkflowFilePath).fileName().isEmpty()
        ? QStringLiteral("Workflow Graph")
        : QFileInfo(m_activeWorkflowFilePath).fileName();
    const QString tabLabel = hasActiveGraph
        ? QString("Graph: %1").arg(sourceFileLabel)
        : QStringLiteral("Workflow Graph");
    const int tabIndex = m_centerTabs->indexOf(m_workflowCenterView);
    if(tabIndex >= 0) {
        m_centerTabs->setTabText(tabIndex, tabLabel);
        m_centerTabs->setTabToolTip(tabIndex, QString::fromLatin1(kWorkflowCenterTabKey));
    }

    if(m_workflowCenterMiniMap) {
        m_workflowCenterMiniMap->setWorkflowGraph(m_activeWorkflowGraph);
    }

    const bool extensionConnected = m_extensionSocket && m_extensionSocket->state() == QLocalSocket::ConnectedState;
    if(m_workflowCenterOpenSourceButton) {
        m_workflowCenterOpenSourceButton->setEnabled(QFileInfo(m_activeWorkflowFilePath).isFile());
    }
    if(m_workflowCenterAddStepButton) {
        m_workflowCenterAddStepButton->setEnabled(extensionConnected
                                                  && hasActiveGraph
                                                  && !workflowOperatorToolDefinitions().isEmpty());
    }
    if(m_workflowCenterSaveButton) {
        m_workflowCenterSaveButton->setEnabled(extensionConnected && hasActiveGraph);
    }

    if(m_workflowCenterSummaryLabel) {
        if(!hasActiveGraph) {
            m_workflowCenterSummaryLabel->setText("No active workflow graph. Load a .mne file to open the graph canvas here.");
        } else {
            QString summary = QString("%1 | %2 resources | %3 pipeline nodes")
                                  .arg(sourceFileLabel)
                                  .arg(m_activeWorkflowGraph.value(QStringLiteral("resources")).toArray().size())
                                  .arg(m_activeWorkflowGraph.value(QStringLiteral("pipeline")).toArray().size());
            if(m_activeWorkflowHasUnsavedChanges) {
                summary += QStringLiteral(" | unsaved runtime edits");
            }
            m_workflowCenterSummaryLabel->setText(summary);
        }
    }

    if(!hasActiveGraph && m_workflowCenterDetailsView) {
        m_workflowCenterDetailsView->setPlainText("Open a .mne file to inspect the workflow graph in the center view.");
    }

    refreshWorkflowGraphDockingUi();
}

QJsonArray MainWindow::workflowOperatorToolDefinitions() const
{
    QJsonArray workflowTools;
    QSet<QString> seenToolNames;

    const QJsonArray tools = availableToolDefinitions();
    for(const QJsonValue& value : tools) {
        const QJsonObject tool = value.toObject();
        const QString toolName = tool.value("name").toString().trimmed();
        if(!tool.value("workflow_operator").toBool(false) || toolName.isEmpty() || seenToolNames.contains(toolName)) {
            continue;
        }

        workflowTools.append(tool);
        seenToolNames.insert(toolName);
    }

    return workflowTools;
}

QJsonObject MainWindow::workflowOperatorToolDefinition(const QString& toolName) const
{
    const QString trimmedToolName = toolName.trimmed();
    if(trimmedToolName.isEmpty()) {
        return QJsonObject();
    }

    const QJsonArray workflowTools = workflowOperatorToolDefinitions();
    for(const QJsonValue& value : workflowTools) {
        const QJsonObject tool = value.toObject();
        if(tool.value("name").toString().trimmed() == trimmedToolName) {
            return tool;
        }
    }

    return QJsonObject();
}

QString MainWindow::selectedWorkflowArtifactUid() const
{
    if(!m_workflowExplorer || !m_workflowExplorer->currentItem()) {
        return QString();
    }

    const QJsonObject payload = m_workflowExplorer->currentItem()->data(0, kWorkflowPayloadRole).toJsonObject();
    const QString kind = payload.value("kind").toString().trimmed();
    if(kind == QLatin1String("resource")) {
        return payload.value("resource").toObject().value("uid").toString().trimmed();
    }
    if(kind == QLatin1String("input")) {
        return payload.value("input_uid").toString().trimmed();
    }
    if(kind == QLatin1String("output")) {
        return payload.value("output_uid").toString().trimmed();
    }
    if(kind == QLatin1String("node")) {
        const QJsonObject outputs = payload.value("node").toObject().value("outputs").toObject();
        if(outputs.size() == 1) {
            return outputs.constBegin().value().toString().trimmed();
        }
    }

    return QString();
}

QJsonObject MainWindow::defaultArgumentsForWorkflowTool(const QString& toolName) const
{
    QJsonObject arguments = defaultArgumentsForTool(toolName);
    const QJsonObject definition = workflowOperatorToolDefinition(toolName);
    if(definition.isEmpty()) {
        return arguments;
    }

    const QString selectedArtifactUid = selectedWorkflowArtifactUid();
    if(selectedArtifactUid.isEmpty()) {
        return arguments;
    }

    QStringList inputFields;
    const QJsonObject properties = definition.value("input_schema").toObject().value("properties").toObject();
    for(auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        if(it.value().toObject().value("x_workflow_section").toString() == QLatin1String("inputs")) {
            inputFields.append(it.key());
        }
    }

    if(inputFields.size() == 1 && !arguments.contains(inputFields.first())) {
        arguments.insert(inputFields.first(), selectedArtifactUid);
    }

    return arguments;
}

QString MainWindow::resolveStudioCompanionExecutable(const QString& executableName) const
{
    const QString trimmedExecutableName = executableName.trimmed();
    if(trimmedExecutableName.isEmpty()) {
        return QString();
    }

    const QStringList relativeCandidates{
        trimmedExecutableName,
        QStringLiteral("../bin/%1").arg(trimmedExecutableName),
        QStringLiteral("../../bin/%1").arg(trimmedExecutableName),
        QStringLiteral("out/Release/bin/%1").arg(trimmedExecutableName),
        QStringLiteral("../out/Release/bin/%1").arg(trimmedExecutableName),
        QStringLiteral("../../out/Release/bin/%1").arg(trimmedExecutableName),
        QStringLiteral("build/out/Release/bin/%1").arg(trimmedExecutableName),
        QStringLiteral("../build/out/Release/bin/%1").arg(trimmedExecutableName)
    };

    const QStringList seedDirectories{
        QApplication::applicationDirPath(),
        QDir::currentPath()
    };

    for(const QString& seedDirectory : seedDirectories) {
        QDir searchDir(seedDirectory);
        for(int depth = 0; depth < 8; ++depth) {
            for(const QString& relativeCandidate : relativeCandidates) {
                const QFileInfo candidateInfo(searchDir.filePath(relativeCandidate));
                if(candidateInfo.exists() && candidateInfo.isFile() && candidateInfo.isExecutable()) {
                    return candidateInfo.absoluteFilePath();
                }
            }

            if(!searchDir.cdUp()) {
                break;
            }
        }
    }

    return QString();
}

void MainWindow::ensureBackendConnection(QLocalSocket* socket,
                                         const QString& socketName,
                                         const QString& executableName,
                                         QProcess* process,
                                         const QString& displayName)
{
    if(!socket || m_isShuttingDown) {
        return;
    }

    socket->abort();
    socket->connectToServer(socketName);
    if(socket->waitForConnected(150)) {
        appendOutputMessage(QString("%1 connected.").arg(displayName));
        return;
    }

    const QString executablePath = resolveStudioCompanionExecutable(executableName);
    if(executablePath.isEmpty()) {
        appendProblemMessage(QString("%1 binary could not be resolved for auto-start: %2")
                                 .arg(displayName, executableName));
        statusBar()->showMessage(QString("%1 is not connected and could not be auto-started.").arg(displayName), 6000);
        return;
    }

    if(process && process->state() == QProcess::NotRunning) {
        process->setProgram(executablePath);
        process->setArguments(QStringList());
        process->setWorkingDirectory(QFileInfo(executablePath).absolutePath());
        process->setProcessEnvironment(studioBackendProcessEnvironment(executablePath));
        process->start();
        if(!process->waitForStarted(1500)) {
            appendProblemMessage(QString("Failed to start %1 at %2")
                                     .arg(displayName, executablePath));
            statusBar()->showMessage(QString("Failed to start %1.").arg(displayName), 6000);
            return;
        }

        appendOutputMessage(QString("Started %1 at %2").arg(displayName, executablePath));
    }

    socket->abort();
    socket->connectToServer(socketName);
    if(!socket->waitForConnected(1500)) {
        appendProblemMessage(QString("%1 did not connect on socket %2 after launch.")
                                 .arg(displayName, socketName));
        statusBar()->showMessage(QString("%1 did not connect after launch.").arg(displayName), 6000);
    }
}

void MainWindow::shutdownManagedBackends()
{
    if(m_isShuttingDown) {
        return;
    }

    m_isShuttingDown = true;
    shutdownManagedBackend(m_extensionSocket, m_extensionHostProcess, QStringLiteral("Extension Host"));
    shutdownManagedBackend(m_kernelSocket, m_kernelProcess, QStringLiteral("Neuro-Kernel"));
}

void MainWindow::shutdownManagedBackend(QLocalSocket* socket,
                                        QProcess* process,
                                        const QString& displayName)
{
    if(socket) {
        if(socket->state() == QLocalSocket::ConnectedState) {
            socket->flush();
            socket->disconnectFromServer();
            if(socket->state() != QLocalSocket::UnconnectedState) {
                socket->waitForDisconnected(500);
            }
        } else {
            socket->abort();
        }
    }

    if(!process || process->state() == QProcess::NotRunning) {
        return;
    }

    process->terminate();
    if(process->waitForFinished(2500)) {
        return;
    }

    appendOutputMessage(QString("%1 did not exit after terminate(); forcing shutdown.").arg(displayName));
    process->kill();
    process->waitForFinished(1000);
}

void MainWindow::setWorkflowStatusBanner(const QString& message, const QString& severity)
{
    m_workflowStatusMessage = message.trimmed();
    m_workflowStatusSeverity = severity.trimmed().toLower();
    if(m_workflowStatusSeverity.isEmpty()) {
        m_workflowStatusSeverity = QStringLiteral("info");
    }

    refreshWorkflowStatusBanner();
}

void MainWindow::refreshWorkflowStatusBanner()
{
    if(!m_workflowStatusBanner) {
        return;
    }

    const bool hasMessage = !m_workflowStatusMessage.isEmpty();
    m_workflowStatusBanner->setVisible(hasMessage);
    if(!hasMessage) {
        m_workflowStatusBanner->clear();
        m_workflowStatusBanner->setStyleSheet(QString());
        return;
    }

    QString backgroundColor = QStringLiteral("#172534");
    QString foregroundColor = QStringLiteral("#8ab4f8");
    QString borderColor = QStringLiteral("#244a6b");

    if(m_workflowStatusSeverity == QLatin1String("error")) {
        backgroundColor = QStringLiteral("#3d1f26");
        foregroundColor = QStringLiteral("#ffb3ba");
        borderColor = QStringLiteral("#8b3446");
    } else if(m_workflowStatusSeverity == QLatin1String("success")) {
        backgroundColor = QStringLiteral("#153421");
        foregroundColor = QStringLiteral("#9be9a8");
        borderColor = QStringLiteral("#2b8250");
    } else if(m_workflowStatusSeverity == QLatin1String("warning")) {
        backgroundColor = QStringLiteral("#3c2f16");
        foregroundColor = QStringLiteral("#f2cc60");
        borderColor = QStringLiteral("#8c6a20");
    }

    m_workflowStatusBanner->setText(m_workflowStatusMessage);
    m_workflowStatusBanner->setStyleSheet(QString("QLabel {"
                                                  "background: %1;"
                                                  "color: %2;"
                                                  "border: 1px solid %3;"
                                                  "border-radius: 10px;"
                                                  "padding: 8px 10px;"
                                                  "font-weight: 600;"
                                                  "}")
                                              .arg(backgroundColor, foregroundColor, borderColor));
}

void MainWindow::requestWorkflowLoad(const QString& filePath)
{
    const QString trimmedFilePath = filePath.trimmed();
    if(trimmedFilePath.isEmpty()) {
        setWorkflowStatusBanner(QStringLiteral("Workflow activation requires a non-empty .mne file path."),
                                QStringLiteral("error"));
        return;
    }

    const QFileInfo requestedInfo(trimmedFilePath);
    const QString normalizedRequestedPath = requestedInfo.exists()
        ? requestedInfo.absoluteFilePath()
        : trimmedFilePath;
    const QFileInfo previousInfo(m_activeWorkflowFilePath.trimmed());
    const QString previousWorkflowPath = previousInfo.exists()
        ? previousInfo.absoluteFilePath()
        : m_activeWorkflowFilePath.trimmed();
    const bool switchingWorkflowFiles = !previousWorkflowPath.isEmpty()
        && previousWorkflowPath != normalizedRequestedPath;

    m_activeWorkflowFilePath = normalizedRequestedPath;
    m_activeWorkflowHasUnsavedChanges = false;
    if(m_activeWorkflowGraph.isEmpty() || switchingWorkflowFiles) {
        m_activeWorkflowGraph = QJsonObject();
    }

    const QString workflowLabel = QFileInfo(normalizedRequestedPath).fileName().isEmpty()
        ? normalizedRequestedPath
        : QFileInfo(normalizedRequestedPath).fileName();
    setWorkflowStatusBanner(QString("Loading workflow graph from %1...").arg(workflowLabel),
                            QStringLiteral("info"));
    rebuildWorkflowNavigatorUi();

    if(m_pendingWorkflowLoads.values().contains(normalizedRequestedPath)) {
        return;
    }

    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        setWorkflowStatusBanner(QString("Extension Host is not connected, so %1 could not be activated as a workflow graph.")
                                    .arg(workflowLabel),
                                QStringLiteral("error"));
        return;
    }

    const QString requestId = QString("workbench-extension-workflow-load-%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_pendingWorkflowLoads.insert(requestId, normalizedRequestedPath);

    const QJsonObject params{
        {"name", "studio.workflow.load"},
        {"arguments", QJsonObject{{"file", normalizedRequestedPath}}}
    };
    const QJsonObject message = JsonRpcMessage::createRequest(requestId, "tools/call", params);
    m_extensionSocket->write(JsonRpcMessage::serialize(message));
    m_extensionSocket->flush();
}

void MainWindow::requestActiveWorkflowGraph()
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        if(m_activeWorkflowGraph.isEmpty()) {
            setWorkflowStatusBanner(QStringLiteral("Extension Host is not connected. Live workflow graph is unavailable."),
                                    QStringLiteral("warning"));
        }
        return;
    }

    const QString requestId = QString("workbench-extension-resource-read-%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QJsonObject message = JsonRpcMessage::createRequest(requestId,
                                                             "resources/read",
                                                             QJsonObject{{"uri", kActiveWorkflowGraphUri}});
    m_extensionSocket->write(JsonRpcMessage::serialize(message));
    m_extensionSocket->flush();
}

void MainWindow::adoptWorkflowGraph(const QJsonObject& result, const QString& fallbackFilePath)
{
    const QJsonObject graph = result.value("graph").toObject();
    if(graph.isEmpty()) {
        if(result.value("tool_name").toString() == QLatin1String("resources/read")
           && result.value("uri").toString().trimmed() == QLatin1String(kActiveWorkflowGraphUri)) {
            m_activeWorkflowGraph = QJsonObject();
            rebuildWorkflowNavigatorUi();
            if(m_pendingWorkflowLoads.isEmpty() && m_activeWorkflowFilePath.trimmed().isEmpty()) {
                setWorkflowStatusBanner(QStringLiteral("No active workflow graph is currently published."),
                                        QStringLiteral("info"));
            }
            if(m_pendingWorkflowLoads.isEmpty()
               && !m_activeWorkflowFilePath.trimmed().isEmpty()
               && QFileInfo::exists(m_activeWorkflowFilePath)) {
                requestWorkflowLoad(m_activeWorkflowFilePath);
            }
        }
        return;
    }

    m_activeWorkflowGraph = graph;

    const QString sourceFilePath = result.value("source_file").toString().trimmed().isEmpty()
        ? fallbackFilePath.trimmed()
        : result.value("source_file").toString().trimmed();
    if(!sourceFilePath.isEmpty()) {
        m_activeWorkflowFilePath = sourceFilePath;
    }

    const QString toolName = result.value("tool_name").toString().trimmed();
    if(toolName == QLatin1String("studio.workflow.load")
       || toolName == QLatin1String("studio.workflow.save")) {
        m_activeWorkflowHasUnsavedChanges = false;
    } else if(result.contains("node_uid")) {
        m_activeWorkflowHasUnsavedChanges = true;
    }

    const QString workflowLabel = QFileInfo(m_activeWorkflowFilePath).fileName().isEmpty()
        ? m_activeWorkflowFilePath
        : QFileInfo(m_activeWorkflowFilePath).fileName();
    QString bannerMessage = QString("%1 active: %2 resources, %3 pipeline nodes.")
                                .arg(workflowLabel.isEmpty() ? QStringLiteral("Workflow graph") : workflowLabel)
                                .arg(graph.value(QStringLiteral("resources")).toArray().size())
                                .arg(graph.value(QStringLiteral("pipeline")).toArray().size());
    QString bannerSeverity = QStringLiteral("success");
    if(m_activeWorkflowHasUnsavedChanges) {
        bannerMessage += QStringLiteral(" Runtime changes are not saved yet.");
        bannerSeverity = QStringLiteral("warning");
    }
    setWorkflowStatusBanner(bannerMessage, bannerSeverity);
    rebuildWorkflowNavigatorUi();
    refreshWorkflowCenterView();
}

void MainWindow::rebuildWorkflowNavigatorUi()
{
    if(!m_workflowExplorer) {
        return;
    }

    if(m_workflowMiniMap) {
        m_workflowMiniMap->setWorkflowGraph(m_activeWorkflowGraph);
    }

    const QString previousStableId = m_workflowExplorer->currentItem()
        ? m_workflowExplorer->currentItem()->data(0, kWorkflowStableIdRole).toString()
        : QString();

    QTreeWidgetItem* selectedItem = nullptr;
    {
        QSignalBlocker blocker(m_workflowExplorer);
        m_workflowExplorer->clear();

        const bool hasLocalWorkflowFile = QFileInfo(m_activeWorkflowFilePath).isFile();
        const bool extensionConnected = m_extensionSocket && m_extensionSocket->state() == QLocalSocket::ConnectedState;
        m_workflowRefreshButton->setEnabled((m_extensionSocket && m_extensionSocket->state() == QLocalSocket::ConnectedState)
                                            || hasLocalWorkflowFile);
        m_workflowAddStepButton->setEnabled(extensionConnected
                                            && !m_activeWorkflowGraph.isEmpty()
                                            && !workflowOperatorToolDefinitions().isEmpty());
        m_workflowSaveButton->setEnabled(extensionConnected && !m_activeWorkflowGraph.isEmpty());

        if(m_activeWorkflowGraph.isEmpty()) {
            const QString message = m_activeWorkflowFilePath.trimmed().isEmpty()
                ? QStringLiteral("No active workflow graph. Open a .mne file to populate the workflow navigator.")
                : QStringLiteral("Awaiting workflow graph activation for %1.")
                      .arg(QFileInfo(m_activeWorkflowFilePath).fileName());
            QTreeWidgetItem* statusItem = new QTreeWidgetItem(QStringList() << message);
            const QJsonObject payload{
                {"kind", "workflow_status"},
                {"message", message},
                {"source_file", m_activeWorkflowFilePath},
                {"graph_resource_uri", kActiveWorkflowGraphUri}
            };
            statusItem->setData(0, kWorkflowPayloadRole, payload);
            statusItem->setData(0, kWorkflowOpenPathRole, hasLocalWorkflowFile ? m_activeWorkflowFilePath : QString());
            statusItem->setData(0, kWorkflowStableIdRole, QStringLiteral("workflow:status"));
            m_workflowExplorer->addTopLevelItem(statusItem);
            selectedItem = statusItem;
        } else {
            const QString sourceFilePath = m_activeWorkflowFilePath;
            const QFileInfo sourceInfo(sourceFilePath);
            const QHash<QString, QJsonObject> resourceByUid = workflowResourceMap(m_activeWorkflowGraph);
            const QHash<QString, QString> outputProducerByUid = workflowOutputProducerMap(m_activeWorkflowGraph);
            const QVector<QJsonObject> orderedNodes = workflowNodesTopologicallyOrdered(m_activeWorkflowGraph);

            QHash<QString, QStringList> dependentNodeUidsByNodeUid;
            QJsonObject statusCounts;
            QJsonArray topologicalOrder;
            for(const QJsonObject& node : orderedNodes) {
                const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
                const QString status = workflowNodeStatus(node);
                statusCounts.insert(status, statusCounts.value(status).toInt() + 1);
                topologicalOrder.append(nodeUid);

                const QStringList dependencies = workflowDependencyNodeUids(node, outputProducerByUid);
                for(const QString& dependencyNodeUid : dependencies) {
                    insertSortedUnique(dependentNodeUidsByNodeUid[dependencyNodeUid], nodeUid);
                }
            }

            const QJsonObject summaryPayload{
                {"kind", "workflow_summary"},
                {"source_file", sourceFilePath},
                {"graph_resource_uri", kActiveWorkflowGraphUri},
                {"source_file_exists", sourceInfo.isFile()},
                {"resource_count", m_activeWorkflowGraph.value(QStringLiteral("resources")).toArray().size()},
                {"node_count", orderedNodes.size()},
                {"status_counts", statusCounts},
                {"topological_order", topologicalOrder},
                {"graph", m_activeWorkflowGraph}
            };
            QTreeWidgetItem* summaryItem = new QTreeWidgetItem(QStringList()
                                                               << QString("Active Workflow | %1 resources | %2 nodes")
                                                                      .arg(summaryPayload.value(QStringLiteral("resource_count")).toInt())
                                                                      .arg(summaryPayload.value(QStringLiteral("node_count")).toInt()));
            summaryItem->setToolTip(0, sourceFilePath);
            summaryItem->setData(0, kWorkflowPayloadRole, summaryPayload);
            summaryItem->setData(0, kWorkflowOpenPathRole, sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString());
            summaryItem->setData(0, kWorkflowStableIdRole, QStringLiteral("workflow:summary"));
            summaryItem->addChild(new QTreeWidgetItem(QStringList()
                                                      << QString("Source: %1")
                                                             .arg(sourceInfo.fileName().isEmpty() ? sourceFilePath : sourceInfo.fileName())));
            summaryItem->addChild(new QTreeWidgetItem(QStringList()
                                                      << QString("Graph Resource: %1").arg(kActiveWorkflowGraphUri)));
            summaryItem->setExpanded(true);
            m_workflowExplorer->addTopLevelItem(summaryItem);

            QTreeWidgetItem* sourceRoot = new QTreeWidgetItem(QStringList() << "Source File");
            const QJsonObject sourcePayload{
                {"kind", "source_file"},
                {"source_file", sourceFilePath},
                {"exists", sourceInfo.isFile()},
                {"graph_resource_uri", kActiveWorkflowGraphUri}
            };
            QTreeWidgetItem* sourceItem = new QTreeWidgetItem(QStringList()
                                                              << (sourceInfo.fileName().isEmpty()
                                                                      ? QStringLiteral("Unresolved workflow file")
                                                                      : sourceInfo.fileName()));
            sourceItem->setToolTip(0, sourceFilePath);
            sourceItem->setData(0, kWorkflowPayloadRole, sourcePayload);
            sourceItem->setData(0, kWorkflowOpenPathRole, sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString());
            sourceItem->setData(0, kWorkflowStableIdRole, QStringLiteral("workflow:source"));
            sourceRoot->addChild(sourceItem);
            sourceRoot->setExpanded(true);
            m_workflowExplorer->addTopLevelItem(sourceRoot);

            const QJsonArray resources = m_activeWorkflowGraph.value(QStringLiteral("resources")).toArray();
            QTreeWidgetItem* resourcesRoot = new QTreeWidgetItem(QStringList()
                                                                 << QString("Resources (%1)").arg(resources.size()));
            for(const QJsonValue& value : resources) {
                const QJsonObject resource = value.toObject();
                const QString resourceUid = resource.value(QStringLiteral("uid")).toString().trimmed();
                const QString resourceType = resource.value(QStringLiteral("type")).toString().trimmed();
                const QString resourceUri = resource.value(QStringLiteral("uri")).toString().trimmed();
                const QString resolvedLocalPath = resolveWorkflowUriToLocalPath(resourceUri, sourceFilePath);

                QString label = QString("%1 [%2]")
                                    .arg(resourceUid, resourceType.isEmpty() ? QStringLiteral("resource") : resourceType);
                if(!resolvedLocalPath.isEmpty()) {
                    label += QString(" | %1").arg(QFileInfo(resolvedLocalPath).fileName());
                } else if(!resourceUri.isEmpty()) {
                    label += QString(" | %1").arg(resourceUri);
                }

                const QJsonObject payload{
                    {"kind", "resource"},
                    {"source_file", sourceFilePath},
                    {"resource", resource},
                    {"resolved_local_path", resolvedLocalPath}
                };
                QTreeWidgetItem* resourceItem = new QTreeWidgetItem(QStringList() << label);
                resourceItem->setToolTip(0, resolvedLocalPath.isEmpty() ? resourceUri : resolvedLocalPath);
                resourceItem->setData(0, kWorkflowPayloadRole, payload);
                resourceItem->setData(0, kWorkflowOpenPathRole, resolvedLocalPath.isEmpty()
                                                                 ? (sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString())
                                                                 : resolvedLocalPath);
                resourceItem->setData(0, kWorkflowStableIdRole, QStringLiteral("resource:%1").arg(resourceUid));
                resourcesRoot->addChild(resourceItem);
            }
            resourcesRoot->setExpanded(true);
            m_workflowExplorer->addTopLevelItem(resourcesRoot);

            QTreeWidgetItem* pipelineRoot = new QTreeWidgetItem(QStringList()
                                                                << QString("Pipeline (%1 nodes)").arg(orderedNodes.size()));
            for(int index = 0; index < orderedNodes.size(); ++index) {
                const QJsonObject node = orderedNodes.at(index);
                const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
                const QString nodeLabel = workflowNodeDisplayLabel(node);
                const QString nodeStage = node.value(QStringLiteral("stage")).toString().trimmed();
                const QString skillId = node.value(QStringLiteral("skill_id")).toString().trimmed();
                const QString status = workflowNodeStatus(node);
                const QStringList dependencyNodeUids = workflowDependencyNodeUids(node, outputProducerByUid);
                const QStringList dependentNodeUids = dependentNodeUidsByNodeUid.value(nodeUid);

                const QJsonObject nodePayload{
                    {"kind", "node"},
                    {"source_file", sourceFilePath},
                    {"node_index", index + 1},
                    {"node_uid", nodeUid},
                    {"label", nodeLabel},
                    {"stage", nodeStage},
                    {"skill_id", skillId},
                    {"status", status},
                    {"dependency_node_uids", QJsonArray::fromStringList(dependencyNodeUids)},
                    {"dependent_node_uids", QJsonArray::fromStringList(dependentNodeUids)},
                    {"node", node}
                };
                QString nodeTitle = QString("%1. [%2] %3")
                                        .arg(index + 1, 2, 10, QLatin1Char('0'))
                                        .arg(status)
                                        .arg(nodeLabel.isEmpty() ? nodeUid : nodeLabel);
                if(!nodeStage.isEmpty()) {
                    nodeTitle += QString(" | %1").arg(nodeStage);
                }
                if(nodeLabel != nodeUid && !nodeUid.isEmpty()) {
                    nodeTitle += QString(" | %1").arg(nodeUid);
                }
                QTreeWidgetItem* nodeItem = new QTreeWidgetItem(QStringList()
                                                                << nodeTitle);
                const QString nodeDescription = node.value(QStringLiteral("description")).toString().trimmed();
                nodeItem->setToolTip(0, nodeDescription.isEmpty()
                                            ? skillId
                                            : QString("%1\n%2").arg(skillId, nodeDescription));
                nodeItem->setData(0, kWorkflowPayloadRole, nodePayload);
                nodeItem->setData(0, kWorkflowOpenPathRole, sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString());
                nodeItem->setData(0, kWorkflowStableIdRole, QStringLiteral("node:%1").arg(nodeUid));

                const QJsonObject inputs = node.value(QStringLiteral("inputs")).toObject();
                if(!inputs.isEmpty()) {
                    QTreeWidgetItem* inputsRoot = new QTreeWidgetItem(QStringList() << "Inputs");
                    for(auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
                        const QString inputUid = it.value().toString().trimmed();
                        const QJsonObject resolvedResource = resourceByUid.value(inputUid);
                        const QString resolvedLocalPath = resolveWorkflowUriToLocalPath(resolvedResource.value(QStringLiteral("uri")).toString(),
                                                                                        sourceFilePath);
                        const QJsonObject payload{
                            {"kind", "input"},
                            {"source_file", sourceFilePath},
                            {"node_uid", nodeUid},
                            {"role", it.key()},
                            {"value", it.value()},
                            {"input_uid", inputUid},
                            {"resolved_resource", resolvedResource},
                            {"resolved_local_path", resolvedLocalPath}
                        };
                        QString label = QString("%1 -> %2").arg(it.key(), summarizeJsonValue(it.value()));
                        if(!resolvedResource.isEmpty()) {
                            label += QString(" [%1]").arg(resolvedResource.value(QStringLiteral("type")).toString());
                        }
                        QTreeWidgetItem* inputItem = new QTreeWidgetItem(QStringList() << label);
                        inputItem->setToolTip(0, resolvedLocalPath.isEmpty()
                                                 ? resolvedResource.value(QStringLiteral("uri")).toString()
                                                 : resolvedLocalPath);
                        inputItem->setData(0, kWorkflowPayloadRole, payload);
                        inputItem->setData(0, kWorkflowOpenPathRole, resolvedLocalPath.isEmpty()
                                                                          ? (sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString())
                                                                          : resolvedLocalPath);
                        inputItem->setData(0,
                                           kWorkflowStableIdRole,
                                           QStringLiteral("node:%1:input:%2").arg(nodeUid, it.key()));
                        inputsRoot->addChild(inputItem);
                    }
                    nodeItem->addChild(inputsRoot);
                }

                const QJsonObject parameters = node.value(QStringLiteral("parameters")).toObject();
                if(!parameters.isEmpty()) {
                    QTreeWidgetItem* parametersRoot = new QTreeWidgetItem(QStringList() << "Parameters");
                    for(auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
                        const QJsonObject payload{
                            {"kind", "parameter"},
                            {"source_file", sourceFilePath},
                            {"node_uid", nodeUid},
                            {"name", it.key()},
                            {"value", it.value()}
                        };
                        QTreeWidgetItem* parameterItem = new QTreeWidgetItem(QStringList()
                                                                            << QString("%1 = %2")
                                                                                   .arg(it.key(), summarizeJsonValue(it.value())));
                        parameterItem->setData(0, kWorkflowPayloadRole, payload);
                        parameterItem->setData(0, kWorkflowOpenPathRole, sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString());
                        parameterItem->setData(0,
                                               kWorkflowStableIdRole,
                                               QStringLiteral("node:%1:parameter:%2").arg(nodeUid, it.key()));
                        parametersRoot->addChild(parameterItem);
                    }
                    nodeItem->addChild(parametersRoot);
                }

                const QJsonObject declaredOutputs = node.value(QStringLiteral("outputs")).toObject();
                if(!declaredOutputs.isEmpty()) {
                    const QJsonObject resolvedOutputs = node.value(QStringLiteral("runtime"))
                                                           .toObject()
                                                           .value(QStringLiteral("resolved_outputs"))
                                                           .toObject();
                    QTreeWidgetItem* outputsRoot = new QTreeWidgetItem(QStringList() << "Outputs");
                    for(auto it = declaredOutputs.constBegin(); it != declaredOutputs.constEnd(); ++it) {
                        const QString outputUid = it.value().toString().trimmed();
                        QJsonObject resolvedResource = resolvedOutputs.value(it.key()).toObject();
                        if(resolvedResource.isEmpty()) {
                            resolvedResource = resourceByUid.value(outputUid);
                        }
                        const QString resolvedLocalPath = resolveWorkflowUriToLocalPath(resolvedResource.value(QStringLiteral("uri")).toString(),
                                                                                        sourceFilePath);
                        const QJsonObject payload{
                            {"kind", "output"},
                            {"source_file", sourceFilePath},
                            {"node_uid", nodeUid},
                            {"role", it.key()},
                            {"output_uid", outputUid},
                            {"resolved_resource", resolvedResource},
                            {"resolved_local_path", resolvedLocalPath}
                        };
                        QString label = QString("%1 -> %2").arg(it.key(), outputUid);
                        if(!resolvedResource.isEmpty()) {
                            label += QString(" [%1]").arg(resolvedResource.value(QStringLiteral("type")).toString());
                        }
                        QTreeWidgetItem* outputItem = new QTreeWidgetItem(QStringList() << label);
                        outputItem->setToolTip(0, resolvedLocalPath.isEmpty()
                                                  ? resolvedResource.value(QStringLiteral("uri")).toString()
                                                  : resolvedLocalPath);
                        outputItem->setData(0, kWorkflowPayloadRole, payload);
                        outputItem->setData(0, kWorkflowOpenPathRole, resolvedLocalPath.isEmpty()
                                                                           ? (sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString())
                                                                           : resolvedLocalPath);
                        outputItem->setData(0,
                                            kWorkflowStableIdRole,
                                            QStringLiteral("node:%1:output:%2").arg(nodeUid, it.key()));
                        outputsRoot->addChild(outputItem);
                    }
                    nodeItem->addChild(outputsRoot);
                }

                const QJsonObject lastResult = node.value(QStringLiteral("runtime"))
                                                   .toObject()
                                                   .value(QStringLiteral("last_result"))
                                                   .toObject();
                if(!lastResult.isEmpty()) {
                    const QJsonObject payload{
                        {"kind", "result"},
                        {"source_file", sourceFilePath},
                        {"node_uid", nodeUid},
                        {"result", lastResult}
                    };
                    QTreeWidgetItem* resultItem = new QTreeWidgetItem(QStringList() << "Last Result");
                    resultItem->setData(0, kWorkflowPayloadRole, payload);
                    resultItem->setData(0, kWorkflowOpenPathRole, sourceInfo.isFile() ? sourceInfo.absoluteFilePath() : QString());
                    resultItem->setData(0,
                                        kWorkflowStableIdRole,
                                        QStringLiteral("node:%1:last_result").arg(nodeUid));
                    nodeItem->addChild(resultItem);
                }

                pipelineRoot->addChild(nodeItem);
            }
            pipelineRoot->setExpanded(true);
            m_workflowExplorer->addTopLevelItem(pipelineRoot);

            selectedItem = findWorkflowItemByStableId(m_workflowExplorer, previousStableId);
            if(!selectedItem) {
                selectedItem = summaryItem;
            }
        }

        if(selectedItem) {
            m_workflowExplorer->setCurrentItem(selectedItem);
        }
    }

    updateSelectedWorkflowItem(selectedItem);
    refreshWorkflowCenterView();
}

void MainWindow::appendWorkflowStep()
{
    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        setWorkflowStatusBanner(QStringLiteral("Extension Host is not connected, so workflow steps cannot be appended right now."),
                                QStringLiteral("error"));
        appendProblemMessage(QStringLiteral("Workflow authoring requires a connected Extension Host."));
        return;
    }

    if(m_activeWorkflowGraph.isEmpty()) {
        setWorkflowStatusBanner(QStringLiteral("Open or activate a workflow graph before appending new steps."),
                                QStringLiteral("warning"));
        appendProblemMessage(QStringLiteral("No active workflow graph is available for runtime extension."));
        return;
    }

    const QJsonArray workflowTools = workflowOperatorToolDefinitions();
    if(workflowTools.isEmpty()) {
        setWorkflowStatusBanner(QStringLiteral("No workflow operator tools are currently available for runtime graph edits."),
                                QStringLiteral("warning"));
        appendProblemMessage(QStringLiteral("No workflow operator tools were discovered from the Extension Host."));
        return;
    }

    openWorkflowCenterView(true);

    QString selectedToolName;
    QJsonObject selectedToolDefinition;
    if(workflowTools.size() == 1) {
        selectedToolDefinition = workflowTools.first().toObject();
        selectedToolName = selectedToolDefinition.value("name").toString().trimmed();
    } else {
        QStringList labels;
        QHash<QString, QJsonObject> toolByLabel;
        for(const QJsonValue& value : workflowTools) {
            const QJsonObject tool = value.toObject();
            const QString toolName = tool.value("name").toString().trimmed();
            if(toolName.isEmpty()) {
                continue;
            }

            const QString label = QString("%1 (%2)")
                                      .arg(tool.value("display_name").toString(toolName), toolName);
            labels.append(label);
            toolByLabel.insert(label, tool);
        }

        std::sort(labels.begin(), labels.end());
        bool accepted = false;
        const QString chosenLabel = QInputDialog::getItem(this,
                                                          QStringLiteral("Append Workflow Step"),
                                                          QStringLiteral("Choose a workflow operator"),
                                                          labels,
                                                          0,
                                                          false,
                                                          &accepted);
        if(!accepted || chosenLabel.trimmed().isEmpty()) {
            return;
        }

        selectedToolDefinition = toolByLabel.value(chosenLabel);
        selectedToolName = selectedToolDefinition.value("name").toString().trimmed();
    }

    if(selectedToolName.isEmpty()) {
        appendProblemMessage(QStringLiteral("The chosen workflow operator is missing a tool name."));
        return;
    }

    QJsonObject arguments = defaultArgumentsForWorkflowTool(selectedToolName);
    if(!editArgumentsForTool(selectedToolName, arguments)) {
        return;
    }

    const QString commandText = QString("tools.call %1 %2")
        .arg(selectedToolName,
             QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact)));
    appendTerminalMessage(QString("$ %1").arg(commandText));
    appendOutputMessage(QString("Appending workflow step via %1").arg(selectedToolDefinition.value("display_name")
                                                                          .toString(selectedToolName)));
    sendExtensionToolCall(selectedToolName, arguments);
}

void MainWindow::saveActiveWorkflowGraph()
{
    if(m_activeWorkflowGraph.isEmpty()) {
        setWorkflowStatusBanner(QStringLiteral("There is no active workflow graph to save."),
                                QStringLiteral("warning"));
        appendProblemMessage(QStringLiteral("No active workflow graph is available to save."));
        return;
    }

    if(!m_extensionSocket || m_extensionSocket->state() != QLocalSocket::ConnectedState) {
        setWorkflowStatusBanner(QStringLiteral("Extension Host is not connected, so the workflow graph cannot be saved right now."),
                                QStringLiteral("error"));
        appendProblemMessage(QStringLiteral("Workflow saving requires a connected Extension Host."));
        return;
    }

    QString targetPath = m_activeWorkflowFilePath.trimmed();
    if(targetPath.isEmpty()) {
        targetPath = QFileDialog::getSaveFileName(this,
                                                  QStringLiteral("Save Workflow Graph"),
                                                  QDir::current().filePath(QStringLiteral("untitled.mne")),
                                                  QStringLiteral("MNE Analysis (*.mne);;JSON Files (*.json);;All Files (*)"));
        if(targetPath.trimmed().isEmpty()) {
            return;
        }
    }

    if(QFileInfo(targetPath).suffix().trimmed().isEmpty()) {
        targetPath += QStringLiteral(".mne");
    }

    m_activeWorkflowFilePath = QFileInfo(targetPath).absoluteFilePath();
    setWorkflowStatusBanner(QString("Saving workflow graph to %1...")
                                .arg(QFileInfo(m_activeWorkflowFilePath).fileName()),
                            QStringLiteral("info"));
    appendOutputMessage(QString("Saving active workflow graph to %1").arg(m_activeWorkflowFilePath));
    sendExtensionToolCall(QStringLiteral("studio.workflow.save"),
                          QJsonObject{{"file", m_activeWorkflowFilePath}});
}

void MainWindow::updateSelectedWorkflowItem(QTreeWidgetItem* item)
{
    m_workflowOpenFileButton->setEnabled(!openableWorkflowPathForItem(item).isEmpty());

    const QJsonObject payload = item ? item->data(0, kWorkflowPayloadRole).toJsonObject() : QJsonObject();
    const QString focusNodeUid = workflowFocusNodeUid(payload);
    if(m_workflowMiniMap) {
        m_workflowMiniMap->setFocusNodeUid(focusNodeUid);
    }
    if(m_workflowCenterMiniMap) {
        m_workflowCenterMiniMap->setFocusNodeUid(focusNodeUid);
    }

    const auto setWorkflowDetailText = [this](const QString& text) {
        m_workflowDetailsView->setPlainText(text);
        if(m_workflowCenterDetailsView) {
            m_workflowCenterDetailsView->setPlainText(text);
        }
    };

    const QHash<QString, QStringList> dependenciesByNodeUid = m_activeWorkflowGraph.isEmpty()
        ? QHash<QString, QStringList>()
        : workflowDependencyNodeMap(m_activeWorkflowGraph);
    const QHash<QString, QStringList> dependentsByNodeUid = m_activeWorkflowGraph.isEmpty()
        ? QHash<QString, QStringList>()
        : workflowDependentNodeMap(m_activeWorkflowGraph);

    const QStringList directDependencies = focusNodeUid.isEmpty()
        ? QStringList()
        : dependenciesByNodeUid.value(focusNodeUid);
    const QStringList directDependents = focusNodeUid.isEmpty()
        ? QStringList()
        : dependentsByNodeUid.value(focusNodeUid);
    const QStringList upstreamNodeUids = focusNodeUid.isEmpty()
        ? QStringList()
        : workflowReachableNodeUids(focusNodeUid, dependenciesByNodeUid);
    const QStringList downstreamNodeUids = focusNodeUid.isEmpty()
        ? QStringList()
        : workflowReachableNodeUids(focusNodeUid, dependentsByNodeUid);

    if(m_workflowExplorer) {
        QList<QTreeWidgetItem*> pendingItems;
        for(int i = 0; i < m_workflowExplorer->topLevelItemCount(); ++i) {
            pendingItems.append(m_workflowExplorer->topLevelItem(i));
        }

        while(!pendingItems.isEmpty()) {
            QTreeWidgetItem* candidate = pendingItems.takeFirst();
            QString candidateNodeUid;
            if(workflowItemRepresentsPipelineNode(candidate, &candidateNodeUid)) {
                candidate->setBackground(0, QBrush());
                candidate->setForeground(0, QBrush());
                QFont font = candidate->font(0);
                font.setBold(false);
                font.setItalic(false);
                font.setUnderline(false);

                if(candidateNodeUid == focusNodeUid) {
                    font.setBold(true);
                    candidate->setForeground(0, QBrush(Qt::white));
                    candidate->setBackground(0, QBrush(QColor(47, 129, 247, 185)));
                } else if(directDependencies.contains(candidateNodeUid)) {
                    font.setBold(true);
                    candidate->setForeground(0, QBrush(QColor(242, 204, 96)));
                    candidate->setBackground(0, QBrush(QColor(173, 114, 24, 72)));
                } else if(directDependents.contains(candidateNodeUid)) {
                    font.setBold(true);
                    candidate->setForeground(0, QBrush(QColor(86, 211, 100)));
                    candidate->setBackground(0, QBrush(QColor(35, 134, 54, 80)));
                } else if(upstreamNodeUids.contains(candidateNodeUid)) {
                    font.setItalic(true);
                    candidate->setForeground(0, QBrush(QColor(227, 179, 65)));
                    candidate->setBackground(0, QBrush(QColor(173, 114, 24, 34)));
                } else if(downstreamNodeUids.contains(candidateNodeUid)) {
                    font.setItalic(true);
                    candidate->setForeground(0, QBrush(QColor(63, 185, 80)));
                    candidate->setBackground(0, QBrush(QColor(35, 134, 54, 38)));
                }

                candidate->setFont(0, font);
            }

            for(int childIndex = 0; childIndex < candidate->childCount(); ++childIndex) {
                pendingItems.append(candidate->child(childIndex));
            }
        }
    }

    if(!item) {
        if(m_activeWorkflowGraph.isEmpty()) {
            setWorkflowDetailText("Open a .mne file to inspect resources and pipeline nodes here.");
        } else {
            setWorkflowDetailText("Select a workflow resource or pipeline node to inspect it here.");
        }
        return;
    }

    if(payload.isEmpty()) {
        setWorkflowDetailText(item->text(0));
        return;
    }

    QJsonObject details = payload;
    if(!focusNodeUid.isEmpty()) {
        details.insert(QStringLiteral("dependency_view"),
                       QJsonObject{
                           {"focus_node_uid", focusNodeUid},
                           {"direct_dependencies", QJsonArray::fromStringList(directDependencies)},
                           {"direct_dependents", QJsonArray::fromStringList(directDependents)},
                           {"upstream_nodes", QJsonArray::fromStringList(upstreamNodeUids)},
                           {"downstream_nodes", QJsonArray::fromStringList(downstreamNodeUids)},
                           {"legend", QJsonObject{
                                {"selected_node", "blue"},
                                {"direct_upstream", "amber"},
                                {"direct_downstream", "green"},
                                {"transitive_upstream", "light_amber"},
                                {"transitive_downstream", "light_green"}
                            }}
                       });
    }

    setWorkflowDetailText(QString::fromUtf8(QJsonDocument(details).toJson(QJsonDocument::Indented)));

    if(!focusNodeUid.isEmpty()) {
        statusBar()->showMessage(QString("Workflow focus: %1 | %2 upstream | %3 downstream")
                                     .arg(focusNodeUid)
                                     .arg(upstreamNodeUids.size())
                                     .arg(downstreamNodeUids.size()),
                                 4000);
    }
}

QString MainWindow::openableWorkflowPathForItem(QTreeWidgetItem* item) const
{
    if(!item && m_workflowExplorer) {
        item = m_workflowExplorer->currentItem();
    }
    if(!item) {
        return QString();
    }

    const QString directPath = item->data(0, kWorkflowOpenPathRole).toString().trimmed();
    if(QFileInfo(directPath).isFile()) {
        return QFileInfo(directPath).absoluteFilePath();
    }

    const QJsonObject payload = item->data(0, kWorkflowPayloadRole).toJsonObject();
    const QString resolvedLocalPath = payload.value(QStringLiteral("resolved_local_path")).toString().trimmed();
    if(QFileInfo(resolvedLocalPath).isFile()) {
        return QFileInfo(resolvedLocalPath).absoluteFilePath();
    }

    const QJsonObject resource = payload.value(QStringLiteral("resource")).toObject();
    const QString resourceUri = resource.value(QStringLiteral("uri")).toString().trimmed();
    const QString resourcePath = resolveWorkflowUriToLocalPath(resourceUri, m_activeWorkflowFilePath);
    if(QFileInfo(resourcePath).isFile()) {
        return QFileInfo(resourcePath).absoluteFilePath();
    }

    const QJsonObject resolvedResource = payload.value(QStringLiteral("resolved_resource")).toObject();
    const QString resolvedResourceUri = resolvedResource.value(QStringLiteral("uri")).toString().trimmed();
    const QString resolvedResourcePath = resolveWorkflowUriToLocalPath(resolvedResourceUri, m_activeWorkflowFilePath);
    if(QFileInfo(resolvedResourcePath).isFile()) {
        return QFileInfo(resolvedResourcePath).absoluteFilePath();
    }

    const QString sourceFilePath = payload.value(QStringLiteral("source_file")).toString().trimmed().isEmpty()
        ? m_activeWorkflowFilePath.trimmed()
        : payload.value(QStringLiteral("source_file")).toString().trimmed();
    return QFileInfo(sourceFilePath).isFile() ? QFileInfo(sourceFilePath).absoluteFilePath() : QString();
}

void MainWindow::openSelectedWorkflowFile()
{
    const QString filePath = openableWorkflowPathForItem(m_workflowExplorer ? m_workflowExplorer->currentItem() : nullptr);
    if(filePath.isEmpty()) {
        appendProblemMessage("The selected workflow item is not backed by a local file that can be opened.");
        statusBar()->showMessage("No openable local file for the selected workflow item.", 4000);
        return;
    }

    addWorkspaceDirectory(QFileInfo(filePath).absolutePath());
    openFileInView(filePath);
}

bool MainWindow::isExtensionHostTool(const QString& toolName) const
{
    const QString trimmedToolName = toolName.trimmed();
    if(trimmedToolName.isEmpty()) {
        return false;
    }

    if(trimmedToolName == QLatin1String("studio.workflow.load")) {
        return true;
    }

    if(trimmedToolName.startsWith(QLatin1String("view.hosted.")) || trimmedToolName.startsWith(QLatin1String("settings."))) {
        return false;
    }

    return !toolDefinition(trimmedToolName).value("extension_id").toString().trimmed().isEmpty();
}

void MainWindow::sendKernelToolCall(const QString& toolName, const QJsonObject& arguments)
{
    QJsonObject params{
        {"name", toolName},
        {"arguments", arguments}
    };

    const QString requestId = QString("workbench-kernel-tool-%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QJsonObject message = JsonRpcMessage::createRequest(requestId, "tools/call", params);
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

    const QString requestId = QString("workbench-extension-tool-%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QJsonObject message = JsonRpcMessage::createRequest(requestId, "tools/call", params);
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

    QJsonArray currentConversationEntries;
    QJsonArray archivedConversationSessions;
    const QByteArray currentConversationPayload = settings.value("workspace/current_conversation_entries").toByteArray();
    if(!currentConversationPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(currentConversationPayload);
        if(document.isArray()) {
            currentConversationEntries = document.array();
        }
    }
    const QByteArray archivedConversationPayload = settings.value("workspace/archived_conversation_sessions").toByteArray();
    if(!archivedConversationPayload.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(archivedConversationPayload);
        if(document.isArray()) {
            archivedConversationSessions = document.array();
        }
    }
    m_agentChatDock->restoreConversationState(currentConversationEntries, archivedConversationSessions);

    const QStringList workspaceFiles = settings.value("workspace/files").toStringList();
    const QStringList workspaceDirectories = settings.value("workspace/directories").toStringList();
    m_activeWorkflowFilePath = settings.value("workspace/active_workflow_file").toString();
    rebuildWorkflowNavigatorUi();
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
        if(QFileInfo(filePath).isFile()) {
            workspaceFiles.append(QFileInfo(filePath).absoluteFilePath());
        }
    }

    settings.setValue("workspace/directories", workspaceDirectories);
    settings.setValue("workspace/files", workspaceFiles);
    settings.setValue("workspace/active_workflow_file", m_activeWorkflowFilePath);
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

    const QJsonArray currentConversationEntries = m_agentChatDock ? m_agentChatDock->currentConversationEntries() : QJsonArray();
    const QJsonArray archivedConversationSessions = m_agentChatDock ? m_agentChatDock->archivedConversationSessions() : QJsonArray();

    settings.setValue("workspace/current_conversation_entries", QJsonDocument(currentConversationEntries).toJson(QJsonDocument::Compact));
    settings.setValue("workspace/archived_conversation_sessions",
                      QJsonDocument(archivedConversationSessions).toJson(QJsonDocument::Compact));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    persistWorkspace();
    shutdownManagedBackends();
    QMainWindow::closeEvent(event);
}

bool MainWindow::isSupportedWorkspaceFile(const QString& filePath) const
{
    if(isWorkflowAnalysisFile(filePath)) {
        return true;
    }

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
