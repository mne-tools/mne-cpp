//=============================================================================================================
/**
 * @file     fiff_coord_trans_old.h
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
 * @brief    FiffCoordTransOld class declaration.
 *
 */

#ifndef FIFFCOORDTRANSOLD_H
#define FIFFCOORDTRANSOLD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"

#include "../fiff_types.h"
#include "../fiff_stream.h"
#include "../fiff_dir_node.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

namespace FIFFLIB
{
class FiffTag;
}

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Implements an Fiff Coordinate Descriptor (Replaces *fiffCoordTrans, fiffCoordTransRec; struct of MNE-C fiff_types.h).
 *
 * @brief Coordinate transformation descriptor
 */
class FIFFSHARED_EXPORT FiffCoordTransOld
{
public:
    typedef QSharedPointer<FiffCoordTransOld> SPtr;              /**< Shared pointer type for FiffCoordTransOld. */
    typedef QSharedPointer<const FiffCoordTransOld> ConstSPtr;   /**< Const shared pointer type for FiffCoordTransOld. */

    //=========================================================================================================
    /**
     * Constructs the FiffCoordTransOld
     */
    FiffCoordTransOld();

    //=========================================================================================================
    /**
     * Copy constructor.
     * Refactored: fiff_dup_transform (fiff_trans.c)
     *
     * @param[in] p_FiffCoordTransOld    FiffCoordTransOld which should be copied.
     */
    FiffCoordTransOld(const FiffCoordTransOld& p_FiffCoordTransOld);

    static FiffCoordTransOld* catenate(FiffCoordTransOld* t1,FiffCoordTransOld* t2);

    //=========================================================================================================
    /**
     * Compose the coordinate transformation structure from a known forward transform
     * Refactored: fiff_make_transform (make_volume_source_space.c)
     *
     */
    FiffCoordTransOld(int from,int to,float rot[3][3],float move[3]);

    //=========================================================================================================
    /**
     * Destroys the FiffCoordTransOld
     * Refactored:  (.c)
     */
    ~FiffCoordTransOld();

    //=========================================================================================================
    /*
     * Transform Old coord transformation to new class.
     */
    FiffCoordTrans toNew();

    //============================= make_volume_source_space.c =============================

    /*
     * Add inverse transform to an existing one
     */
    static int add_inverse(FiffCoordTransOld* t);

    //============================= fiff_trans.c =============================
    FiffCoordTransOld* fiff_invert_transform () const;

    static void fiff_coord_trans (float r[3], const FiffCoordTransOld* t,int do_move);

    static FiffCoordTransOld* fiff_combine_transforms (int from,int to,FiffCoordTransOld* t1,FiffCoordTransOld* t2);

    static void fiff_coord_trans_inv (float r[3],FiffCoordTransOld* t,int do_move);

    //============================= mne_coord_transforms.c =============================

    static const char *mne_coord_frame_name(int frame);

    static void mne_print_coord_transform_label(FILE *log,char *label, FiffCoordTransOld* t);

    static void mne_print_coord_transform(FILE *log, FiffCoordTransOld* t);

    static FiffCoordTransOld* mne_read_transform(const QString& name,int from, int to);

    static FiffCoordTransOld* mne_read_transform_from_node(//fiffFile in,
                                                           FIFFLIB::FiffStream::SPtr& stream,
                                                           const FIFFLIB::FiffDirNode::SPtr& node,
                                                           int from, int to);

    static FiffCoordTransOld* mne_read_mri_transform(const QString& name);

    static FiffCoordTransOld* mne_read_meas_transform(const QString& name);

    static FiffCoordTransOld* mne_read_transform_ascii(char *name, int from, int to);

    static FiffCoordTransOld* mne_read_FShead2mri_transform(char *name);

    static FiffCoordTransOld* mne_identity_transform(int from, int to);

    static FiffCoordTransOld * fiff_make_transform_card (int from,int to,
                                                     float *rL,
                                                     float *rN,
                                                     float *rR);

    //============================= digitizer.c =============================

    static FiffCoordTransOld* procrustes_align(int   from_frame,  /* The coordinate frames */
                           int   to_frame,
                           float **fromp,     /* Point locations in these two coordinate frames */
                           float **top,
                           float *w,	  /* Optional weights */
                           int   np,	  /* How many points */
                           float max_diff);	  /* Maximum allowed difference */

    //=============================================================================================================
    //TODO: remove later on
    static FiffCoordTransOld* read_helper( QSharedPointer<FIFFLIB::FiffTag>& tag );

    //========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffCoordTransOld &a, const FiffCoordTransOld &b);

public:
    FIFFLIB::fiff_int_t from;       /**< Source coordinate system. */
    FIFFLIB::fiff_int_t to;         /**< Destination coordinate system. */
    Eigen::Matrix3f     rot;        /**< The forward transform (rotation part). */
    Eigen::Vector3f     move;       /**< The forward transform (translation part). */
    Eigen::Matrix3f     invrot;     /**< The inverse transform (rotation part). */
    Eigen::Vector3f     invmove; /**< The inverse transform (translation part). */

    // ### OLD STRUCT ###
    //typedef struct _fiffCoordTransRec {
    //    fiff_int_t   from;                  /**< Source coordinate system. */
    //    fiff_int_t   to;                    /**< Destination coordinate system. */
    //    fiff_float_t rot[3][3];             /**< The forward transform (rotation part). */
    //    fiff_float_t move[3];               /**< The forward transform (translation part). */
    //    fiff_float_t invrot[3][3];          /**< The inverse transform (rotation part). */
    //    fiff_float_t invmove[3];            /**< The inverse transform (translation part). */
    //} *fiffCoordTrans, fiffCoordTransRec;   /**< Coordinate transformation descriptor. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool operator== (const FiffCoordTransOld &a, const FiffCoordTransOld &b)
{
    return (a.from == b.from &&
            a.to == b.to &&
            a.rot.isApprox(b.rot, 0.0001f) &&
            a.move.isApprox(b.move, 0.0001f) &&
            a.invrot.isApprox(b.invrot, 0.0001f) &&
            a.invmove.isApprox(b.invmove, 0.0001f));
}
} // NAMESPACE FIFFLIB

#endif // FIFFCOORDTRANSOLD_H
