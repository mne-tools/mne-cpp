//=============================================================================================================
/**
 * @file     dummy3dhostedviewwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the dummy 3D hosted view widget and registers its hosted view factory.
 */

#include "dummy3dhostedviewwidget.h"

#include <extensionviewfactoryregistry.h>
#include <iextensionviewfactory.h>

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

namespace
{

class Dummy3DExtensionViewFactory final : public IExtensionViewFactory
{
public:
    QString widgetType() const override
    {
        return "placeholder_3d";
    }

    QWidget* createView(const QJsonObject& sessionDescriptor, QWidget* parent) const override
    {
        Dummy3DHostedViewWidget* widget = new Dummy3DHostedViewWidget(parent);
        widget->setSessionDescriptor(sessionDescriptor);
        return widget;
    }
};

class Dummy3DExtensionViewFactoryRegistration
{
public:
    Dummy3DExtensionViewFactoryRegistration()
    {
        ExtensionViewFactoryRegistry::instance().registerFactory(&m_factory);
    }

private:
    Dummy3DExtensionViewFactory m_factory;
};

Dummy3DExtensionViewFactoryRegistration s_dummy3dFactoryRegistration;

}

Dummy3DHostedViewWidget::Dummy3DHostedViewWidget(QWidget* parent)
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
        if(sessionId().isEmpty()) {
            return;
        }

        const QJsonObject opacityControl = m_descriptor.value("controls").toObject().value("opacity").toObject();
        const QString commandName = opacityControl.value("command").toString("set_opacity").trimmed();
        const QString argumentName = opacityControl.value("target_argument").toString("opacity").trimmed();
        const double opacity = static_cast<double>(m_opacitySlider->value()) / 100.0;
        emit viewCommandRequested(sessionId(), commandName, QJsonObject{{argumentName, opacity}});
    });

    rebuildUi();
}

void Dummy3DHostedViewWidget::setSessionDescriptor(const QJsonObject& descriptor)
{
    m_descriptor = descriptor;
    rebuildUi();
}

QString Dummy3DHostedViewWidget::sessionId() const
{
    return m_descriptor.value("session_id").toString();
}

QString Dummy3DHostedViewWidget::filePath() const
{
    return m_descriptor.value("file").toString();
}

void Dummy3DHostedViewWidget::applySessionUpdate(const QJsonObject& update)
{
    for(auto it = update.constBegin(); it != update.constEnd(); ++it) {
        m_descriptor.insert(it.key(), it.value());
    }

    rebuildUi();
    emit statusMessage(m_descriptor.value("message").toString("Dummy 3D session updated."));
}

void Dummy3DHostedViewWidget::rebuildUi()
{
    const QString providerName = m_descriptor.value("provider_display_name").toString("Dummy 3D View");
    const QString extensionName = m_descriptor.value("extension_display_name").toString("Dummy 3D Extension");
    const QString sceneId = m_descriptor.value("scene_id").toString();
    const QString fileName = m_descriptor.value("file").toString();

    m_titleLabel->setText(providerName);

    QString summary = QString("Hosted by %1").arg(extensionName);
    if(!fileName.isEmpty()) {
        summary += QString(" | File: %1").arg(fileName);
    }
    if(!sceneId.isEmpty()) {
        summary += QString(" | Scene: %1").arg(sceneId);
    }
    m_summaryLabel->setText(summary);

    const QString message = m_descriptor.value("message").toString("Dummy 3D session ready.");
    m_statusLabel->setText(message);

    const QJsonObject state = m_descriptor.value("state").toObject();
    QStringList stateParts;
    stateParts << QString("Hemisphere: %1").arg(state.value("hemisphere").toString("both"));
    stateParts << QString("Camera: %1").arg(state.value("camera").toString("default"));
    stateParts << QString("Opacity: %1").arg(QString::number(state.value("opacity").toDouble(0.8), 'f', 2));
    m_stateLabel->setText(QString("State | %1").arg(stateParts.join(" | ")));

    const double opacity = state.value("opacity").toDouble(0.8);
    m_opacitySlider->setValue(static_cast<int>(opacity * 100.0));
    m_opacityValueLabel->setText(QString::number(opacity, 'f', 2));

    rebuildActionButtons(m_descriptor.value("actions").toArray());
    m_detailsView->setPlainText(QString::fromUtf8(QJsonDocument(m_descriptor).toJson(QJsonDocument::Indented)));
}

void Dummy3DHostedViewWidget::rebuildActionButtons(const QJsonArray& actions)
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
