//=============================================================================================================
/**
 * @file     fiff_digitizer_data.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    FiffDigitizerData class declaration.
 *
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
 * Digitizer data including points and transformations.
 *
 * @brief Digitization points container and description.
 */
class FIFFSHARED_EXPORT FiffDigitizerData
{
public:
    typedef QSharedPointer<FiffDigitizerData> SPtr;              /**< Shared pointer type for FiffDigitizerData. */
    typedef QSharedPointer<const FiffDigitizerData> ConstSPtr;   /**< Const shared pointer type for FiffDigitizerData. */

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
    ~FiffDigitizerData() = default;

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
