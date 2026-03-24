//=============================================================================================================
/**
 * @file     dummy3dhostedviewwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the hosted 3D surface widget backed by the MNE Inspect brain view.
 */

#ifndef MNE_ANALYZE_STUDIO_DUMMY3DHOSTEDVIEWWIDGET_H
#define MNE_ANALYZE_STUDIO_DUMMY3DHOSTEDVIEWWIDGET_H

#include <QJsonObject>
#include <QStringList>
#include <QWidget>

class QLabel;
class QListWidget;
class QComboBox;

class BrainTreeModel;
class BrainView;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Hosted widget that embeds the MNE Inspect 3D brain surface view inside the studio workbench.
 */
class Dummy3DHostedViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Dummy3DHostedViewWidget(QWidget* parent = nullptr);

    void setSessionDescriptor(const QJsonObject& descriptor);
    QString sessionId() const;
    QString sceneId() const;
    QString filePath() const;
    QStringList loadedFiles() const;
    QString displayTitle() const;

    Q_INVOKABLE bool addFileToScene(const QString& filePath);
    Q_INVOKABLE bool hasLoadedFile(const QString& filePath) const;

public slots:
    void applySessionUpdate(const QJsonObject& update);

signals:
    void outputMessage(const QString& message);
    void statusMessage(const QString& message);

private:
    bool loadFileInternal(const QString& filePath, QString* errorMessage = nullptr, bool notifyUser = true);
    bool loadSurfaceFile(const QString& filePath, QString* errorMessage = nullptr);
    bool loadBemFile(const QString& filePath, QString* errorMessage = nullptr);
    void rebuildUi();
    void refreshLoadedFileList();
    void refreshSurfaceTypeSelector();
    void updateDescriptorProperties();
    QStringList requestedSceneFiles() const;
    QString inferHemisphere(const QString& filePath) const;
    QString inferSurfaceType(const QString& filePath) const;
    QString inferSubjectName(const QString& filePath) const;

    QJsonObject m_descriptor;
    QStringList m_loadedFiles;
    QStringList m_surfaceTypes;
    QLabel* m_titleLabel;
    QLabel* m_summaryLabel;
    QLabel* m_statusLabel;
    QComboBox* m_surfaceTypeCombo;
    QListWidget* m_loadedFilesList;
    BrainView* m_brainView;
    BrainTreeModel* m_model;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_DUMMY3DHOSTEDVIEWWIDGET_H
