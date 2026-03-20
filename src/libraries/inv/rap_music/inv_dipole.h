//=============================================================================================================
/**
 * @file     inv_dipole.h
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
 * @brief    InvDipole class declaration for RAP MUSIC dipole results.
 *
 */

#ifndef INV_DIPOLE_H
#define INV_DIPOLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <iostream>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
template<typename T>
class InvDipole;

//=============================================================================================================
/**
 * Declares a InvDipolePair structure consisting of two correlated dipoles which are the result of the RAP MUSIC
 * searching algorithm.
 *
 * @brief Pair of correlated dipole indices and orientations found by the RAP MUSIC scanning step
 */
template<typename T>
struct InvDipolePair
{
    int m_iIdx1;            /**< Index of dipole one. */
    InvDipole<T> m_Dipole1;    /**< InvDipole one. */

    int m_iIdx2;            /**< Index of dipole two. */
    InvDipole<T> m_Dipole2;    /**< InvDipole two. */

    T m_vCorrelation;     /**< Correlation of the dipole pair. */
};

//=============================================================================================================
/**
 * DECLARE CLASS Dipoles
 *
 * @brief Stores position, orientation, and correlation of a single current dipole estimated by RAP MUSIC
 */
template<class T>
class InvDipole
{
//typedef Eigen::Matrix<T, 3, 1> Point3D;

public:

    //=========================================================================================================
    /**
     * Default constructor
     */
    InvDipole();

    //=========================================================================================================
    /**
     * ctor
     *
     * @param[in] p_sDataPath the path to the directory which contains the data folder.
     */
    /*  InvDipole();*/

    //=========================================================================================================
    /**
     * dtor
     * Do garbage collecting
     */
    virtual ~InvDipole();

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
typedef INVLIB::InvDipole<int>     InvDipole_INT;
typedef INVLIB::InvDipole<float>   InvDipole_FLOAT;
typedef INVLIB::InvDipole<double>  InvDipole_DOUBLE;

//Make the template definition visible to compiler in the first point of instantiation
#include "inv_dipole.cpp"

#endif // INV_DIPOLE_H
