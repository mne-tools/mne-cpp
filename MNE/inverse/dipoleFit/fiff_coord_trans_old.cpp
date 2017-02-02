//=============================================================================================================
/**
* @file     fiff_coord_trans_old.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FiffCoordTransOld Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_coord_trans_old.h"

#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;





#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif



#define FREE_20(x) if ((char *)(x) != NULL) free((char *)(x))


#define MALLOC_20(x,t) (t *)malloc((x)*sizeof(t))


#define ALLOC_CMATRIX_20(x,y) mne_cmatrix_20((x),(y))



static void matrix_error_20(int kind, int nr, int nc)

{
    if (kind == 1)
        printf("Failed to allocate memory pointers for a %d x %d matrix\n",nr,nc);
    else if (kind == 2)
        printf("Failed to allocate memory for a %d x %d matrix\n",nr,nc);
    else
        printf("Allocation error for a %d x %d matrix\n",nr,nc);
    if (sizeof(void *) == 4) {
        printf("This is probably because you seem to be using a computer with 32-bit architecture.\n");
        printf("Please consider moving to a 64-bit platform.");
    }
    printf("Cannot continue. Sorry.\n");
    exit(1);
}


float **mne_cmatrix_20(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_20(nr,float *);
    if (!m) matrix_error_20(1,nr,nc);
    whole = MALLOC_20(nr*nc,float);
    if (!whole) matrix_error_20(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}





/*
 * float matrices
 */

#define FREE_CMATRIX_20(m) mne_free_cmatrix_20((m))



void mne_free_cmatrix_20 (float **m)
{
    if (m) {
        FREE_20(*m);
        FREE_20(m);
    }
}





Eigen::MatrixXf toFloatEigenMatrix_20(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromFloatEigenMatrix_20(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_20(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_20(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}




float **mne_lu_invert_20(float **mat,int dim)
/*
      * Invert a matrix using the LU decomposition from
      * LAPACK
      */
{
    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_20(mat, dim, dim);
    Eigen::MatrixXf eigen_mat_inv = eigen_mat.inverse();
    fromFloatEigenMatrix_20(eigen_mat_inv, mat);
    return mat;
}




//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCoordTransOld::FiffCoordTransOld()
{

}


//*************************************************************************************************************

FiffCoordTransOld::FiffCoordTransOld(const FiffCoordTransOld &p_FiffCoordTransOld)
{
    this->from = p_FiffCoordTransOld.from;
    this->to = p_FiffCoordTransOld.to;

    for (int j = 0; j < 3; j++) {
        this->move[j] = p_FiffCoordTransOld.move[j];
        this->invmove[j] = p_FiffCoordTransOld.invmove[j];
        for (int k = 0; k < 3; k++) {
            this->rot[j][k] = p_FiffCoordTransOld.rot[j][k];
            this->invrot[j][k] = p_FiffCoordTransOld.invrot[j][k];
        }
    }
}


//*************************************************************************************************************

FiffCoordTransOld::FiffCoordTransOld(int from, int to, float rot[3][3], float move[3])
{
    int j,k;

    this->from = from;
    this->to   = to;

    for (j = 0; j < 3; j++) {
        this->move[j] = move[j];
        for (k = 0; k < 3; k++)
            this->rot[j][k] = rot[j][k];
    }
    add_inverse(this);
}


//*************************************************************************************************************

FiffCoordTransOld::~FiffCoordTransOld()
{

}


//*************************************************************************************************************

int FiffCoordTransOld::add_inverse(FiffCoordTransOld *t)
{
    int   j,k;
    float **m = ALLOC_CMATRIX_20(4,4);

    for (j = 0; j < 3; j++) {
        for (k = 0; k < 3; k++)
            m[j][k] = t->rot[j][k];
        m[j][3] = t->move[j];
    }
    for (k = 0; k < 3; k++)
        m[3][k] = 0.0;
    m[3][3] = 1.0;
    if (mne_lu_invert_20(m,4) == NULL) {
        FREE_CMATRIX_20(m);
        return FAIL;
    }
    for (j = 0; j < 3; j++) {
        for (k = 0; k < 3; k++)
            t->invrot[j][k] = m[j][k];
        t->invmove[j] = m[j][3];
    }
    FREE_CMATRIX_20(m);
    return OK;
}


//*************************************************************************************************************

FiffCoordTransOld *FiffCoordTransOld::fiff_invert_transform() const
{
    FiffCoordTransOld* ti = new FiffCoordTransOld;
    int j,k;

    for (j = 0; j < 3; j++) {
        ti->move[j] = this->invmove[j];
        ti->invmove[j] = this->move[j];
        for (k = 0; k < 3; k++) {
            ti->rot[j][k]    = this->invrot[j][k];
            ti->invrot[j][k] = this->rot[j][k];
        }
    }
    ti->from = this->to;
    ti->to   = this->from;
    return (ti);
}


//*************************************************************************************************************

void FiffCoordTransOld::fiff_coord_trans(float r[], FiffCoordTransOld *t, int do_move)
/*
          * Apply coordinate transformation
          */
{
    int j,k;
    float res[3];

    for (j = 0; j < 3; j++) {
        res[j] = (do_move ? t->move[j] :  0.0);
        for (k = 0; k < 3; k++)
            res[j] += t->rot[j][k]*r[k];
    }
    for (j = 0; j < 3; j++)
        r[j] = res[j];
}


//*************************************************************************************************************

FiffCoordTransOld *FiffCoordTransOld::read_helper( FIFFLIB::FiffTag::SPtr& tag)
{
    FiffCoordTransOld* p_FiffCoordTrans = NULL;
    if(tag->isMatrix() || tag->getType() != FIFFT_COORD_TRANS_STRUCT || tag->data() == NULL)
        return p_FiffCoordTrans;
    else
    {
        p_FiffCoordTrans = new FiffCoordTransOld;
        qint32* t_pInt32 = (qint32*)tag->data();
        p_FiffCoordTrans->from = t_pInt32[0];
        p_FiffCoordTrans->to = t_pInt32[1];

        float* t_pFloat = (float*)tag->data();
        int count = 0;
        int r, c;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans->move[r] = t_pFloat[11+r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans->rot[r][c] = t_pFloat[2+count];
                ++count;
            }
        }

        count = 0;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans->invmove[r] = t_pFloat[23+r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans->invrot[r][c] = t_pFloat[14+count];
                ++count;
            }
        }

        return p_FiffCoordTrans;
    }
}
