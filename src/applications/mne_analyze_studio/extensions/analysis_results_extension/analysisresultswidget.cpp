//=============================================================================================================
/**
 * @file     analysisresultswidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements extension-owned renderers for structured analysis result views.
 */

#include "analysisresultswidget.h"

#include <iresultrendererfactory.h>
#include <resultrendererfactoryregistry.h>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

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

class AnalysisResultRendererFactory : public IResultRendererFactory
{
public:
    QString rendererId() const override
    {
        return "analysis.channel_and_peak_renderer";
    }

    QString widgetType() const override
    {
        return "analysis_result_renderer";
    }

    QStringList supportedToolNames() const override
    {
        return QStringList()
               << "neurokernel.channel_stats"
               << "neurokernel.raw_stats"
               << "neurokernel.find_peak_window"
               << "studio.pipeline.run";
    }

    QWidget* createRenderer(QWidget* parent = nullptr) const override
    {
        return new AnalysisResultsWidget(parent);
    }
};

const bool s_analysisResultRendererRegistered = []() {
    static AnalysisResultRendererFactory s_factory;
    ResultRendererFactoryRegistry::instance().registerFactory(&s_factory);
    return true;
}();

}

AnalysisResultsWidget::AnalysisResultsWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel("Analysis Result", this))
, m_primaryActionButton(new QPushButton("Run Action", this))
, m_secondaryActionButton(new QPushButton("Run Step", this))
, m_stack(new QStackedWidget(this))
, m_tree(new QTreeWidget(this))
, m_table(new QTableWidget(this))
, m_pipelineView(new QWidget(this))
, m_pipelineSplitter(new QSplitter(Qt::Vertical, m_pipelineView))
, m_pipelineStepsTable(new QTableWidget(m_pipelineView))
, m_pipelineStepDetailsTree(new QTreeWidget(m_pipelineView))
{
    Q_UNUSED(s_analysisResultRendererRegistered)

    m_titleLabel->setObjectName("terminalStatusLabel");
    m_primaryActionButton->setVisible(false);
    m_primaryActionButton->setEnabled(false);
    m_secondaryActionButton->setVisible(false);
    m_secondaryActionButton->setEnabled(false);

    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels(QStringList() << "Field" << "Value");
    m_tree->setAlternatingRowColors(true);

    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(QStringList() << "Name" << "RMS" << "Mean |x|" << "Peak |x|");
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setStretchLastSection(true);

    m_pipelineStepsTable->setColumnCount(6);
    m_pipelineStepsTable->setHorizontalHeaderLabels(QStringList() << "#" << "Status" << "Started" << "Finished" << "Tool" << "Command");
    m_pipelineStepsTable->setSortingEnabled(false);
    m_pipelineStepsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pipelineStepsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pipelineStepsTable->horizontalHeader()->setStretchLastSection(true);

    m_pipelineStepDetailsTree->setColumnCount(2);
    m_pipelineStepDetailsTree->setHeaderLabels(QStringList() << "Field" << "Value");
    m_pipelineStepDetailsTree->setAlternatingRowColors(true);

    m_pipelineSplitter->addWidget(m_pipelineStepsTable);
    m_pipelineSplitter->addWidget(m_pipelineStepDetailsTree);
    m_pipelineSplitter->setStretchFactor(0, 2);
    m_pipelineSplitter->setStretchFactor(1, 1);

    QVBoxLayout* pipelineLayout = new QVBoxLayout(m_pipelineView);
    pipelineLayout->setContentsMargins(0, 0, 0, 0);
    pipelineLayout->setSpacing(0);
    pipelineLayout->addWidget(m_pipelineSplitter);

    m_stack->addWidget(m_tree);
    m_stack->addWidget(m_table);
    m_stack->addWidget(m_pipelineView);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_titleLabel);
    QHBoxLayout* actionLayout = new QHBoxLayout;
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->addWidget(m_primaryActionButton);
    actionLayout->addWidget(m_secondaryActionButton);
    actionLayout->addStretch(1);
    layout->addLayout(actionLayout);
    layout->addWidget(m_stack, 1);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AnalysisResultsWidget::updatePrimaryActionFromSelection);
    connect(m_primaryActionButton, &QPushButton::clicked, this, &AnalysisResultsWidget::runPrimaryAction);
    connect(m_secondaryActionButton, &QPushButton::clicked, this, &AnalysisResultsWidget::runSecondaryAction);
    connect(m_pipelineStepsTable, &QTableWidget::itemSelectionChanged, this, &AnalysisResultsWidget::updatePipelineSelection);
}

