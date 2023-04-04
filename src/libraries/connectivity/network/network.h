//=============================================================================================================
/**
 * @file     network.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
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
 * @brief     Network class declaration.
 *
 */

#ifndef NETWORK_H
#define NETWORK_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

class NetworkEdge;
class NetworkNode;

struct VisualizationInfo {
    QString sMethod = "Map";                                    /**< The color method: Map (uses sColormap parameter) or Color (uses colNodes and colEdges).*/
    QString sColormap = "Viridis";                                 /**< The colormap.*/
    Eigen::Vector4i colNodes = Eigen::Vector4i(255, 0, 0, 255); /**< The node color.*/
    Eigen::Vector4i colEdges = Eigen::Vector4i(255, 0, 0, 255); /**< The edge color.*/
};

//=============================================================================================================
/**
 * This class holds information (nodes and connecting edges) about a network, can compute a distance table and provide network metrics.
 *
 * @brief This class holds information about a network, can compute a distance table and provide network metrics.
 */

class CONNECTIVITYSHARED_EXPORT Network
{

public:
    typedef QSharedPointer<Network> SPtr;            /**< Shared pointer type for Network. */
    typedef QSharedPointer<const Network> ConstSPtr; /**< Const shared pointer type for Network. */

    //=========================================================================================================
    /**
     * Constructs a Network object.
     *
     * @param[in] sConnectivityMethod    The connectivity measure method used to create the data of this network structure.
     * @param[in] dThreshold             The threshold of the network. Default is 0.0.
     */
    explicit Network(const QString& sConnectivityMethod = "Unknown",
                     double dThreshold = 0.0);

    //=========================================================================================================
    /**
     * Returns the full connectivity matrix for this network structure.
     *
     * @param[in] bGetMirroredVersion    Flag whether to return the mirrored version of the connectivity matrix, if the network.
     *                                   is a non-directional one. Otherwise returns zeros for the lower part of the matrix.
     *                                   Default is set to true.
     *
     * @return    The full connectivity matrix generated from the current network information.
     */
    Eigen::MatrixXd getFullConnectivityMatrix(bool bGetMirroredVersion = true) const;

    //=========================================================================================================
    /**
     * Returns the thresholded connectivity matrix for this network structure.
     *
     * @param[in] bGetMirroredVersion    Flag whether to return the mirrored version of the connectivity matrix, if the network.
     *                                   is a non-directional one. Otherwise returns zeros for the lower part of the matrix.
     *                                   Default is set to true.
     *
     * @return    The thresholded connectivity matrix generated from the current network information.
     */
    Eigen::MatrixXd getThresholdedConnectivityMatrix(bool bGetMirroredVersion = true) const;

    //=========================================================================================================
    /**
     * Returns the full and non thresholded edges.
     *
     * @return Returns the network edges.
     */
    const QList<QSharedPointer<NetworkEdge> >& getFullEdges() const;

    //=========================================================================================================
    /**
     * Returns the thresholded edges.
     *
     * @return Returns the network edges.
     */
    const QList<QSharedPointer<NetworkEdge> >& getThresholdedEdges() const;

    //=========================================================================================================
    /**
     * Returns the nodes.
     *
     * @return Returns the network nodes.
     */
    const QList<QSharedPointer<NetworkNode> >& getNodes() const;

    //=========================================================================================================
    /**
     * Returns the edge at a specific position.
     *
     * @param[in] i      The index to look up the edge. i must be a valid index position in the network list (i.e., 0 <= i < size()).
     *
     * @return Returns the network edge.
     */
    QSharedPointer<NetworkEdge> getEdgeAt(int i);

    //=========================================================================================================
    /**
     * Returns the node at a specific position.
     *
     * @param[in] i      The index to look up the node. i must be a valid index position in the network list (i.e., 0 <= i < size()).
     *
     * @return Returns the network node.
     */
    QSharedPointer<NetworkNode> getNodeAt(int i);

