//=============================================================================================================
/**
 * @file     networkedge.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief     NetworkEdge class declaration.
 *
 */

#ifndef NETWORKEDGE_H
#define NETWORKEDGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include<Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class NetworkNode;

//=============================================================================================================
/**
 * This class holds an object to describe the edge of a network.
 *
 * @brief This class holds an object to describe the edge of a network.
 */

class CONNECTIVITYSHARED_EXPORT NetworkEdge
{

public:
    typedef QSharedPointer<NetworkEdge> SPtr;            /**< Shared pointer type for NetworkEdge. */
    typedef QSharedPointer<const NetworkEdge> ConstSPtr; /**< Const shared pointer type for NetworkEdge. */

    //=========================================================================================================
    /**
     * Constructs a NetworkEdge object.
     *
     * @param[in] iStartNodeID      The start node id of the edge.
     * @param[in] iEndNodeID        The end node id of the edge.
     * @param[in] matWeight         The edge weight.
     * @param[in] bIsActive         The active flag of this edge. Default is true.
     * @param[in] iStartWeightBin   The bin index to start avergaing from. Default is -1 which means an average over all weights.
     * @param[in] iEndWeightBin     The bin index to end avergaing to. Default is -1 which means an average over all weights.
     */
    explicit NetworkEdge(int iStartNodeID,
                         int iEndNodeID,
                         const Eigen::MatrixXd& matWeight,
                         bool bIsActive = true,
                         int iStartWeightBin = -1,
                         int iEndWeightBin = -1);

    //=========================================================================================================
    /**
     * Returns the start node of this edge.
     *
     * @return The start node of the edge.
     */
    int getStartNodeID();

    //=========================================================================================================
    /**
     * Returns the end node of this edge.
     *
     * @return The end node of the edge.
     */
    int getEndNodeID();

    //=========================================================================================================
    /**
     * Sets the activity flag of this edge.
     *
     * @param[in] bActiveFlag        The new activity flag of this edge.
     */
    void setActive(bool bActiveFlag);

    //=========================================================================================================
    /**
     * Returns the activity flag of this edge.
     *
     * @return The current activity flag of this edge.
     */
    bool isActive();

    //=========================================================================================================
    /**
     * Returns the edge weight. The weights are averaged between the specified bin indeces and their corresponding tapers.
     *
     * @return    The current edge weight.
     */
    double getWeight() const;

    //=========================================================================================================
    /**
     * Returns the edge weight non-averaged in form of a matrix.
     *
     * @return    The current edge weight matrix.
     */
    Eigen::MatrixXd getMatrixWeight() const;

    //=========================================================================================================
    /**
     * Set the averaged edge weight.
     *
     * @param[in] dAveragedWeight        The new weight.
     */
    void setWeight(double dAveragedWeight);

    //=========================================================================================================
    /**
     * Calculates the edge weight based on the currently set min/max frequency bins.
     */
    void calculateAveragedWeight();

    //=========================================================================================================
    /**
     * Sets the frequency bins to average from/to.
     *
     * @param[in] minMaxFreqBins        The new lower/upper bin to average from/to.
     */
    void setFrequencyBins(const QPair<int, int> &minMaxFreqBins);

    //=========================================================================================================
    /**
     * Returns the current frequency bins to average from/to.
     *
     * @return The current upper/lower bin to average from/to.
     */
    const QPair<int,int>& getFrequencyBins();

protected:
    int             m_iStartNodeID;         /**< The start node of the edge.*/
    int             m_iEndNodeID;           /**< The end node of the edge.*/

    bool            m_bIsActive;            /**< The activity flag indicating whether this edge is part of a thresholded network.*/

    QPair<int,int>  m_iMinMaxFreqBins;      /**< The lower/upper bin indeces to start avergaing from/to. Default is -1 which means an average over all weights.*/

    Eigen::MatrixXd m_matWeight;            /**< The weight matrix of the edge. E.g. rows could be different frequency bins/bands and columns could be different instances in time.*/

    double          m_dAveragedWeight;      /**< The current averaged edge weight.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace CONNECTIVITYLIB

#endif // NETWORKEDGE_H
