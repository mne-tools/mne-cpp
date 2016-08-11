//=============================================================================================================
/**
* @file     mneprojecttosurface.h
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     %{currentdate} Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief     MNEProjectToSurface class declaration.
*
*/

#ifndef MNELIB_MNEPROJECTTOSURFACE_H
#define MNELIB_MNEPROJECTTOSURFACE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB {


//*************************************************************************************************************
//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNEBemSurface;
class MNESurface;


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class MNEProjectToSurface
{

public:
    typedef QSharedPointer<MNEProjectToSurface> SPtr;            /**< Shared pointer type for MNEProjectToSurface. */
    typedef QSharedPointer<const MNEProjectToSurface> ConstSPtr; /**< Const shared pointer type for MNEProjectToSurface. */

    //=========================================================================================================
    /**
    * Constructs a MNEProjectToSurface object.
    */
    MNEProjectToSurface();

    //=========================================================================================================
    /**
    * Constructs a MNEProjectToSurface object.
    */
    MNEProjectToSurface(const MNELIB::MNEBemSurface &p_MNEBemSurf);

    //=========================================================================================================
    /**
    * Constructs a MNEProjectToSurface object.
    */
    MNEProjectToSurface(const MNELIB::MNESurface &p_MNESurf);

//    //=========================================================================================================
//    /**
//     * @brief mne_triangle_coords
//     * @param r
//     * @param tri
//     * @param rTri
//     * @return
//     */
//    bool mne_triangle_coords(const Eigen::Vector3f *r, const int tri, Eigen::Vector3f *rTri);

    //=========================================================================================================
    /**
     * @brief mne_find_closest_on_surface
     * @param r
     * @param rTri
     * @param nearest
     * @param dist
     * @return
     */
    bool mne_find_closest_on_surface(const Eigen::MatrixX3f &r, const int ntri, Eigen::MatrixX3f &rTri,
                                     Eigen::VectorXi &nearest, Eigen::Vector3f &dist);

protected:

private:
    //=========================================================================================================
    /**
     * @brief mne_project_to_surface
     * @param r
     * @param rTri
     */
    bool mne_project_to_surface(const Eigen::Vector3f &r, Eigen::Vector3f &rTri, int bestTri);

    //=========================================================================================================
    /**
     * @brief nearest_triangle_point
     * @param r
     * @param p
     * @param q
     * @param dist
     * @return
     */

    bool nearest_triangle_point(const Eigen::Vector3f &r, const int tri, float p, float q, float dist);

    //=========================================================================================================
    /**
     * @brief project_to_triangle
     * @param rTri
     * @param p
     * @param q
     * @param tri
     */
    bool project_to_triangle(Eigen::Vector3f &rTri, const float p, const float q, const int tri);

    Eigen::MatrixX3f r1;         /**< Cartesian Vector to the first triangel corner */
    Eigen::MatrixX3f r12;        /**< Cartesian Vector from the first to the second triangel corner */
    Eigen::MatrixX3f r13;        /**< Cartesian Vector from the first to the third triangel corner */
    Eigen::MatrixX3f nn;         /**< Cartesian Vector of the triangle plane normal */
    Eigen::Vector3f a;           /**< r12*r12 */
    Eigen::Vector3f b;           /**< r13*r13 */
    Eigen::Vector3f c;           /**< r12*r13 */
    Eigen::Vector3f det;         /**< Determinant of the Matrix [a c, c b] */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace MNELIB

#endif // MNELIB_MNEPROJECTTOSURFACE_H
