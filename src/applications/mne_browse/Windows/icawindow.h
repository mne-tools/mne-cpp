//=============================================================================================================
/**
 * @file     icawindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     June, 2026
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
 *
 * @brief    Declaration of the IcaWindow class.
 *
 */

#ifndef ICAWINDOW_H
#define ICAWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/ica.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QVBoxLayout;

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * @brief IcaWindow – dock widget for computing and browsing ICA components.
 *
 * Provides a "Compute" button that runs FastICA on the loaded raw data,
 * displays each component's time-course as a mini waveform image,
 * lets the user mark components for exclusion, and applies the exclusion
 * back to the raw data.
 */
class IcaWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit IcaWindow(QWidget *parent = nullptr);

    void init();

signals:
    /**
     * Emitted when the user applies component exclusion.
     * @param[in] cleanedData  Channels × samples matrix after removing excluded components.
     */
    void icaCleaned(const Eigen::MatrixXd &cleanedData);
    void icaReset(const Eigen::MatrixXd &originalData);

public slots:
    /**
     * Provide the raw data and metadata so ICA can be computed.
     *
     * @param[in] rawData    Channels × samples matrix (double).
     * @param[in] fiffInfo   Shared pointer to FiffInfo for channel names.
     * @param[in] firstSample Absolute sample index of column 0.
     */
    void setRawData(const Eigen::MatrixXd &rawData,
                    QSharedPointer<FIFFLIB::FiffInfo> fiffInfo,
                    int firstSample = 0);

    void clearIca();

private slots:
    void onCompute();
    void onApply();
    void onReset();

private:
    void setupUi();
    void rebuildComponentList();
    QImage renderComponentWaveform(int compIdx, int width, int height) const;

    // UI elements
    QWidget      *m_pCentralWidget = nullptr;
    QVBoxLayout  *m_pMainLayout    = nullptr;
    QSpinBox     *m_pNComponentsSpin = nullptr;
    QPushButton  *m_pComputeButton = nullptr;
    QPushButton  *m_pApplyButton   = nullptr;
    QPushButton  *m_pResetButton   = nullptr;
    QScrollArea  *m_pScrollArea    = nullptr;
    QWidget      *m_pComponentListWidget = nullptr;
    QVBoxLayout  *m_pComponentListLayout = nullptr;
    QLabel       *m_pStatusLabel   = nullptr;

    QVector<QCheckBox*> m_componentCheckboxes;

    // Data
    Eigen::MatrixXd                     m_rawData;
    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;
    int                                 m_firstSample = 0;

    // ICA result
    UTILSLIB::IcaResult                 m_icaResult;
    bool                                m_bHasResult = false;
};

} // namespace MNEBROWSE

#endif // ICAWINDOW_H
