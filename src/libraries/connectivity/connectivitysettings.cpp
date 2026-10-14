//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     connectivitysettings.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Implementation of @ref CONNECTIVITYLIB::ConnectivitySettings - the input/parameter container shared by every functional-connectivity metric in @c CONNECTIVITYLIB.
 *
 * Defines the trial append/clear/remove bookkeeping, the lazy invalidation
 * of the cached per-trial DPSS spectra and cross-spectral sums when the
 * sampling frequency / FFT length / window type change, and the three
 * node-position constructors (from a @c FiffInfo channel set, from a
 * forward solution + @c FsSurfaceSet pair, or from a raw 3D matrix).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettings.h"

#include <mne/mne_forward_solution.h>
#include <fs/fs_surfaceset.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace FSLIB;

namespace
{

// Each pair of a trial is taken out of the matching running sum, as long as both have the same length
template<typename T>
void subtractPairs(QVector<QPair<int, T>>& sums, const QVector<QPair<int, T>>& trial, int rows)
{
    if (sums.size() != trial.size())
        return;
    for (int i = 0; i < rows && i < sums.size(); ++i)
        sums[i].second -= trial.at(i).second;
}

// Removes the influence of one trial from the sums over all trials
void subtractFromSums(ConnectivitySettings::IntermediateSumData& sums, const ConnectivitySettings::IntermediateTrialData& trial)
{
    const int rows = static_cast<int>(trial.matData.rows());
    subtractPairs(sums.vecPairCsdSum, trial.vecPairCsd, rows);
    subtractPairs(sums.vecPairCsdNormalizedSum, trial.vecPairCsdNormalized, rows);
    subtractPairs(sums.vecPairCsdImagSignSum, trial.vecPairCsdImagSign, rows);
    subtractPairs(sums.vecPairCsdImagAbsSum, trial.vecPairCsdImagAbs, rows);
    subtractPairs(sums.vecPairCsdImagSqrdSum, trial.vecPairCsdImagSqrd, rows);

    if (sums.matPsdSum.rows() == trial.matPsd.rows() && sums.matPsdSum.cols() == trial.matPsd.cols())
        sums.matPsdSum -= trial.matPsd;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivitySettings::ConnectivitySettings()
: m_sWindowType("hanning")
, m_fSFreq(1000.0f)
, m_fFreqResolution(1.0f)
{
    m_iNfft = int(m_fSFreq / m_fFreqResolution);
    qRegisterMetaType<CONNECTIVITYLIB::ConnectivitySettings>("CONNECTIVITYLIB::ConnectivitySettings");
}

//*******************************************************************************************************

void ConnectivitySettings::clearAllData()
{
    m_trialData.clear();

    clearIntermediateData();
}

//*******************************************************************************************************

void ConnectivitySettings::clearIntermediateData()
{
    for (int i = 0; i < m_trialData.size(); ++i) {
        m_trialData[i].matPsd.resize(0, 0);
        m_trialData[i].vecPairCsd.clear();
        m_trialData[i].vecTapSpectra.clear();
        m_trialData[i].vecPairCsdNormalized.clear();
        m_trialData[i].vecPairCsdImagSign.clear();
        m_trialData[i].vecPairCsdImagAbs.clear();
        m_trialData[i].vecPairCsdImagSqrd.clear();
    }

    m_intermediateSumData.matPsdSum.resize(0, 0);
    m_intermediateSumData.vecPairCsdSum.clear();
    m_intermediateSumData.vecPairCsdNormalizedSum.clear();
    m_intermediateSumData.vecPairCsdImagSignSum.clear();
    m_intermediateSumData.vecPairCsdImagAbsSum.clear();
    m_intermediateSumData.vecPairCsdImagSqrdSum.clear();
}

//*******************************************************************************************************

void ConnectivitySettings::append(const QList<MatrixXd>& matInputData)
{
    for (int i = 0; i < matInputData.size(); ++i) {
        this->append(matInputData.at(i));
    }
}

//*******************************************************************************************************

void ConnectivitySettings::append(const MatrixXd& matInputData)
{
    ConnectivitySettings::IntermediateTrialData tempData;
    tempData.matData = matInputData;

    m_trialData.append(tempData);
}

//*******************************************************************************************************

void ConnectivitySettings::append(const ConnectivitySettings::IntermediateTrialData& inputData)
{
    m_trialData.append(inputData);
}

//*******************************************************************************************************

const ConnectivitySettings::IntermediateTrialData& ConnectivitySettings::at(int i) const
{
    return m_trialData.at(i);
}

//*******************************************************************************************************

int ConnectivitySettings::size() const
{
    return m_trialData.size();
}

//*******************************************************************************************************

bool ConnectivitySettings::isEmpty() const
{
    return m_trialData.isEmpty();
}

//*******************************************************************************************************

void ConnectivitySettings::removeFirst(int iAmount)
{
    if (m_trialData.isEmpty()) {
        qDebug() << "ConnectivitySettings::removeFirst - No elements to delete. Returning.";
        return;
    }

    if (m_trialData.size() < iAmount) {
        qDebug() << "ConnectivitySettings::removeFirst - Not enough elements stored in list in order to delete them. Returning.";
        return;
    }

    for (int j = 0; j < iAmount; ++j) {
        subtractFromSums(m_intermediateSumData, m_trialData.first());
        m_trialData.removeFirst();
    }
}

//*******************************************************************************************************

void ConnectivitySettings::removeLast(int iAmount)
{
    if (m_trialData.isEmpty()) {
        qDebug() << "ConnectivitySettings::removeLast - No elements to delete. Returning.";
        return;
    }

    if (m_trialData.size() < iAmount) {
        qDebug() << "ConnectivitySettings::removeLast - Not enough elements stored in list in order to delete them. Returning.";
        return;
    }

    for (int j = 0; j < iAmount; ++j) {
        subtractFromSums(m_intermediateSumData, m_trialData.last());
        m_trialData.removeLast();
    }
}

//*******************************************************************************************************

void ConnectivitySettings::setConnectivityMethods(const QStringList& sConnectivityMethods)
{
    m_sConnectivityMethods = sConnectivityMethods;
}

//*******************************************************************************************************

const QStringList& ConnectivitySettings::getConnectivityMethods() const
{
    return m_sConnectivityMethods;
}

//*******************************************************************************************************

void ConnectivitySettings::setSamplingFrequency(int iSFreq)
{
    if (m_fSFreq == iSFreq) {
        return;
    }

    clearIntermediateData();

    m_fSFreq = iSFreq;

    if (m_fFreqResolution != 0.0f) {
        m_iNfft = int(m_fSFreq / m_fFreqResolution);
    }
}

//*******************************************************************************************************

int ConnectivitySettings::getSamplingFrequency() const
{
    return m_fSFreq;
}

//*******************************************************************************************************

void ConnectivitySettings::setFFTSize(int iNfft)
{
    if (iNfft == 0) {
        return;
    }

    clearIntermediateData();

    m_iNfft = iNfft;
    m_fFreqResolution = m_fSFreq / m_iNfft;
}

//*******************************************************************************************************

int ConnectivitySettings::getFFTSize() const
{
    return m_iNfft;
}

//*******************************************************************************************************

void ConnectivitySettings::setWindowType(const QString& sWindowType)
{
    // Clear all intermediate data since this will have an effect on the frequency calculation
    clearIntermediateData();

    m_sWindowType = sWindowType;
}

//*******************************************************************************************************

const QString& ConnectivitySettings::getWindowType() const
{
    return m_sWindowType;
}

//*******************************************************************************************************

void ConnectivitySettings::setNodePositions(const FiffInfo& fiffInfo,
                                            const RowVectorXi& picks)
{
    m_matNodePositions = MatrixX3f::Zero(picks.cols(), 3);

    for (int i = 0; i < picks.cols(); ++i) {
        const FiffChInfo& ch = fiffInfo.chs.at(picks(i));
        if (ch.kind == FIFFV_EEG_CH || ch.kind == FIFFV_MEG_CH) {
            m_matNodePositions.row(i) = ch.chpos.r0.transpose();
        }
    }
}

//*******************************************************************************************************

void ConnectivitySettings::setNodePositions(const MNEForwardSolution& forwardSolution, const FsSurfaceSet& surfSet)
{
    //Generate node vertices
    MatrixX3f matNodeVertLeft, matNodeVertRight;

    if (forwardSolution.isClustered()) {
        auto* lhHemi = forwardSolution.src.hemisphereAt(0);
        auto* rhHemi = forwardSolution.src.hemisphereAt(1);
        matNodeVertLeft.resize(lhHemi->cluster_info.centroidVertno.size(), 3);
        for (int j = 0; j < matNodeVertLeft.rows(); ++j) {
            matNodeVertLeft.row(j) = surfSet[0].rr().row(lhHemi->cluster_info.centroidVertno.at(j)) - surfSet[0].offset().transpose();
        }

        matNodeVertRight.resize(rhHemi->cluster_info.centroidVertno.size(), 3);
        for (int j = 0; j < matNodeVertRight.rows(); ++j) {
            matNodeVertRight.row(j) = surfSet[1].rr().row(rhHemi->cluster_info.centroidVertno.at(j)) - surfSet[1].offset().transpose();
        }
    } else {
        matNodeVertLeft.resize(forwardSolution.src[0].vertno.rows(), 3);
        for (int j = 0; j < matNodeVertLeft.rows(); ++j) {
            matNodeVertLeft.row(j) = surfSet[0].rr().row(forwardSolution.src[0].vertno(j)) - surfSet[0].offset().transpose();
        }

        matNodeVertRight.resize(forwardSolution.src[1].vertno.rows(), 3);
        for (int j = 0; j < matNodeVertRight.rows(); ++j) {
            matNodeVertRight.row(j) = surfSet[1].rr().row(forwardSolution.src[1].vertno(j)) - surfSet[1].offset().transpose();
        }
    }

    m_matNodePositions.resize(matNodeVertLeft.rows() + matNodeVertRight.rows(), 3);
    m_matNodePositions << matNodeVertLeft, matNodeVertRight;
}

//*******************************************************************************************************

void ConnectivitySettings::setNodePositions(const MatrixX3f& matNodePositions)
{
    m_matNodePositions = matNodePositions;
}

//*******************************************************************************************************

const MatrixX3f& ConnectivitySettings::getNodePositions() const
{
    return m_matNodePositions;
}

//*******************************************************************************************************

QList<ConnectivitySettings::IntermediateTrialData>& ConnectivitySettings::getTrialData()
{
    return m_trialData;
}

//*******************************************************************************************************

ConnectivitySettings::IntermediateSumData& ConnectivitySettings::getIntermediateSumData()
{
    return m_intermediateSumData;
}
