//=============================================================================================================
/**
 * @file     hpifitdatahandler.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     December, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben Dörfel. All rights reserved.
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
 * @brief     HpiFitDataHandler class declaration.
 *
 */

#ifndef HPIFITDATAHANDLER_H
#define HPIFITDATAHANDLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
    class FiffChInfo;
    class FiffDigPointSet;
}

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// INVERSELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT HpiFitDataHandler
{

public:
    typedef QSharedPointer<HpiFitDataHandler> SPtr;            /**< Shared pointer type for HpiFitDataHandler. */
    typedef QSharedPointer<const HpiFitDataHandler> ConstSPtr; /**< Const shared pointer type for HpiFitDataHandler. */

    //=========================================================================================================
    /**
    * Constructs a HpiFitDataHandler object.
    */
    HpiFitDataHandler(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Reduce data to only use good channels.
     *
     * @param[in] matProjectors     The projector matrix.
     *
     */
    void prepareData(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Reduce projectors to only use good channels.
     *
     * @param[in] matProjectors     The projector matrix.
     *
     */
    void prepareProjectors(const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
     * inline get functions for private member variables.
     *
     */
    inline Eigen::MatrixXd getProjectors() const;
    inline Eigen::MatrixXd getHpiDigitizer() const;
    inline QList<FIFFLIB::FiffChInfo> getChannels() const;
    inline QList<QString> getBads() const;

protected:

private:
    //=========================================================================================================
    /**
     * Update the channellist for init and if bads changed
     *
     * @param[in] pFiffInfo       The FiffInfo file from the measurement.
     *
     */
    void updateChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Update the list of bad channels
     *
     * @param[in] pFiffInfo       The FiffInfo file from the measurement.
     *
     */
    void updateBadChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Update the digitized HPI coils.
     * @param[in]   lDig          The digitizer list to extract the hpi coils from.
     *
     */
    void updateHpiDigitizer(const QList<FIFFLIB::FiffDigPoint>& lDig);

    QList<FIFFLIB::FiffChInfo> m_lChannels; /**< Channellist with bads excluded. */
    QVector<int> m_vecInnerind;             /**< index of inner channels . */
    QList<QString> m_lBads;                 /**< contains bad channels . */
    Eigen::MatrixXd m_matHpiDigitizer;      /**< The coordinates of the digitized HPI coils in head space*/
    Eigen::MatrixXd m_matProjectors;        /**< The projectors ready to use*/
    Eigen::MatrixXd m_matInnerdata;         /**< The data ready to use*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QList<FIFFLIB::FiffChInfo> HpiFitDataHandler::getChannels() const
{
    return m_lChannels;
}

inline QList<QString> HpiFitDataHandler::getBads() const
{
    return m_lBads;
}


inline Eigen::MatrixXd HpiFitDataHandler::getProjectors() const
{
    return m_matProjectors;
}

inline Eigen::MatrixXd HpiFitDataHandler::getHpiDigitizer() const
{
    return m_matHpiDigitizer;
}

} // namespace INVERSELIB

#endif // HPIFITDATAHANDLER_H