void AnalysisResultsWidget::setResult(const QString& toolName, const QJsonObject& result)
{
    m_toolName = toolName;
    m_result = result;
    m_primaryActionCommand.clear();
    m_secondaryActionCommand.clear();
    m_primaryActionButton->setVisible(false);
    m_primaryActionButton->setEnabled(false);
    m_secondaryActionButton->setVisible(false);
    m_secondaryActionButton->setEnabled(false);
    m_tree->clear();
    m_table->clearContents();
    m_table->setRowCount(0);
    m_pipelineStepsTable->clearContents();
    m_pipelineStepsTable->setRowCount(0);
    m_pipelineStepDetailsTree->clear();
    m_stack->setCurrentWidget(m_tree);
    m_titleLabel->setText(result.value("message").toString(toolName));

    if(toolName == "studio.pipeline.run") {
        setPipelineResult(result);
        return;
    }

    if(toolName == "neurokernel.channel_stats" && result.value("channels").isArray()) {
        setChannelTable(result.value("channels").toArray(), true);
        return;
    }

    if(toolName == "neurokernel.raw_stats" && result.value("top_channels").isArray()) {
        setChannelTable(result.value("top_channels").toArray(), true);
        return;
    }

    if(toolName == "neurokernel.find_peak_window") {
        setTreeResult(result);
        const int peakSample = result.value("peak_sample").toInt(-1);
        if(peakSample >= 0) {
            m_primaryActionCommand = QString("tools.call view.raw.goto {\"sample\":%1}").arg(peakSample);
            m_primaryActionButton->setText(QString("Jump To Peak Sample %1").arg(peakSample));
            m_primaryActionButton->setVisible(true);
            m_primaryActionButton->setEnabled(true);
        }
        emitSelectionContext();
        return;
    }

    setTreeResult(result);
    emitSelectionContext();
}

QString AnalysisResultsWidget::toolName() const
{
    return m_toolName;
}

QJsonObject AnalysisResultsWidget::result() const
{
    return m_result;
}

void AnalysisResultsWidget::setRuntimeContext(const QJsonObject& context)
{
    m_runtimeContext = context;
    updatePrimaryActionFromSelection();
    updatePipelineSelection();
}

QJsonObject AnalysisResultsWidget::toolDefaults(const QString& toolName) const
{
    return m_runtimeContext.value("tool_defaults").toObject().value(toolName).toObject();
}

void AnalysisResultsWidget::updatePrimaryActionFromSelection()
{
    if(m_toolName != "neurokernel.channel_stats" && m_toolName != "neurokernel.raw_stats") {
        emitSelectionContext();
        return;
    }

    const QList<QTableWidgetItem*> items = m_table->selectedItems();
    if(items.isEmpty()) {
        m_primaryActionCommand.clear();
        m_primaryActionButton->setVisible(true);
        m_primaryActionButton->setEnabled(false);
        m_primaryActionButton->setText("Find Peak For Selected Channel");
        emitSelectionContext();
        return;
    }

    QTableWidgetItem* nameItem = m_table->item(items.first()->row(), 0);
    if(!nameItem || nameItem->text().trimmed().isEmpty()) {
        m_primaryActionCommand.clear();
        m_primaryActionButton->setEnabled(false);
        emitSelectionContext();
        return;
    }

    const QString channelName = nameItem->text().trimmed();
    QJsonObject arguments = toolDefaults("neurokernel.find_peak_window");
    if(arguments.isEmpty()) {
        arguments.insert("window_samples", 4000);
    }
    arguments.insert("match", channelName);
    const QString argumentsText = QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact));
    m_primaryActionCommand = QString("tools.call neurokernel.find_peak_window %1").arg(argumentsText);
    m_primaryActionButton->setText(QString("Find Peak For %1").arg(channelName));
    m_primaryActionButton->setVisible(true);
    m_primaryActionButton->setEnabled(true);
    emitSelectionContext();
}

void AnalysisResultsWidget::runPrimaryAction()
{
    if(!m_primaryActionCommand.trimmed().isEmpty()) {
        emit toolCommandRequested(m_primaryActionCommand);
    }
}

