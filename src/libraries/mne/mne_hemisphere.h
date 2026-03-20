//=============================================================================================================
/**
 * @file     mne_hemisphere.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     MNEHemisphere class declaration.
 *
 */

#ifndef MNE_HEMISPHERE_H
#define MNE_HEMISPHERE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_source_space.h"
#include "mne_cluster_info.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Hemisphere source space geometry information
 *
 * @brief Hemisphere provides geometry information
 */
class MNESHARED_EXPORT MNEHemisphere : public MNESourceSpace
{
public:
    using SPtr = std::shared_ptr<MNEHemisphere>;             /**< Shared pointer type for MNEHemisphere. */
    using ConstSPtr = std::shared_ptr<const MNEHemisphere>;  /**< Const shared pointer type for MNEHemisphere. */

    //=========================================================================================================
    /**
     * Constructors the hemisphere source space.
     */
    MNEHemisphere();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNEHemisphere    Hemisphere source space which should be copied.
     */
    MNEHemisphere(const MNEHemisphere& p_MNEHemisphere);

    //=========================================================================================================
    /**
     * Destroys the hemisphere source space.
     */
    ~MNEHemisphere() override;

    //=========================================================================================================
    /**
     * Creates a deep copy of this hemisphere, preserving the MNEHemisphere type.
     *
     * @return a shared_ptr<MNESourceSpace> pointing to a cloned MNEHemisphere.
     */
    MNESourceSpace::SPtr clone() const override;

    //=========================================================================================================
    /**
     * Add vertex normals and neighbourhood information
     *
     * @param[in, out] p_pHemisphere   Hemisphere to be completed.
     *
     * @return true if succeeded, false otherwise.
     */
    bool add_geometry_info();

    //=========================================================================================================
    /**
     * Complete triangulation information (triangle centers, normals, areas)
     * for both the main and use triangulations.
     *
     * @return true if succeeded, false otherwise.
     */
    bool complete_source_space_info();

    //=========================================================================================================
    /**
     * Compute patch statistics from the nearest-vertex data. Fills
     * pinfo (per-patch vertex lists) and patch_inds (patch index per vertex).
     *
     * @return true if patch info was computed, false if nearest data is empty.
     */
    bool compute_patch_info();

    //=========================================================================================================
    /**
     * Initializes the hemisphere source space.
     */
    void clear();

    //=========================================================================================================
    /**
     * Qt 3d geometry information. Data are generated within first call.
     *
     * @param[in] p_fScaling  Scale factor of the returned geometry tri model.
     *
     * @return the geometry model.
     */
    Eigen::MatrixXf& getTriCoords(float p_fScaling = 1.0f);

    //=========================================================================================================
    /**
     * is hemisphere clustered?
     *
     * @return true if hemisphere is clustered, false otherwise.
     */
    inline bool isClustered() const;

    //=========================================================================================================
    /**
     * mne_transform_source_space_to
     *
     * ### MNE toolbox root function ###
     *
     * Definition of the mne_transform_source_space_to for a single hemisphere function
     * Transform source space data to the desired coordinate system.
     *
     * @param[in] dest       The id of the destination coordinate system (FIFFV_COORD_...).
     * @param[in] p_Trans    The coordinate transformation structure to use.
     *
     * @return true if succeeded, false otherwise.
     */
    bool transform_hemisphere_to(FIFFLIB::fiff_int_t dest, const FIFFLIB::FiffCoordTrans &p_Trans);

    //=========================================================================================================
    /**
     * mne_python _write_one_source_space
     *
     * ### MNE toolbox root function ###
     *
     * Write the hemisphere to a FIF stream
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FIFFLIB::FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * Copy assignment operator.
     *
     * @param[in] other    The MNEHemisphere to assign from.
     *
     * @return Reference to this object.
     */
    MNEHemisphere& operator=(const MNEHemisphere& other);

    //ToDo write(IODevice &)

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const MNEHemisphere &a, const MNEHemisphere &b);

