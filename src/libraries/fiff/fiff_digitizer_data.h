//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_digitizer_data.h
 * @since February 2026
 * @brief High-level digitization data: dig points plus the device→head transform and fitting metadata that together define a coregistration source.
 *
 * Where @ref FiffDigPointSet is a flat point list, @ref FiffDigitizerData
 * is the registration-ready view: the dig points themselves, the
 * device→head @ref FiffCoordTrans recovered from HPI fits, the per-coil
 * goodness of fit, and any comments that travelled with the block. It is
 * the structure the coregistration tooling persists into
 * ``-dig.fif`` / ``-fiducials.fif`` files and the structure that the
 * forward / inverse pipeline consumes when registering a subject's MRI to
 * the MEG sensor frame.
 */

#ifndef FIFF_DIGITIZER_DATA_H
#define FIFF_DIGITIZER_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_dig_point.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <memory>

#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffCoordTrans;

//=============================================================================================================
/**
 * @brief Registration-ready digitization data: dig points, device→head transform and HPI fit metadata.
 *
 * Pairs a @ref FiffDigPointSet with the @ref FiffCoordTrans that maps it
 * into the head frame, plus optional HPI goodness-of-fit values. Used by
 * the coregistration GUI and by every downstream tool that needs a single
 * self-contained record describing where a subject's head sat in the
 * helmet for one measurement.
 */
class FIFFSHARED_EXPORT FiffDigitizerData
{
public:
    using SPtr = QSharedPointer<FiffDigitizerData>;            /**< Shared pointer type for FiffDigitizerData. */
    using ConstSPtr = QSharedPointer<const FiffDigitizerData>; /**< Const shared pointer type for FiffDigitizerData. */
    using UPtr = std::unique_ptr<FiffDigitizerData>;             /**< Unique pointer type for FiffDigitizerData. */
    using ConstUPtr = std::unique_ptr<const FiffDigitizerData>;  /**< Const unique pointer type for FiffDigitizerData. */

    //=========================================================================================================
    /**
     * Constructs the FiffDigitizerData.
     */
    FiffDigitizerData();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDigitizerData   Digitization point descriptor which should be copied.
     */
    FiffDigitizerData(const FiffDigitizerData& p_FiffDigitizerData);

    //=========================================================================================================
    /**
     * Copy assignment operator (deep-copies unique_ptr members).
     *
     * @param[in] rhs   Digitization point descriptor to assign from.
     * @return Reference to this object.
     */
    FiffDigitizerData& operator=(const FiffDigitizerData& rhs);

    //=========================================================================================================
    /**
     * Constructs a FiffDigitizerData by reading from an IO device.
     *
     * @param[in] p_IODevice   Input device to read data from.
     */
    FiffDigitizerData(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Destroys the digitization point description.
     */
    ~FiffDigitizerData();

    //=========================================================================================================
    /**
     * Prints class contents.
     */
    void print() const;

    //=========================================================================================================
    /**
     * Returns the number of MRI fiducial points (cardinal points
     * transformed into MRI coordinates via head_mri_t_adj).
     *
     * Kept as nfids() for API compatibility with the original MNE C code
     * (digitizerDataRec.nfids in mne_analyze/analyze_types.h).
     *
     * @return Number of fiducials in mri_fids.
     */
    inline int nfids() const;

    //=========================================================================================================
    /**
     * Extracts cardinal digitizer points (LPA, Nasion, RPA) from the digitizer
     * point list and transforms them into MRI coordinates using head_mri_t_adj,
     * populating the mri_fids list.
     *
     * This is a port of the original C function update_fids_from_dig_data
     * from mne_analyze/adjust_alignment.c.
     */
    void pickCardinalFiducials();


public:
    QString        filename;                 /**< Source file path. */
    std::unique_ptr<FiffCoordTrans> head_mri_t;            /**< Head to MRI coordinate transformation. */
    std::unique_ptr<FiffCoordTrans> head_mri_t_adj;        /**< Adjusted head to MRI transformation. */
    QList<FIFFLIB::FiffDigPoint>   points;           /**< The digitizer points. */
    int            coord_frame;               /**< The coordinate frame of the above points. */
    QList<int>     active;                   /**< Which points are active. */
    QList<int>     discard;                  /**< Which points should be discarded. */
    int            npoint;                    /**< Number of points. */
    QList<FIFFLIB::FiffDigPoint> mri_fids;      /**< MRI coordinate system fiducials. */
    bool           show;                      /**< Whether the digitizer data should be shown. */
    bool           show_minimal;              /**< Show fiducials and coils only. */
    Eigen::VectorXf dist;                     /**< Distance of each point from the head surface. */
    Eigen::VectorXi closest;                  /**< Closest vertex number on the head surface. */
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> closest_point; /**< Closest vertex locations on the head surface (npoint x 3). */
    bool           dist_valid;                /**< Whether the above distance data is valid. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int FiffDigitizerData::nfids() const
{
    return static_cast<int>(mri_fids.size());
}

} // NAMESPACE

#endif // FIFF_DIGITIZER_DATA_H