void AnalysisResultsWidget::updatePipelineSelection()
{
    m_pipelineStepDetailsTree->clear();
    m_secondaryActionCommand.clear();
    m_secondaryActionButton->setVisible(true);
    m_secondaryActionButton->setEnabled(false);
    m_secondaryActionButton->setText("Run Selected Step");

    const QList<QTableWidgetItem*> items = m_pipelineStepsTable->selectedItems();
    if(items.isEmpty()) {
        emitSelectionContext();
        return;
    }

    const int row = items.first()->row();
    const QVariant payload = m_pipelineStepsTable->item(row, 0)->data(Qt::UserRole);
    const QJsonObject step = payload.toJsonObject();
    for(auto it = step.constBegin(); it != step.constEnd(); ++it) {
        m_pipelineStepDetailsTree->addTopLevelItem(buildJsonTreeItem(it.key(), it.value()));
    }
    m_pipelineStepDetailsTree->expandToDepth(1);
    for(int column = 0; column < m_pipelineStepDetailsTree->columnCount(); ++column) {
        m_pipelineStepDetailsTree->resizeColumnToContents(column);
    }

    const QString runId = step.value("run_id").toString().trimmed();
    const int stepNumber = step.value("step_number").toInt(-1);
    if(!runId.isEmpty() && stepNumber > 0) {
        m_secondaryActionCommand = QString("tools.call studio.pipeline.rerun_step {\"run_id\":\"%1\",\"step_number\":%2,\"mode\":\"rehydrate\"}")
                                       .arg(runId)
                                       .arg(stepNumber);
        m_secondaryActionButton->setText("Run Selected Step With Current Defaults");
        m_secondaryActionButton->setEnabled(true);
        emitSelectionContext();
        return;
    }

    const QTableWidgetItem* commandItem = m_pipelineStepsTable->item(row, 5);
    if(commandItem && !commandItem->text().trimmed().isEmpty()) {
        m_secondaryActionCommand = commandItem->text().trimmed();
        m_secondaryActionButton->setText("Replay Exact Step");
        m_secondaryActionButton->setEnabled(true);
    }
    emitSelectionContext();
}

void AnalysisResultsWidget::runSecondaryAction()
{
    if(!m_secondaryActionCommand.trimmed().isEmpty()) {
        emit toolCommandRequested(m_secondaryActionCommand);
    }
}

void AnalysisResultsWidget::emitSelectionContext()
{
    QJsonObject context{
        {"tool_name", m_toolName},
        {"result_status", m_result.value("status").toString()},
        {"has_primary_action", !m_primaryActionCommand.trimmed().isEmpty()},
        {"has_secondary_action", !m_secondaryActionCommand.trimmed().isEmpty()},
        {"primary_action_command", m_primaryActionCommand},
        {"secondary_action_command", m_secondaryActionCommand}
    };

    if(m_toolName == QLatin1String("neurokernel.channel_stats")
       || m_toolName == QLatin1String("neurokernel.raw_stats")) {
        const QList<QTableWidgetItem*> items = m_table->selectedItems();
        if(!items.isEmpty()) {
            const int row = items.first()->row();
            if(QTableWidgetItem* nameItem = m_table->item(row, 0)) {
                const QString channelName = nameItem->text().trimmed();
                if(!channelName.isEmpty()) {
                    context.insert("selection_kind", "channel");
                    context.insert("selected_channel_name", channelName);
                    context.insert("selected_channel_row", row);
                }
            }
        }
    } else if(m_toolName == QLatin1String("studio.pipeline.run")) {
        const QList<QTableWidgetItem*> items = m_pipelineStepsTable->selectedItems();
        if(!items.isEmpty()) {
            const int row = items.first()->row();
            const QVariant payload = m_pipelineStepsTable->item(row, 0)->data(Qt::UserRole);
            const QJsonObject step = payload.toJsonObject();
            if(!step.isEmpty()) {
                context.insert("selection_kind", "pipeline_step");
                context.insert("selected_pipeline_step", step);
                context.insert("selected_step_number", step.value("step_number").toInt(row + 1));
                context.insert("selected_pipeline_run_id", step.value("run_id").toString());
                context.insert("selected_pipeline_id", step.value("pipeline_id").toString(m_result.value("pipeline_id").toString()));
            }
        } else if(!m_result.isEmpty()) {
            context.insert("selection_kind", "pipeline_result");
            context.insert("selected_pipeline_run_id", m_result.value("run_id").toString());
            context.insert("selected_pipeline_id", m_result.value("pipeline_id").toString());
        }
    }

    emit selectionContextChanged(context);
}

void AnalysisResultsWidget::setTreeResult(const QJsonObject& result)
{
    m_stack->setCurrentWidget(m_tree);
    for(auto it = result.constBegin(); it != result.constEnd(); ++it) {
        m_tree->addTopLevelItem(buildJsonTreeItem(it.key(), it.value()));
    }
    m_tree->expandToDepth(1);
    for(int column = 0; column < m_tree->columnCount(); ++column) {
        m_tree->resizeColumnToContents(column);
    }
}

