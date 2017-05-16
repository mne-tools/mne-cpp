//=============================================================================================================
/**
* @file     projectingkdtree.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    ProjectingKdTree class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectingkdtree.h"
#include<mne/mne_bem_surface.h>

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GEOMETRYINFO;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================




ProjectingKdTree::ProjectingKdTree(const MNELIB::MNEBemSurface &inSurface, quint32 bucketSize) : m_surface(inSurface)
{
    m_vertIndices = QVector<qint32>(inSurface.rr.rows());
    std::iota(m_vertIndices.begin(), m_vertIndices.end(), 0);

    if(bucketSize > 0 )
    {
        const qint32 maxDepth = std::ceil(log2(inSurface.rr.rows() / bucketSize));
        m_root = recursiveBuild(m_vertIndices.data(), m_vertIndices.size(), 0, maxDepth);
    }
    else
    {
        std::cout << "ERROR: BucketSize of projecting tree is = 0\n";
    }
}

qint32 ProjectingKdTree::findNearestNeighbor(const Eigen::Vector3d &sensorPosition) const
{
    qint32 champion = -1;
    double minDistance = std::numeric_limits<double>::max();
    recursiveSearch(sensorPosition, m_root, champion, minDistance);
    if(champion < 0)
    {
        std::cout << "ERROR: No neighbor found!\n";
    }
    return champion;
}



//*************************************************************************************************************

QSharedPointer<ProjectingKdTree::ProjectingKdTreeNode> ProjectingKdTree::recursiveBuild(qint32 *vertIndices, qint32 numPoints, qint32 depth, qint32 maxDepth)
{
    if( numPoints <= 0 || depth > maxDepth)
    {
        return nullptr;
    }

    const qint8 axis = depth % 3;
    QSharedPointer<ProjectingKdTreeNode> nodePtr = QSharedPointer<ProjectingKdTreeNode>::create(axis);

    //this node is a leaf: bucket + no subtrees
    if(depth == maxDepth)
    {
        nodePtr->m_bucketPtr = vertIndices;
        nodePtr->m_bucketSize = numPoints;
        nodePtr->m_subTrees[0] = nullptr;
        nodePtr->m_subTrees[1] = nullptr;
    }
    //subtrees + no bucket
    else
    {

        //TO DO  pivot element with random
        const qint32 pivot = (numPoints - 1) / 2;

        //partition
        std::nth_element(vertIndices, vertIndices + pivot, vertIndices + numPoints, [&](qint32 lhs, qint32 rhs)
        {
            return m_surface.rr(lhs, axis) < m_surface.rr(rhs, axis);
        });


        nodePtr->m_vertIndex = vertIndices[pivot];

        //left subtree
        nodePtr->m_subTrees[0] = recursiveBuild(vertIndices, pivot, depth + 1, maxDepth);
        //right subtree
        nodePtr->m_subTrees[1] = recursiveBuild(vertIndices + pivot, numPoints - pivot - 1, depth + 1, maxDepth);
    }

    return nodePtr;

}

void ProjectingKdTree::recursiveSearch(const Eigen::Vector3d &sensorPosition, QSharedPointer<ProjectingKdTree::ProjectingKdTreeNode> node, qint32 &champion, double &minDistance) const
{
    const qint32 index = node->m_vertIndex;
    //Leaf reached?
    if(index == -1)
    {
        //lin search in bucket
        for(qint32 i = 0; i < node->m_bucketSize; ++i)
        {
            const double dist = distance3D(sensorPosition, *node->m_bucketPtr + i);
            if(dist < minDistance)
            {
                champion = *node->m_bucketPtr + i;
                minDistance = dist;
            }
        }
        return;
    }

    const double dist = distance3D(sensorPosition, index);
    if(dist < minDistance)
    {
        minDistance = dist;
        champion = index;
    }
    const qint8 axis = node->m_axis;
    // left or right first ?
    const qint8 subTreeIndx = sensorPosition[axis] < m_surface.rr(index, axis) ? 0 : 1;
    recursiveSearch(sensorPosition, node->m_subTrees[subTreeIndx], champion, minDistance);

    const double diff = std::fabs(sensorPosition[axis] - m_surface.rr(index, axis));
    //search the other subtree if needed
    if(diff < minDistance)
    {
        recursiveSearch(sensorPosition, node->m_subTrees[!subTreeIndx], champion, minDistance);
    }
}
