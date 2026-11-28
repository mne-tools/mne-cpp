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
#include <QColor>
#include <QColorDialog>
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
    layout->setContentsMargins(0, 0, 0, 0);
    build();

    m_listener = m_form.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (value) {
            showValue(key);
            updateConditions();
        }
    });
    m_structureListener = m_form.add_structure_listener([this](const std::string& key) {
        // A changed field is updated in place; added and removed fields need the rows built again
        const SettingField* field = m_form.field(key);
        const bool shown = field && !field->hidden;
        if (shown != m_rows.contains(key)) {
            rebuild();
        } else {
            updateField(key);
        }
    });
}

//=============================================================================================================

void SettingsPanel::build()
{
    m_content = new QWidget(this);
    layout()->addWidget(m_content);
    auto* layout = new QVBoxLayout(m_content);
    layout->setSpacing(16);
    for (const std::string& group : m_form.groups()) { // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks): Qt parents own the layouts
        const auto shown = [&group](const SettingField& field) {
            return field.group == group && !field.hidden;
        };
        if (std::none_of(m_form.fields().begin(), m_form.fields().end(), shown)) {
            continue;
        }
        QFormLayout* formLayout = nullptr;
        if (group.empty()) {
            auto* holder = new QWidget(m_content);
            layout->addWidget(holder);
            formLayout = new QFormLayout(holder);
            formLayout->setContentsMargins(0, 0, 0, 0);
        } else {
            auto* box = new QGroupBox(toQString(group), m_content);
            formLayout = new QFormLayout(box);
            formLayout->setContentsMargins(16, 12, 16, 12);
            layout->addWidget(box);
            m_groups[group] = box;
        }
        // Labels on the left, editors aligned on the right, as in viz::FormView
        formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
        formLayout->setVerticalSpacing(10);
        for (const SettingField& field : m_form.fields()) {
            if (shown(field)) {
                buildField(field, formLayout);
            }
        }
    }
    layout->addStretch(1);
    updateConditions();
}

//=============================================================================================================

void SettingsPanel::rebuild()
{
    // The old rows may be emitting the signal that changed the form, so they are deleted later
    m_rows.clear();
    m_groups.clear();
    if (m_content) {
        // Detached, so lookups by object name only find the new rows
        m_content->hide();
        layout()->removeWidget(m_content);
        m_content->setParent(nullptr);
        m_content->deleteLater();
    }
    build();
    emit rebuilt();
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
            auto* box = new QCheckBox(toQString(field.label), m_content);
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
                auto* slider = new QSlider(Qt::Horizontal, m_content);
                connect(slider, &QSlider::valueChanged, this, [this, key](int position) {
                    if (const SettingField* f = m_form.field(key)) {
                        m_form.set_value(key, f->kind == SettingField::Kind::Int ? SettingValue(position) : SettingValue(position / sliderFactor(*f)));
                    }
                });
                row.editor = slider;
            } else if (field.kind == SettingField::Kind::Int) {
                auto* spin = new QSpinBox(m_content);
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
                auto* spin = new QDoubleSpinBox(m_content);
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
            auto* combo = new QComboBox(m_content);
            connect(combo, &QComboBox::currentIndexChanged, this, [this, key, combo](int index) {
                if (index >= 0) {
                    m_form.set_value(key, combo->itemData(index).toString().toStdString());
                }
            });
            row.editor = combo;
            break;
        }
        case SettingField::Kind::Text: {
            auto* edit = new QLineEdit(m_content);
            if (field.commit == SettingField::Commit::WhenFinished) {
                connect(edit, &QLineEdit::editingFinished, this, [this, key, edit] {
                    m_form.set_value(key, edit->text().toStdString());
                });
            } else {
                connect(edit, &QLineEdit::textEdited, this, [this, key](const QString& text) {
                    m_form.set_value(key, text.toStdString());
                });
            }
            row.editor = edit;
            break;
        }
        case SettingField::Kind::Path:
        case SettingField::Kind::Directory:
            row.editor = buildChooser(field);
            break;
        case SettingField::Kind::Color: {
            auto* button = new QToolButton(m_content);
            button->setProperty("settingColor", true);
            button->setMinimumWidth(48);
            connect(button, &QToolButton::clicked, this, [this, key] {
                const QColor current(toQString(m_form.get<std::string>(key)));
                const SettingField* f = m_form.field(key);
                const QColor chosen = QColorDialog::getColor(current, this, f ? toQString(f->label) : QString(), QColorDialog::ShowAlphaChannel);
                if (chosen.isValid()) {
                    const QColor::NameFormat format = chosen.alpha() == 255 ? QColor::HexRgb : QColor::HexArgb;
                    QString name = chosen.name(format);
                    if (format == QColor::HexArgb) {
                        // Qt writes #aarrggbb, the form #rrggbbaa
                        name = QLatin1Char('#') + name.mid(3) + name.mid(1, 2);
                    }
                    m_form.set_value(key, name.toStdString());
                }
            });
            row.editor = button;
            break;
        }
        case SettingField::Kind::Info: {
            auto* text = new QLabel(m_content);
            text->setWordWrap(true);
            text->setTextInteractionFlags(Qt::TextSelectableByMouse);
            if (field.label.empty()) {
                layout->addRow(text);
            }
            row.editor = text;
            break;
        }
        case SettingField::Kind::Action: {
            auto* button = new QPushButton(toQString(field.label), m_content);
            connect(button, &QPushButton::clicked, this, [this, key] {
                m_form.trigger(key);
            });
            // As wide as its text, on the right
            auto* line = new QWidget(m_content);
            auto* lineLayout = new QHBoxLayout(line);
            lineLayout->setContentsMargins(0, 0, 0, 0);
            lineLayout->addStretch(1);
            button->setParent(line);
            lineLayout->addWidget(button);
            layout->addRow(line);
            row.editor = button;
            row.container = line;
            break;
        }
        case SettingField::Kind::Custom: {
            auto* container = new QWidget(m_content);
            auto* inner = new QVBoxLayout(container);
            inner->setContentsMargins(0, 0, 0, 0);
            if (field.label.empty()) {
                layout->addRow(container);
            } else {
                row.label = new QLabel(toQString(field.label), m_content);
                layout->addRow(row.label, container);
            }
            row.editor = container;
            break;
        }
    }
    const bool ownRow = field.kind == SettingField::Kind::Bool || field.kind == SettingField::Kind::Action || field.kind == SettingField::Kind::Custom ||
        (field.kind == SettingField::Kind::Info && field.label.empty());
    if (!row.label && !ownRow) {
        row.label = new QLabel(toQString(field.label), m_content);
        // Editors keep the theme's width and sit on the right; read-only texts align right
        auto* line = new QWidget(m_content);
        auto* lineLayout = new QHBoxLayout(line);
        lineLayout->setContentsMargins(0, 0, 0, 0);
        lineLayout->addStretch(1);
        row.editor->setParent(line);
        if (auto* text = qobject_cast<QLabel*>(row.editor)) {
            text->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            lineLayout->addWidget(text);
        } else {
            row.editor->setFixedWidth(static_cast<int>(m_theme.editor_width));
            lineLayout->addWidget(row.editor);
        }
        layout->addRow(row.label, line);
        row.container = line;
    }
    row.editor->setObjectName(toQString(key));
    row.editor->setToolTip(toQString(field.tooltip));
    m_rows[key] = row;
    updateField(key);
}

