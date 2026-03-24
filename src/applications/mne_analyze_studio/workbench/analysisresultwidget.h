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

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;

namespace MNEANALYZESTUDIO
{

class SpectrumPlotWidget;

/**
 * @brief Read-only viewer widget used to open persisted analysis artifacts in center tabs.
 */
class AnalysisResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnalysisResultWidget(QWidget* parent = nullptr);

    void setResult(const QString& toolName, const QJsonObject& result);
    QString toolName() const;
    QJsonObject result() const;

private:
    QTreeWidget* buildJsonTree(const QJsonObject& result) const;

    QString m_toolName;
    QJsonObject m_result;
    QLabel* m_titleLabel;
    QStackedWidget* m_stack;
    QTreeWidget* m_tree;
    QTableWidget* m_table;
    SpectrumPlotWidget* m_spectrum;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_ANALYSISRESULTWIDGET_H
