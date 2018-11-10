//=============================================================================================================
/**
* @file     connectivitysettings.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

struct ConnectivityTrialData {
    Eigen::MatrixXd matData;
    Eigen::MatrixXd matPsd;
    QVector<QPair<int,Eigen::MatrixXcd> > vecPairCsd;
    QVector<QPair<int,Eigen::MatrixXcd> > vecPairCsdNormalized;
    QVector<QPair<int,Eigen::MatrixXd> > vecPairCsdImagSign;
    QVector<Eigen::MatrixXcd> vecTapSpectra;
};

struct ConnectivityData {
    Eigen::MatrixXd matPsdSum;
    QVector<QPair<int,Eigen::MatrixXcd> > vecPairCsdSum;
    QVector<QPair<int,Eigen::MatrixXcd> > vecPairCsdNormalizedSum;
    QVector<QPair<int,Eigen::MatrixXd> > vecPairCsdImagSignSum;
};


//*************************************************************************************************************
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

    //=========================================================================================================
    /**
    * Constructs a ConnectivitySettings object.
    */
    explicit ConnectivitySettings();

    void clearData() {
        m_dataList.clear();
        data.matPsdSum.resize(0,0);
        data.vecPairCsdSum.clear();
        data.vecPairCsdNormalizedSum.clear();
    }

    void resetData() {
        for (int i = 0; i < m_dataList.size(); ++i) {
            m_dataList[i].matPsd.resize(0,0);
            m_dataList[i].vecPairCsd.clear();
            m_dataList[i].vecTapSpectra.clear();
        }

        data.matPsdSum.resize(0,0);
        data.vecPairCsdSum.clear();
    }

    void append(const QList<Eigen::MatrixXd>& matInputData) {
        for(int i = 0; i < matInputData.size(); ++i) {
            this->append(matInputData.at(i));
        }
    }

    void append(const Eigen::MatrixXd& matInputData) {
        ConnectivityTrialData tempData;
        tempData.matData = matInputData;

        m_dataList.append(tempData);
    }

    int size() const {
        return m_dataList.size();
    }

    void removeFirst() {
        if(!m_dataList.isEmpty()) {
            // Substract PSD of first trial from overall summed up PSD
            if(data.matPsdSum.rows() == m_dataList.first().matPsd.rows() &&
               data.matPsdSum.cols() == m_dataList.first().matPsd.cols() ) {
                data.matPsdSum -= m_dataList.first().matPsd;
            }

            // Substract CSD of first trial from overall summed up CSD
            if(data.vecPairCsdSum.size() == m_dataList.first().vecPairCsd.size()) {
                for (int i = 0; i < data.vecPairCsdSum.size(); ++i) {
                    data.vecPairCsdSum[i].second -= m_dataList.first().vecPairCsd.at(i).second;
                }
            }

            // Substract normalized CSD of first trial from overall summed up normalized CSD
            if(data.vecPairCsdNormalizedSum.size() == m_dataList.first().vecPairCsdNormalized.size()) {
                for (int i = 0; i < data.vecPairCsdNormalizedSum.size(); ++i) {
                    data.vecPairCsdNormalizedSum[i].second -= m_dataList.first().vecPairCsdNormalized.at(i).second;
                }
            }

            // Substract sign CSD of first trial from overall summed up sign CSD
            if(data.vecPairCsdImagSignSum.size() == m_dataList.first().vecPairCsdImagSign.size()) {
                for (int i = 0; i < data.vecPairCsdImagSignSum.size(); ++i) {
                    data.vecPairCsdImagSignSum[i].second -= m_dataList.first().vecPairCsdImagSign.at(i).second;
                }
            }

            m_dataList.removeFirst();
        }
    };

    QStringList                 m_sConnectivityMethods;         /**< The connectivity methods. */

    QList<ConnectivityTrialData>     m_dataList;                     /**< The input data. */

    Eigen::MatrixX3f            m_matNodePositions;             /**< The node position in 3D space. */

    int                         m_iNfft;                        /**< The FFT length used for spectral estimation. */
    QString                     m_sWindowType;                  /**< The window type used to compute tapered spectra. */

    ConnectivityData            data;

protected:

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#ifndef metatype_connectivitysettings
#define metatype_connectivitysettings
Q_DECLARE_METATYPE(CONNECTIVITYLIB::ConnectivitySettings)
#endif

#endif // CONNECTIVITYSETTINGS_H
