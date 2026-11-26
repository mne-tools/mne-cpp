//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settingspanel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SettingsPanel definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** Slider steps per unit of a Double field shown as a slider. */
double sliderFactor(const SettingField& field)
{
    return field.kind == SettingField::Kind::Double ? std::pow(10.0, field.decimals) : 1.0;
}

QString toQString(const std::string& text)
{
    return QString::fromStdString(text);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsPanel::SettingsPanel(std::shared_ptr<SettingsForm> form, QWidget* parent)
: QWidget(parent)
, m_pForm(std::move(form))
, m_form(*m_pForm)
{
    setWindowTitle(toQString(m_form.title()));
    auto* layout = new QVBoxLayout(this);
    for (const std::string& group : m_form.groups()) { // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks): Qt parents own the layouts
        const auto shown = [&group](const SettingField& field) {
            return field.group == group && !field.hidden;
        };
        if (std::none_of(m_form.fields().begin(), m_form.fields().end(), shown)) {
            continue;
        }
        QFormLayout* formLayout = nullptr;
        if (group.empty()) {
            auto* holder = new QWidget(this);
            layout->addWidget(holder);
            formLayout = new QFormLayout(holder);
            formLayout->setContentsMargins(0, 0, 0, 0);
        } else {
            auto* box = new QGroupBox(toQString(group), this);
            formLayout = new QFormLayout(box);
            layout->addWidget(box);
            m_groups[group] = box;
        }
        for (const SettingField& field : m_form.fields()) {
            if (shown(field)) {
                buildField(field, formLayout);
            }
        }
    }
    layout->addStretch(1);

    m_listener = m_form.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (value) {
            showValue(key);
            updateConditions();
        }
    });
    m_structureListener = m_form.add_structure_listener([this](const std::string& key) {
        updateField(key);
    });
    updateConditions();
}

//=============================================================================================================

SettingsPanel::~SettingsPanel()
{
    m_form.remove_listener(m_listener);
    m_form.remove_listener(m_structureListener);
}

//=============================================================================================================

