//=============================================================================================================
/**
 * @file     extensionhostedviewwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the hosted extension view container for center editor tabs.
 */

#include "extensionhostedviewwidget.h"

#include <QJsonArray>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

ExtensionHostedViewWidget::ExtensionHostedViewWidget(QWidget* parent)
: QWidget(parent)
, m_titleLabel(new QLabel(this))
, m_summaryLabel(new QLabel(this))
, m_statusLabel(new QLabel(this))
, m_stateLabel(new QLabel(this))
, m_opacityValueLabel(new QLabel(this))
, m_opacitySlider(new QSlider(Qt::Horizontal, this))
, m_actionsWidget(new QWidget(this))
, m_actionsLayout(new QVBoxLayout(m_actionsWidget))
, m_detailsView(new QTextEdit(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_titleLabel->setObjectName("terminalStatusLabel");
    m_summaryLabel->setWordWrap(true);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setObjectName("terminalStatusLabel");
    m_stateLabel->setWordWrap(true);
    m_stateLabel->setObjectName("terminalStatusLabel");

    QWidget* opacityRow = new QWidget(this);
    QHBoxLayout* opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(8);
    opacityLayout->addWidget(new QLabel("Opacity", opacityRow));
    opacityLayout->addWidget(m_opacitySlider, 1);
    opacityLayout->addWidget(m_opacityValueLabel);

    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setSingleStep(5);
    m_actionsLayout->setContentsMargins(0, 0, 0, 0);
    m_actionsLayout->setSpacing(8);

    m_detailsView->setReadOnly(true);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_stateLabel);
    layout->addWidget(opacityRow);
    layout->addWidget(m_actionsWidget);
    layout->addWidget(m_detailsView, 1);

    connect(m_opacitySlider, &QSlider::sliderReleased, this, [this]() {
        const QString currentSessionId = sessionId();
        if(currentSessionId.isEmpty()) {
            return;
        }

        const double opacity = static_cast<double>(m_opacitySlider->value()) / 100.0;
        emit viewCommandRequested(currentSessionId,
                                  "set_opacity",
                                  QJsonObject{{"opacity", opacity}});
    });

    rebuildUi();
}

void ExtensionHostedViewWidget::setSessionDescriptor(const QJsonObject& descriptor)
{
    m_descriptor = descriptor;
    rebuildUi();
}

QString ExtensionHostedViewWidget::sessionId() const
{
    return m_descriptor.value("session_id").toString();
}

QString ExtensionHostedViewWidget::filePath() const
{
    return m_descriptor.value("file").toString();
}

void ExtensionHostedViewWidget::applySessionUpdate(const QJsonObject& update)
{
    for(auto it = update.constBegin(); it != update.constEnd(); ++it) {
        m_descriptor.insert(it.key(), it.value());
    }

    rebuildUi();
}

void ExtensionHostedViewWidget::rebuildUi()
{
    const QString providerName = m_descriptor.value("provider_display_name").toString("Extension View");
    const QString extensionName = m_descriptor.value("extension_display_name").toString("Extension Host");
    const QString slotName = m_descriptor.value("slot").toString("center");
    const QString sceneId = m_descriptor.value("scene_id").toString();
    const QString fileName = m_descriptor.value("file").toString();
    const QString sessionName = sessionId();

    m_titleLabel->setText(QString("%1").arg(providerName));

    QString summary = QString("Hosted by %1 | Session: %2 | Slot: %3")
                          .arg(extensionName,
                               sessionName.isEmpty() ? QString("pending") : sessionName,
                               slotName);
    if(!fileName.isEmpty()) {
        summary += QString(" | File: %1").arg(fileName);
    }
    if(!sceneId.isEmpty()) {
        summary += QString(" | Scene: %1").arg(sceneId);
    }
    m_summaryLabel->setText(summary);

    const QString status = m_descriptor.value("message").toString("Extension view session is ready.");
    m_statusLabel->setText(status);

    const QJsonObject state = m_descriptor.value("state").toObject();
    QStringList stateParts;
    const QString hemisphere = state.value("hemisphere").toString().trimmed();
    if(!hemisphere.isEmpty()) {
        stateParts << QString("Hemisphere: %1").arg(hemisphere);
    }
    const QString camera = state.value("camera").toString().trimmed();
    if(!camera.isEmpty()) {
        stateParts << QString("Camera: %1").arg(camera);
    }
    if(state.contains("opacity")) {
        stateParts << QString("Opacity: %1").arg(QString::number(state.value("opacity").toDouble(), 'f', 2));
    }
    m_stateLabel->setText(stateParts.isEmpty()
                              ? QString("State: ready")
                              : QString("State | %1").arg(stateParts.join(" | ")));

    const double opacity = state.value("opacity").toDouble(m_descriptor.value("opacity").toDouble(0.8));
    const bool canControlOpacity = m_descriptor.value("controls").toObject().contains("opacity")
                                   || m_descriptor.value("capabilities").toObject().value("set_opacity").toBool(false);
    m_opacitySlider->parentWidget()->setVisible(canControlOpacity);
    if(canControlOpacity) {
        m_opacitySlider->setValue(static_cast<int>(opacity * 100.0));
        m_opacityValueLabel->setText(QString::number(opacity, 'f', 2));
    }

    rebuildActionButtons(m_descriptor.value("actions").toArray());
    m_detailsView->setPlainText(QString::fromUtf8(QJsonDocument(m_descriptor).toJson(QJsonDocument::Indented)));
}

void ExtensionHostedViewWidget::rebuildActionButtons(const QJsonArray& actions)
{
    while(QLayoutItem* item = m_actionsLayout->takeAt(0)) {
        if(QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if(actions.isEmpty()) {
        m_actionsWidget->setVisible(false);
        return;
    }

    m_actionsWidget->setVisible(true);
    QLabel* titleLabel = new QLabel("Session Actions", m_actionsWidget);
    titleLabel->setObjectName("terminalStatusLabel");
    m_actionsLayout->addWidget(titleLabel);

    QWidget* buttonRow = new QWidget(m_actionsWidget);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    const QString currentSessionId = sessionId();
    for(const QJsonValue& value : actions) {
        const QJsonObject action = value.toObject();
        const QString commandName = action.value("command").toString().trimmed();
        if(commandName.isEmpty()) {
            continue;
        }

        QPushButton* button = new QPushButton(action.value("label").toString(commandName), buttonRow);
        button->setToolTip(action.value("description").toString());
        const QJsonObject arguments = action.value("arguments").toObject();
        connect(button, &QPushButton::clicked, this, [this, currentSessionId, commandName, arguments]() {
            if(currentSessionId.isEmpty()) {
                return;
            }

            emit viewCommandRequested(currentSessionId, commandName, arguments);
        });
        buttonLayout->addWidget(button);
    }

    buttonLayout->addStretch(1);
    m_actionsLayout->addWidget(buttonRow);
}
