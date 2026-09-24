//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_rt_commands.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Contains mne rt constants
 */

#ifndef MNE_RT_COMMANDS_H
#define MNE_RT_COMMANDS_H

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// MNE RT Communication Constants
//=============================================================================================================

#define MNE_RT_GET_CLIENT_ID        1       /**< Request client id at mne_rt_server. */
#define MNE_RT_SET_CLIENT_ALIAS     2       /**< Set client alias at mne_rt_server. */
} // NAMESPACE

#endif // MNE_RT_COMMANDS_H
