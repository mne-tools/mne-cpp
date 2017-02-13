//=============================================================================================================
/**
* @file     fiff_coord_trans_old.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>


namespace FIFFLIB
{
class FiffTag;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{


//=============================================================================================================
/**
* Implements an Fiff Coordinate Descriptor (Replaces *fiffCoordTrans, fiffCoordTransRec; struct of MNE-C fiff_types.h).
*
* @brief Coordinate transformation descriptor
*/
class INVERSESHARED_EXPORT FiffCoordTransOld
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
    * @param[in] p_FiffCoordTransOld    FiffCoordTransOld which should be copied
    */
    FiffCoordTransOld(const FiffCoordTransOld& p_FiffCoordTransOld);





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





    //============================= make_volume_source_space.c =============================


    /*
    * Add inverse transform to an existing one
    */
    static int add_inverse(FiffCoordTransOld* t);





    //============================= fiff_trans.c =============================
    FiffCoordTransOld* fiff_invert_transform () const;




    static void fiff_coord_trans (float r[3],FiffCoordTransOld* t,int do_move);








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

    //*************************************************************************************************************
    //TODO: remove later on
    static FiffCoordTransOld* read_helper( QSharedPointer<FIFFLIB::FiffTag>& tag );

public:
    FIFFLIB::fiff_int_t   from;             /**< Source coordinate system. */
    FIFFLIB::fiff_int_t   to;               /**< Destination coordinate system. */
    FIFFLIB::fiff_float_t rot[3][3];        /**< The forward transform (rotation part) */
    FIFFLIB::fiff_float_t move[3];          /**< The forward transform (translation part) */
    FIFFLIB::fiff_float_t invrot[3][3];     /**< The inverse transform (rotation part) */
    FIFFLIB::fiff_float_t invmove[3];       /**< The inverse transform (translation part) */

    // ### OLD STRUCT ###
    //typedef struct _fiffCoordTransRec {
    //    fiff_int_t   from;                  /**< Source coordinate system. */
    //    fiff_int_t   to;                    /**< Destination coordinate system. */
    //    fiff_float_t rot[3][3];             /**< The forward transform (rotation part) */
    //    fiff_float_t move[3];               /**< The forward transform (translation part) */
    //    fiff_float_t invrot[3][3];          /**< The inverse transform (rotation part) */
    //    fiff_float_t invmove[3];            /**< The inverse transform (translation part) */
    //} *fiffCoordTrans, fiffCoordTransRec;   /**< Coordinate transformation descriptor */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE INVERSELIB

#endif // FIFFCOORDTRANSOLD_H
