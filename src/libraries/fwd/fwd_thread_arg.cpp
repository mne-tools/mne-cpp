//=============================================================================================================
/**
 * @file     fwd_thread_arg.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FwdThreadArg class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_thread_arg.h"
#include <mne/mne_source_space.h>
#include "fwd_coil_set.h"
#include "fwd_bem_model.h"
#include "fwd_comp_data.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdThreadArg::FwdThreadArg()
:res           (nullptr)
,res_grad      (nullptr)
,off           (0)
,field_pot     (nullptr)
,vec_field_pot (nullptr)
,field_pot_grad(nullptr)
,coils_els     (nullptr)
,client        (nullptr)
,s             (nullptr)
,fixed_ori     (false)
,stat          (-1)
,comp          (-1)
{
}

//=============================================================================================================

FwdThreadArg::~FwdThreadArg()
{
    if (client_free)
        client_free();
}

//=============================================================================================================

FwdThreadArg::UPtr FwdThreadArg::create_eeg_multi_thread_duplicate(FwdThreadArg& one, bool bem_model)
{
    auto res = std::make_unique<FwdThreadArg>();

     *res = one;
    res->client_free = nullptr;  /* Don't copy the source's deleter */
    if (bem_model) {
        auto bem = std::make_shared<FwdBemModel>();
        *bem = *static_cast<FwdBemModel*>(res->client);
        bem->v0.resize(0);
        res->client = bem.get();
        res->client_free = [bem]() {};  /* shared_ptr releases FwdBemModel on destruction */
    }
    return res;
}

//=============================================================================================================

FwdThreadArg::UPtr FwdThreadArg::create_meg_multi_thread_duplicate(FwdThreadArg& one, bool bem_model)
{
    auto res = std::make_unique<FwdThreadArg>();
    FwdCompData*  orig = static_cast<FwdCompData*>(one.client);

     *res = one;
    res->client_free = nullptr;  /* Don't copy the source's deleter */

    auto comp = std::make_shared<FwdCompData>();
    *comp = *orig;
    comp->comp_coils = nullptr;  /* Non-owning: shared with original, prevent ~FwdCompData from deleting */
    comp->set        = nullptr;  /* Will be replaced below; prevent dtor from deleting orig's copy */
    comp->work.resize(0);
    comp->vec_work.resize(0, 0);

    std::shared_ptr<MNECTFCompDataSet> set_guard(
        orig->set ? new MNECTFCompDataSet(*(orig->set)) : nullptr);
    comp->set = set_guard.get();

    res->client = comp.get();

    if (bem_model) {
        auto bem = std::make_shared<FwdBemModel>();
        *bem = *static_cast<FwdBemModel*>(comp->client);
        bem->v0.resize(0);
        comp->client = bem.get();
        /* shared_ptrs release their objects when client_free is destroyed */
        res->client_free = [comp, set_guard, bem]() {
            comp->set = nullptr;  /* Prevent ~FwdCompData double-free; set_guard owns it */
        };
    }
    else {
        res->client_free = [comp, set_guard]() {
            comp->set = nullptr;  /* Prevent ~FwdCompData double-free; set_guard owns it */
        };
    }
    return res;
}