public:
    // --- Fields inherited from MNESurfaceOrVolume via MNESourceSpace ---
    // type, id, np, ntri, coord_frame, rr, nn, nuse, inuse, vertno,
    // nuse_tri, dist_limit, dist, nearest, neighbor_tri, neighbor_vert
    // are all available through the base class.
    // Use nearestVertIdx() / nearestDistVec() accessors for VectorXi/VectorXd views.

    // --- Shadowing fields (different type from base — TODO: unify in Phase 2) ---
    // tris / use_tris removed: now use inherited itris / use_itris (TrianglesT, row-major)
    // nearest / nearest_dist removed: now use inherited vector<MNENearest> nearest
    //   + nearestVertIdx() / nearestDistVec() accessors
    // dist removed: now use inherited FiffSparseMatrix dist + toEigenSparse() / fromEigenSparse()
    QList<Eigen::VectorXi> pinfo;       /**< Patch information (using option -cps during mne_setup_source_space). */
    Eigen::VectorXi patch_inds;         /**< List of neighboring vertices in the high resolution triangulation. */

    // --- MNEHemisphere-specific fields ---
    Eigen::MatrixX3d tri_cent;          /**< Triangle centers. */
    Eigen::MatrixX3d tri_nn;            /**< Triangle normals. */
    Eigen::VectorXd tri_area;           /**< Triangle areas. */
    Eigen::MatrixX3d use_tri_cent;      /**< Triangle centers of used triangles. */
    Eigen::MatrixX3d use_tri_nn;        /**< Triangle normals of used triangles. */
    Eigen::VectorXd use_tri_area;       /**< Triangle areas of used triangles. */

    MNEClusterInfo cluster_info; /**< Holds the cluster information. */
private:
    // Newly added
    Eigen::MatrixXf m_TriCoords; /**< Holds the rr tri Matrix transformed to geometry data. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEHemisphere::isClustered() const
{
    return !cluster_info.isEmpty();
}

//=============================================================================================================

inline bool operator== (const MNEHemisphere &a, const MNEHemisphere &b)
{
    if(a.pinfo.size() == b.pinfo.size()) {
        for(int i = 0; i < a.pinfo.size(); ++i) {
            if(!a.pinfo.at(i).isApprox(b.pinfo.at(i))) {
                return false;
            }
        }
    } else {
        return false;
    }

    return (a.type == b.type &&
            a.id == b.id &&
            a.np == b.np &&
            a.ntri == b.ntri &&
            a.coord_frame == b.coord_frame &&
            a.rr.isApprox(b.rr, 0.0001f) &&
            a.nn.isApprox(b.nn, 0.0001f) &&
            a.itris.isApprox(b.itris) &&
            a.nuse == b.nuse &&
            a.inuse.isApprox(b.inuse) &&
            a.vertno.isApprox(b.vertno) &&
            a.nuse_tri == b.nuse_tri &&
            a.use_itris.isApprox(b.use_itris) &&
            a.nearestVertIdx().isApprox(b.nearestVertIdx()) &&
            a.nearestDistVec().isApprox(b.nearestDistVec(), 0.0001) &&
            a.patch_inds.isApprox(b.patch_inds) &&
            //a.dist_limit == b.dist_limit && //TODO: We still not sure if dist_limit can also be a matrix. This needs to be debugged
            [&a, &b]() {
                auto sa = a.dist.toEigenSparse();
                auto sb = b.dist.toEigenSparse();
                if (sa.rows() == 0 && sb.rows() == 0 && sa.cols() == 0 && sb.cols() == 0)
                    return true;
                return sa.isApprox(sb, 0.0001);
            }() &&
            a.tri_cent.isApprox(b.tri_cent, 0.0001) &&
            a.tri_nn.isApprox(b.tri_nn, 0.0001) &&
            a.tri_area.isApprox(b.tri_area, 0.0001) &&
            a.use_tri_cent.isApprox(b.use_tri_cent, 0.0001) &&
            a.use_tri_nn.isApprox(b.use_tri_nn, 0.0001) &&
            a.use_tri_area.isApprox(b.use_tri_area, 0.0001) &&
            [&a, &b]() {
                if (a.neighbor_tri.size() != b.neighbor_tri.size()) return false;
                for (size_t i = 0; i < a.neighbor_tri.size(); ++i) {
                    if (a.neighbor_tri[i].size() != b.neighbor_tri[i].size()) return false;
                    if (a.neighbor_tri[i].size() > 0 && !(a.neighbor_tri[i].array() == b.neighbor_tri[i].array()).all()) return false;
                }
                return true;
            }() &&
            [&a, &b]() {
                if (a.neighbor_vert.size() != b.neighbor_vert.size()) return false;
                for (size_t i = 0; i < a.neighbor_vert.size(); ++i) {
                    if (a.neighbor_vert[i].size() != b.neighbor_vert[i].size()) return false;
                    if (a.neighbor_vert[i].size() > 0 && !(a.neighbor_vert[i].array() == b.neighbor_vert[i].array()).all()) return false;
                }
                return true;
            }() &&
            a.cluster_info == b.cluster_info &&
            a.m_TriCoords.isApprox(b.m_TriCoords, 0.0001f));
}
} // NAMESPACE

#endif // MNE_HEMISPHERE_H
