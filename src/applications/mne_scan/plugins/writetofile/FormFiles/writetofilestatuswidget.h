//=============================================================================================================
/**
 * @file     writetofilestatuswidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, MNE-CPP Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
