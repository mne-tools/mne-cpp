//=============================================================================================================
/**
* @file     mne_named_matrix.cpp
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
* @brief    Implementation of the  MNE Named Matrix (MneNamedMatrix) Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_named_matrix.h"
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;


#define MALLOC_14(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_14(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define ALLOC_CMATRIX_14(x,y) mne_cmatrix_14((x),(y))

static void matrix_error_14(int kind, int nr, int nc)

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



float **mne_cmatrix_14(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_14(nr,float *);
    if (!m) matrix_error_14(1,nr,nc);
    whole = MALLOC_14(nr*nc,float);
    if (!whole) matrix_error_14(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}



char *mne_strdup_14(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}


char **mne_dup_name_list_14(char **list, int nlist)
/*
 * Duplicate a name list
 */
{
    char **res;
    int  k;
    if (list == NULL || nlist == 0)
        return NULL;
    res = MALLOC_14(nlist,char *);

    for (k = 0; k < nlist; k++)
        res[k] = mne_strdup_14(list[k]);
    return res;
}




#define FREE_14(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_14(m) mne_free_cmatrix_14((m))


void mne_free_cmatrix_14 (float **m)
{
    if (m) {
        FREE_14(*m);
        FREE_14(m);
    }
}


void free_name_list_14(char **list, int nlist)
/*
* Free a name list array
*/
{
    int k;
    if (list == NULL || nlist == 0)
        return;
    for (k = 0; k < nlist; k++) {
#ifdef FOO
        fprintf(stderr,"%d %s\n",k,list[k]);
#endif
        FREE_14(list[k]);
    }
    FREE_14(list);
    return;
}





