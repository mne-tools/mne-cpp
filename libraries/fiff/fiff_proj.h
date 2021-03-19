//=============================================================================================================
/**
 * @file     fiff_proj.h
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
 * @brief    FiffProj class declaration.
 *
 */

#ifndef FIFF_PROJ_H
#define FIFF_PROJ_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_named_matrix.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Provides SSP projector data.
 *
 * @brief SSP projector data.
 */
class FIFFSHARED_EXPORT FiffProj {

public:
    typedef QSharedPointer<FiffProj> SPtr;              /**< Shared pointer type for FiffProj. */
    typedef QSharedPointer<const FiffProj> ConstSPtr;   /**< Const shared pointer type for FiffProj. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    FiffProj();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffProj  SSP projector data which should be copied.
     */
    FiffProj(const FiffProj& p_FiffProj);

    //=========================================================================================================
    /**
     * Constructor
     */
    explicit FiffProj(fiff_int_t p_kind,
                      bool p_active,
                      QString p_desc,
                      FiffNamedMatrix& p_data);

    //=========================================================================================================
    /**
     * Destroys the FiffProj.
     */
    ~FiffProj();

    //=========================================================================================================
    /**
     * Set all projections to active
     *
     * @param[in, out] p_qListFiffProj  activates projectors in place.
     */
    static void activate_projs(QList<FiffProj> &p_qListFiffProj);

    //=========================================================================================================
    /**
     * mne_make_projector
     *
     * ToDo move this to fiff_proj; Before: check if info is needed and if make_projector_info should be also moved.
     *
     * ### MNE toolbox root function ### Definition of the mne_make_projector function
     *
     * Make an SSP operator
     *
     * @param[in] projs      A set of projection vectors.
     * @param[in] ch_names   A cell array of channel names.
     * @param[out] proj      The projection operator to apply to the data.
     * @param[in] bads       Bad channels to exclude.
     * @param[out] U         The orthogonal basis of the projection vectors (optional).
     *
     * @return nproj - How many items in the projector.
     */
    static fiff_int_t make_projector(const QList<FiffProj>& projs,
                                     const QStringList& ch_names,
                                     Eigen::MatrixXd& proj,
                                     const QStringList& bads = defaultQStringList,
                                     Eigen::MatrixXd& U = defaultMatrixXd);

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out           The stream to which the fiff projector should be assigned to.
     * @param[in] p_FiffProj    Fiff projector which should be assigned to the stream.
     *
     * @return the stream with the attached fiff projector.
     */
    friend std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffProj &p_FiffProj);

public:
    fiff_int_t kind;                /**< Fiff kind. */
    bool active;                    /**< If fiff projector active. */
    QString desc;                   /**< Projector description. */

    FiffNamedMatrix::SDPtr data;    /**< Projector data, rows are equal to the length of channels. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffProj &p_FiffProj)
{
    out << "#### Fiff Projector ####\n";
    out << "\tKind: " << p_FiffProj.kind << std::endl;
    out << "\tactive: " << p_FiffProj.active << std::endl;
    out << "\tdesc: " << p_FiffProj.desc.toUtf8().constData() << std::endl;
    out << "\tdata:\n\t" << *p_FiffProj.data.data() << std::endl;
    return out;
}
} // NAMESPACE

#endif // FIFF_PROJ_H
