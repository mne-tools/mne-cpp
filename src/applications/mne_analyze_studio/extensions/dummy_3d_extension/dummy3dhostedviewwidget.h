//=============================================================================================================
/**
 * @file     dummy3dhostedviewwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the dummy 3D hosted view widget contributed by the dummy 3D extension.
 */

#ifndef MNE_ANALYZE_STUDIO_DUMMY3DHOSTEDVIEWWIDGET_H
#define MNE_ANALYZE_STUDIO_DUMMY3DHOSTEDVIEWWIDGET_H

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QSlider;
class QTextEdit;
class QVBoxLayout;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Extension-owned placeholder 3D widget that reflects hosted session state and actions.
 */
class Dummy3DHostedViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Dummy3DHostedViewWidget(QWidget* parent = nullptr);

    void setSessionDescriptor(const QJsonObject& descriptor);
    QString sessionId() const;
    QString filePath() const;

public slots:
    void applySessionUpdate(const QJsonObject& update);

signals:
    void viewCommandRequested(const QString& sessionId, const QString& commandName, const QJsonObject& arguments);
    void outputMessage(const QString& message);
    void statusMessage(const QString& message);

private:
    void rebuildUi();
    void rebuildActionButtons(const QJsonArray& actions);

    QJsonObject m_descriptor;
    QLabel* m_titleLabel;
    QLabel* m_summaryLabel;
    QLabel* m_statusLabel;
    QLabel* m_stateLabel;
    QLabel* m_opacityValueLabel;
    QSlider* m_opacitySlider;
    QWidget* m_actionsWidget;
    QVBoxLayout* m_actionsLayout;
    QTextEdit* m_detailsView;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_DUMMY3DHOSTEDVIEWWIDGET_H
