//=============================================================================================================
/**
 * @file     fs_atlas_lookup.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    FsAtlasLookup class — FreeSurfer volume parcellation atlas lookup.
 *
 */

#ifndef FS_ATLAS_LOOKUP_H
#define FS_ATLAS_LOOKUP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//=============================================================================================================
/**
 * @brief Atlas lookup for FreeSurfer volume parcellations (e.g. aparc+aseg.mgz).
 *
 * Loads a FreeSurfer MGH/MGZ volume and provides label lookup at RAS coordinates.
 */
class FSSHARED_EXPORT FsAtlasLookup
{
public:
    //=========================================================================================================
    /**
     * Constructs an empty atlas lookup.
     */
    FsAtlasLookup();

    //=========================================================================================================
    /**
     * @brief Load a FreeSurfer MGH/MGZ volume parcellation.
     * @param[in] sParcellationPath  Path to the volume file (e.g. aparc+aseg.mgz).
     * @return true if loaded successfully.
     */
    bool load(const QString& sParcellationPath);

    //=========================================================================================================
    /**
     * @brief Look up the anatomical label at a RAS coordinate.
     * @param[in] ras  3D position in RAS space (in mm).
     * @return Label string, or "Unknown" if out of bounds or unlabeled.
     */
    QString labelAtRas(const Eigen::Vector3f& ras) const;

    //=========================================================================================================
    /**
     * @brief Look up labels for multiple RAS positions.
     * @param[in] positions  List of 3D RAS positions.
     * @return List of label strings (one per position).
     */
    QStringList labelsForPositions(const QVector<Eigen::Vector3f>& positions) const;

    //=========================================================================================================
    /**
     * @brief Check whether a parcellation volume has been loaded.
     * @return true if loaded.
     */
    bool isLoaded() const;

private:
    QVector<int> m_voxelData;           /**< Flat voxel data (label indices). */
    int m_dimX = 0;                     /**< Volume dimension X. */
    int m_dimY = 0;                     /**< Volume dimension Y. */
    int m_dimZ = 0;                     /**< Volume dimension Z. */

    Eigen::Matrix4f m_ras2vox;          /**< RAS-to-voxel affine (inverse of vox2ras). */

    QMap<int, QString> m_lookupTable;   /**< Label index → region name. */

    bool m_loaded = false;

    //=========================================================================================================
    /**
     * @brief Initialize the FreeSurfer color lookup table.
     */
    void initLookupTable();

    //=========================================================================================================
    /**
     * @brief Convert a RAS coordinate to voxel indices.
     * @param[in] ras  3D RAS position.
     * @return Rounded voxel indices.
     */
    Eigen::Vector3i rasToVoxel(const Eigen::Vector3f& ras) const;
};

} // namespace FSLIB

#endif // FS_ATLAS_LOOKUP_H