void SettingsPanel::buildField(const SettingField& field, QFormLayout* layout)
{
    Row row;
    const std::string key = field.key;
    switch (field.kind) {
        case SettingField::Kind::Bool: {
            auto* box = new QCheckBox(toQString(field.label), this);
            connect(box, &QCheckBox::toggled, this, [this, key](bool on) {
                m_form.set_value(key, on);
            });
            layout->addRow(box);
            row.editor = box;
            break;
        }
        case SettingField::Kind::Int:
        case SettingField::Kind::Double:
            if (field.style == SettingField::Style::Slider) {
                auto* slider = new QSlider(Qt::Horizontal, this);
                connect(slider, &QSlider::valueChanged, this, [this, key](int position) {
                    if (const SettingField* f = m_form.field(key)) {
                        m_form.set_value(key, f->kind == SettingField::Kind::Int ? SettingValue(position) : SettingValue(position / sliderFactor(*f)));
                    }
                });
                row.editor = slider;
            } else if (field.kind == SettingField::Kind::Int) {
                auto* spin = new QSpinBox(this);
                if (field.commit == SettingField::Commit::WhenFinished) {
                    connect(spin, &QSpinBox::editingFinished, this, [this, key, spin] {
                        m_form.set_value(key, spin->value());
                    });
                } else {
                    connect(spin, &QSpinBox::valueChanged, this, [this, key](int number) {
                        m_form.set_value(key, number);
                    });
                }
                row.editor = spin;
            } else {
                auto* spin = new QDoubleSpinBox(this);
                if (field.commit == SettingField::Commit::WhenFinished) {
                    connect(spin, &QDoubleSpinBox::editingFinished, this, [this, key, spin] {
                        m_form.set_value(key, spin->value());
                    });
                } else {
                    connect(spin, &QDoubleSpinBox::valueChanged, this, [this, key](double number) {
                        m_form.set_value(key, number);
                    });
                }
                row.editor = spin;
            }
            break;
        case SettingField::Kind::Choice: {
            auto* combo = new QComboBox(this);
            connect(combo, &QComboBox::currentIndexChanged, this, [this, key, combo](int index) {
                if (index >= 0) {
                    m_form.set_value(key, combo->itemData(index).toString().toStdString());
                }
            });
            row.editor = combo;
            break;
        }
        case SettingField::Kind::Text:
        case SettingField::Kind::Path: {
            auto* edit = new QLineEdit(this);
            if (field.commit == SettingField::Commit::WhenFinished) {
                connect(edit, &QLineEdit::editingFinished, this, [this, key, edit] {
                    m_form.set_value(key, edit->text().toStdString());
                });
            } else {
                connect(edit, &QLineEdit::textEdited, this, [this, key](const QString& text) {
                    m_form.set_value(key, text.toStdString());
                });
            }
            if (field.kind == SettingField::Kind::Text) {
                row.editor = edit;
                break;
            }
            auto* container = new QWidget(this);
            auto* line = new QHBoxLayout(container);
            line->setContentsMargins(0, 0, 0, 0);
            edit->setParent(container);
            line->addWidget(edit);
            auto* browse = new QToolButton(container);
            browse->setText(QStringLiteral("..."));
            line->addWidget(browse);
            connect(browse, &QToolButton::clicked, this, [this, key, edit] {
                const SettingField* f = m_form.field(key);
                const QString file = QFileDialog::getOpenFileName(this, f ? toQString(f->label) : QString(), edit->text(), f ? toQString(f->file_filter) : QString());
                if (!file.isEmpty()) {
                    m_form.set_value(key, file.toStdString());
                }
            });
            row.editor = container;
            break;
        }
        case SettingField::Kind::Action: {
            auto* button = new QPushButton(toQString(field.label), this);
            connect(button, &QPushButton::clicked, this, [this, key] {
                m_form.trigger(key);
            });
            layout->addRow(button);
            row.editor = button;
            break;
        }
        case SettingField::Kind::Custom: {
            auto* container = new QWidget(this);
            auto* inner = new QVBoxLayout(container);
            inner->setContentsMargins(0, 0, 0, 0);
            if (field.label.empty()) {
                layout->addRow(container);
            } else {
                row.label = new QLabel(toQString(field.label), this);
                layout->addRow(row.label, container);
            }
            row.editor = container;
            break;
        }
    }
    if (!row.label && field.kind != SettingField::Kind::Bool && field.kind != SettingField::Kind::Action && field.kind != SettingField::Kind::Custom) {
        row.label = new QLabel(toQString(field.label), this);
        layout->addRow(row.label, row.editor);
    }
    row.editor->setObjectName(toQString(key));
    row.editor->setToolTip(toQString(field.tooltip));
    m_rows[key] = row;
    updateField(key);
}

//=============================================================================================================

void SettingsPanel::updateField(const std::string& key)
{
    const auto it = m_rows.find(key);
    const SettingField* field = m_form.field(key);
    if (it == m_rows.end() || !field) {
        return;
    }
    QWidget* editor = it->second.editor;
    if (it->second.label) {
        it->second.label->setText(toQString(field->label));
    }
    const QSignalBlocker blocker(editor);
    if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
        spin->setRange(static_cast<int>(std::lround(field->minimum)), static_cast<int>(std::lround(field->maximum)));
        spin->setSingleStep(std::max(1, static_cast<int>(std::lround(field->step))));
        spin->setPrefix(toQString(field->prefix));
        spin->setSuffix(toQString(field->unit));
    } else if (auto* doubleSpin = qobject_cast<QDoubleSpinBox*>(editor)) {
        doubleSpin->setDecimals(field->decimals);
        doubleSpin->setRange(field->minimum, field->maximum);
        doubleSpin->setSingleStep(field->step);
        doubleSpin->setPrefix(toQString(field->prefix));
        doubleSpin->setSuffix(toQString(field->unit));
    } else if (auto* slider = qobject_cast<QSlider*>(editor)) {
        const double factor = sliderFactor(*field);
        slider->setRange(static_cast<int>(std::lround(field->minimum * factor)), static_cast<int>(std::lround(field->maximum * factor)));
        slider->setSingleStep(std::max(1, static_cast<int>(std::lround(field->step * factor))));
    } else if (auto* combo = qobject_cast<QComboBox*>(editor)) {
        combo->clear();
        for (const SettingChoice& choice : field->choices) {
            combo->addItem(toQString(choice.label), toQString(choice.value));
        }
    } else if (auto* button = qobject_cast<QAbstractButton*>(editor)) {
        button->setText(toQString(field->label));
    }
    showValue(key);
}

