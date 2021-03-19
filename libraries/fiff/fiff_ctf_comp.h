//=============================================================================================================
/**
 * @file     fiff_ctf_comp.h
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
 * @brief    FiffCtfComp class declaration.
 *
 */

#ifndef FIFF_CTF_COMP_H
#define FIFF_CTF_COMP_H

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
 * CTF software compensation data
 *
 * @brief CTF software compensation data
 */
class FIFFSHARED_EXPORT FiffCtfComp {

public:
    typedef QSharedPointer<FiffCtfComp> SPtr;               /**< Shared pointer type for FiffCtfComp. */
    typedef QSharedPointer<const FiffCtfComp> ConstSPtr;    /**< Const shared pointer type for FiffCtfComp. */

    //=========================================================================================================
    /**
     * Constructs the CTF software compensation data
     */
    FiffCtfComp();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffCtfComp   CTF software compensation data which should be copied.
     */
    FiffCtfComp(const FiffCtfComp &p_FiffCtfComp);

    //=========================================================================================================
    /**
     * Destroys the CTF software compensation data.
     */
    ~FiffCtfComp();

    //=========================================================================================================
    /**
     * Initializes the CTF software compensation data.
     */
    void clear();

public:
    fiff_int_t ctfkind;             /**< CTF kind. */
    fiff_int_t kind;                /**< Fiff kind -> fiff_constants.h. */
    bool save_calibrated;           /**< If data should be safed calibrated. */
    Eigen::MatrixXd rowcals;        /**< Row calibrations. */
    Eigen::MatrixXd colcals;        /**< Colum calibrations. */
    FiffNamedMatrix::SDPtr data;    /**< Compensation data. */
};
} // NAMESPACE

#endif // FIFF_CTF_COMP_H
