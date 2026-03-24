//=============================================================================================================
/**
 * @file     psdresultwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the PSD extension-owned result renderer widget.
 */

#ifndef MNE_ANALYZE_STUDIO_PSDRESULTWIDGET_H
#define MNE_ANALYZE_STUDIO_PSDRESULTWIDGET_H

#include <iresultrendererwidget.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class QComboBox;
class QPushButton;
class QSpinBox;

namespace MNEANALYZESTUDIO
{

class SpectrumPlotWidget;

/**
 * @brief Extension-owned renderer widget for PSD tool results.
 */
class PsdResultWidget : public QWidget, public IResultRendererWidget
{
    Q_OBJECT

public:
    explicit PsdResultWidget(QWidget* parent = nullptr);

    void setResult(const QString& toolName, const QJsonObject& result) override;
    QString toolName() const override;
    QJsonObject result() const override;
    void setResultHistory(const QJsonArray& history) override;
    void setRuntimeContext(const QJsonObject& context) override;

signals:
    void toolCommandRequested(const QString& commandText);

public slots:
    void setComparisonResult(const QJsonObject& result);
    void clearComparisonResult();

private slots:
    void emitRerunCommand();
    void updateComparisonFromSelection();

private:
    void applySpectrumResult(const QJsonObject& result, bool comparison);
    void rebuildControls();

    QString m_toolName;
    QJsonObject m_result;
    QJsonArray m_history;
    QJsonObject m_runtimeContext;
    QComboBox* m_matchCombo;
    QSpinBox* m_nfftSpin;
    QComboBox* m_compareCombo;
    QPushButton* m_updateButton;
    SpectrumPlotWidget* m_plot;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_PSDRESULTWIDGET_H