    //=========================================================================================================
    /**
     * Returns network distribution, also known as network degree, corresponding to the full network.
     *
     * @return   The network distribution calculated as degrees of all nodes together.
     */
    qint16 getFullDistribution() const;

    //=========================================================================================================
    /**
     * Returns network distribution, also known as network degree, corresponding to the thresholded network.
     *
     * @return   The network distribution calculated as degrees of all nodes together.
     */
    qint16 getThresholdedDistribution() const;

    //=========================================================================================================
    /**
     * Sets the connectivity measure method used to create the data of this network structure.
     *
     * @param[in] sConnectivityMethod    The connectivity measure method used to create the data of this network structure.
     */
    void setConnectivityMethod(const QString& sConnectivityMethod);

    //=========================================================================================================
    /**
     * Returns the connectivity measure method used to create the data of this network structure.
     *
     * @return   The connectivity measure method used to create the data of this network structure.
     */
    QString getConnectivityMethod() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum weight strength of the entire network.
     *
     * @return   The minimum and maximum weight strength of the entire network.
     */
    QPair<double, double> getMinMaxFullWeights() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum weight strength of the thresholded network.
     *
     * @return   The minimum and maximum weight strength of the entire network.
     */
    QPair<double, double> getMinMaxThresholdedWeights() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum degree (in and out) corresponding to the full network.
     *
     * @return   The minimum and maximum degree of the entire network.
     */
    QPair<int,int> getMinMaxFullDegrees() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum degree (in and out) corresponding to the thresholded network.
     *
     * @return   The minimum and maximum degree of the entire network.
     */
    QPair<int,int> getMinMaxThresholdedDegrees() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum indegree corresponding to the full network.
     *
     * @return   The minimum and maximum indegree of the entire network.
     */
    QPair<int,int> getMinMaxFullIndegrees() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum indegree corresponding to the thresholded network.
     *
     * @return   The minimum and maximum indegree of the entire network.
     */
    QPair<int,int> getMinMaxThresholdedIndegrees() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum outdegree corresponding to the full network.
     *
     * @return   The minimum and maximum outdegree of the entire network.
     */
    QPair<int,int> getMinMaxFullOutdegrees() const;

    //=========================================================================================================
    /**
     * Returns the minimum and maximum outdegree corresponding to the thresholded network.
     *
     * @return   The minimum and maximum outdegree of the entire network.
     */
    QPair<int,int> getMinMaxThresholdedOutdegrees() const;

    //=========================================================================================================
    /**
     * Sets the threshold of the network and updates the resulting active edges.
     *
     * @param[in] dThreshold        The new threshold.
     */
    void setThreshold(double dThreshold = 0.0);

    //=========================================================================================================
    /**
     * Returns the current threshold of the network.
     *
     * @return The current threshold.
     */
    double getThreshold();

    //=========================================================================================================
    /**
     * Sets the frequency range to average from/to.
     *
     * @param[in] fLowerFreq        The new lower frequency edge to average from.
     * @param[in] fUpperFreq        The new upper frequency edge to average to.
     */
    void setFrequencyRange(float fLowerFreq, float fUpperFreq);

    //=========================================================================================================
    /**
     * Returns the current frequency edge to average from/to.
     *
     * @return The current upper/lower frequency edge to average from/to.
     */
    const QPair<float,float>& getFrequencyRange() const;

    //=========================================================================================================
    /**
     * Appends a network edge to this network node.
     *
     * @param[in] newEdge    The new edge item.
     */
    void append(QSharedPointer<NetworkEdge> newEdge);

    //=========================================================================================================
    /**
     * Appends a network edge to this network node.
     *
     * @param[in] newNode    The new node item as a reference.
     */
    void append(QSharedPointer<NetworkNode> newNode);

    //=========================================================================================================
    /**
     * Returns whether the Network is empty by checking the number of nodes and edges.
     *
     * @return   The flag identifying whether the Network is empty.
     */
    bool isEmpty() const;

