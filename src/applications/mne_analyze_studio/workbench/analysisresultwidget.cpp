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

#include <iresultrendererfactory.h>
#include <iresultrendererwidget.h>
#include <resultrendererfactoryregistry.h>

#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMetaObject>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

namespace
{

bool hasQtSignal(const QObject* object, const char* normalizedSignal)
{
    if(!object || !normalizedSignal) {
        return false;
    }

    return object->metaObject()->indexOfSignal(normalizedSignal) >= 0;
}

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
, m_extensionRenderer(nullptr)
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
    m_stack->setCurrentWidget(m_tree);

    if(QWidget* renderer = ensureExtensionRenderer(toolName)) {
        if(IResultRendererWidget* resultRenderer = dynamic_cast<IResultRendererWidget*>(renderer)) {
            resultRenderer->setResult(toolName, result);
            resultRenderer->setResultHistory(m_history);
            resultRenderer->setRuntimeContext(m_runtimeContext);
            m_stack->setCurrentWidget(renderer);
            return;
        }
    }

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

    for(auto it = result.constBegin(); it != result.constEnd(); ++it) {
        m_tree->addTopLevelItem(buildJsonTreeItem(it.key(), it.value()));
    }
    m_tree->expandToDepth(1);
    for(int column = 0; column < m_tree->columnCount(); ++column) {
        m_tree->resizeColumnToContents(column);
    }
}

void AnalysisResultWidget::setResultHistory(const QJsonArray& history)
{
    m_history = history;

    if(IResultRendererWidget* resultRenderer = dynamic_cast<IResultRendererWidget*>(m_extensionRenderer)) {
        resultRenderer->setResultHistory(m_history);
    }
}

void AnalysisResultWidget::setRuntimeContext(const QJsonObject& context)
{
    m_runtimeContext = context;

    if(IResultRendererWidget* resultRenderer = dynamic_cast<IResultRendererWidget*>(m_extensionRenderer)) {
        resultRenderer->setRuntimeContext(m_runtimeContext);
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

QWidget* AnalysisResultWidget::ensureExtensionRenderer(const QString& toolName)
{
    const IResultRendererFactory* factory = ResultRendererFactoryRegistry::instance().factoryForToolName(toolName);
    if(!factory) {
        if(m_extensionRenderer) {
            m_stack->removeWidget(m_extensionRenderer);
            m_extensionRenderer->deleteLater();
            m_extensionRenderer = nullptr;
        }
        return nullptr;
    }

    if(m_extensionRenderer
       && m_extensionRenderer->property("mne_result_renderer_tool").toString() == toolName) {
        return m_extensionRenderer;
    }

    if(m_extensionRenderer) {
        m_stack->removeWidget(m_extensionRenderer);
        m_extensionRenderer->deleteLater();
        m_extensionRenderer = nullptr;
    }

    m_extensionRenderer = factory->createRenderer(this);
    if(!m_extensionRenderer) {
        return nullptr;
    }

    m_extensionRenderer->setProperty("mne_result_renderer_tool", toolName);
    if(hasQtSignal(m_extensionRenderer, "toolCommandRequested(QString)")) {
        QObject::connect(m_extensionRenderer,
                         SIGNAL(toolCommandRequested(QString)),
                         this,
                         SIGNAL(toolCommandRequested(QString)));
    }
    if(hasQtSignal(m_extensionRenderer, "selectionContextChanged(QJsonObject)")) {
        QObject::connect(m_extensionRenderer,
                         SIGNAL(selectionContextChanged(QJsonObject)),
                         this,
                         SIGNAL(selectionContextChanged(QJsonObject)));
    }
    m_stack->addWidget(m_extensionRenderer);
    return m_extensionRenderer;
}
