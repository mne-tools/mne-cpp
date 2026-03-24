//=============================================================================================================
/**
 * @file     pillselectorwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares a lightweight pill-shaped selector used in the workbench chat surfaces.
 */

#ifndef MNE_ANALYZE_STUDIO_PILLSELECTORWIDGET_H
#define MNE_ANALYZE_STUDIO_PILLSELECTORWIDGET_H

#include <QList>
#include <QPair>
#include <QWidget>

class QComboBox;

namespace MNEANALYZESTUDIO
{

class PillSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PillSelectorWidget(QWidget* parent = nullptr);

    void setPlaceholderText(const QString& placeholderText);
    void setEmptyText(const QString& emptyText);
    void setItems(const QList<QPair<QString, QString>>& items);
    void setCurrentValue(const QString& value);
    QString currentValue() const;
    QString currentText() const;
    bool hasItems() const;

signals:
    void currentValueChanged(const QString& value);

private slots:
    void emitCurrentValueChanged(int index);

private:
    QString displayTextForValue(const QString& value) const;

    QList<QPair<QString, QString>> m_items;
    QString m_currentValue;
    QString m_placeholderText;
    QString m_emptyText;
    QComboBox* m_comboBox;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_PILLSELECTORWIDGET_H
