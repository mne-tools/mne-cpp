//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     networkedge.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Weighted edge between two @ref NetworkNode instances; stores the full per-frequency weight matrix and the scalar band-averaged weight.
 *
 * Every functional-connectivity estimator in @c CONNLIB produces one
 * @ref NetworkEdge per ordered channel pair (or per unordered pair for
 * symmetric metrics) and attaches it to the @ref Network. The edge owns
 * the full weight matrix returned by the metric - typically one row per
 * frequency bin and one or more columns per estimator-specific output -
 * and a configurable @c [iStartWeightBin, iEndWeightBin) reduction window
 * that defines the single scalar @ref weight used by the visualisation
 * and thresholding paths.
 *
 * The active/inactive flag and the threshold-aware comparison operators
 * are read by @ref Network::getThresholdedEdges so that GUI plugins can
 * toggle the displayed edge density without rebuilding the underlying
 * connectivity result.
 */

#ifndef NETWORKEDGE_H
#define NETWORKEDGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../conn_global.h"

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
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

class NetworkNode;

//=============================================================================================================
/**
 * Directed weighted edge connecting two @ref NetworkNode instances.
 *
 * Holds the start- and end-node IDs, the full per-frequency weight matrix
 * delivered by the metric, an active/inactive flag honoured by the
 * thresholded views on @ref Network, and the @c [iStartWeightBin,
 * iEndWeightBin) window that reduces the matrix to the scalar weight
 * used for thresholding and rendering. For symmetric metrics the
 * directionality is purely a storage convention.
 *
 * @brief Weighted, directional edge in a @ref Network; carries per-frequency weights plus a band-averaged scalar.
 */

class CONNSHARED_EXPORT NetworkEdge
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

} // namespace CONNLIB

#endif // NETWORKEDGE_H
