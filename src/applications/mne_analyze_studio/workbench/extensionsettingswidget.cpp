//=============================================================================================================
/**
 * @file     extensionsettingswidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements a generic manifest-driven extension settings widget.
 */

#include "extensionsettingswidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

ExtensionSettingsWidget::ExtensionSettingsWidget(const ExtensionManifest& manifest,
                                                 const UiContribution::SettingsTabContribution& tab,
                                                 QWidget* parent)
: QWidget(parent)
, m_manifest(manifest)
, m_tab(tab)
, m_titleLabel(new QLabel(QString("%1 Settings").arg(manifest.displayName), this))
, m_subtitleLabel(new QLabel(QString("%1 | %2").arg(tab.title, tab.description), this))
, m_statusLabel(new QLabel("Settings are stored per studio workspace profile.", this))
, m_formLayout(new QFormLayout)
{
    m_titleLabel->setObjectName("terminalStatusLabel");
    m_subtitleLabel->setWordWrap(true);
    m_statusLabel->setObjectName("terminalStatusLabel");
    m_statusLabel->setWordWrap(true);

    QWidget* formWidget = new QWidget(this);
    formWidget->setLayout(m_formLayout);

    QPushButton* saveButton = new QPushButton("Save Settings", this);
    QPushButton* restoreButton = new QPushButton("Restore Defaults", this);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(restoreButton);
    buttonLayout->addStretch(1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_subtitleLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(formWidget);
    layout->addLayout(buttonLayout);
    layout->addStretch(1);

    buildForm();
    loadSettings();

    connect(saveButton, &QPushButton::clicked, this, &ExtensionSettingsWidget::saveSettings);
    connect(restoreButton, &QPushButton::clicked, this, &ExtensionSettingsWidget::restoreDefaults);
}

QString ExtensionSettingsWidget::settingsKeyPrefix() const
{
    return QString("extensions/settings/%1/%2").arg(m_manifest.id, m_tab.id);
}

QVariant ExtensionSettingsWidget::fieldDefaultValue(const QJsonObject& field) const
{
    return field.value("default").toVariant();
}

void ExtensionSettingsWidget::buildForm()
{
    for(const QJsonValue& value : m_tab.fields) {
        const QJsonObject field = value.toObject();
        const QString id = field.value("id").toString().trimmed();
        if(id.isEmpty()) {
            continue;
        }

        const QString type = field.value("type").toString("string").trimmed().toLower();
        const QString label = field.value("label").toString(id);
        const QString description = field.value("description").toString().trimmed();

        QWidget* editor = nullptr;
        if(type == "boolean" || type == "bool") {
            editor = new QCheckBox(this);
        } else if(type == "integer" || type == "int") {
            QSpinBox* spinBox = new QSpinBox(this);
            spinBox->setRange(field.value("minimum").toInt(-1000000),
                              field.value("maximum").toInt(1000000));
            editor = spinBox;
        } else if(type == "number" || type == "double") {
            QDoubleSpinBox* spinBox = new QDoubleSpinBox(this);
            spinBox->setDecimals(field.value("decimals").toInt(3));
            spinBox->setSingleStep(field.value("step").toDouble(0.1));
            spinBox->setRange(field.value("minimum").toDouble(-1e9),
                              field.value("maximum").toDouble(1e9));
            editor = spinBox;
        } else if(type == "enum") {
            QComboBox* comboBox = new QComboBox(this);
            for(const QJsonValue& option : field.value("options").toArray()) {
                comboBox->addItem(option.toString());
            }
            editor = comboBox;
        } else {
            editor = new QLineEdit(this);
        }

        editor->setToolTip(description);
        QLabel* fieldLabel = new QLabel(description.isEmpty() ? label : QString("%1\n%2").arg(label, description), this);
        fieldLabel->setWordWrap(true);
        m_formLayout->addRow(fieldLabel, editor);
        m_fieldWidgets.insert(id, editor);
    }
}

void ExtensionSettingsWidget::loadSettings()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");

    for(const QJsonValue& value : m_tab.fields) {
        const QJsonObject field = value.toObject();
        const QString id = field.value("id").toString().trimmed();
        QWidget* widget = m_fieldWidgets.value(id, nullptr);
        if(!widget) {
            continue;
        }

        const QVariant storedValue = settings.value(QString("%1/%2").arg(settingsKeyPrefix(), id),
                                                    fieldDefaultValue(field));
        if(QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
            checkBox->setChecked(storedValue.toBool());
        } else if(QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
            spinBox->setValue(storedValue.toInt());
        } else if(QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
            spinBox->setValue(storedValue.toDouble());
        } else if(QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
            const int index = comboBox->findText(storedValue.toString(), Qt::MatchFixedString);
            comboBox->setCurrentIndex(index >= 0 ? index : 0);
        } else if(QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            lineEdit->setText(storedValue.toString());
        }
    }
}

void ExtensionSettingsWidget::saveSettings()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");

    for(const QJsonValue& value : m_tab.fields) {
        const QJsonObject field = value.toObject();
        const QString id = field.value("id").toString().trimmed();
        QWidget* widget = m_fieldWidgets.value(id, nullptr);
        if(!widget) {
            continue;
        }

        QVariant storedValue;
        if(QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
            storedValue = checkBox->isChecked();
        } else if(QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
            storedValue = spinBox->value();
        } else if(QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
            storedValue = spinBox->value();
        } else if(QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
            storedValue = comboBox->currentText();
        } else if(QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            storedValue = lineEdit->text();
        }

        settings.setValue(QString("%1/%2").arg(settingsKeyPrefix(), id), storedValue);
    }

    const QString message = QString("Saved settings for %1 / %2").arg(m_manifest.displayName, m_tab.title);
    m_statusLabel->setText(message);
    emit outputMessage(message);
    emit statusMessage(message);
}

void ExtensionSettingsWidget::restoreDefaults()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    for(const QJsonValue& value : m_tab.fields) {
        const QString id = value.toObject().value("id").toString().trimmed();
        if(!id.isEmpty()) {
            settings.remove(QString("%1/%2").arg(settingsKeyPrefix(), id));
        }
    }

    loadSettings();

    const QString message = QString("Restored default settings for %1 / %2").arg(m_manifest.displayName, m_tab.title);
    m_statusLabel->setText(message);
    emit outputMessage(message);
    emit statusMessage(message);
}
