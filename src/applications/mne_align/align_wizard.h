//=============================================================================================================
/**
 * @file     align_wizard.h
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
 * @brief    Five-step capture wizard for the MNE Align app.
 *
 *           Steps: Setup → Fiducials → EegCap → HeadShape → Export.
 *           Each digitisation step listens to a single
 *           @ref PolhemusConnection signal for the live cursor and
 *           appends to a shared @ref AcquiredPoints store on demand.
 */

#ifndef MNE_ALIGN_WIZARD_H
#define MNE_ALIGN_WIZARD_H

#include "acquired_points.h"

#include <utils/montage/standard_montage.h>

#include <QPointer>
#include <QStackedWidget>
#include <QString>
#include <QVector3D>

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;

namespace MNEALIGN
{

class PolhemusConnection;

//=============================================================================================================
/** @brief Identifies the five wizard steps. */
enum class AlignStep {
    Setup = 0,    ///< Pick BEM file + EEG cap.
    Fiducials,    ///< Capture NAS / LPA / RPA in turn.
    EegCap,       ///< Walk the cap labels and capture each electrode.
    HeadShape,    ///< Free continuous capture of HSP points.
    Export        ///< Write a FIFF digitizer set to disk.
};

//=============================================================================================================
/**
 * @brief Capture wizard hosted by @ref MneAlign.
 */
class AlignWizard : public QStackedWidget
{
    Q_OBJECT

public:
    AlignWizard(AcquiredPoints* acquired,
                PolhemusConnection* digitizer,
                QWidget* parent = nullptr);
    ~AlignWizard() override;

    AlignStep currentStep() const;
    void      goToStep(AlignStep step);
    void      setBemPath(const QString& path);

    QString                            bemPath() const { return m_bemPath; }
    UTILSLIB::StandardMontage::System  cap()     const { return m_capSystem; }

    static int     stepCount();
    static QString titleFor(AlignStep step);

signals:
    void stepChanged(MNEALIGN::AlignStep step);
    void bemPathChanged(const QString& path);
    void capChanged(UTILSLIB::StandardMontage::System system);
    void requestExport(const QString& outPath);

public slots:
    void next();
    void back();

private slots:
    void onLivePoint(int station, const QVector3D& pos, const QQuaternion& ori);
    void onPickBemFile();
    void onCapComboChanged(int index);
    void onExportClicked();
    void onAcquiredChanged();

    void onCaptureFiducial();
    void onUndoFiducial();
    void onCaptureEeg();
    void onUndoEeg();
    void onCaptureHsp();
    void onUndoHsp();

private:
    QWidget* buildSetupPage();
    QWidget* buildFiducialsPage();
    QWidget* buildEegCapPage();
    QWidget* buildHeadShapePage();
    QWidget* buildExportPage();

    void refreshFiducialUi();
    void refreshEegUi();
    void refreshHeadShapeUi();
    void refreshExportUi();

    /** @return next un-captured fiducial in the {NAS,LPA,RPA} order. */
    FiducialId nextFiducial(bool* allDone) const;
    /** @return next un-captured EEG label, or empty string when complete. */
    QString    nextEegLabel(int* outIdent) const;

    AcquiredPoints*     m_pPoints    = nullptr;
    PolhemusConnection* m_pDigitizer = nullptr;

    QString                                       m_bemPath;
    UTILSLIB::StandardMontage::System             m_capSystem
        = UTILSLIB::StandardMontage::System::Standard_1020;

    QVector3D                                     m_lastLivePos;
    bool                                          m_haveLive = false;

    QPointer<QLabel>      m_pSetupBemLabel;
    QPointer<QComboBox>   m_pSetupCapCombo;

    QPointer<QLabel>      m_pFidStatusLabel;
    QPointer<QLabel>      m_pFidLiveLabel;
    QPointer<QPushButton> m_pFidCaptureBtn;
    QPointer<QPushButton> m_pFidUndoBtn;

    QPointer<QListWidget> m_pEegList;
    QPointer<QLabel>      m_pEegLiveLabel;
    QPointer<QPushButton> m_pEegCaptureBtn;
    QPointer<QPushButton> m_pEegUndoBtn;

    QPointer<QLabel>      m_pHspCountLabel;
    QPointer<QLabel>      m_pHspLiveLabel;
    QPointer<QPushButton> m_pHspCaptureBtn;
    QPointer<QPushButton> m_pHspUndoBtn;

    QPointer<QLabel>      m_pExportSummaryLabel;
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_WIZARD_H
