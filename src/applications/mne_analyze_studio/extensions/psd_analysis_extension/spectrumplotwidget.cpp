//=============================================================================================================
/**
 * @file     spectrumplotwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the PSD analysis extension spectrum plot widget.
 */

#include "spectrumplotwidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

SpectrumPlotWidget::SpectrumPlotWidget(QWidget* parent)
: QWidget(parent)
{
    setMinimumHeight(180);
}

void SpectrumPlotWidget::setSpectrum(const QVector<double>& frequencies,
                                     const QVector<double>& values,
                                     const QString& title,
                                     const QString& xLabel,
                                     const QString& yLabel)
{
    m_frequencies = frequencies;
    m_values = values;
    m_title = title;
    m_xLabel = xLabel;
    m_yLabel = yLabel;
    update();
}

void SpectrumPlotWidget::clear()
{
    m_frequencies.clear();
    m_values.clear();
    m_comparisonFrequencies.clear();
    m_comparisonValues.clear();
    m_title.clear();
    m_xLabel = "Frequency (Hz)";
    m_yLabel = "PSD";
    m_comparisonLabel.clear();
    update();
}

void SpectrumPlotWidget::setComparisonSpectrum(const QVector<double>& frequencies,
                                               const QVector<double>& values,
                                               const QString& label)
{
    m_comparisonFrequencies = frequencies;
    m_comparisonValues = values;
    m_comparisonLabel = label;
    update();
}

void SpectrumPlotWidget::clearComparisonSpectrum()
{
    m_comparisonFrequencies.clear();
    m_comparisonValues.clear();
    m_comparisonLabel.clear();
    update();
}

void SpectrumPlotWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#22272e"));

    const QRect plotRect = rect().adjusted(56, 24, -20, -42);
    painter.setPen(QColor("#8b949e"));
    painter.drawText(QRect(rect().left(), 4, rect().width(), 18), Qt::AlignHCenter, m_title);

    if(m_frequencies.isEmpty() || m_values.isEmpty() || m_frequencies.size() != m_values.size() || plotRect.width() <= 0) {
        painter.setPen(QColor("#8b949e"));
        painter.drawText(plotRect, Qt::AlignCenter, "No spectrum available.");
        return;
    }

    const auto [minFreqIt, maxFreqIt] = std::minmax_element(m_frequencies.constBegin(), m_frequencies.constEnd());
    double minFreq = *minFreqIt;
    double maxFreq = *maxFreqIt;
    double minValue = *std::min_element(m_values.constBegin(), m_values.constEnd());
    double maxValue = *std::max_element(m_values.constBegin(), m_values.constEnd());

    const bool hasComparison = !m_comparisonFrequencies.isEmpty()
                               && m_comparisonFrequencies.size() == m_comparisonValues.size();
    if(hasComparison) {
        const auto [comparisonMinFreqIt, comparisonMaxFreqIt] =
            std::minmax_element(m_comparisonFrequencies.constBegin(), m_comparisonFrequencies.constEnd());
        minFreq = std::min(minFreq, *comparisonMinFreqIt);
        maxFreq = std::max(maxFreq, *comparisonMaxFreqIt);
        minValue = std::min(minValue, *std::min_element(m_comparisonValues.constBegin(), m_comparisonValues.constEnd()));
        maxValue = std::max(maxValue, *std::max_element(m_comparisonValues.constBegin(), m_comparisonValues.constEnd()));
    }

    const double valueSpan = std::max(1e-12, maxValue - minValue);
    const double freqSpan = std::max(1e-12, maxFreq - minFreq);

    painter.setPen(QColor("#30363d"));
    painter.drawRect(plotRect);
    for(int i = 1; i < 5; ++i) {
        const int y = plotRect.top() + (plotRect.height() * i) / 5;
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);
    }

    QPainterPath path;
    for(int i = 0; i < m_frequencies.size(); ++i) {
        const double xNorm = (m_frequencies.at(i) - minFreq) / freqSpan;
        const double yNorm = (m_values.at(i) - minValue) / valueSpan;
        const QPointF point(plotRect.left() + xNorm * plotRect.width(),
                            plotRect.bottom() - yNorm * plotRect.height());
        if(i == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }

    painter.setPen(QPen(QColor("#2f81f7"), 2.0));
    painter.drawPath(path);

    if(hasComparison) {
        QPainterPath comparisonPath;
        for(int i = 0; i < m_comparisonFrequencies.size(); ++i) {
            const double xNorm = (m_comparisonFrequencies.at(i) - minFreq) / freqSpan;
            const double yNorm = (m_comparisonValues.at(i) - minValue) / valueSpan;
            const QPointF point(plotRect.left() + xNorm * plotRect.width(),
                                plotRect.bottom() - yNorm * plotRect.height());
            if(i == 0) {
                comparisonPath.moveTo(point);
            } else {
                comparisonPath.lineTo(point);
            }
        }

        painter.setPen(QPen(QColor("#ffa657"), 1.6, Qt::DashLine));
        painter.drawPath(comparisonPath);
    }

    painter.setPen(QColor("#8b949e"));
    painter.drawText(QRect(plotRect.left(), rect().bottom() - 24, plotRect.width(), 18), Qt::AlignCenter, m_xLabel);
    painter.save();
    painter.translate(18, plotRect.center().y());
    painter.rotate(-90.0);
    painter.drawText(QRect(-plotRect.height() / 2, -18, plotRect.height(), 18), Qt::AlignCenter, m_yLabel);
    painter.restore();

    painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 4, 120, 16), Qt::AlignLeft,
                     QString::number(minFreq, 'f', 1));
    painter.drawText(QRect(plotRect.right() - 120, plotRect.bottom() + 4, 120, 16), Qt::AlignRight,
                     QString::number(maxFreq, 'f', 1));
    painter.drawText(QRect(4, plotRect.top() - 6, 48, 16), Qt::AlignRight,
                     QString::number(maxValue, 'g', 4));
    painter.drawText(QRect(4, plotRect.bottom() - 8, 48, 16), Qt::AlignRight,
                     QString::number(minValue, 'g', 4));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#2f81f7"));
    painter.drawRect(QRect(plotRect.left() + 8, plotRect.top() + 8, 12, 4));
    painter.setPen(QColor("#8b949e"));
    painter.drawText(QRect(plotRect.left() + 26, plotRect.top() + 2, 220, 16), Qt::AlignLeft, "Current");

    if(hasComparison) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#ffa657"));
        painter.drawRect(QRect(plotRect.left() + 8, plotRect.top() + 26, 12, 4));
        painter.setPen(QColor("#8b949e"));
        painter.drawText(QRect(plotRect.left() + 26, plotRect.top() + 20, plotRect.width() - 32, 16),
                         Qt::AlignLeft,
                         m_comparisonLabel.isEmpty() ? QString("Comparison") : m_comparisonLabel);
    }
}
