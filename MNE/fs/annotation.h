//=============================================================================================================
/**
* @file     annotation.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Bruce Fischl
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Annotation class declaration
*
*/

#ifndef ANNOTATION_H
#define ANNOTATION_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "colortable.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Free surfer annotation contains vertix label relations and a color/name lookup table
*
* @brief Free surfer annotation
*/
class FSSHARED_EXPORT Annotation
{

public:
    typedef QSharedPointer<Annotation> SPtr;            /**< Shared pointer type for Annotation. */
    typedef QSharedPointer<const Annotation> ConstSPtr; /**< Const shared pointer type for Annotation. */

    //=========================================================================================================
    /**
    * Construts the annotation by reading it of the given file.
    *
    * @param[in] p_sFileName    Annotation file
    */
    explicit Annotation(const QString& p_sFileName);

    //=========================================================================================================
    /**
    * Destroys the annotation.
    */
    ~Annotation();

    //=========================================================================================================
    /**
    * Returns the vertix indeces
    *
    * @return vertix indeces
    */
    inline VectorXi& getVertices()
    {
        return m_Vertices;
    }

    //=========================================================================================================
    /**
    * Returns the vertix labels
    *
    * @return vertix labels
    */
    inline VectorXi& getLabel()
    {
        return m_Label;
    }

    //=========================================================================================================
    /**
    * Returns the coloratable containing the label based nomenclature
    *
    * @return colortable
    */
    inline Colortable& getColortable()
    {
        return m_Colortable;
    }

    //=========================================================================================================
    /**
    * Reads an annotation of a file
    *
    * @param[in] p_sFileName    Annotation file
    */
    void read_annotation(const QString& p_sFileName);

private:
    QString m_sFileName;        /**< Annotation file */

    VectorXi m_Vertices;        /**< Vertice indeces */
    VectorXi m_Label;           /**< Vertice labels */

    Colortable m_Colortable;    /**< Lookup table label colors & names */
};

} // NAMESPACE

#endif // ANNOTATION_H
