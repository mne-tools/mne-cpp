//=============================================================================================================
/**
 * @file     mne_align.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Top-level window for the MNE Align app — a stripped-down
 *           variant of `mne_inspect` focused on Polhemus FastTrak
 *           digitisation against a head BEM and an EEG cap.
 */

#ifndef MNE_ALIGN_MNE_ALIGN_H
#define MNE_ALIGN_MNE_ALIGN_H

#include <QMainWindow>
#include <QPointer>

class QSplitter;
class QLabel;

namespace MNELIB { class MNEBem; }

namespace MNEALIGN
{

class AlignWizard;
class Align3DView;
class AcquiredPoints;
class PolhemusConnection;

class MneAlign : public QMainWindow
{
    Q_OBJECT

public:
    explicit MneAlign(QWidget* parent = nullptr);
    ~MneAlign() override;

private slots:
    void onOpenBem();
    void onConnectDigitizer();
    void onDisconnectDigitizer();
    void onAbout();
    void onWizardExportRequest(const QString& outPath);
    void onWizardBemPathChanged(const QString& path);
    void onDigitizerConnectedChanged(bool connected);

private:
    void buildUi();
    void buildMenus();

    AcquiredPoints*       m_pPoints     = nullptr;
    PolhemusConnection*   m_pDigitizer  = nullptr;

    QPointer<QSplitter>   m_pSplitter;
    QPointer<AlignWizard> m_pWizard;
    QPointer<Align3DView> m_pView3d;
    QPointer<QLabel>      m_pStatusDigitizer;
    QPointer<QLabel>      m_pStatusBem;
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_MNE_ALIGN_H
