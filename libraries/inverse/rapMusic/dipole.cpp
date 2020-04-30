//=============================================================================================================
/**
 * @file     dipole.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2011
 *
 * @section  LICENSE
 *
 * Copyright (C) 2011, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * No part of this program may be photocopied, reproduced,
 * or translated to another program language without the
 * prior written consent of the author.
 *
 *
 * @brief    ToDo Documentation...
 *
 */

#ifndef DIPOLE_SOURCES //Because this cpp is part of the header -> template
#define DIPOLE_SOURCES

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipole.h"

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template <class T>
Dipole<T>::Dipole()
: m_vecPosition(Matrix<T, 3, 1>::Zero(3))
, m_vecDirection(Matrix<T, 3, 1>::Zero(3))
, m_dLength(1)
, m_dFrequency(0)
{
}

//=============================================================================================================

template <class T>
Dipole<T>::~Dipole()
{
}
}//Namespace

#endif //DIPOLE_SOURCES
