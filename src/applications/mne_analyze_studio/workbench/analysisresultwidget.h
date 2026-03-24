//=============================================================================================================
/**
 * @file     analysisresultwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares a read-only center-view widget for analysis result artifacts.
 */

#ifndef MNE_ANALYZE_STUDIO_ANALYSISRESULTWIDGET_H
#define MNE_ANALYZE_STUDIO_ANALYSISRESULTWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;
class QWidget;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Read-only viewer widget used to open persisted analysis artifacts in center tabs.
 */
class AnalysisResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnalysisResultWidget(QWidget* parent = nullptr);

    void setResult(const QString& toolName, const QJsonObject& result);
    void setResultHistory(const QJsonArray& history);
    void setRuntimeContext(const QJsonObject& context);
    QString toolName() const;
    QJsonObject result() const;

signals:
    void toolCommandRequested(const QString& commandText);
    void selectionContextChanged(const QJsonObject& context);

private:
    QTreeWidget* buildJsonTree(const QJsonObject& result) const;
    QWidget* ensureExtensionRenderer(const QString& toolName);

    QString m_toolName;
    QJsonObject m_result;
    QJsonArray m_history;
    QJsonObject m_runtimeContext;
    QLabel* m_titleLabel;
    QStackedWidget* m_stack;
    QTreeWidget* m_tree;
    QTableWidget* m_table;
    QWidget* m_extensionRenderer;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_ANALYSISRESULTWIDGET_H