    //=========================================================================================================
    /**
     * Normalize the network.
     */
    void normalize();

    //=========================================================================================================
    /**
     * Get the current visualization info.
     *
     * @return The current visualization info.
     */
    VisualizationInfo getVisualizationInfo() const;

    //=========================================================================================================
    /**
     * Set the current visualization info.
     *
     * @param[in] visualizationInfo        The new visualization info.
     */
    void setVisualizationInfo(const VisualizationInfo& visualizationInfo);

    //=========================================================================================================
    /**
     * Get the currently set sampling frequency.
     *
     * @return The currently set sampling frequency.
     */
    float getSamplingFrequency() const;

    //=========================================================================================================
    /**
     * Set the new sampling frequency.
     *
     * @param[in] sFreq        The new sampling frequency.
     */
    void setSamplingFrequency(float fSFreq);

    //=========================================================================================================
    /**
     * Get the currently set number of frequency bins.
     *
     * @return The currently set number of samples.
     */
    int getUsedFreqBins() const;

    //=========================================================================================================
    /**
     * Set the new number of used frequency bins.
     *
     * @param[in] iNumberFreqBins        The new number of used frequency bins.
     */
    void setUsedFreqBins(int iNumberFreqBins);

    //=========================================================================================================
    /**
     * Set the new FFT size.
     *
     * @param[in] iFFTSize        The used FFT size (number of total frequency bins for a half spectrum - only positive frequencies).
     */
    void setFFTSize(int iFFTSize);

    //=========================================================================================================
    /**
     * Returns the current FFT size.
     *
     * @return   The used FFT size (number of total frequency bins for a half spectrum - only positive frequencies).
     */
    int getFFTSize();

protected:
    QList<QSharedPointer<NetworkEdge> >     m_lFullEdges;               /**< List with all edges of the network.*/
    QList<QSharedPointer<NetworkEdge> >     m_lThresholdedEdges;        /**< List with all the active (thresholded) edges of the network.*/

    QList<QSharedPointer<NetworkNode> >     m_lNodes;                   /**< List with all nodes of the network.*/

    Eigen::MatrixXd                         m_matDistMatrix;            /**< The distance matrix.*/

    QString                                 m_sConnectivityMethod;      /**< The connectivity measure method used to create the data of this network structure.*/

    QPair<double,double>                    m_minMaxFullWeights;        /**< The minimum and maximum weight strength of the entire network.*/
    QPair<double,double>                    m_minMaxThresholdedWeights; /**< The minimum and maximum weight strength of the active edges.*/
    QPair<float,float>                      m_minMaxFrequency;          /**< The minimum and maximum frequency bins to average from/to.*/

    double                                  m_dThreshold;               /**< The current value which was used to threshold the edge weigths.*/
    float                                   m_fSFreq;                   /**< The sampling frequency used to collect the data which this network is based on.*/
    int                                     m_iNumberFreqBins;          /**< The number of used frequency bins.*/
    int                                     m_iFFTSize;                 /**< The used FFT size (number of total frequency bins for a half spectrum - only positive frequencies).*/

    VisualizationInfo                       m_visualizationInfo;        /**< The current visualization info used to plot the network later on.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNECTIVITYLIB

#ifndef metatype_networks
#define metatype_networks
Q_DECLARE_METATYPE(CONNECTIVITYLIB::Network);
#endif

#ifndef metatype_networkslist
#define metatype_networkslist
Q_DECLARE_METATYPE(QList<CONNECTIVITYLIB::Network>);
#endif

#ifndef metatype_networkssptr
#define metatype_networkssptr
Q_DECLARE_METATYPE(CONNECTIVITYLIB::Network::SPtr);
#endif

#ifndef metatype_networkssptrlist
#define metatype_networkssptrlist
Q_DECLARE_METATYPE(QList<CONNECTIVITYLIB::Network::SPtr>);
#endif

#endif // NETWORK_H
