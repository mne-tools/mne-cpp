//=============================================================================================================
/**
 * @file     dummy3dhostedviewwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the hosted 3D surface widget backed by the MNE Inspect brain view.
 */

#include "dummy3dhostedviewwidget.h"

#include <extensionviewfactoryregistry.h>
#include <iextensionviewfactory.h>

#include <disp3D/model/braintreemodel.h>
#include <disp3D/view/brainview.h>
#include <fs/fs_surface.h>
#include <mne/mne_bem.h>

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#include <QSplitter>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

namespace
{

QStringList jsonArrayToStringList(const QJsonArray& values)
{
    QStringList result;
    for(const QJsonValue& value : values) {
        const QString text = value.toString().trimmed();
        if(!text.isEmpty() && !result.contains(text)) {
            result.append(text);
        }
    }

    return result;
}

QString bemSurfaceName(const MNELIB::MNEBemSurface& bemSurface, int index)
{
    switch(bemSurface.id) {
        case 4:
            return "head";
        case 3:
            return "outer_skull";
        case 1:
            return "inner_skull";
        default:
            return QString("bem_%1").arg(index);
    }
}

class Dummy3DExtensionViewFactory final : public IExtensionViewFactory
{
public:
    QString widgetType() const override
    {
        return "inspect_3d_surface";
    }

    QWidget* createView(const QJsonObject& sessionDescriptor, QWidget* parent) const override
    {
        Dummy3DHostedViewWidget* widget = new Dummy3DHostedViewWidget(parent);
        widget->setSessionDescriptor(sessionDescriptor);

        const QString requestedFile = sessionDescriptor.value("file").toString().trimmed();
        if(!requestedFile.isEmpty() && !widget->hasLoadedFile(requestedFile)) {
            delete widget;
            return nullptr;
        }

        return widget;
    }
};

class Dummy3DExtensionViewFactoryRegistration
{
public:
    Dummy3DExtensionViewFactoryRegistration()
    {
        ExtensionViewFactoryRegistry::instance().registerFactory(&m_factory);
    }

private:
    Dummy3DExtensionViewFactory m_factory;
};

Dummy3DExtensionViewFactoryRegistration s_dummy3dFactoryRegistration;

}

Dummy3DHostedViewWidget::Dummy3DHostedViewWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel(this))
, m_summaryLabel(new QLabel(this))
, m_statusLabel(new QLabel(this))
, m_surfaceTypeCombo(new QComboBox(this))
, m_loadedFilesList(new QListWidget(this))
, m_brainView(new BrainView(this))
, m_model(new BrainTreeModel(this))
{
    setMinimumSize(920, 640);

    m_titleLabel->setObjectName("terminalStatusLabel");
    m_summaryLabel->setWordWrap(true);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setObjectName("terminalStatusLabel");

    QLabel* surfaceTypeLabel = new QLabel("Surface", this);
    surfaceTypeLabel->setObjectName("terminalStatusLabel");

    QWidget* headerRow = new QWidget(this);
    QHBoxLayout* headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);
    headerLayout->addWidget(m_titleLabel, 1);
    headerLayout->addWidget(surfaceTypeLabel);
    headerLayout->addWidget(m_surfaceTypeCombo);

    QWidget* sidePanel = new QWidget(this);
    QVBoxLayout* sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(8);

    QLabel* layersTitle = new QLabel("Scene Layers", sidePanel);
    layersTitle->setObjectName("terminalStatusLabel");
    m_loadedFilesList->setSelectionMode(QAbstractItemView::NoSelection);
    sideLayout->addWidget(layersTitle);
    sideLayout->addWidget(m_loadedFilesList, 1);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(sidePanel);
    splitter->addWidget(m_brainView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 240 << 900);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(headerRow);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(splitter, 1);

    m_brainView->setModel(m_model);

    connect(m_surfaceTypeCombo,
            &QComboBox::currentTextChanged,
            this,
            [this](const QString& surfaceType) {
                if(!surfaceType.trimmed().isEmpty()) {
                    m_brainView->setActiveSurface(surfaceType.trimmed());
                }
            });

    rebuildUi();
}

void Dummy3DHostedViewWidget::setSessionDescriptor(const QJsonObject& descriptor)
{
    m_descriptor = descriptor;

    QStringList filesToLoad = requestedSceneFiles();
    if(filesToLoad.isEmpty()) {
        const QString requestedFile = descriptor.value("file").toString().trimmed();
        if(!requestedFile.isEmpty()) {
            filesToLoad.append(requestedFile);
        }
    }

    QStringList loadErrors;
    for(const QString& sceneFile : std::as_const(filesToLoad)) {
        QString errorMessage;
        if(!loadFileInternal(sceneFile, &errorMessage, false) && !errorMessage.isEmpty()) {
            loadErrors.append(errorMessage);
        }
    }

    if(!loadErrors.isEmpty()) {
        const QString message = loadErrors.join(" | ");
        m_descriptor.insert("message", message);
        m_statusLabel->setText(message);
    }

    rebuildUi();
}

QString Dummy3DHostedViewWidget::sessionId() const
{
    return m_descriptor.value("session_id").toString();
}

