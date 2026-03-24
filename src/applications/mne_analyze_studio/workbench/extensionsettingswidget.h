//=============================================================================================================
/**
 * @file     extensionsettingswidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares a generic manifest-driven extension settings widget.
 */

#ifndef MNE_ANALYZE_STUDIO_EXTENSIONSETTINGSWIDGET_H
#define MNE_ANALYZE_STUDIO_EXTENSIONSETTINGSWIDGET_H

#include <extensionmanifest.h>

#include <QHash>
#include <QWidget>

class QFormLayout;
class QLabel;

namespace MNEANALYZESTUDIO
{

class ExtensionSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExtensionSettingsWidget(const ExtensionManifest& manifest,
                                     const UiContribution::SettingsTabContribution& tab,
                                     QWidget* parent = nullptr);

signals:
    void outputMessage(const QString& message);
    void statusMessage(const QString& message);

private slots:
    void saveSettings();
    void restoreDefaults();

private:
    QString settingsKeyPrefix() const;
    QVariant fieldDefaultValue(const QJsonObject& field) const;
    void buildForm();
    void loadSettings();

    ExtensionManifest m_manifest;
    UiContribution::SettingsTabContribution m_tab;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_statusLabel;
    QFormLayout* m_formLayout;
    QHash<QString, QWidget*> m_fieldWidgets;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_EXTENSIONSETTINGSWIDGET_H
