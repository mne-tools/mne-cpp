//=============================================================================================================
/**
 * @file     mne_sss_data.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNE SSS Data (MneSssData) class declaration.
 *
 */

#ifndef MNESSSDATA_H
#define MNESSSDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffStream;
    class FiffDirNode;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Implements MNE SSS Data (Replaces *mneSssData,mneSssDataRec struct of MNE-C mne_types.h).
 *
 * @brief MNE SSS Data description
 */
class MNESHARED_EXPORT MneSssData
{
public:
    typedef QSharedPointer<MneSssData> SPtr;              /**< Shared pointer type for MneSssData. */
    typedef QSharedPointer<const MneSssData> ConstSPtr;   /**< Const shared pointer type for MneSssData. */

    //=========================================================================================================
    /**
     * Constructs the MNE SSS Data
     */
    MneSssData();

    //=========================================================================================================
    /**
     * Copy constructor.
     * Refactored: mne_dup_sss_data (mne_sss_data.c)
     *
     * @param[in] p_MneSssData   MNE SSS Data which should be copied.
     */
    MneSssData(const MneSssData& p_MneSssData);

    //=========================================================================================================
    /**
     * Destroys the MNE SSS Data description
     */
    ~MneSssData();

    //=========================================================================================================
    /**
     * Read SSS data from anywhere in a file
     * Refactored: mne_read_sss_data (mne_sss_data.c)
     *
     * @param[in] name       Name of the file to read the SSS data from.
     *
     * @return   The read SSS data.
     */
    static MneSssData *read_sss_data(const QString& name);

    //=========================================================================================================
    /**
     * Read the SSS data from the given node of an open fiff stream
     * Refactored: mne_read_sss_data_from_node (mne_sss_data.c)
     *
     * @param[in] stream     The open fiff stream.
     * @param[in] start      The node/tree to read the SSS data from.
     *
     * @return   The read SSS data.
     */
    static MneSssData* read_sss_data_from_node( QSharedPointer<FIFFLIB::FiffStream>& stream, const QSharedPointer<FIFFLIB::FiffDirNode>& start );

    //=========================================================================================================
    /**
     * Output the SSS information for debugging purposes
     * Refactored: mne_print_sss_data (mne_sss_data.c)
     *
     * @param[in] f      the file stream to print to;.
     */
    void print(FILE *f) const;

public:
    int   job;          /**< Value of FIFF_SSS_JOB tag. */
    int   coord_frame;  /**< Coordinate frame. */
    float origin[3];    /**< The expansion origin. */
    int   nchan;        /**< How many channels. */
    int   out_order;    /**< Order of the outside expansion. */
    int   in_order;     /**< Order of the inside expansion. */
    int*  comp_info;     /**< Which components are included. */
    int   ncomp;        /**< How many entries in the above. */
    int   in_nuse;      /**< How many components included in the inside expansion. */
    int   out_nuse;     /**< How many components included in the outside expansion. */

// ### OLD STRUCT ###
//typedef struct {
//    int   job;            /* Value of FIFF_SSS_JOB tag */
//    int   coord_frame;    /* Coordinate frame */
//    float origin[3];      /* The expansion origin */
//    int   nchan;          /* How many channels */
//    int   out_order;      /* Order of the outside expansion */
//    int   in_order;       /* Order of the inside expansion */
//    int   *comp_info;     /* Which components are included */
//    int   ncomp;          /* How many entries in the above */
//    int   in_nuse;        /* How many components included in the inside expansion */
//    int   out_nuse;       /* How many components included in the outside expansion */
//} *mneSssData,mneSssDataRec;	/* Essential information about SSS */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNESSSDATA_H
