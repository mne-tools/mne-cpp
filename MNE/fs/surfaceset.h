//=============================================================================================================
/**
* @file     surfaceset.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    SurfaceSet class declaration
*
*/

#ifndef SURFACESET_H
#define SURFACESET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "surface.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* The set of surfaces holds right and left hemipshere surfaces
*
* @brief A hemisphere set of surfaces
*/
class FSSHARED_EXPORT SurfaceSet
{
public:
    typedef QSharedPointer<SurfaceSet> SPtr;            /**< Shared pointer type for SurfaceSet class. */
    typedef QSharedPointer<const SurfaceSet> ConstSPtr; /**< Const shared pointer type for SurfaceSet class. */
    
    //=========================================================================================================
    /**
    * Default constructor
    */
    SurfaceSet();

    //=========================================================================================================
    /**
    * Constructs a surface set by assembling given surfaces
    *
    * @param[in] p_LHSurface    Left hemisphere surface
    * @param[in] p_RHSurface    Right hemisphere surface
    */
    explicit SurfaceSet(const Surface& p_LHSurface, const Surface& p_RHSurface);

    //=========================================================================================================
    /**
    * Constructs an annotation set by reading from annotation files
    *
    * @param[in] p_sLHFileName  Left hemisphere annotation file
    * @param[in] p_sRHFileName  Right hemisphere annotation file
    */
    explicit SurfaceSet(const QString& p_sLHFileName, const QString& p_sRHFileName);

    //=========================================================================================================
    /**
    * Destroys the SurfaceSet class.
    */
    ~SurfaceSet();
    
    //=========================================================================================================
    /**
    * Initializes the AnnotationSet.
    */
    void clear();

    //=========================================================================================================
    /**
    * True if SurfaceSet is empty.
    *
    * @return true if SurfaceSet is empty
    */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
    * Insert a surface
    *
    * @param[in] p_Surface  Surface to insert
    */
    void insert(const Surface& p_Surface);

    //=========================================================================================================
    /**
    * Reads different surface files and assembles them to a SurfaceSet
    *
    * @param[in] p_sLHFileName      Left hemisphere surface file
    * @param[in] p_sRHFileName      Right hemisphere surface file
    * @param[out] p_SurfaceSet      The read surface set
    *
    * @return true if succesfull, false otherwise
    */
    static bool read(const QString& p_sLHFileName, const QString& p_sRHFileName, SurfaceSet &p_SurfaceSet);

    //=========================================================================================================
    /**
    * Subscript operator [] to access surface by index
    *
    * @param[in] idx    the hemisphere index (0 or 1).
    *
    * @return Surface related to the parameter index.
    */
    const Surface& operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
    * Subscript operator [] to access surface by index
    *
    * @param[in] idx    the hemisphere index (0 or 1).
    *
    * @return Surface related to the parameter index.
    */
    Surface& operator[] (qint32 idx);

    //=========================================================================================================
    /**
    * Subscript operator [] to access surface by identifier
    *
    * @param[in] idt    the hemisphere identifier ("lh" or "rh").
    *
    * @return Surface related to the parameter identifier.
    */
    const Surface& operator[] (QString idt) const;

    //=========================================================================================================
    /**
    * Subscript operator [] to access surface by identifier
    *
    * @param[in] idt    the hemisphere identifier ("lh" or "rh").
    *
    * @return Surface related to the parameter identifier.
    */
    Surface& operator[] (QString idt);

    //=========================================================================================================
    /**
    * Returns number of loaded hemispheres
    *
    * @return number of loaded hemispheres
    */
    inline qint32 size() const;

private:
    QMap<qint32, Surface> m_qMapSurfs;   /**< Hemisphere surfaces (lh = 0; rh = 1). */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool SurfaceSet::isEmpty() const
{
    return m_qMapSurfs.isEmpty();
}


//*************************************************************************************************************

inline qint32 SurfaceSet::size() const
{
    return m_qMapSurfs.size();
}

} // NAMESPACE

#endif // SURFACESET_H

