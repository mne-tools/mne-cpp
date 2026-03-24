//=============================================================================================================
/**
 * @file     analysisresultwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements a read-only center-view widget for analysis result artifacts.
 */

#include "analysisresultwidget.h"

#include "spectrumplotwidget.h"

#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

namespace
{

QTreeWidgetItem* buildJsonTreeItem(const QString& key, const QJsonValue& value)
{
    QString displayValue;
    if(value.isObject()) {
        displayValue = "{...}";
    } else if(value.isArray()) {
        displayValue = QString("[%1]").arg(value.toArray().size());
    } else if(value.isString()) {
        displayValue = value.toString();
    } else if(value.isDouble()) {
        displayValue = QString::number(value.toDouble(), 'g', 8);
    } else if(value.isBool()) {
        displayValue = value.toBool() ? QString("true") : QString("false");
    } else if(value.isNull()) {
        displayValue = "null";
    } else {
        displayValue = QString::fromUtf8(QJsonDocument::fromVariant(value.toVariant()).toJson(QJsonDocument::Compact));
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << key << displayValue);
    if(value.isObject()) {
        const QJsonObject object = value.toObject();
        for(auto it = object.constBegin(); it != object.constEnd(); ++it) {
            item->addChild(buildJsonTreeItem(it.key(), it.value()));
        }
    } else if(value.isArray()) {
        const QJsonArray array = value.toArray();
        for(int i = 0; i < array.size(); ++i) {
            item->addChild(buildJsonTreeItem(QString("[%1]").arg(i), array.at(i)));
        }
    }

    return item;
}

}

AnalysisResultWidget::AnalysisResultWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel("Analysis Result", this))
, m_stack(new QStackedWidget(this))
, m_tree(new QTreeWidget(this))
, m_table(new QTableWidget(this))
, m_spectrum(new SpectrumPlotWidget(this))
{
    m_titleLabel->setObjectName("terminalStatusLabel");
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels(QStringList() << "Field" << "Value");
    m_tree->setAlternatingRowColors(true);

    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(QStringList() << "Name" << "RMS" << "Mean |x|" << "Peak |x|");
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setStretchLastSection(true);

    m_stack->addWidget(m_tree);
    m_stack->addWidget(m_table);
    m_stack->addWidget(m_spectrum);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_stack, 1);
}

void AnalysisResultWidget::setResult(const QString& toolName, const QJsonObject& result)
{
    m_toolName = toolName;
    m_result = result;
    m_titleLabel->setText(result.value("message").toString(toolName));
    m_tree->clear();
    m_table->clearContents();
    m_table->setRowCount(0);
    m_spectrum->clear();
    m_stack->setCurrentWidget(m_tree);

    if(toolName == "neurokernel.channel_stats" && result.value("channels").isArray()) {
        const QJsonArray channels = result.value("channels").toArray();
        m_table->setRowCount(channels.size());
        for(int row = 0; row < channels.size(); ++row) {
            const QJsonObject channel = channels.at(row).toObject();
            m_table->setItem(row, 0, new QTableWidgetItem(channel.value("name").toString()));
            m_table->setItem(row, 1, new QTableWidgetItem(QString::number(channel.value("rms").toDouble(), 'g', 8)));
            m_table->setItem(row, 2, new QTableWidgetItem(QString::number(channel.value("mean_abs").toDouble(), 'g', 8)));
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(channel.value("peak_abs").toDouble(), 'g', 8)));
        }
        m_stack->setCurrentWidget(m_table);
        return;
    }

    if(toolName == "neurokernel.raw_stats" && result.value("top_channels").isArray()) {
        const QJsonArray channels = result.value("top_channels").toArray();
        m_table->setRowCount(channels.size());
        for(int row = 0; row < channels.size(); ++row) {
            const QJsonObject channel = channels.at(row).toObject();
            m_table->setItem(row, 0, new QTableWidgetItem(channel.value("name").toString()));
            m_table->setItem(row, 1, new QTableWidgetItem(QString::number(channel.value("rms").toDouble(), 'g', 8)));
            m_table->setItem(row, 2, new QTableWidgetItem("-"));
            m_table->setItem(row, 3, new QTableWidgetItem("-"));
        }
        m_stack->setCurrentWidget(m_table);
        return;
    }

    if(toolName == "neurokernel.psd_summary"
       && result.value("frequencies").isArray()
       && result.value("psd").isArray()) {
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

        m_spectrum->setSpectrum(frequencies, values, result.value("message").toString(toolName));
        m_stack->setCurrentWidget(m_spectrum);
        return;
    }

    for(auto it = result.constBegin(); it != result.constEnd(); ++it) {
        m_tree->addTopLevelItem(buildJsonTreeItem(it.key(), it.value()));
    }
    m_tree->expandToDepth(1);
    for(int column = 0; column < m_tree->columnCount(); ++column) {
        m_tree->resizeColumnToContents(column);
    }
}

QString AnalysisResultWidget::toolName() const
{
    return m_toolName;
}

QJsonObject AnalysisResultWidget::result() const
{
    return m_result;
}
