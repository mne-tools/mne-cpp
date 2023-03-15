//=============================================================================================================
/**
 * @file     connectivitysettings.h
 * @author   Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Daniel Strohmeier, Lorenz Esch. All rights reserved.
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
 * @brief     ConnectivitySettings class declaration.
 *
 */

#ifndef CONNECTIVITYSETTINGS_H
#define CONNECTIVITYSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"

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
    class SurfaceSet;
}

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class is a container for connectivity settings.
 *
 * @brief This class is a container for connectivity settings.
 */
class CONNECTIVITYSHARED_EXPORT ConnectivitySettings
{

public:
    typedef QSharedPointer<ConnectivitySettings> SPtr;            /**< Shared pointer type for ConnectivitySettings. */
    typedef QSharedPointer<const ConnectivitySettings> ConstSPtr; /**< Const shared pointer type for ConnectivitySettings. */

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
                          const FSLIB::SurfaceSet& surfSet);

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
} // namespace CONNECTIVITYLIB

#ifndef metatype_connectivitysettings
#define metatype_connectivitysettings
Q_DECLARE_METATYPE(CONNECTIVITYLIB::ConnectivitySettings)
#endif

#endif // CONNECTIVITYSETTINGS_H