QString Dummy3DHostedViewWidget::sceneId() const
{
    return m_descriptor.value("scene_id").toString();
}

QString Dummy3DHostedViewWidget::filePath() const
{
    return m_descriptor.value("file").toString();
}

QStringList Dummy3DHostedViewWidget::loadedFiles() const
{
    return m_loadedFiles;
}

QString Dummy3DHostedViewWidget::displayTitle() const
{
    const QString descriptorTitle = m_descriptor.value("title").toString().trimmed();
    if(!descriptorTitle.isEmpty()) {
        return descriptorTitle;
    }

    if(!m_loadedFiles.isEmpty()) {
        return QFileInfo(m_loadedFiles.constFirst()).fileName();
    }

    return m_descriptor.value("provider_display_name").toString("Inspect Surface View");
}

bool Dummy3DHostedViewWidget::addFileToScene(const QString& filePath)
{
    const QString normalizedPath = filePath.trimmed();
    if(normalizedPath.isEmpty()) {
        return false;
    }

    if(hasLoadedFile(normalizedPath)) {
        const QString message = QString("%1 is already loaded in this 3D scene.")
                                    .arg(QFileInfo(normalizedPath).fileName());
        m_descriptor.insert("message", message);
        rebuildUi();
        emit statusMessage(message);
        return true;
    }

    QString errorMessage;
    return loadFileInternal(normalizedPath, &errorMessage, true);
}

bool Dummy3DHostedViewWidget::hasLoadedFile(const QString& filePath) const
{
    return m_loadedFiles.contains(filePath.trimmed());
}

void Dummy3DHostedViewWidget::applySessionUpdate(const QJsonObject& update)
{
    for(auto it = update.constBegin(); it != update.constEnd(); ++it) {
        m_descriptor.insert(it.key(), it.value());
    }

    const QStringList filesToLoad = requestedSceneFiles();
    for(const QString& sceneFile : filesToLoad) {
        loadFileInternal(sceneFile, nullptr, false);
    }

    rebuildUi();
}

bool Dummy3DHostedViewWidget::loadFileInternal(const QString& filePath,
                                               QString* errorMessage,
                                               bool notifyUser)
{
    const QString normalizedPath = filePath.trimmed();
    if(normalizedPath.isEmpty()) {
        if(errorMessage) {
            *errorMessage = "No surface file path was provided.";
        }
        return false;
    }

    if(m_loadedFiles.contains(normalizedPath)) {
        return true;
    }

    bool loaded = false;
    QString localError;
    const QString suffix = QFileInfo(normalizedPath).suffix().toLower();
    if(suffix == "bem") {
        loaded = loadBemFile(normalizedPath, &localError);
    } else {
        loaded = loadSurfaceFile(normalizedPath, &localError);
    }

    if(!loaded) {
        if(errorMessage) {
            *errorMessage = localError;
        }

        if(notifyUser && !localError.isEmpty()) {
            m_descriptor.insert("message", localError);
            rebuildUi();
            emit statusMessage(localError);
            emit outputMessage(localError);
        }
        return false;
    }

    m_loadedFiles.append(normalizedPath);
    m_descriptor.insert("message",
                        QString("Loaded %1 into the 3D scene.")
                            .arg(QFileInfo(normalizedPath).fileName()));
    updateDescriptorProperties();
    rebuildUi();

    if(notifyUser) {
        emit statusMessage(m_descriptor.value("message").toString());
        emit outputMessage(QString("3D view added %1").arg(normalizedPath));
    }

    return true;
}

bool Dummy3DHostedViewWidget::loadSurfaceFile(const QString& filePath, QString* errorMessage)
{
    FSLIB::FsSurface surface(filePath);
    if(surface.isEmpty()) {
        if(errorMessage) {
            *errorMessage = QString("Failed to load FreeSurfer surface %1.").arg(filePath);
        }
        return false;
    }

    const QString hemi = inferHemisphere(filePath);
    const QString surfaceType = inferSurfaceType(filePath);
    const QString subjectName = inferSubjectName(filePath);

    m_model->addSurface(subjectName, hemi, surfaceType, surface);
    if(!m_surfaceTypes.contains(surfaceType)) {
        m_surfaceTypes.append(surfaceType);
    }

    m_brainView->setActiveSurface(surfaceType);
    return true;
}

bool Dummy3DHostedViewWidget::loadBemFile(const QString& filePath, QString* errorMessage)
{
    QFile file(filePath);
    if(!file.exists()) {
        if(errorMessage) {
            *errorMessage = QString("BEM file not found: %1").arg(filePath);
        }
        return false;
    }

    MNELIB::MNEBem bem(file);
    if(bem.isEmpty()) {
        if(errorMessage) {
            *errorMessage = QString("Failed to load BEM surface set %1.").arg(filePath);
        }
        return false;
    }

    const QString subjectName = inferSubjectName(filePath);
    for(int i = 0; i < bem.size(); ++i) {
        m_model->addBemSurface(subjectName, bemSurfaceName(bem[i], i), bem[i]);
    }

    return true;
}

