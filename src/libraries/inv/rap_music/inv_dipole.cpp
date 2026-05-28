//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_dipole.cpp
 * @since March 2026
 * @brief Template definitions for @ref INVLIB::InvDipole<T> (included from the matching header).
 *
 * Provides the explicit ctor / dtor and the @c clean reset method for
 * @ref INVLIB::InvDipole<T>. This file is included from
 * @c inv_dipole.h rather than compiled standalone so the templates are
 * available to every translation unit that uses them without an
 * explicit instantiation list.
 */

#ifndef DIPOLE_SOURCES //Because this cpp is part of the header -> template
#define DIPOLE_SOURCES

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_dipole.h"

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template <class T>
InvDipole<T>::InvDipole()
: m_vecPosition(Matrix<T, 3, 1>::Zero(3))
, m_vecDirection(Matrix<T, 3, 1>::Zero(3))
, m_dLength(1)
, m_dFrequency(0)
{
}

//=============================================================================================================

template <class T>
InvDipole<T>::~InvDipole()
{
}

//=============================================================================================================

template <class T>
void InvDipole<T>::clean()
{
    m_vecPosition.setZero();
    m_vecDirection.setZero();
    m_dLength = 1;
    m_dFrequency = 0;
}
}//Namespace

#endif //DIPOLE_SOURCES
