//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Daniel Strohmeier <daniel.strohmeier@gmail.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file connectivitysettings.h
 * @since March 2017
 * @brief Input-data and parameter container shared by every functional-connectivity metric in @c CONNLIB.
 *
 * @ref ConnectivitySettings carries the per-trial time-domain matrices, the
 * derived per-trial cross-spectral data (DPSS tapered FFTs, CSDs and the
 * imaginary-part variants needed by PLI / wPLI / dwPLI), and the
 * accumulated cross-trial sums used to normalise the final estimates.
 * Holding all of this in one object lets the dispatcher in @ref Connectivity
 * compute several metrics in one pass without re-tapering and re-FFT'ing
 * the same trials: the @ref IntermediateTrialData fields are filled lazily
 * by the first metric that needs them and reused by everyone else.
 *
 * The container also stores the spectral-domain parameters (sampling rate,
 * FFT length, taper window name) and the 3D node positions used to lay out
 * the resulting @ref Network - the latter can be sourced either from a
 * @c FiffInfo channel set (sensor space) or from a forward solution and
 * @c FsSurfaceSet pair (source space).
 */

#ifndef CONNECTIVITYSETTINGS_H
#define CONNECTIVITYSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "conn_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEForwardSolution;
}

namespace FSLIB {
    class FsSurfaceSet;
}

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Mutable container that aggregates the inputs and intermediate spectral
 * results required by every metric implementation in @c CONNLIB.
 *
 * Each call to @ref append adds one trial; the trial matrices live in
 * @ref m_trialData, while DPSS tapered spectra, cross-spectral densities
 * and their imaginary-part derivatives are filled in lazily by whichever
 * metric runs first. Cross-trial sums accumulate in @ref m_intermediateSumData
 * so that a second metric run on the same data set reuses those sums and
 * skips the FFT pass entirely. Node positions can be derived from a
 * @c FiffInfo (sensor space) or from a forward solution + @c FsSurfaceSet
 * (source space) and are propagated into the resulting @ref Network nodes.
 *
 * @brief Aggregates trial data, spectral cache and node geometry shared by all @c CONNLIB metrics.
 */
class CONNSHARED_EXPORT ConnectivitySettings
{

public:
    typedef QSharedPointer<ConnectivitySettings> SPtr;            /**< Shared pointer type for ConnectivitySettings. */
    typedef QSharedPointer<const ConnectivitySettings> ConstSPtr; /**< Const shared pointer type for ConnectivitySettings. */

    /**
     * @brief Per-trial intermediate frequency-domain data used during connectivity computation
     */
    struct IntermediateTrialData {
        Eigen::MatrixXd     matData;
        Eigen::MatrixXd     matPsd;
        QVector<Eigen::MatrixXcd>               vecTapSpectra;
        QVector<QPair<int,Eigen::MatrixXcd> >   vecPairCsd;
        QVector<QPair<int,Eigen::MatrixXcd> >   vecPairCsdNormalized;
        QVector<QPair<int,Eigen::MatrixXd> >    vecPairCsdImagSign;
        QVector<QPair<int,Eigen::MatrixXd> >    vecPairCsdImagAbs;
        QVector<QPair<int,Eigen::MatrixXd> >    vecPairCsdImagSqrd;
    };

    /**
     * @brief Accumulated cross-spectral and auto-spectral sums across trials for final metric normalization
     */
    struct IntermediateSumData {
        Eigen::MatrixXd     matPsdSum;
        QVector<QPair<int,Eigen::MatrixXcd> >   vecPairCsdSum;
        QVector<QPair<int,Eigen::MatrixXcd> >   vecPairCsdNormalizedSum;
        QVector<QPair<int,Eigen::MatrixXd> >    vecPairCsdImagSignSum;
        QVector<QPair<int,Eigen::MatrixXd> >    vecPairCsdImagAbsSum;
        QVector<QPair<int,Eigen::MatrixXd> >    vecPairCsdImagSqrdSum;
    };

    //=========================================================================================================
    /**
     * Constructs a ConnectivitySettings object.
     */
    explicit ConnectivitySettings();

    void clearAllData();

    void clearIntermediateData();

    void append(const QList<Eigen::MatrixXd>& matInputData);

    void append(const Eigen::MatrixXd& matInputData);

    void append(const ConnectivitySettings::IntermediateTrialData& inputData);

    const IntermediateTrialData& at(int i) const;

    int size() const;

    bool isEmpty() const;

    void removeFirst(int iAmount = 1);

    void removeLast(int iAmount = 1);

    void setConnectivityMethods(const QStringList& sConnectivityMethods);

    const QStringList& getConnectivityMethods() const;

    void setSamplingFrequency(int iSFreq);

    int getSamplingFrequency() const;

    void setFFTSize(int iNfft);

    int getFFTSize() const;

    void setWindowType(const QString& sWindowType);

    const QString& getWindowType() const;

    void setNodePositions(const FIFFLIB::FiffInfo& fiffInfo,
                          const Eigen::RowVectorXi& picks);

    void setNodePositions(const MNELIB::MNEForwardSolution& forwardSolution,
                          const FSLIB::FsSurfaceSet& surfSet);

    void setNodePositions(const Eigen::MatrixX3f& matNodePositions);

    const Eigen::MatrixX3f& getNodePositions() const;

    QList<IntermediateTrialData>& getTrialData();

    IntermediateSumData& getIntermediateSumData();

protected:
    QStringList                     m_sConnectivityMethods;         /**< The connectivity methods. */
    QString                         m_sWindowType;                  /**< The window type used to compute tapered spectra. */

    float                           m_fSFreq;                       /**< The sampling frequency. */
    int                             m_iNfft;                        /**< The FFT length. Also includes the negativ frequencies. Gets recalculated if the sFreq or spectrum resolution change. */
    float                           m_fFreqResolution;              /**< The spectrum's resolution. */

    Eigen::MatrixX3f                m_matNodePositions;             /**< The node position in 3D space. */

    IntermediateSumData             m_intermediateSumData;          /**< The intermediate sum data holds data calculated over all trials as a whole. */
    QList<IntermediateTrialData>    m_trialData;                    /**< The trial data holds the actual and intermediate data calcualted for each trial. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#ifndef metatype_connectivitysettings
#define metatype_connectivitysettings
Q_DECLARE_METATYPE(CONNLIB::ConnectivitySettings)
#endif

#endif // CONNECTIVITYSETTINGS_H
