//=============================================================================================================
/**
 * @file     pillselectorwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements a lightweight pill-shaped selector used in the workbench chat surfaces.
 */

#include "pillselectorwidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QSignalBlocker>

using namespace MNEANALYZESTUDIO;

PillSelectorWidget::PillSelectorWidget(QWidget* parent)
: QWidget(parent)
, m_placeholderText("Select")
, m_emptyText("Unavailable")
, m_comboBox(new QComboBox(this))
{
    setObjectName("pillSelectorWidget");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_comboBox);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_comboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &PillSelectorWidget::emitCurrentValueChanged);
}

void PillSelectorWidget::setPlaceholderText(const QString& placeholderText)
{
    m_placeholderText = placeholderText.trimmed().isEmpty() ? QString("Select") : placeholderText.trimmed();
    if(m_comboBox->count() == 0) {
        m_comboBox->setToolTip(m_placeholderText);
    }
}

void PillSelectorWidget::setEmptyText(const QString& emptyText)
{
    m_emptyText = emptyText.trimmed().isEmpty() ? QString("Unavailable") : emptyText.trimmed();
    if(m_comboBox->count() == 0) {
        m_comboBox->setToolTip(m_emptyText);
    }
}

void PillSelectorWidget::setItems(const QList<QPair<QString, QString>>& items)
{
    const QSignalBlocker blocker(m_comboBox);
    m_items = items;
    m_comboBox->clear();
    for(const auto& item : m_items) {
        m_comboBox->addItem(item.first, item.second);
    }

    const QString displayText = m_items.isEmpty() ? m_emptyText : m_placeholderText;
    m_comboBox->setEnabled(!m_items.isEmpty());
    m_comboBox->setToolTip(displayText);
    setCurrentValue(m_currentValue);
}

void PillSelectorWidget::setCurrentValue(const QString& value)
{
    const QString trimmedValue = value.trimmed();
    m_currentValue = trimmedValue;

    const QSignalBlocker blocker(m_comboBox);
    const int index = m_comboBox->findData(trimmedValue);
    if(index >= 0) {
        m_comboBox->setCurrentIndex(index);
        return;
    }

    if(m_comboBox->count() > 0) {
        m_comboBox->setCurrentIndex(0);
        m_currentValue = m_comboBox->currentData().toString();
    }
}

QString PillSelectorWidget::currentValue() const
{
    return m_currentValue;
}

QString PillSelectorWidget::currentText() const
{
    return displayTextForValue(m_currentValue);
}

bool PillSelectorWidget::hasItems() const
{
    return !m_items.isEmpty();
}

void PillSelectorWidget::emitCurrentValueChanged(int index)
{
    if(index < 0) {
        return;
    }

    m_currentValue = m_comboBox->itemData(index).toString();
    emit currentValueChanged(m_currentValue);
}

QString PillSelectorWidget::displayTextForValue(const QString& value) const
{
    for(const auto& item : m_items) {
        if(item.second == value) {
            return item.first;
        }
    }

    return QString();
}
