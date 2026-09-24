//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     icawindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     June, 2026
 * @brief    Declaration of the IcaWindow class.
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
