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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acquired_points.h"
#include "polhemus_connection.h"

#include <utils/montage/standard_montage.h>

#include <QPointer>
#include <QMatrix4x4>
#include <QStackedWidget>
#include <QString>
#include <QVector3D>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;

//=============================================================================================================
// DEFINE NAMESPACE MNEALIGN
//=============================================================================================================

namespace MNEALIGN
{

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
 * @brief Five-step capture wizard for the MNE Align digitisation workflow.
 *
 * Steps: Setup → Fiducials → EegCap → HeadShape → Export.
 * Each digitisation step listens to the @ref PolhemusConnection for the
 * live cursor and appends to a shared @ref AcquiredPoints store.
 */
class AlignWizard : public QStackedWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * @brief Constructs the wizard.
     *
     * @param[in] acquired    Shared point store (not owned).
     * @param[in] digitizer   Polhemus connection (not owned, may be null).
     * @param[in] parent      Parent widget.
     */
    AlignWizard(AcquiredPoints* acquired,
                PolhemusConnection* digitizer,
                QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * @brief Destroys the wizard.
     */
    ~AlignWizard() override;

    /** @return The current wizard step. */
    AlignStep currentStep() const;

    /**
     * @brief Jump to the given step.
     *
     * @param[in] step   Target step.
     */
    void      goToStep(AlignStep step);

    /**
     * @brief Set the BEM file path displayed in the Setup page.
     *
     * @param[in] path   Absolute file path.
     */
    void      setBemPath(const QString& path);

    /**
     * @brief Set the active pen station number (1–4).
     *
     * @param[in] station   Polhemus station id.
     */
    void      setPenStation(int station);

    /**
     * @brief Push the current tracker→MRI transform for live coordinate display.
     *
     * @param[in] t   Combined device→MRI transform.
     */
    void      setTrackerTransform(const QMatrix4x4& t) { m_trackerToMri = t; }

    /** @return The current BEM file path. */
    QString                            bemPath() const { return m_bemPath; }

    /** @return The selected EEG cap montage system. */
    UTILSLIB::StandardMontage::System  cap()     const { return m_capSystem; }

    /** @return The number of wizard steps. */
    static int     stepCount();

    /** @return Human-readable title for the given step. */
    static QString titleFor(AlignStep step);

signals:
    /** @brief Emitted whenever the wizard advances or goes back a step. */
    void stepChanged(MNEALIGN::AlignStep step);

    /** @brief Emitted when the BEM file path changes in the Setup step. */
    void bemPathChanged(const QString& path);

    /** @brief Emitted when the EEG cap selection changes. */
    void capChanged(UTILSLIB::StandardMontage::System system);

    /** @brief Emitted when the user clicks Export in the Export step. */
    void requestExport(const QString& outPath);

    /** @brief Emitted when the user requests an ICP refinement fit. */
    void requestIcpFit();

public slots:
    /** @brief Advance to the next wizard step. */
    void next();

    /** @brief Go back to the previous wizard step. */
    void back();

    /**
     * @brief Handle a double-click on the BEM surface.
     *
     * Places twin fiducials in the Setup step.
     *
     * @param[in] worldPos   World-space intersection point.
     */
    void onSurfaceDoubleClicked(const QVector3D& worldPos);

private slots:
    void onLivePoint(int station, const QVector3D& pos, const QQuaternion& ori);
    void onPenButtonPressed(int station, const QVector3D& pos, const QQuaternion& ori);
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
    void refreshSetupTwinUi();

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
    int                                           m_penStation = 1;
    FiducialId                                    m_selectedFiducial = FiducialId::NAS;
    QMatrix4x4                                    m_trackerToMri;

    QPointer<QLabel>      m_pSetupBemLabel;
    QPointer<QComboBox>   m_pSetupCapCombo;
    QPointer<QLabel>      m_pSetupTwinFidLabel;
    QPointer<QPushButton> m_pTwinNasBtn;
    QPointer<QPushButton> m_pTwinLpaBtn;
    QPointer<QPushButton> m_pTwinRpaBtn;
    FiducialId            m_selectedTwinFid = FiducialId::NAS;

    QPointer<QLabel>      m_pFidStatusLabel;
    QPointer<QLabel>      m_pFidLiveLabel;
    QPointer<QPushButton> m_pFidCaptureBtn;
    QPointer<QPushButton> m_pFidUndoBtn;
    QPointer<QPushButton> m_pFidNasBtn;
    QPointer<QPushButton> m_pFidLpaBtn;
    QPointer<QPushButton> m_pFidRpaBtn;

    QPointer<QListWidget> m_pEegList;
    QPointer<QLabel>      m_pEegLiveLabel;
    QPointer<QPushButton> m_pEegCaptureBtn;
    QPointer<QPushButton> m_pEegUndoBtn;

    QPointer<QLabel>      m_pHspCountLabel;
    QPointer<QLabel>      m_pHspLiveLabel;
    QPointer<QPushButton> m_pHspCaptureBtn;
    QPointer<QPushButton> m_pHspUndoBtn;

    QPointer<QLabel>      m_pExportSummaryLabel;
    QPointer<QLabel>      m_pIcpStatusLabel;
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_WIZARD_H
