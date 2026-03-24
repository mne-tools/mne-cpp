//=============================================================================================================
/**
 * @file     analysisresultswidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares extension-owned renderers for structured analysis result views.
 */

#ifndef MNE_ANALYZE_STUDIO_ANALYSISRESULTSWIDGET_H
#define MNE_ANALYZE_STUDIO_ANALYSISRESULTSWIDGET_H

#include <iresultrendererwidget.h>

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;
class QSplitter;
class QTableWidget;
class QTreeWidget;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Extension-owned renderer for channel statistics, raw statistics, and peak-finding results.
 */
class AnalysisResultsWidget : public QWidget, public IResultRendererWidget
{
    Q_OBJECT

public:
    explicit AnalysisResultsWidget(QWidget* parent = nullptr);

    void setResult(const QString& toolName, const QJsonObject& result) override;
    QString toolName() const override;
    QJsonObject result() const override;
    void setRuntimeContext(const QJsonObject& context) override;

signals:
    void toolCommandRequested(const QString& commandText);
    void selectionContextChanged(const QJsonObject& context);

private slots:
    void updatePrimaryActionFromSelection();
    void runPrimaryAction();
    void updatePipelineSelection();
    void runSecondaryAction();

private:
    QJsonObject toolDefaults(const QString& toolName) const;
    void emitSelectionContext();
    void setTreeResult(const QJsonObject& result);
    void setChannelTable(const QJsonArray& channels, bool allowPeakAction);
    void setPipelineResult(const QJsonObject& result);

    QString m_toolName;
    QJsonObject m_result;
    QJsonObject m_runtimeContext;
    QString m_primaryActionCommand;
    QString m_secondaryActionCommand;
    QLabel* m_titleLabel;
    QPushButton* m_primaryActionButton;
    QPushButton* m_secondaryActionButton;
    QStackedWidget* m_stack;
    QTreeWidget* m_tree;
    QTableWidget* m_table;
    QWidget* m_pipelineView;
    QSplitter* m_pipelineSplitter;
    QTableWidget* m_pipelineStepsTable;
    QTreeWidget* m_pipelineStepDetailsTree;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_ANALYSISRESULTSWIDGET_H
