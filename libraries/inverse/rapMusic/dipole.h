//=============================================================================================================
/**
 * @file     dipole.h
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

#ifndef DIPOLE_H
#define DIPOLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <iostream>

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
template<typename T>
class Dipole;

//=============================================================================================================
/**
 * Declares a DipolePair structure consisting of two correlated dipoles which are the result of the RAP MUSIC
 * searching algorithm.
 */
template<typename T>
struct DipolePair
{
    int m_iIdx1;            /**< Index of dipole one. */
    Dipole<T> m_Dipole1;    /**< Dipole one. */

    int m_iIdx2;            /**< Index of dipole two. */
    Dipole<T> m_Dipole2;    /**< Dipole two. */

    T m_vCorrelation;     /**< Correlation of the dipole pair. */
};

//=============================================================================================================
/**
 * DECLARE CLASS Dipoles
 *
 * @brief ToDo
 */
template<class T>
class Dipole
{
//typedef Eigen::Matrix<T, 3, 1> Point3D;

public:

    //=========================================================================================================
    /**
     * Default constructor
     */
    Dipole();

    //=========================================================================================================
    /**
     * ctor
     *
     * @param[in] p_sDataPath the path to the directory which contains the data folder.
     */
    /*  Dipole();*/

    //=========================================================================================================
    /**
     * dtor
     * Do garbage collecting
     */
    virtual ~Dipole();

    inline T& x() { return m_vecPosition[0] ; }
    inline T& y() { return m_vecPosition[1] ; }
    inline T& z() { return m_vecPosition[2] ; }
    inline T x() const { return m_vecPosition[0] ; }
    inline T y() const { return m_vecPosition[1] ; }
    inline T z() const { return m_vecPosition[2] ; }

    inline T& phi_x() { return m_vecDirection[0] ; }
    inline T& phi_y() { return m_vecDirection[1] ; }
    inline T& phi_z() { return m_vecDirection[2] ; }
    inline T phi_x() const { return m_vecDirection[0] ; }
    inline T phi_y() const { return m_vecDirection[1] ; }
    inline T phi_z() const { return m_vecDirection[2] ; }

    //=========================================================================================================
    /**
     * clean
     */
    void clean();

protected:

private:

    Eigen::Matrix<T, 3, 1> m_vecPosition;
    Eigen::Matrix<T, 3, 1> m_vecDirection;

    double  m_dLength;
    double  m_dFrequency;

    //TGreensFunction* green;
};
} // NAMESPACE

//TypeDefs
typedef INVERSELIB::Dipole<int>     Dipole_INT;
typedef INVERSELIB::Dipole<float>   Dipole_FLOAT;
typedef INVERSELIB::Dipole<double>  Dipole_DOUBLE;

//Make the template definition visible to compiler in the first point of instantiation
#include "dipole.cpp"

#endif // DIPOLE_H