void fromFloatEigenMatrix_14(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_14(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_14(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}


void mne_string_to_name_list_14(char *s,char ***listp,int *nlistp)
/*
      * Convert a colon-separated list into a string array
      */
{
    char **list = NULL;
    int  nlist  = 0;
    char *one,*now=NULL;

    if (s != NULL && strlen(s) > 0) {
        s = mne_strdup_14(s);
        //strtok_r linux variant; strtok_s windows varainat
#ifdef __linux__
        for (one = strtok_r(s,":",&now); one != NULL; one = strtok_r(NULL,":",&now)) {
#elif _WIN32
        for (one = strtok_s(s,":",&now); one != NULL; one = strtok_s(NULL,":",&now)) {
#else
        for (one = strtok_r(s,":",&now); one != NULL; one = strtok_r(NULL,":",&now)) {
#endif
            list = REALLOC_14(list,nlist+1,char *);
            list[nlist++] = mne_strdup_14(one);
        }
        FREE_14(s);
    }
    *listp  = list;
    *nlistp = nlist;
    return;
}



void mne_free_name_list_14(char **list, int nlist)
/*
* Free a name list array
*/
{
    int k;
    if (list == NULL || nlist == 0)
        return;
    for (k = 0; k < nlist; k++) {
#ifdef FOO
        fprintf(stderr,"%d %s\n",k,list[k]);
#endif
        FREE_14(list[k]);
    }
    FREE_14(list);
    return;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneNamedMatrix::MneNamedMatrix()
: nrow(0)
, ncol(0)
, rowlist(NULL)
, collist(NULL)
, data(NULL)
{

}


//*************************************************************************************************************

MneNamedMatrix::MneNamedMatrix(const MneNamedMatrix &p_MneNamedMatrix)
{
    float **data = ALLOC_CMATRIX_14(p_MneNamedMatrix.nrow,p_MneNamedMatrix.ncol);
    int   j,k;

    for (j = 0; j < p_MneNamedMatrix.nrow; j++)
        for (k = 0; k < p_MneNamedMatrix.ncol; k++)
            data[j][k] = p_MneNamedMatrix.data[j][k];
    MneNamedMatrix* res = build_named_matrix(   p_MneNamedMatrix.nrow,p_MneNamedMatrix.ncol,
                                                mne_dup_name_list_14(p_MneNamedMatrix.rowlist,p_MneNamedMatrix.nrow),
                                                mne_dup_name_list_14(p_MneNamedMatrix.collist,p_MneNamedMatrix.ncol),data);
    this->nrow = res->nrow;
    this->ncol = res->ncol;
    this->rowlist = res->rowlist;
    this->collist = res->collist;
    this->data = res->data;
}


//*************************************************************************************************************

MneNamedMatrix::~MneNamedMatrix()
{
    free_name_list_14(rowlist,nrow);
    free_name_list_14(collist,ncol);
    FREE_CMATRIX_14(data);
}


//*************************************************************************************************************

MneNamedMatrix *MneNamedMatrix::build_named_matrix(int nrow, int ncol, char **rowlist, char **collist, float **data)
{
    MneNamedMatrix* mat = new MneNamedMatrix;
    mat->nrow    = nrow;
    mat->ncol    = ncol;
    mat->rowlist = rowlist;
    mat->collist = collist;
    mat->data    = data;
    return mat;
}


//*************************************************************************************************************

MneNamedMatrix *MneNamedMatrix::pick_from_named_matrix(char **pickrowlist, int picknrow, char **pickcollist, int pickncol) const
{
    int *pick_row = NULL;
    int *pick_col = NULL;
    char **my_pickrowlist = NULL;
    char **my_pickcollist = NULL;
    float **pickdata = NULL;
    float **data;
    int   row,j,k;
    char  *one;

    if (pickcollist && !this->collist) {
        printf("Cannot pick columns: no names for columns in original.");
        return NULL;
    }
    if (pickcollist && !this->collist) {
        printf("Cannot pick columns: no names for columns in original.");
        return NULL;
    }
    if (!pickrowlist)
        picknrow = this->nrow;
    if (!pickcollist)
        pickncol = this->ncol;
    pick_row = MALLOC_14(picknrow,int);
    pick_col = MALLOC_14(pickncol,int);
    /*
       * Decide what to pick
       */
    if (pickrowlist) {
        for (j = 0; j < picknrow; j++) {
            one = pickrowlist[j];
            pick_row[j] = -1;
            for (k = 0; k < this->nrow; k++) {
                if (strcmp(one,this->rowlist[k]) == 0) {
                    pick_row[j] = k;
                    break;
                }
            }
            if (pick_row[j] == -1) {
                printf("Row called %s not found in original matrix",one);
                goto bad;
            }
            my_pickrowlist = mne_dup_name_list_14(pickrowlist,picknrow);
        }
    }
    else {
        for (k = 0; k < picknrow; k++)
            pick_row[k] = k;
        my_pickrowlist = mne_dup_name_list_14(this->rowlist,this->nrow);
    }
    if (pickcollist) {
        for (j = 0; j < pickncol; j++) {
            one = pickcollist[j];
            pick_col[j] = -1;
            for (k = 0; k < this->ncol; k++) {
                if (strcmp(one,this->collist[k]) == 0) {
                    pick_col[j] = k;
                    break;
                }
            }
            if (pick_col[j] == -1) {
                printf("Column called %s not found in original matrix",one);
                goto bad;
            }
            my_pickcollist = mne_dup_name_list_14(pickcollist,pickncol);
        }
    }
    else {
        for (k = 0; k < pickncol; k++)
            pick_col[k] = k;
        my_pickcollist = mne_dup_name_list_14(this->collist,this->ncol);
    }
    /*
        * Do the picking of the data accordingly
        */
    pickdata = ALLOC_CMATRIX_14(picknrow,pickncol);

    data = this->data;
    for (j = 0; j < picknrow; j++) {
        row = pick_row[j];
        for (k = 0; k < pickncol; k++)
            pickdata[j][k] = data[row][pick_col[k]];
    }

    FREE_14(pick_col);
    FREE_14(pick_row);
    return build_named_matrix(picknrow,pickncol,my_pickrowlist,my_pickcollist,pickdata);

bad : {
        FREE_14(pick_col);
        FREE_14(pick_row);
        mne_free_name_list_14(my_pickrowlist,picknrow);
        mne_free_name_list_14(my_pickcollist,pickncol);
        return NULL;
    }
}


//*************************************************************************************************************

MneNamedMatrix *MneNamedMatrix::read_named_matrix(FiffStream::SPtr &stream, const FiffDirNode::SPtr &node, int kind)
{
    char **colnames = NULL;
    char **rownames = NULL;
    int  ncol = 0;
    int  nrow = 0;
    qint32 ndim;
    QVector<qint32> dims;
    float **data = NULL;
    int  val;
    char *s;
    FiffTag::SPtr t_pTag;
    int     k;

    FiffDirNode::SPtr tmp_node = node;

    /*
    * If the node is a named-matrix mode, use it.
    * Otherwise, look in first-generation children
    */
    if (tmp_node->type == FIFFB_MNE_NAMED_MATRIX) {
        if(!tmp_node->find_tag(stream, kind, t_pTag))
            goto bad;

        qint32 ndim;
        QVector<qint32> dims;
        t_pTag->getMatrixDimensions(ndim, dims);

        if (ndim != 2) {
            qCritical("mne_read_named_matrix only works with two-dimensional matrices");
            goto bad;
        }

        MatrixXf tmp_data = t_pTag->toFloatMatrix().transpose();
        data = ALLOC_CMATRIX_14(tmp_data.rows(),tmp_data.cols());
        fromFloatEigenMatrix_14(tmp_data, data);
    }
    else {
        for (k = 0; k < tmp_node->nchild(); k++) {
            if (tmp_node->children[k]->type == FIFFB_MNE_NAMED_MATRIX) {
                if(tmp_node->children[k]->find_tag(stream, kind, t_pTag)) {
                    t_pTag->getMatrixDimensions(ndim, dims);
                    if (ndim != 2) {
                        qCritical("mne_read_named_matrix only works with two-dimensional matrices");
                        goto bad;
                    }

                    MatrixXf tmp_data = t_pTag->toFloatMatrix().transpose();
                    data = ALLOC_CMATRIX_14(tmp_data.rows(),tmp_data.cols());
                    fromFloatEigenMatrix_14(tmp_data, data);

                    tmp_node = tmp_node->children[k];
                    break;
                }
            }
        }
        if (!data)
            goto bad;
    }
    /*
        * Separate FIFF_MNE_NROW is now optional
        */
    if (!tmp_node->find_tag(stream, FIFF_MNE_NROW, t_pTag))
        nrow = dims[0];
    else {
        nrow = *t_pTag->toInt();
        if (nrow != dims[0]) {
            qCritical("Number of rows in the FIFF_MNE_NROW tag and in the matrix data conflict.");
            goto bad;
        }
    }
    /*
        * Separate FIFF_MNE_NCOL is now optional
        */
    if(!tmp_node->find_tag(stream, FIFF_MNE_NCOL, t_pTag))
        ncol = dims[1];
    else {
        ncol = *t_pTag->toInt();
        if (ncol != dims[1]) {
            qCritical("Number of columns in the FIFF_MNE_NCOL tag and in the matrix data conflict.");
            goto bad;
        }
    }
    if(!tmp_node->find_tag(stream, FIFF_MNE_ROW_NAMES, t_pTag)) {
        s = (char *)(t_pTag->data());
        mne_string_to_name_list_14(s,&rownames,&val);
        if (val != nrow) {
            qCritical("Incorrect number of entries in the row name list");
            nrow = val;
            goto bad;
        }
    }
    if(!tmp_node->find_tag(stream, FIFF_MNE_COL_NAMES, t_pTag)) {
        s = (char *)(t_pTag->data());
        mne_string_to_name_list_14(s,&colnames,&val);
        if (val != ncol) {
            qCritical("Incorrect number of entries in the column name list");
            ncol = val;
            goto bad;
        }
    }
    return build_named_matrix(nrow,ncol,rownames,colnames,data);

bad : {
        mne_free_name_list_14(rownames,nrow);
        mne_free_name_list_14(colnames,ncol);
        FREE_CMATRIX_14(data);
        return NULL;
    }
}
