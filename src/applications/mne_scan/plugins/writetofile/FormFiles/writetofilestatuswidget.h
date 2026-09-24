//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     writetofilestatuswidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Declaration of WriteToFileStatusWidget.
 */

#ifndef WRITETOFILESTATUSWIDGET_H
#define WRITETOFILESTATUSWIDGET_H

#include "../writetofile_global.h"

#include <QWidget>
#include <QPointer>

class QLabel;

namespace WRITETOFILEPLUGIN
{

class WriteToFile;

//=============================================================================================================
/**
 * Compact toolbar widget that subscribes to a WriteToFile plugin's recordingStatus
 * signal and displays a red recording dot plus the elapsed-time / file-size summary.
 */
class WRITETOFILESHARED_EXPORT WriteToFileStatusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WriteToFileStatusWidget(WriteToFile* pPlugin, QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Current displayed summary string. Useful for tests.
     */
    QString currentText() const;

    //=========================================================================================================
    /**
     * Whether the recording indicator is currently active (red dot visible).
     */
    bool isActive() const;

private:
    void onRecordingStatus(const QString& sSummary);
    void onRecordingActiveChanged(bool bActive);

    QPointer<QLabel> m_pDot;
    QPointer<QLabel> m_pText;
    bool             m_bActive;
};

} // namespace

#endif // WRITETOFILESTATUSWIDGET_H