//=============================================================================================================

QWidget* SettingsPanel::buildChooser(const SettingField& field)
{
    const std::string key = field.key;
    auto* container = new QWidget(m_content);
    auto* line = new QHBoxLayout(container);
    line->setContentsMargins(0, 0, 0, 0);
    auto* edit = new QLineEdit(container);
    line->addWidget(edit);
    if (field.commit == SettingField::Commit::WhenFinished) {
        connect(edit, &QLineEdit::editingFinished, this, [this, key, edit] {
            m_form.set_value(key, edit->text().toStdString());
        });
    } else {
        connect(edit, &QLineEdit::textEdited, this, [this, key](const QString& text) {
            m_form.set_value(key, text.toStdString());
        });
    }
    auto* browse = new QToolButton(container);
    browse->setText(QStringLiteral("..."));
    line->addWidget(browse);
    connect(browse, &QToolButton::clicked, this, [this, key, edit] {
        const SettingField* f = m_form.field(key);
        if (!f) {
            return;
        }
        QString chosen;
        if (f->kind == SettingField::Kind::Directory) {
            chosen = QFileDialog::getExistingDirectory(this, toQString(f->label), edit->text());
        } else if (f->save_path) {
            chosen = QFileDialog::getSaveFileName(this, toQString(f->label), edit->text(), toQString(f->file_filter));
        } else {
            chosen = QFileDialog::getOpenFileName(this, toQString(f->label), edit->text(), toQString(f->file_filter));
        }
        if (!chosen.isEmpty()) {
            m_form.set_value(key, chosen.toStdString());
        }
    });
    return container;
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
    } else if (auto* text = qobject_cast<QLabel*>(editor)) {
        // Info fields with a tone are status chips in the tone's colour, as in viz::FormView
        std::optional<Color> color;
        switch (field->tone) {
            case SettingField::Tone::Positive:
                color = m_theme.positive;
                break;
            case SettingField::Tone::Warning:
                color = m_theme.warning;
                break;
            case SettingField::Tone::Negative:
                color = m_theme.negative;
                break;
            case SettingField::Tone::Normal:
                break;
        }
        text->setStyleSheet(!color ? QString()
                                   : QStringLiteral("color: rgb(%1, %2, %3); background: rgba(%1, %2, %3, 26); border-radius: %4px; padding: 2px 8px; "
                                                    "font-weight: 600; font-family: \"%5\";")
                                         .arg(color->r)
                                         .arg(color->g)
                                         .arg(color->b)
                                         .arg(static_cast<int>(m_theme.small_font_size))
                                         .arg(QString::fromStdString(m_theme.font_family)));
    } else if (auto* button = qobject_cast<QAbstractButton*>(editor); button && !button->property("settingColor").toBool()) {
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
    } else if (auto* text = qobject_cast<QLabel*>(editor)) {
        text->setText(toQString(m_form.get<std::string>(key)));
    } else if (auto* colorButton = qobject_cast<QToolButton*>(editor); colorButton && colorButton->property("settingColor").toBool()) {
        const std::string color = m_form.get<std::string>(key);
        colorButton->setStyleSheet(QStringLiteral("background-color: %1;").arg(toQString(color.substr(0, 7))));
        colorButton->setToolTip(toQString(color));
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
        if (row.container) {
            row.container->setVisible(visible);
        }
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

void SettingsPanel::setTheme(const viz::Theme& theme)
{
    m_theme = theme;
    for (const auto& [key, row] : m_rows) {
        updateField(key);
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
            case SettingField::Kind::Directory:
            case SettingField::Kind::Color:
                m_form.set_value(field.key, stored.toString().toStdString());
                break;
            case SettingField::Kind::Action:
            case SettingField::Kind::Custom:
            case SettingField::Kind::Info:
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
