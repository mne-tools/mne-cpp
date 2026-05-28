//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file filter_thread_arg.cpp
 * @since March 2026
 * @brief Implementation of @ref MNELIB::FilterThreadArg.
 *
 * Provides constructors and the deep-copy semantics needed when the
 * argument is queued across a thread boundary.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_thread_arg.h"

#include <fiff/fiff_coord_trans.h>

constexpr int FAIL = -1;
constexpr int OK   =  0;

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterThreadArg::FilterThreadArg()
:s          (nullptr)
,limit      (-1)
,filtered   (nullptr)
,stat       (FAIL)
{
}

//=============================================================================================================

FilterThreadArg::~FilterThreadArg()
{
}
