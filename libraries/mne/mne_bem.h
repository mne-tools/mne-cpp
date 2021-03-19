//=============================================================================================================
/**
 * @file     mne_bem.h
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEBem class declaration.
 *
 */

#ifndef MNE_BEM_H
#define MNE_BEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_bem_surface.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff.h>

#include <algorithm>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QIODevice>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB
{
class Label;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * BEM descritpion
 *
 * @brief BEM descritpion
 */
class MNESHARED_EXPORT MNEBem
{

public:
    typedef QSharedPointer<MNEBem> SPtr;            /**< Shared pointer type for MNEBem. */
    typedef QSharedPointer<const MNEBem> ConstSPtr; /**< Const shared pointer type for MNEBem. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNEBem();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNEBem   MNE BEM.
     */
    MNEBem(const MNEBem &p_MNEBem);

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNEBem(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Destroys the MNE Bem
     */
    ~MNEBem();

    //=========================================================================================================
    /**
     * Initializes MNE Bem
     */
    void clear();

    //=========================================================================================================
    /**
     * True if MNE Bem is empty.
     *
     * @return true if MNE Bem is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_read_bem_surface function
     *
     * Reads Bem surface from a fif file
     *
     * @param[in, out] p_pStream     The opened fif file.
     * @param[in] add_geom          Add geometry information to the Bem Surface.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool readFromStream(FIFFLIB::FiffStream::SPtr& p_pStream, bool add_geom, MNEBem &p_Bem);

    //=========================================================================================================
    /**
     * Returns the number of stored bem surfaces
     *
     * @return number of stored bem surfaces.
     */
    inline qint32 size() const;

    //=========================================================================================================
    /**
     * MNE Toolbox function mne_write_bem_surfaces_block
     *
     * Write the Bem to a FIF file
     *
     * @param[in] p_IODevice   IO device to write the bem to.
     */
    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * MNE Toolbox function mne_write_bem_surfaces_block
     *
     * Write the Bem to a FIF stream
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FIFFLIB::FiffStream *p_pStream);

    //=========================================================================================================
    /**
     * Subscript operator [] to access bem_surface by index
     *
     * @param[in] idx    the surface index (0,1 or 2).
     *
     * @return MNEBemSurface related to the parameter index.
     */
    const MNEBemSurface& operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access bem_surface by index
     *
     * @param[in] idx    the surface index (0,1 or 2).
     *
     * @return MNEBemSurface related to the parameter index.
     */
    MNEBemSurface& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator << to add a new bem_surface
     *
     * @param[in] surf   BemSurface to be added.
     *
     * @return MNEBem.
     */
    MNEBem& operator<< (const MNEBemSurface& surf);

    //=========================================================================================================
    /**
     * Subscript operator << to add a new bem_surface
     *
     * @param[in] surf   BemSurface to be added.
     *
     * @return MNEBem.
     */
    MNEBem& operator<< (const MNEBemSurface* surf);

    //=========================================================================================================
    /**
     * Warp the Bem
     *
     * @param[in] sLm       3D Landmarks of the source geometry.
     * @param[in] dLm       3D Landmarks of the destination geometry.
     */
    void warp(const Eigen::MatrixXf &sLm, const Eigen::MatrixXf &dLm);

    //=========================================================================================================
    /**
     * Transform the Bem
     *
     * @param[in] trans     The Transformation Matrix.
     */
    void transform(const FIFFLIB::FiffCoordTrans& trans);

    //=========================================================================================================
    /**
     * Transform the Bem using the inverse
     *
     * @param[in] trans     The Transformation Matrix.
     */
    void invtransform(const FIFFLIB::FiffCoordTrans& trans);

protected:
    //=========================================================================================================
    /**
     * Definition of the read_bem_surface function in e.g. mne_read_bem_surface.m
     * Reads a single bem surface
     *
     * @param[in] p_pStream         The opened fif file.
     * @param[in] p_Tree            Search for the bem surface here.
     * @param[in, out] p_BemSurface     The read BemSurface.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool readBemSurface(FIFFLIB::FiffStream::SPtr& p_pStream, const FIFFLIB::FiffDirNode::SPtr& p_Tree, MNEBemSurface& p_BemSurface);

private:
    QList<MNEBemSurface> m_qListBemSurface;    /**< List of the BEM Surfaces. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEBem::isEmpty() const
{
    return m_qListBemSurface.size() == 0;
}

//=============================================================================================================

inline qint32 MNEBem::size() const
{
    return m_qListBemSurface.size();
}
} // NAMESPACE

#ifndef metatype_bem
#define metatype_bem
Q_DECLARE_METATYPE(MNELIB::MNEBem); /**< Provides QT META type declaration of the FMNELIB::MNEBem type. For signal/slot usage.*/
#endif

#ifndef metatype_bem
#define metatype_bem
Q_DECLARE_METATYPE(MNELIB::MNEBem::SPtr); /**< Provides QT META type declaration of the MNELIB::MNEBem type. For signal/slot usage.*/
#endif

#endif // MNE_BEM_H
