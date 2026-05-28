//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file mne_cluster_info.cpp
 * @since 2022
 * @date  March 2026
 * @brief Implementation of @ref MNELIB::MNEClusterInfo.
 *
 * Implements constructors and the helpers that group source vertices
 * into clusters / undo the grouping when expanding back to the original
 * source space.
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
    QFile file(p_sFileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug("Unable to open file.");
        return;
    }
    QTextStream out(&file);
    out << "MNE Cluster Info\n";

    for(qint32 i = 0; i < clusterLabelIds.size(); ++i)
    {
        out << "\nLabel : " << clusterLabelNames[i] << "\n";
        out << "FsLabel ID : " << clusterLabelIds[i] << "\n";
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
