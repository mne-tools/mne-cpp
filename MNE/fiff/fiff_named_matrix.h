//=============================================================================================================
/**
* @file     fiff_named_matrix.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief    Contains the FiffNamedMatrix class declaration.
*
*/

#ifndef FIFF_NAMED_MATRIX_H
#define FIFF_NAMED_MATRIX_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_types.h"
#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QFile>
#include <QStringList>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* A matrix with named rows and cols. ToDo: derive this from Eigen::Matrix
*
* @brief A named matrix
*/
class FIFFSHARED_EXPORT FiffNamedMatrix {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffNamedMatrix();

    //=========================================================================================================
    /**
    * ctor
    */
    FiffNamedMatrix(    fiff_int_t p_nrow,
                        fiff_int_t p_ncol,
                        QStringList& p_row_names,
                        QStringList& p_col_names,
                        MatrixXd& p_data);

    //=========================================================================================================
    /**
    * Copy ctor
    */
    FiffNamedMatrix(const FiffNamedMatrix* p_pFiffNamedMatrix);

    //=========================================================================================================
    /**
    * Destroys the fiffTag.
    */
    ~FiffNamedMatrix();

    //=========================================================================================================
    /**
    * Returns true if named matrix contains no data.
    *
    * @return true if named matrix is empty.
    */
    inline bool isEmpty()
    {
        return !(this->data.size() > 0);
    }

    //ToDo return the transposed matrix instead of applying it to its members
    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_transpose_named_matrix function
    *
    * Transpose a named matrix (FiffNamedMatrix)
    */
    void transpose_named_matrix();

public:
    fiff_int_t nrow;
    fiff_int_t  ncol;
    QStringList row_names;
    QStringList col_names;
    MatrixXd data;
};

} // NAMESPACE

#endif // FIFF_SOLUTION_H