void AnalysisResultsWidget::setChannelTable(const QJsonArray& channels, bool allowPeakAction)
{
    m_table->setRowCount(channels.size());
    for(int row = 0; row < channels.size(); ++row) {
        const QJsonObject channel = channels.at(row).toObject();
        m_table->setItem(row, 0, new QTableWidgetItem(channel.value("name").toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(channel.value("rms").toDouble(), 'g', 8)));
        m_table->setItem(row, 2, new QTableWidgetItem(channel.contains("mean_abs")
                                                          ? QString::number(channel.value("mean_abs").toDouble(), 'g', 8)
                                                          : QString("-")));
        m_table->setItem(row, 3, new QTableWidgetItem(channel.contains("peak_abs")
                                                          ? QString::number(channel.value("peak_abs").toDouble(), 'g', 8)
                                                          : QString("-")));
    }
    m_stack->setCurrentWidget(m_table);

    if(allowPeakAction) {
        m_primaryActionButton->setText("Find Peak For Selected Channel");
        m_primaryActionButton->setVisible(true);
        updatePrimaryActionFromSelection();
    } else {
        emitSelectionContext();
    }
}

void AnalysisResultsWidget::setPipelineResult(const QJsonObject& result)
{
    const QString displayName = result.value("display_name").toString(result.value("pipeline_id").toString());
    const QString status = result.value("status").toString();
    const int completedSteps = result.value("completed_steps").toInt();
    const int totalSteps = result.value("total_steps").toInt();
    m_titleLabel->setText(QString("%1 | %2 | %3/%4 steps")
                              .arg(displayName,
                                   status,
                                   QString::number(completedSteps),
                                   QString::number(totalSteps)));

    const QString runId = result.value("run_id").toString().trimmed();
    const QString pipelineId = result.value("pipeline_id").toString().trimmed();
    const int pendingSteps = result.value("pending_steps").toInt();
    if(!runId.isEmpty() && pendingSteps > 0) {
        m_primaryActionCommand = QString("tools.call studio.pipeline.resume {\"run_id\":\"%1\"}").arg(runId);
        m_primaryActionButton->setText("Resume Remaining Steps");
        m_primaryActionButton->setVisible(true);
        m_primaryActionButton->setEnabled(true);
    } else if(!pipelineId.isEmpty()) {
        QJsonObject arguments = toolDefaults("studio.pipeline.run");
        arguments.insert("pipeline_id", pipelineId);
        const QString argumentsText = QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact));
        m_primaryActionCommand = QString("tools.call studio.pipeline.run %1").arg(argumentsText);
        m_primaryActionButton->setText("Rerun Pipeline");
        m_primaryActionButton->setVisible(true);
        m_primaryActionButton->setEnabled(true);
    }

    const QJsonArray steps = result.value("steps").toArray();
    m_pipelineStepsTable->setRowCount(steps.size());
    for(int row = 0; row < steps.size(); ++row) {
        QJsonObject step = steps.at(row).toObject();
        if(!runId.isEmpty() && !step.contains("run_id")) {
            step.insert("run_id", runId);
        }
        if(!pipelineId.isEmpty() && !step.contains("pipeline_id")) {
            step.insert("pipeline_id", pipelineId);
        }
        QTableWidgetItem* indexItem = new QTableWidgetItem(QString::number(step.value("step_number").toInt(row + 1)));
        indexItem->setData(Qt::UserRole, step);
        m_pipelineStepsTable->setItem(row, 0, indexItem);
        m_pipelineStepsTable->setItem(row, 1, new QTableWidgetItem(step.value("status").toString()));
        m_pipelineStepsTable->setItem(row, 2, new QTableWidgetItem(step.value("started_at").toString()));
        m_pipelineStepsTable->setItem(row, 3, new QTableWidgetItem(step.value("completed_at").toString(step.value("failed_at").toString())));
        m_pipelineStepsTable->setItem(row, 4, new QTableWidgetItem(step.value("tool_name").toString()));
        m_pipelineStepsTable->setItem(row, 5, new QTableWidgetItem(step.value("command").toString()));
    }

    m_stack->setCurrentWidget(m_pipelineView);
    if(m_pipelineStepsTable->rowCount() > 0) {
        m_pipelineStepsTable->selectRow(m_pipelineStepsTable->rowCount() - 1);
        updatePipelineSelection();
    } else {
        m_pipelineStepDetailsTree->addTopLevelItem(new QTreeWidgetItem(QStringList()
                                                                       << "hint"
                                                                       << "No pipeline steps recorded yet."));
        if(!m_secondaryActionButton->isVisible()) {
            m_secondaryActionButton->setVisible(true);
            m_secondaryActionButton->setEnabled(false);
        }
        emitSelectionContext();
    }
}
