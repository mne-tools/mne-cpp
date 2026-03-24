//=============================================================================================================
/**
 * @file     psdresultwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the PSD extension-owned result renderer widget.
 */

#include "psdresultwidget.h"

#include "spectrumplotwidget.h"

#include <iresultrendererfactory.h>
#include <resultrendererfactoryregistry.h>

#include <QComboBox>
#include <QFileInfo>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

namespace
{

class PsdResultRendererFactory : public IResultRendererFactory
{
public:
    QString rendererId() const override
    {
        return "psd.result_renderer";
    }

    QString widgetType() const override
    {
        return "psd_result_renderer";
    }

    QStringList supportedToolNames() const override
    {
        return QStringList() << "neurokernel.psd_summary";
    }

    QWidget* createRenderer(QWidget* parent = nullptr) const override
    {
        return new PsdResultWidget(parent);
    }
};

const bool s_psdResultRendererRegistered = []() {
    static PsdResultRendererFactory s_factory;
    ResultRendererFactoryRegistry::instance().registerFactory(&s_factory);
    return true;
}();

}

PsdResultWidget::PsdResultWidget(QWidget* parent)
: QWidget(parent)
, m_matchCombo(new QComboBox(this))
, m_nfftSpin(new QSpinBox(this))
, m_compareCombo(new QComboBox(this))
, m_updateButton(new QPushButton("Update PSD", this))
, m_plot(new SpectrumPlotWidget(this))
{
    Q_UNUSED(s_psdResultRendererRegistered)

    m_matchCombo->addItems(QStringList() << "EEG" << "MEG" << "EOG" << "All");
    m_nfftSpin->setRange(32, 8192);
    m_nfftSpin->setSingleStep(32);
    m_nfftSpin->setValue(256);
    m_compareCombo->addItem("No comparison");

    QHBoxLayout* controlsLayout = new QHBoxLayout;
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(8);
    controlsLayout->addWidget(new QLabel("PSD Match", this));
    controlsLayout->addWidget(m_matchCombo);
    controlsLayout->addWidget(new QLabel("FFT", this));
    controlsLayout->addWidget(m_nfftSpin);
    controlsLayout->addWidget(new QLabel("Compare", this));
    controlsLayout->addWidget(m_compareCombo, 1);
    controlsLayout->addWidget(m_updateButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addLayout(controlsLayout);
    layout->addWidget(m_plot);

    connect(m_updateButton, &QPushButton::clicked, this, &PsdResultWidget::emitRerunCommand);
    connect(m_compareCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &PsdResultWidget::updateComparisonFromSelection);
}

void PsdResultWidget::setResult(const QString& toolName, const QJsonObject& result)
{
    m_toolName = toolName;
    m_result = result;
    applySpectrumResult(result, false);
    rebuildControls();
}

QString PsdResultWidget::toolName() const
{
    return m_toolName;
}

QJsonObject PsdResultWidget::result() const
{
    return m_result;
}

void PsdResultWidget::setResultHistory(const QJsonArray& history)
{
    m_history = history;
    rebuildControls();
}

void PsdResultWidget::setRuntimeContext(const QJsonObject& context)
{
    m_runtimeContext = context;
    m_updateButton->setEnabled(context.value("has_active_raw_view").toBool(false));
    rebuildControls();
}

void PsdResultWidget::setComparisonResult(const QJsonObject& result)
{
    applySpectrumResult(result, true);
}

void PsdResultWidget::clearComparisonResult()
{
    m_plot->clearComparisonSpectrum();
}

void PsdResultWidget::emitRerunCommand()
{
    if(m_toolName != "neurokernel.psd_summary") {
        return;
    }

    QJsonObject arguments = m_runtimeContext.value("tool_defaults").toObject().value("neurokernel.psd_summary").toObject();
    const QString selectedMatch = m_matchCombo->currentText().trimmed();
    const QString match = selectedMatch.compare("All", Qt::CaseInsensitive) == 0 ? QString() : selectedMatch;
    const int windowSamples = std::max(32, m_result.value("window_samples").toInt(arguments.value("window_samples").toInt(1200)));

    arguments.insert("window_samples", windowSamples);
    arguments.insert("nfft", m_nfftSpin->value());
    if(!match.isEmpty()) {
        arguments.insert("match", match);
    } else {
        arguments.remove("match");
    }

    const QString argumentsText = QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact));
    emit toolCommandRequested(QString("tools.call neurokernel.psd_summary %1").arg(argumentsText));
}