void Dummy3DHostedViewWidget::rebuildUi()
{
    refreshSurfaceTypeSelector();
    refreshLoadedFileList();
    updateDescriptorProperties();

    m_titleLabel->setText(displayTitle());

    QStringList summaryParts;
    summaryParts << QString("Provider: %1")
                        .arg(m_descriptor.value("provider_display_name").toString("Inspect Surface View"));
    if(!sceneId().isEmpty()) {
        summaryParts << QString("Scene: %1").arg(sceneId());
    }
    summaryParts << QString("Layers: %1").arg(m_loadedFiles.size());
    m_summaryLabel->setText(summaryParts.join(" | "));

    m_statusLabel->setText(m_descriptor.value("message").toString("Inspect 3D scene ready."));
}

void Dummy3DHostedViewWidget::refreshLoadedFileList()
{
    m_loadedFilesList->clear();
    for(const QString& loadedFile : std::as_const(m_loadedFiles)) {
        QListWidgetItem* item = new QListWidgetItem(QFileInfo(loadedFile).fileName(), m_loadedFilesList);
        item->setToolTip(loadedFile);
    }
}

void Dummy3DHostedViewWidget::refreshSurfaceTypeSelector()
{
    const QSignalBlocker blocker(m_surfaceTypeCombo);
    const QString currentSurfaceType = m_surfaceTypeCombo->currentText();

    m_surfaceTypeCombo->clear();
    for(const QString& surfaceType : std::as_const(m_surfaceTypes)) {
        m_surfaceTypeCombo->addItem(surfaceType);
    }

    m_surfaceTypeCombo->setEnabled(m_surfaceTypeCombo->count() > 0);

    if(!currentSurfaceType.isEmpty()) {
        const int existingIndex = m_surfaceTypeCombo->findText(currentSurfaceType);
        if(existingIndex >= 0) {
            m_surfaceTypeCombo->setCurrentIndex(existingIndex);
            return;
        }
    }

    if(m_surfaceTypeCombo->count() > 0) {
        m_surfaceTypeCombo->setCurrentIndex(m_surfaceTypeCombo->count() - 1);
    }
}

void Dummy3DHostedViewWidget::updateDescriptorProperties()
{
    const QJsonArray sceneLayers = QJsonArray::fromStringList(m_loadedFiles);
    m_descriptor.insert("scene_layers", sceneLayers);
    m_descriptor.insert("layer_count", m_loadedFiles.size());
    setProperty("mne_loaded_files", m_loadedFiles);
    setProperty("mne_session_descriptor", m_descriptor);
}

QStringList Dummy3DHostedViewWidget::requestedSceneFiles() const
{
    QStringList files = jsonArrayToStringList(m_descriptor.value("scene_layers").toArray());

    const QStringList alternateFiles = jsonArrayToStringList(m_descriptor.value("sceneLayers").toArray());
    for(const QString& alternateFile : alternateFiles) {
        if(!files.contains(alternateFile)) {
            files.append(alternateFile);
        }
    }

    const QString primaryFile = m_descriptor.value("file").toString().trimmed();
    if(!primaryFile.isEmpty() && !files.contains(primaryFile)) {
        files.prepend(primaryFile);
    }

    return files;
}

QString Dummy3DHostedViewWidget::inferHemisphere(const QString& filePath) const
{
    const QString lowerFileName = QFileInfo(filePath).fileName().toLower();
    if(lowerFileName.contains("lh.") || lowerFileName.startsWith("lh_") || lowerFileName.startsWith("lh.")) {
        return "lh";
    }
    if(lowerFileName.contains("rh.") || lowerFileName.startsWith("rh_") || lowerFileName.startsWith("rh.")) {
        return "rh";
    }
    if(lowerFileName.contains("left")) {
        return "lh";
    }
    if(lowerFileName.contains("right")) {
        return "rh";
    }

    return "unknown";
}

QString Dummy3DHostedViewWidget::inferSurfaceType(const QString& filePath) const
{
    const QString lowerFileName = QFileInfo(filePath).fileName().toLower();
    if(lowerFileName.contains("inflated")) {
        return "inflated";
    }
    if(lowerFileName.contains("white")) {
        return "white";
    }
    if(lowerFileName.contains("orig")) {
        return "orig";
    }
    if(lowerFileName.contains("pial")) {
        return "pial";
    }

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if(!suffix.isEmpty()) {
        return suffix;
    }

    return "surface";
}

QString Dummy3DHostedViewWidget::inferSubjectName(const QString& filePath) const
{
    const QString descriptorSubject = m_descriptor.value("subjectId").toString().trimmed();
    if(!descriptorSubject.isEmpty()) {
        return descriptorSubject;
    }

    QDir parentDir = QFileInfo(filePath).dir();
    const QString parentName = parentDir.dirName().toLower();
    if((parentName == "surf" || parentName == "bem" || parentName == "label") && parentDir.cdUp()) {
        const QString subjectName = parentDir.dirName().trimmed();
        if(!subjectName.isEmpty()) {
            return subjectName;
        }
    }

    const QString directParentName = QFileInfo(filePath).dir().dirName().trimmed();
    if(!directParentName.isEmpty()) {
        return directParentName;
    }

    return "User";
}
