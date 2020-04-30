//=============================================================================================================
/**
 * @file     mne_cluster_info.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEClusterInfo class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_cluster_info.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEClusterInfo::MNEClusterInfo()
{
}

//=============================================================================================================

void MNEClusterInfo::clear()
{
    clusterLabelNames.clear();
    clusterLabelIds.clear();
    centroidVertno.clear();
    centroidSource_rr.clear();
    clusterVertnos.clear();
    clusterSource_rr.clear();
    clusterDistances.clear();
}

//=============================================================================================================

void MNEClusterInfo::write(QString p_sFileName) const
{
    QFile file("./"+p_sFileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug("Unable to open file.");
        return;
    }
    QTextStream out(&file);
    out << "MNE Cluster Info\n";

    for(qint32 i = 0; i < clusterLabelIds.size(); ++i)
    {
        out << "\nLabel : " << clusterLabelNames[i] << "\n";
        out << "Label ID : " << clusterLabelIds[i] << "\n";
        out << "Centroid Vertno : " << centroidVertno[i] << "\n";
        out << "Centroid rr : " << centroidSource_rr[i](0) << ", " << clusterSource_rr[i](1) << ", " << clusterSource_rr[i](2) << "\n";
        out << "Vertnos :\n";
        for(qint32 j = 0; j < clusterVertnos[i].size(); ++j)
            out << clusterVertnos[i][j] << ", ";
        out << "\nDistances :\n";
        for(qint32 j = 0; j < clusterDistances[i].size(); ++j)
            out << clusterDistances[i][j] << ", ";
        out << "\nrr :\n";
        for(qint32 j = 0; j < clusterSource_rr[i].rows(); ++j)
            out << clusterSource_rr[i](j,0) << ", " << clusterSource_rr[i](j,1) << ", " << clusterSource_rr[i](j,2) << "\n";

        out << "\n";
    }

    // optional, as QFile destructor will already do it:
    file.close();

    QFile file_centroids("./centroids_"+p_sFileName);

    if(file_centroids.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out_centroids(&file_centroids);

        for(qint32 i = 0; i < clusterLabelIds.size(); ++i)
            out_centroids << centroidVertno[i] << ", ";

        // optional, as QFile destructor will already do it:
        file_centroids.close();
    }
}
