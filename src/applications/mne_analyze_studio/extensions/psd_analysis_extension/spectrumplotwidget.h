//=============================================================================================================
/**
 * @file     spectrumplotwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the PSD analysis extension spectrum plot widget.
 */

#ifndef MNE_ANALYZE_STUDIO_SPECTRUMPLOTWIDGET_H
#define MNE_ANALYZE_STUDIO_SPECTRUMPLOTWIDGET_H

#include <QVector>
#include <QWidget>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Simple line-plot widget for displaying frequency-domain spectra.
 */
class SpectrumPlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumPlotWidget(QWidget* parent = nullptr);

    void setSpectrum(const QVector<double>& frequencies,
                     const QVector<double>& values,
                     const QString& title,
                     const QString& xLabel = QString("Frequency (Hz)"),
                     const QString& yLabel = QString("PSD"));
    void setComparisonSpectrum(const QVector<double>& frequencies,
                               const QVector<double>& values,
                               const QString& label);
    void clearComparisonSpectrum();
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_frequencies;
    QVector<double> m_values;
    QVector<double> m_comparisonFrequencies;
    QVector<double> m_comparisonValues;
    QString m_title;
    QString m_xLabel;
    QString m_yLabel;
    QString m_comparisonLabel;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_SPECTRUMPLOTWIDGET_H
