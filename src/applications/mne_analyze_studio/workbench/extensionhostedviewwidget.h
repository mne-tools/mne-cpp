//=============================================================================================================
/**
 * @file     extensionhostedviewwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the hosted extension view container used by the lean workbench core.
 */

#ifndef MNE_ANALYZE_STUDIO_EXTENSIONHOSTEDVIEWWIDGET_H
#define MNE_ANALYZE_STUDIO_EXTENSIONHOSTEDVIEWWIDGET_H

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QSlider;
class QTextEdit;
class QVBoxLayout;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Lightweight hosted-view shell that renders an extension view session descriptor in a core tab slot.
 */
class ExtensionHostedViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExtensionHostedViewWidget(QWidget* parent = nullptr);

    void setSessionDescriptor(const QJsonObject& descriptor);
    QString sessionId() const;
    QString filePath() const;

public slots:
    void applySessionUpdate(const QJsonObject& update);

signals:
    void viewCommandRequested(const QString& sessionId, const QString& commandName, const QJsonObject& arguments);

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

#endif // MNE_ANALYZE_STUDIO_EXTENSIONHOSTEDVIEWWIDGET_H