//=============================================================================================================

void SettingsPanel::showValue(const std::string& key)
{
    const auto it = m_rows.find(key);
    const SettingField* field = m_form.field(key);
    const std::optional<SettingValue> value = m_form.value(key);
    if (it == m_rows.end() || !field || !value) {
        return;
    }
    QWidget* editor = it->second.editor;
    const QSignalBlocker blocker(editor);
    if (auto* box = qobject_cast<QCheckBox*>(editor)) {
        box->setChecked(m_form.get<bool>(key));
    } else if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
        spin->setValue(m_form.get<int>(key));
    } else if (auto* doubleSpin = qobject_cast<QDoubleSpinBox*>(editor)) {
        doubleSpin->setValue(m_form.get<double>(key));
    } else if (auto* slider = qobject_cast<QSlider*>(editor)) {
        slider->setValue(static_cast<int>(std::lround(m_form.get<double>(key) * sliderFactor(*field))));
    } else if (auto* combo = qobject_cast<QComboBox*>(editor)) {
        combo->setCurrentIndex(combo->findData(toQString(m_form.get<std::string>(key))));
    } else if (auto* edit = qobject_cast<QLineEdit*>(editor)) {
        edit->setText(toQString(m_form.get<std::string>(key)));
    } else if (auto* pathEdit = editor->findChild<QLineEdit*>()) {
        const QSignalBlocker pathBlocker(pathEdit);
        pathEdit->setText(toQString(m_form.get<std::string>(key)));
    }
}

//=============================================================================================================

void SettingsPanel::updateConditions()
{
    for (const auto& [key, row] : m_rows) {
        const bool enabled = m_form.is_enabled(key);
        const bool visible = m_form.is_visible(key);
        row.editor->setEnabled(enabled);
        row.editor->setVisible(visible);
        if (row.label) {
            row.label->setEnabled(enabled);
            row.label->setVisible(visible);
        }
    }
    // A group box shows while one of its fields does
    for (const auto& [group, box] : m_groups) {
        const bool any = std::any_of(m_form.fields().begin(), m_form.fields().end(), [this, &group](const SettingField& field) {
            return field.group == group && m_rows.contains(field.key) && m_form.is_visible(field.key);
        });
        box->setVisible(any);
    }
}

//=============================================================================================================

QWidget* SettingsPanel::editor(const std::string& key) const
{
    const auto it = m_rows.find(key);
    return it == m_rows.end() ? nullptr : it->second.editor;
}

//=============================================================================================================

QWidget* SettingsPanel::customWidget(const std::string& key) const
{
    const SettingField* field = m_form.field(key);
    return field && field->kind == SettingField::Kind::Custom ? editor(key) : nullptr;
}

//=============================================================================================================

void SettingsPanel::loadSettings(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    QSettings settings(QStringLiteral("MNECPP"));
    for (const SettingField& field : m_form.fields()) {
        const QString name = path + QLatin1Char('/') + toQString(field.key);
        if (!field.persistent || !settings.contains(name)) {
            continue;
        }
        const QVariant stored = settings.value(name);
        switch (field.kind) {
            case SettingField::Kind::Bool:
                m_form.set_value(field.key, stored.toBool());
                break;
            case SettingField::Kind::Int:
                m_form.set_value(field.key, stored.toInt());
                break;
            case SettingField::Kind::Double:
                m_form.set_value(field.key, stored.toDouble());
                break;
            case SettingField::Kind::Choice:
            case SettingField::Kind::Text:
            case SettingField::Kind::Path:
                m_form.set_value(field.key, stored.toString().toStdString());
                break;
            case SettingField::Kind::Action:
            case SettingField::Kind::Custom:
                break;
        }
    }
}

//=============================================================================================================

void SettingsPanel::saveSettings(const QString& path) const
{
    if (path.isEmpty()) {
        return;
    }
    QSettings settings(QStringLiteral("MNECPP"));
    for (const auto& [key, value] : m_form.values()) {
        if (const SettingField* field = m_form.field(key); !field || !field->persistent) {
            continue;
        }
        const QString name = path + QLatin1Char('/') + toQString(key);
        std::visit(
            [&settings, &name](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    settings.setValue(name, QString::fromStdString(v));
                } else {
                    settings.setValue(name, v);
                }
            },
            value);
    }
}