void PsdResultWidget::updateComparisonFromSelection()
{
    if(m_compareCombo->currentIndex() <= 0) {
        clearComparisonResult();
        return;
    }

    const QJsonObject comparison = m_compareCombo->currentData().toJsonObject();
    if(!comparison.isEmpty()) {
        setComparisonResult(comparison);
    }
}

void PsdResultWidget::rebuildControls()
{
    {
        const QSignalBlocker blocker(m_matchCombo);
        const QJsonObject defaults = m_runtimeContext.value("tool_defaults").toObject().value("neurokernel.psd_summary").toObject();
        const QString defaultMatch = defaults.value("match").toString().trimmed().toUpper();
        const QString match = m_result.value("match").toString(defaultMatch).trimmed().toUpper();
        const int matchIndex = m_matchCombo->findText(match.isEmpty() ? "All" : match, Qt::MatchFixedString);
        m_matchCombo->setCurrentIndex(matchIndex >= 0 ? matchIndex : m_matchCombo->findText("All"));
    }

    {
        const QSignalBlocker blocker(m_nfftSpin);
        const QJsonObject defaults = m_runtimeContext.value("tool_defaults").toObject().value("neurokernel.psd_summary").toObject();
        const int nfft = m_result.value("nfft").toInt(defaults.value("nfft").toInt(256));
        m_nfftSpin->setValue(std::max(m_nfftSpin->minimum(), std::min(m_nfftSpin->maximum(), nfft)));
    }

    {
        const QSignalBlocker blocker(m_compareCombo);
        m_compareCombo->clear();
        m_compareCombo->addItem("No comparison");
        for(int i = m_history.size() - 1; i >= 0; --i) {
            const QJsonObject entry = m_history.at(i).toObject();
            if(entry.isEmpty() || entry == m_result) {
                continue;
            }

            const QString label = QString("%1 | %2 | nfft %3 | %4 ch")
                                      .arg(entry.value("match").toString().trimmed().isEmpty()
                                               ? QString("All")
                                               : entry.value("match").toString().trimmed(),
                                           QFileInfo(entry.value("file").toString()).fileName())
                                      .arg(entry.value("nfft").toInt(0))
                                      .arg(entry.value("channel_count").toInt(0));
            m_compareCombo->addItem(label, entry);
        }
    }

    m_updateButton->setEnabled(m_runtimeContext.value("has_active_raw_view").toBool(false));
    updateComparisonFromSelection();
}

void PsdResultWidget::applySpectrumResult(const QJsonObject& result, bool comparison)
{
    QVector<double> frequencies;
    QVector<double> values;
    const QJsonArray freqArray = result.value("frequencies").toArray();
    const QJsonArray valueArray = result.value("psd").toArray();
    const int pointCount = std::min(freqArray.size(), valueArray.size());
    frequencies.reserve(pointCount);
    values.reserve(pointCount);
    for(int i = 0; i < pointCount; ++i) {
        frequencies.append(freqArray.at(i).toDouble());
        values.append(valueArray.at(i).toDouble());
    }

    const QString label = result.value("message").toString(m_toolName);
    if(comparison) {
        m_plot->setComparisonSpectrum(frequencies, values, label);
    } else {
        m_plot->setSpectrum(frequencies, values, label);
    }
}
