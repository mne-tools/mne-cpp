//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_thread_arg.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    FwdThreadArg implementation — trivial allocation/cleanup of the per-thread work packet (dipole range, coil set, callback pointers, output view) dispatched by the parallel source-space loop.
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
: res(nullptr)
, res_grad(nullptr)
, off(0)
, field_pot(nullptr)
, vec_field_pot(nullptr)
, field_pot_grad(nullptr)
, coils_els(nullptr)
, client(nullptr)
, s(nullptr)
, fixed_ori(false)
, comp(-1)
, stat(-1)
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
    res->client_free = nullptr; /* Don't copy the source's deleter */
    if (bem_model) {
        auto bem = std::make_shared<FwdBemModel>();
        *bem = *static_cast<FwdBemModel*>(res->client);
        bem->v0.resize(0);
        res->client = bem.get();
        res->client_free = [bem]() {
        }; /* shared_ptr releases FwdBemModel on destruction */
    }
    return res;
}

//=============================================================================================================

FwdThreadArg::UPtr FwdThreadArg::create_meg_multi_thread_duplicate(FwdThreadArg& one, bool bem_model)
{
    auto res = std::make_unique<FwdThreadArg>();
    FwdCompData* orig = static_cast<FwdCompData*>(one.client);

    *res = one;
    res->client_free = nullptr; /* Don't copy the source's deleter */

    // Each thread gets its own compensation set (it is mutated while applying)
    // and work buffers; the compensation coils are read-only and shared.
    auto comp = std::make_shared<FwdCompData>(*orig);
    comp->set = orig->set ? std::make_shared<MNECTFCompDataSet>(*orig->set) : nullptr;
    comp->work.resize(0);
    comp->vec_work.resize(0, 0);

    res->client = comp.get();

    if (bem_model) {
        auto bem = std::make_shared<FwdBemModel>();
        *bem = *static_cast<FwdBemModel*>(comp->client);
        bem->v0.resize(0);
        comp->client = bem.get();
        /* shared_ptrs release their objects when client_free is destroyed */
        res->client_free = [comp, bem]() {
        };
    } else {
        res->client_free = [comp]() {
        };
    }
    return res;
}
