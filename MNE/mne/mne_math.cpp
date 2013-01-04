//=============================================================================================================
/**
* @file     mne_math.cpp
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
* @brief    Contains the implementation of the MNEMath Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne.h"
#include <fiff/fiff.h>
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

VectorXd* MNEMath::combine_xyz(const VectorXd& vec)
{
    if (vec.size() % 3 != 0)
    {
        printf("Input must be a row or a column vector with 3N components");
        return NULL;
    }

    MatrixXd tmp = MatrixXd(vec.transpose());
    SparseMatrix<double>* s = make_block_diag(&tmp,3);

    SparseMatrix<double> sC = *s*s->transpose();
    VectorXd* comb = new VectorXd(sC.rows());

    for(qint32 i = 0; i < sC.rows(); ++i)
        (*comb)[i] = sC.coeff(i,i);

    delete s;
    return comb;
}


//*************************************************************************************************************

//    static inline MatrixXd extract_block_diag(MatrixXd& A, qint32 n)
//    {


//        //
//        // Principal Investigators and Developers:
//        // ** Richard M. Leahy, PhD, Signal & Image Processing Institute,
//        //    University of Southern California, Los Angeles, CA
//        // ** John C. Mosher, PhD, Biophysics Group,
//        //    Los Alamos National Laboratory, Los Alamos, NM
//        // ** Sylvain Baillet, PhD, Cognitive Neuroscience & Brain Imaging Laboratory,
//        //    CNRS, Hopital de la Salpetriere, Paris, France
//        //
//        // Copyright (c) 2005 BrainStorm by the University of Southern California
//        // This software distributed  under the terms of the GNU General Public License
//        // as published by the Free Software Foundation. Further details on the GPL
//        // license can be found at http://www.gnu.org/copyleft/gpl.html .
//        //
//        //FOR RESEARCH PURPOSES ONLY. THE SOFTWARE IS PROVIDED "AS IS," AND THE
//        // UNIVERSITY OF SOUTHERN CALIFORNIA AND ITS COLLABORATORS DO NOT MAKE ANY
//        // WARRANTY, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
//        // MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, NOR DO THEY ASSUME ANY
//        // LIABILITY OR RESPONSIBILITY FOR THE USE OF THIS SOFTWARE.
//        //
//        // Author: John C. Mosher 1993 - 2004
//        //
//        //
//        // Modifications for mne Matlab toolbox
//        //
//        //   Matti Hamalainen
//        //   2006


//          [mA,na] = size(A);		% matrix always has na columns
//          % how many entries in the first column?
//          bdn = na/n;			% number of blocks
//          ma = mA/bdn;			% rows in first block

//          % blocks may themselves contain zero entries.  Build indexing as above
//          tmp = reshape([1:(ma*bdn)]',ma,bdn);
//          i = zeros(ma*n,bdn);
//          for iblock = 1:n,
//            i((iblock-1)*ma+[1:ma],:) = tmp;
//          end

//          i = i(:); 			% row indices foreach sparse bd


//          j = [0:mA:(mA*(na-1))];
//          j = j(ones(ma,1),:);
//          j = j(:);

//          i = i + j;

//          bd = full(A(i)); 	% column vector
//          bd = reshape(bd,ma,na);	% full matrix

//    }


//*************************************************************************************************************

SparseMatrix<double>* MNEMath::make_block_diag(const MatrixXd* A, qint32 n)
{

    qint32 ma = A->rows();
    qint32 na = A->cols();
    float bdn = ((float)na)/n;      // number of submatrices

//    std::cout << std::endl << "ma " << ma << " na " << na << " bdn " << bdn << std::endl;

    if(bdn - floor(bdn))
    {
        printf("Width of matrix must be even multiple of n\n");
        return NULL;
    }

    SparseMatrix<double>* bd = new SparseMatrix<double>((int)floor((float)ma*bdn+0.5),na);

    qint32 current_col, current_row, i, r, c;
    for(i = 0; i < bdn; ++i)
    {
        current_col = i * n;
        current_row = i * ma;

        for(r = 0; r < ma; ++r)
            for(c = 0; c < n; ++c)
                bd->insert(r+current_row,c+current_col) = (*A)(r, c+current_col);
    }
    return bd;
}
