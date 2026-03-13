//=============================================================================================================
/**
 * @file     fwd.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Fwd class declaration, which provides static wrapper functions for the forward library.
 */
//=============================================================================================================

#ifndef FWD_H
#define FWD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_forward_solution.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * @brief The Fwd class provides static wrapper functions for the forward library.
 */
class FWDSHARED_EXPORT Fwd
{
public:

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~Fwd()
    { }

    //=========================================================================================================
    /**
     * mne_read_forward_solution
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FwdForwardSolution::read static function
     *
     * Reads a forward solution from a fif file
     *
     * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in, out] fwd      A forward solution from a fif file.
     * @param[in] force_fixed   Force fixed source orientation mode? (optional).
     * @param[in] surf_ori      Use surface based source coordinate system? (optional).
     * @param[in] include       Include these channels (optional).
     * @param[in] exclude       Exclude these channels (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool read_forward_solution(QIODevice& p_IODevice,
                                             FwdForwardSolution& fwd,
                                             bool force_fixed = false,
                                             bool surf_ori = false,
                                             const QStringList& include = FIFFLIB::defaultQStringList,
                                             const QStringList& exclude = FIFFLIB::defaultQStringList)
    {
        return FwdForwardSolution::read(p_IODevice,
                                        fwd,
                                        force_fixed,
                                        surf_ori,
                                        include,
                                        exclude);
    }
};

} // NAMESPACE FWDLIB

#endif // FWD_H
