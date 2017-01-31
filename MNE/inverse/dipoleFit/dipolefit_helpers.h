#ifndef DIPOLEFITHELPERS_H
#define DIPOLEFITHELPERS_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecd_set.h"
#include "mne_sss_data.h"
#include "mne_meas_data_set.h"
#include "mne_deriv.h"
#include "mne_deriv_set.h"
#include "mne_surface_or_volume.h"
#include <iostream>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

#include <utils/sphere.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <unsupported/Eigen/FFT>


//ToDo don't use access and unlink -> use Qt stuff instead
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#else
#include <unistd.h>
#endif



#include "fwd_eeg_sphere_model_set.h"
#include "guess_data.h"
#include "dipole_fit_data.h"
#include "guess_data.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace INVERSELIB;
using namespace FIFFLIB;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif


/* NOTE:
   The architecture is now deduced from the operating system, which is
   a bit stupid way, since the same operating system can me run on various
   architectures. This may need revision later on. */

#if defined(DARWIN)

#if defined(__LITTLE_ENDIAN__)
#define INTEL_X86_ARCH
#else
#define BIG_ENDIAN_ARCH
#endif

#else

#if defined(__hpux) || defined(__Lynx__) || defined(__sun)
#define BIG_ENDIAN_ARCH
#else
#if defined(__linux) || defined(WIN32) || defined(__APPLE__)
#define INTEL_X86_ARCH
#endif

#endif
#endif

#ifdef  INTEL_X86_ARCH
#define NATIVE_ENDIAN    FIFFV_LITTLE_ENDIAN
#endif

#ifdef  BIG_ENDIAN_ARCH
#define NATIVE_ENDIAN    FIFFV_BIG_ENDIAN
#endif



//============================= dot.h =============================

#define X 0
#define Y 1
#define Z 2
/*
 * Dot product and length
 */
#define VEC_DOT(x,y) ((x)[X]*(y)[X] + (x)[Y]*(y)[Y] + (x)[Z]*(y)[Z])
#define VEC_LEN(x) sqrt(VEC_DOT(x,x))


/*
 * Others...
 */

#define VEC_DIFF(from,to,diff) {\
    (diff)[X] = (to)[X] - (from)[X];\
    (diff)[Y] = (to)[Y] - (from)[Y];\
    (diff)[Z] = (to)[Z] - (from)[Z];\
    }

#define VEC_COPY(to,from) {\
    (to)[X] = (from)[X];\
    (to)[Y] = (from)[Y];\
    (to)[Z] = (from)[Z];\
    }

#define CROSS_PRODUCT(x,y,xy) {\
    (xy)[X] =   (x)[Y]*(y)[Z]-(y)[Y]*(x)[Z];\
    (xy)[Y] = -((x)[X]*(y)[Z]-(y)[X]*(x)[Z]);\
    (xy)[Z] =   (x)[X]*(y)[Y]-(y)[X]*(x)[Y];\
    }




//============================= ctf_types.h =============================

#ifndef FIFFV_COIL_CTF_GRAD
#define FIFFV_COIL_CTF_GRAD           5001
#endif

#ifndef FIFFV_COIL_CTF_REF_MAG
#define FIFFV_COIL_CTF_REF_MAG        5002
#endif

#ifndef FIFFV_COIL_CTF_REF_GRAD
#define FIFFV_COIL_CTF_REF_GRAD       5003
#endif

#ifndef FIFFV_COIL_CTF_OFFDIAG_REF_GRAD
#define FIFFV_COIL_CTF_OFFDIAG_REF_GRAD 5004
#endif


//============================= allocs.h =============================

/*
 * integer matrices
 */
#define ALLOC_ICMATRIX(x,y) mne_imatrix((x),(y))
#define FREE_ICMATRIX(m) mne_free_icmatrix((m))
#define ICMATRIX ALLOC_ICMATRIX


//============================= mne_allocs.h =============================

/*
 * Basics...
 */
#define MALLOC(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE(x) if ((char *)(x) != NULL) free((char *)(x))
/*
 * Float, double, and int arrays
 */
#define ALLOC_FLOAT(x) MALLOC(x,float)
#define ALLOC_DOUBLE(x) MALLOC(x,double)
#define ALLOC_INT(x) MALLOC(x,int)
#define REALLOC_FLOAT(x,y) REALLOC(x,y,float)
#define REALLOC_DOUBLE(x,y) REALLOC(x,y,double)
#define REALLOC_INT(x,y) REALLOC(x,y,int)
/*
 * float matrices
 */
#define ALLOC_CMATRIX(x,y) mne_cmatrix((x),(y))
#define FREE_CMATRIX(m) mne_free_cmatrix((m))
#define CMATRIX ALLOC_CMATRIX
/*
 * double matrices
 */
#define ALLOC_DCMATRIX(x,y) mne_dmatrix((x),(y))
#define ALLOC_COMPLEX_DCMATRIX(x,y) mne_complex_dmatrix((x),(y))
#define FREE_DCMATRIX(m) mne_free_dcmatrix((m))
#define FREE_COMPLEX_DCMATRIX(m) mne_free_dcmatrix((m))


//============================= mne_allocs.c =============================

static void matrix_error(int kind, int nr, int nc)

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

float **mne_cmatrix(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC(nr,float *);
    if (!m) matrix_error(1,nr,nc);
    whole = MALLOC(nr*nc,float);
    if (!whole) matrix_error(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}



double **mne_dmatrix(int nr, int nc)

{
    int i;
    double **m;
    double *whole;

    m = MALLOC(nr,double *);
    if (!m) matrix_error(1,nr,nc);
    whole = MALLOC(nr*nc,double);
    if (!whole) matrix_error(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}



void mne_free_cmatrix (float **m)
{
    if (m) {
        FREE(*m);
        FREE(m);
    }
}


void mne_free_icmatrix (int **m)

{
    if (m) {
        FREE(*m);
        FREE(m);
    }
}


void mne_free_dcmatrix (double **m)

{
    if (m) {
        FREE(*m);
        FREE(m);
    }
}

/*
 * float matrices
 */

#define FREE_CMATRIX(m) mne_free_cmatrix((m))


//============================= Refactoring helpers =============================

//float
Eigen::MatrixXf toFloatEigenMatrix(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromFloatEigenMatrix(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}


void fromFloatEigenVector(const Eigen::VectorXf& from_vec, float *&to_vec, const int n)
{
    if(to_vec == NULL) {
        to_vec = ALLOC_FLOAT(n);
    }

    for ( int i = 0; i < n; ++i)
        to_vec[i] = from_vec[i];
}

void fromFloatEigenVector(const Eigen::VectorXf& from_vec, float *&to_vec)
{
    fromFloatEigenVector(from_vec, to_vec, from_vec.size());
}


//int

void fromIntEigenMatrix(const Eigen::MatrixXi& from_mat, int **&to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromIntEigenMatrix(const Eigen::MatrixXi& from_mat, int **&to_mat)
{
    fromIntEigenMatrix(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}



#include <fiff/fiff_types.h>
#include "mne_types.h"
#include "analyze_types.h"


//============================= data.c =============================

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

int is_selected_in_data(mshMegEegData d, char *ch_name)
/*
 * Is this channel selected in data
 */
{
    int issel = FALSE;
    int k;

    for (k = 0; k < d->meas->nchan; k++)
        if (strcasecmp(ch_name,d->meas->chs[k].ch_name) == 0) {
            issel = d->sels[k];
            break;
        }
    return issel;
}


//============================= mne_matop.c =============================

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define LU_INVERT_REPORT_DIM 100


float **mne_lu_invert(float **mat,int dim)
/*
      * Invert a matrix using the LU decomposition from
      * LAPACK
      */
{
    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix(mat, dim, dim);
    Eigen::MatrixXf eigen_mat_inv = eigen_mat.inverse();
    fromFloatEigenMatrix(eigen_mat_inv, mat);
    return mat;
}


void mne_transpose_square(float **mat, int n)
/*
      * In-place transpose of a square matrix
      */
{
    int j,k;
    float val;

    for (j = 1; j < n; j++)
        for (k = 0; k < j; k++) {
            val = mat[j][k];
            mat[j][k] = mat[k][j];
            mat[k][j] = val;
        }
    return;
}


void mne_scale_vector (double scale,float *v,int   nn)

{
#ifdef BLAS
    float  fscale = scale;
    int    one    = 1;
    sscal(&nn,&fscale,v,&one);
#else
    int k;
    for (k = 0; k < nn; k++)
        v[k] = v[k]*scale;
#endif
}


float mne_dot_vectors (float *v1,
                       float *v2,
                       int   nn)

{
#ifdef BLAS
    int one = 1;
    float res = sdot(&nn,v1,&one,v2,&one);
    return res;
#else
    float res = 0.0;
    int   k;

    for (k = 0; k < nn; k++)
        res = res + v1[k]*v2[k];
    return res;
#endif
}


void mne_add_scaled_vector_to(float *v1,float scale, float *v2,int nn)

{
#ifdef BLAS
    float fscale = scale;
    int   one = 1;
    saxpy(&nn,&fscale,v1,&one,v2,&one);
#else
    int k;
    for (k = 0; k < nn; k++)
        v2[k] = v2[k] + scale*v1[k];
#endif
    return;
}


void mne_mat_vec_mult2 (float **m,float *v,float *result, int d1,int d2)
/*
      * Matrix multiplication
      * result(d1) = m(d1 x d2) * v(d2)
      */

{
    int j;

    for (j = 0; j < d1; j++)
        result[j] = mne_dot_vectors (m[j],v,d2);
    return;
}

float **mne_mat_mat_mult (float **m1,float **m2,int d1,int d2,int d3)
/* Matrix multiplication
      * result(d1 x d3) = m1(d1 x d2) * m2(d2 x d3) */

{
#ifdef BLAS
    float **result = ALLOC_CMATRIX(d1,d3);
    char  *transa = "N";
    char  *transb = "N";
    float zero = 0.0;
    float one  = 1.0;
    sgemm (transa,transb,&d3,&d1,&d2,
           &one,m2[0],&d3,m1[0],&d2,&zero,result[0],&d3);
    return (result);
#else
    float **result = ALLOC_CMATRIX(d1,d3);
    int j,k,p;
    float sum;

    for (j = 0; j < d1; j++)
        for (k = 0; k < d3; k++) {
            sum = 0.0;
            for (p = 0; p < d2; p++)
                sum = sum + m1[j][p]*m2[p][k];
            result[j][k] = sum;
        }
    return (result);
#endif
}


int mne_svd(float **mat,	/* The matrix */
            int   m,int n,	/* m rows n columns */
            float *sing,	/* Singular values (must have size
                             * MIN(m,n)+1 */
            float **uu,		/* Left eigenvectors */
            float **vv)		/* Right eigenvectors */
/*
      * Compute the SVD of mat.
      * The singular vector calculations depend on whether
      * or not u and v are given.
      *
      * The allocations should be done as follows
      *
      * mat = ALLOC_CMATRIX(m,n);
      * vv  = ALLOC_CMATRIX(MIN(m,n),n);
      * uu  = ALLOC_CMATRIX(MIN(m,n),m);
      * sing = MALLOC(MIN(m,n),float);
      *
      * mat is modified by this operation
      *
      * This simply allocates the workspace and calls the
      * LAPACK Fortran routine
      */

{
    int    udim = MIN(m,n);

    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix(mat, m, n);

    //ToDo Optimize computation depending of whether uu or vv are defined
    Eigen::JacobiSVD< Eigen::MatrixXf > svd(eigen_mat ,Eigen::ComputeFullU | Eigen::ComputeFullV);

    fromFloatEigenVector(svd.singularValues(), sing, svd.singularValues().size());

    if (uu != NULL)
        fromFloatEigenMatrix(svd.matrixU().transpose(), uu, udim, m);

    if (vv != NULL)
        fromFloatEigenMatrix(svd.matrixV().transpose(), vv, m, n);

    return 0;
    //  return info;
}


void mne_transpose_dsquare(double **mat, int n)
/*
      * In-place transpose of a square matrix
      */
{
    int j,k;
    double val;

    for (j = 1; j < n; j++)
        for (k = 0; k < j; k++) {
            val = mat[j][k];
            mat[j][k] = mat[k][j];
            mat[k][j] = val;
        }
    return;
}



double **mne_dmatt_dmat_mult2 (double **m1,double **m2, int d1,int d2,int d3)
/* Matrix multiplication
      * result(d1 x d3) = m1(d2 x d1)^T * m2(d2 x d3) */

{
    double **result = ALLOC_DCMATRIX(d1,d3);
#ifdef BLAS
    char  *transa = "N";
    char  *transb = "T";
    double zero = 0.0;
    double one  = 1.0;

    dgemm (transa,transb,&d3,&d1,&d2,
           &one,m2[0],&d3,m1[0],&d1,&zero,result[0],&d3);

    return result;
#else
    int j,k,p;
    double sum;

    for (j = 0; j < d1; j++)
        for (k = 0; k < d3; k++) {
            sum = 0.0;
            for (p = 0; p < d2; p++)
                sum = sum + m1[p][j]*m2[p][k];
            result[j][k] = sum;
        }
    return result;
#endif
}


#include <fiff/fiff_dir_node.h>


//============================= mne_decompose.c =============================


int mne_decompose_eigen (double *mat,
                         double *lambda,
                         float  **vectors, /* Eigenvectors fit into floats easily */
                         int    dim)
/*
      * Compute the eigenvalue decomposition of
      * a symmetric matrix using the LAPACK routines
      *
      * 'mat' contains the lower triangle of the matrix
      */
{
    int    np  =   dim*(dim+1)/2;
    double *w    = MALLOC(dim,double);
    double *z    = MALLOC(dim*dim,double);
    double *work = MALLOC(3*dim,double);
    double *dmat = MALLOC(np,double);
    float  *vecp = vectors[0];

    const char   *uplo  = "U";
    const char   *compz = "V";
    int    info,k;
    int    one = 1;
    int    maxi;
    double scale;

//    maxi = idamax(&np,mat,&one);
// idamax workaround begin
    maxi = 0;
    for(int i = 0; i < np; ++i)
        if (fabs(mat[i]) > fabs(mat[maxi]))
            maxi = i;
// idamax workaround end

    scale = 1.0/mat[maxi];//scale = 1.0/mat[maxi-1];

    for (k = 0; k < np; k++)
        dmat[k] = mat[k]*scale;
//    dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);


// dspev workaround begin
    MatrixXd dmat_tmp = MatrixXd::Zero(dim,dim);
    int idx = 0;
    for (int i = 0; i < dim; ++i) {
        for(int j = 0; j <= i; ++j) {
            dmat_tmp(i,j) = dmat[idx];
            dmat_tmp(j,i) = dmat[idx];
            ++idx;
        }
    }
    SelfAdjointEigenSolver<MatrixXd> es;
    es.compute(dmat_tmp);
    for ( int i = 0; i < dim; ++i )
        w[i] = es.eigenvalues()[i];

    idx = 0;
    for ( int j = 0; j < dim; ++j ) {
        for( int i = 0; i < dim; ++i ) {
            z[idx] = es.eigenvectors()(i,j);// Column Major
            ++idx;
        }
    }
// dspev workaround end

    info = 0;

    qDebug() << "!!!DEBUG ToDo: dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);";

    FREE(work);
    if (info != 0)
        printf("Eigenvalue decomposition failed (LAPACK info = %d)",info);
    else {
        scale = 1.0/scale;
        for (k = 0; k < dim; k++)
            lambda[k] = scale*w[k];
        for (k = 0; k < dim*dim; k++)
            vecp[k] = z[k];
    }
    FREE(w);
    FREE(z);
    if (info == 0)
        return 0;
    else
        return -1;
}





//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info(const QString& name,
                                  fiffChInfo     *megp,	 /* MEG channels */
                                  int            *nmegp,
                                  fiffChInfo     *meg_compp,
                                  int            *nmeg_compp,
                                  fiffChInfo     *eegp,	 /* EEG channels */
                                  int            *neegp,
                                  fiffCoordTrans *meg_head_t,
                                  fiffId         *idp)	 /* The measurement ID */
/*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));


    fiffChInfo chs   = NULL;
    int        nchan = 0;
    fiffChInfo meg   = NULL;
    int        nmeg  = 0;
    fiffChInfo meg_comp = NULL;
    int        nmeg_comp = 0;
    fiffChInfo eeg   = NULL;
    int        neeg  = 0;
    fiffId     id    = NULL;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr info;
    FiffTag::SPtr t_pTag;
    fiffChInfo   this_ch = NULL;
    fiffCoordTrans t = NULL;
    fiff_int_t kind, pos;
    int j,k,to_find;
    extern fiffCoordTrans mne_read_meas_transform(const QString& name);

    if(!stream->open())
        goto bad;

    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (nodes.size() == 0) {
        nodes = stream->tree()->dir_tree_find(FIFFB_MEAS_INFO);
        if (nodes.size() == 0) {
            qCritical ("Could not find the channel information.");
            goto bad;
        }
    }
    info = nodes[0];
    to_find = 0;
    for (k = 0; k < info->nent; k++) {
        kind = info->dir[k]->kind;
        pos  = info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();
            chs = MALLOC(nchan,fiffChInfoRec);
            for (j = 0; j < nchan; j++)
                chs[j].scanNo = -1;
            to_find = nchan;
            break;

        case FIFF_PARENT_BLOCK_ID :
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
//            id = t_pTag->toFiffID();
            *id = *(fiffId)t_pTag->data();
            break;

        case FIFF_COORD_TRANS :
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
//            t = t_pTag->toCoordTrans();
            t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
            *t = *(fiffCoordTrans)t_pTag->data();
            if (t->from != FIFFV_COORD_DEVICE || t->to   != FIFFV_COORD_HEAD)
                t = NULL;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
//            this_ch = t_pTag->toChInfo();
            this_ch = (fiffChInfo)malloc(sizeof(fiffChInfoRec));
            *this_ch = *(fiffChInfo)(t_pTag->data());
            if (this_ch->scanNo <= 0 || this_ch->scanNo > nchan) {
                printf ("FIFF_CH_INFO : scan # out of range %d (%d)!",this_ch->scanNo,nchan);
                goto bad;
            }
            else
                chs[this_ch->scanNo-1] = *this_ch;
            to_find--;
            break;
        }
    }
    if (to_find != 0) {
        qCritical("Some of the channel information was missing.");
        goto bad;
    }
    if (t == NULL && meg_head_t != NULL) {
        /*
     * Try again in a more general fashion
     */
        if ((t = mne_read_meas_transform(name)) == NULL) {
            qCritical("MEG -> head coordinate transformation not found.");
            goto bad;
        }
    }
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            nmeg++;
        else if (chs[k].kind == FIFFV_REF_MEG_CH)
            nmeg_comp++;
        else if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;
    if (nmeg > 0)
        meg = MALLOC(nmeg,fiffChInfoRec);
    if (neeg > 0)
        eeg = MALLOC(neeg,fiffChInfoRec);
    if (nmeg_comp > 0)
        meg_comp = MALLOC(nmeg_comp,fiffChInfoRec);
    neeg = nmeg = nmeg_comp = 0;

    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            meg[nmeg++] = chs[k];
        else if (chs[k].kind == FIFFV_REF_MEG_CH)
            meg_comp[nmeg_comp++] = chs[k];
        else if (chs[k].kind == FIFFV_EEG_CH)
            eeg[neeg++] = chs[k];
//    fiff_close(in);
    stream->close();
    FREE(chs);
    if (megp) {
        *megp  = meg;
        *nmegp = nmeg;
    }
    else
        FREE(meg);
    if (meg_compp) {
        *meg_compp = meg_comp;
        *nmeg_compp = nmeg_comp;
    }
    else
        FREE(meg_comp);
    if (eegp) {
        *eegp  = eeg;
        *neegp = neeg;
    }
    else
        FREE(eeg);
    if (idp == NULL) {
        FREE(id);
    }
    else
        *idp   = id;
    if (meg_head_t == NULL) {
        FREE(t);
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
//        fiff_close(in);
        stream->close();
        FREE(chs);
        FREE(meg);
        FREE(eeg);
        FREE(id);
//        FREE(tag.data);
        FREE(t);
        return FIFF_FAIL;
    }
}


//============================= mne_read_process_forward_solution.c =============================

void mne_merge_channels(fiffChInfo chs1, int nch1,
                        fiffChInfo chs2, int nch2,
                        fiffChInfo *resp, int *nresp)

{
    fiffChInfo res = MALLOC(nch1+nch2,fiffChInfoRec);
    int k,p;
    for (p = 0, k = 0; k < nch1; k++)
        res[p++] = chs1[k];
    for (k = 0; k < nch2;k++)
        res[p++] = chs2[k];
    *resp = res;
    *nresp = nch1+nch2;
    return;
}


//============================= read_ch_info.c =============================

static FiffDirNode::SPtr find_meas (const FiffDirNode::SPtr& node)
/*
      * Find corresponding meas node
      */
{
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == NULL)
            return empty_node;//(NULL);
        tmp_node = tmp_node->parent;
    }
    return (tmp_node);
}

static FiffDirNode::SPtr find_meas_info (const FiffDirNode::SPtr& node)
/*
      * Find corresponding meas info node
      */
{
    int k;
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == NULL)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    for (k = 0; k < tmp_node->nchild; k++)
        if (tmp_node->children[k]->type == FIFFB_MEAS_INFO)
            return (tmp_node->children[k]);
    return empty_node;
}

static int get_all_chs (//fiffFile file,	        /* The file we are reading */
                        FiffStream::SPtr& stream,
                        const FiffDirNode::SPtr& p_node,	/* The directory node containing our data */
                        fiffId *id,		/* The block id from the nearest FIFFB_MEAS
                                                   parent */
                        fiffChInfo *chp,	/* Channel descriptions */
                        int *nchan)		/* Number of channels */
/*
      * Find channel information from
      * nearest FIFFB_MEAS_INFO parent of
      * node.
      */
{
    fiffChInfo ch;
    fiffChInfo this_ch = NULL;
    int j,k;
    int to_find = 0;
    FiffDirNode::SPtr meas;
    FiffDirNode::SPtr meas_info;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;

    *chp     = NULL;
    ch       = NULL;
    *id      = NULL;
    /*
    * Find desired parents
    */
    if (!(meas = find_meas(p_node))) {
        qCritical ("Meas. block not found!");
        goto bad;
    }

    if (!(meas_info = find_meas_info(p_node))) {
        qCritical ("Meas. info not found!");
        goto bad;
    }
    /*
    * Is there a block id is in the FIFFB_MEAS node?
    */
    if (!meas->id.isEmpty()) {
        *id = MALLOC(1,fiffIdRec);
        (*id)->version = meas->id.version;
        (*id)->machid[0] = meas->id.machid[0];
        (*id)->machid[1] = meas->id.machid[1];
        (*id)->time = meas->id.time;
    }
    /*
    * Others from FIFFB_MEAS_INFO
    */
    for (k = 0; k < meas_info->nent; k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *nchan = *t_pTag->toInt();
            ch = MALLOC(*nchan,fiffChInfoRec);
            for (j = 0; j < *nchan; j++)
                ch[j].scanNo = -1;
            to_find = to_find + *nchan - 1;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            this_ch = (fiffChInfo)malloc(sizeof(fiffChInfoRec));
            *this_ch = *(fiffChInfo)t_pTag->data();
            if (this_ch->scanNo <= 0 || this_ch->scanNo > *nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                memcpy(ch+this_ch->scanNo-1,this_ch,
                       sizeof(fiffChInfoRec));
            to_find--;
            break;
        }
    }
    *chp = ch;
    return FIFF_OK;

bad : {
        FREE (ch);
        return FIFF_FAIL;
    }
}


static int read_ch_info(const QString&  name,
                        fiffChInfo      *chsp,
                        int             *nchanp,
                        fiffId          *idp)
/*
* Read channel information from a measurement file
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    fiffChInfo chs = NULL;
    int        nchan = 0;
    fiffId     id = NULL;

    QList<FiffDirNode::SPtr> meas;
    FiffDirNode::SPtr    node;

    if(!stream->open())
        goto bad;

    meas = stream->tree()->dir_tree_find(FIFFB_MEAS);
    if (meas.size() == 0) {
        qCritical ("%s : no MEG data available here",name.toLatin1().data());
        goto bad;
    }
    node = meas[0];
    if (get_all_chs (stream,node,&id,&chs,&nchan) == FIFF_FAIL)
        goto bad;
    *chsp   = chs;
    *nchanp = nchan;
    *idp = id;
    stream->close();
    return FIFF_OK;

bad : {
        FREE(chs);
        FREE(id);
        stream->close();
        return FIFF_FAIL;
    }
}


#define TOO_CLOSE 1e-4

static int at_origin (float *rr)

{
    return (VEC_LEN(rr) < TOO_CLOSE);
}



static int is_valid_eeg_ch(fiffChInfo ch)
/*
      * Is the electrode position information present?
      */
{
    if (ch->kind == FIFFV_EEG_CH) {
        if (at_origin(ch->chpos.r0) ||
                ch->chpos.coil_type == FIFFV_COIL_NONE)
            return FALSE;
        else
            return TRUE;
    }
    return FALSE;
}



static int accept_ch(fiffChInfo ch,
                     char       **bads,
                     int        nbad)

{
    int k;
    for (k = 0; k < nbad; k++)
        if (strcmp(ch->ch_name,bads[k]) == 0)
            return FALSE;
    return TRUE;
}



int read_meg_eeg_ch_info(const QString& name,       /* Input file */
                         int        do_meg,	 /* Use MEG */
                         int        do_eeg,	 /* Use EEG */
                         char       **bads,	 /* List of bad channels */
                         int        nbad,
                         fiffChInfo *chsp,	 /* MEG + EEG channels */
                         int        *nmegp,	 /* Count of each */
                         int        *neegp)
/*
      * Read the channel information and split it into two arrays,
      * one for MEG and one for EEG
      */
{
    fiffChInfo chs   = NULL;
    int        nchan = 0;
    fiffChInfo meg   = NULL;
    int        nmeg  = 0;
    fiffChInfo eeg   = NULL;
    int        neeg  = 0;
    fiffId     id    = NULL;
    int        nch;

    int k;

    if (read_ch_info(name,&chs,&nchan,&id) != FIFF_OK)
        goto bad;
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            nmeg++;
        else if (chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(chs+k))
            neeg++;
    if (nmeg > 0)
        meg = MALLOC(nmeg,fiffChInfoRec);
    if (neeg > 0)
        eeg = MALLOC(neeg,fiffChInfoRec);
    neeg = nmeg = 0;
    for (k = 0; k < nchan; k++)
        if (accept_ch(chs+k,bads,nbad)) {
            if (do_meg && chs[k].kind == FIFFV_MEG_CH)
                meg[nmeg++] = chs[k];
            else if (do_eeg && chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(chs+k))
                eeg[neeg++] = chs[k];
        }
    FREE(chs);
    mne_merge_channels(meg,nmeg,eeg,neeg,chsp,&nch);
    FREE(meg);
    FREE(eeg);
    *nmegp = nmeg;
    *neegp = neeg;
    FREE(id);
    return FIFF_OK;

bad : {
        FREE(chs);
        FREE(meg);
        FREE(eeg);
        FREE(id);
        return FIFF_FAIL;
    }
}




//============================= mne_filename_util.c =============================


char *mne_compose_mne_name(const char *path, const char *filename)
/*
      * Compose a filename under the "$MNE_ROOT" directory
      */
{
    char *res;
    char *mne_root;

    if (filename == NULL) {
        qCritical("No file name specified to mne_compose_mne_name");
        return NULL;
    }
    mne_root = getenv(MNE_ENV_ROOT);
    if (mne_root == NULL || strlen(mne_root) == 0) {
        qCritical("Environment variable MNE_ROOT not set");
        return NULL;
    }
    if (path == NULL || strlen(path) == 0) {
        res = MALLOC(strlen(mne_root)+strlen(filename)+2,char);
        strcpy(res,mne_root);
        strcat(res,"/");
        strcat(res,filename);
    }
    else {
        res = MALLOC(strlen(mne_root)+strlen(filename)+strlen(path)+3,char);
        strcpy(res,mne_root);
        strcat(res,"/");
        strcat(res,path);
        strcat(res,"/");
        strcat(res,filename);
    }
    return res;
}




//============================= misc_util.c =============================

char *mne_strdup(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}





//============================= make_volume_source_space.c =============================

static int add_inverse(fiffCoordTrans t)
/*
      * Add inverse transform to an existing one
      */
{
    int   j,k;
    float **m = ALLOC_CMATRIX(4,4);

    for (j = 0; j < 3; j++) {
        for (k = 0; k < 3; k++)
            m[j][k] = t->rot[j][k];
        m[j][3] = t->move[j];
    }
    for (k = 0; k < 3; k++)
        m[3][k] = 0.0;
    m[3][3] = 1.0;
    if (mne_lu_invert(m,4) == NULL) {
        FREE_CMATRIX(m);
        return FAIL;
    }
    for (j = 0; j < 3; j++) {
        for (k = 0; k < 3; k++)
            t->invrot[j][k] = m[j][k];
        t->invmove[j] = m[j][3];
    }
    FREE_CMATRIX(m);
    return OK;
}





//============================= fiff_trans.c =============================




fiffCoordTrans fiff_invert_transform (fiffCoordTrans t)

{
    fiffCoordTrans ti = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
    int j,k;

    for (j = 0; j < 3; j++) {
        ti->move[j] = t->invmove[j];
        ti->invmove[j] = t->move[j];
        for (k = 0; k < 3; k++) {
            ti->rot[j][k]    = t->invrot[j][k];
            ti->invrot[j][k] = t->rot[j][k];
        }
    }
    ti->from = t->to;
    ti->to   = t->from;
    return (ti);
}








fiffCoordTrans fiff_make_transform (int from,int to,float rot[3][3],float move[3])
/*
      * Compose the coordinate transformation structure
      * from a known forward transform
      */
{
    fiffCoordTrans t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
    int j,k;

    t->from = from;
    t->to   = to;

    for (j = 0; j < 3; j++) {
        t->move[j] = move[j];
        for (k = 0; k < 3; k++)
            t->rot[j][k] = rot[j][k];
    }
    add_inverse (t);
    return (t);
}

void fiff_coord_trans (float r[3],fiffCoordTrans t,int do_move)
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



fiffCoordTrans fiff_dup_transform (fiffCoordTrans t)
/*
      * Simply duplicate
      */
{
    fiffCoordTrans tdup = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));

    memcpy (tdup,t,sizeof(fiffCoordTransRec));
    return (tdup);
}




//============================= mne_coord_transforms.c =============================

typedef struct {
    int frame;
    const char *name;
} frameNameRec;


const char *mne_coord_frame_name(int frame)

{
    static frameNameRec frames[] = {
        {FIFFV_COORD_UNKNOWN,"unknown"},
        {FIFFV_COORD_DEVICE,"MEG device"},
        {FIFFV_COORD_ISOTRAK,"isotrak"},
        {FIFFV_COORD_HPI,"hpi"},
        {FIFFV_COORD_HEAD,"head"},
        {FIFFV_COORD_MRI,"MRI (surface RAS)"},
        {FIFFV_MNE_COORD_MRI_VOXEL, "MRI voxel"},
        {FIFFV_COORD_MRI_SLICE,"MRI slice"},
        {FIFFV_COORD_MRI_DISPLAY,"MRI display"},
        {FIFFV_MNE_COORD_CTF_DEVICE,"CTF MEG device"},
        {FIFFV_MNE_COORD_CTF_HEAD,"CTF/4D/KIT head"},
        {FIFFV_MNE_COORD_RAS,"RAS (non-zero origin)"},
        {FIFFV_MNE_COORD_MNI_TAL,"MNI Talairach"},
        {FIFFV_MNE_COORD_FS_TAL_GTZ,"Talairach (MNI z > 0)"},
        {FIFFV_MNE_COORD_FS_TAL_LTZ,"Talairach (MNI z < 0)"},
        {-1,"unknown"}
    };
    int k;
    for (k = 0; frames[k].frame != -1; k++) {
        if (frame == frames[k].frame)
            return frames[k].name;
    }
    return frames[k].name;
}


void mne_print_coord_transform_label(FILE *log,char *label, fiffCoordTrans t)

{
    int k,p;
    int frame;
    if (!label || strlen(label) == 0)
        fprintf(log,"Coordinate transformation: ");
    else
        fprintf(log,"%s",label);
    for (frame = t->from, k = 0; k < 2; k++) {
        if (k == 0) {
            fprintf(log,"%s -> ",mne_coord_frame_name(frame));
            frame = t->to;
        }
        else {
            fprintf(log,"%s\n",mne_coord_frame_name(frame));
            for (p = 0; p < 3; p++)
                fprintf(log,"\t% 8.6f % 8.6f % 8.6f\t% 7.2f mm\n",
                        t->rot[p][X],t->rot[p][Y],t->rot[p][Z],1000*t->move[p]);
            fprintf(log,"\t% 8.6f % 8.6f % 8.6f  % 7.2f\n",0.0,0.0,0.0,1.0);
        }
    }
}

void mne_print_coord_transform(FILE *log, fiffCoordTrans t)
{
    mne_print_coord_transform_label(log,NULL,t);
}



fiffCoordTrans mne_read_transform(const QString& name,int from, int to)
/*
      * Read the specified coordinate transformation
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));


    fiffCoordTrans res = NULL;
//    fiffFile       in = NULL;
    FiffTag::SPtr t_pTag;
//    fiffTagRec     tag;
//    fiffDirEntry   dir;
    fiff_int_t kind, pos;
    int k;

//    tag.data = NULL;
//    if ((in = fiff_open(name.toLatin1().data())) == NULL)
//        goto out;
    if(!stream->open())
        goto out;

    for (k = 0; k < stream->dir().size(); k++) {
        kind = stream->dir()[k]->kind;
        pos  = stream->dir()[k]->pos;
        if (kind == FIFF_COORD_TRANS) {
//            if (fiff_read_this_tag (in->fd,dir->pos,&tag) == FIFF_FAIL)
//                goto out;
//            res = (fiffCoordTrans)tag.data;
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;

            res = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
            *res = *(fiffCoordTrans)t_pTag->data();
            if (res->from == from && res->to == to) {
//                tag.data = NULL;
                goto out;
            }
            else if (res->from == to && res->to == from) {
                res = fiff_invert_transform(res);
                goto out;
            }
            res = NULL;
        }
    }
    qCritical("No suitable coordinate transformation found in %s.",name.toLatin1().data());
    goto out;

out : {
//        FREE(tag.data);
//        fiff_close(in);
        stream->close();
        return res;
    }

    return res;
}

fiffCoordTrans mne_read_mri_transform(const QString& name)
/*
      * Read the MRI -> HEAD coordinate transformation
      */
{
    return mne_read_transform(name,FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
}


fiffCoordTrans mne_read_meas_transform(const QString& name)
/*
      * Read the MEG device -> HEAD coordinate transformation
      */
{
    return mne_read_transform(name,FIFFV_COORD_DEVICE,FIFFV_COORD_HEAD);
}






//============================= mne_named_vector.c =============================

int mne_pick_from_named_vector(mneNamedVector vec, char **names, int nnames, int require_all, float *res)
/*
* Pick the desired elements from the named vector
*/
{
    int found;
    int k,p;

    if (vec->names == 0) {
        qCritical("No names present in vector. Cannot pick.");
        return FAIL;
    }

    for (k = 0; k < nnames; k++)
        res[k] = 0.0;

    for (k = 0; k < nnames; k++) {
        found = 0;
        for (p = 0; p < vec->nvec; p++) {
            if (strcmp(vec->names[p],names[k]) == 0) {
                res[k] = vec->data[p];
                found = TRUE;
                break;
            }
        }
        if (!found && require_all) {
            qCritical("All required elements not found in named vector.");
            return FAIL;
        }
    }
    return OK;
}




//============================= mne_sparse_matop.c =============================



INVERSELIB::FiffSparseMatrix* mne_convert_to_sparse(float **dense,        /* The dense matrix to be converted */
                                      int   nrow,           /* Number of rows in the dense matrix */
                                      int   ncol,           /* Number of columns in the dense matrix */
                                      int   stor_type,      /* Either FIFFTS_MC_CCS or FIFFTS_MC_RCS */
                                      float small)          /* How small elements should be ignored? */
/*
* Create the compressed row or column storage sparse matrix representation
* including a vector containing the nonzero matrix element values,
* the row or column pointer vector and the appropriate index vector(s).
*/
{
    int j,k;
    int nz;
    int ptr;
    INVERSELIB::FiffSparseMatrix* sparse = NULL;
    int size;

    if (small < 0) {		/* Automatic scaling */
        float maxval = 0.0;
        float val;

        for (j = 0; j < nrow; j++)
            for (k = 0; k < ncol; k++) {
                val = fabs(dense[j][k]);
                if (val > maxval)
                    maxval = val;
            }
        if (maxval > 0)
            small = maxval*fabs(small);
        else
            small = fabs(small);
    }
    for (j = 0, nz = 0; j < nrow; j++)
        for (k = 0; k < ncol; k++) {
            if (fabs(dense[j][k]) > small)
                nz++;
        }

    if (nz <= 0) {
        printf("No nonzero elements found.");
        return NULL;
    }
    if (stor_type == FIFFTS_MC_CCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (ncol+1)*(sizeof(fiff_int_t));
    }
    else if (stor_type == FIFFTS_MC_RCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (nrow+1)*(sizeof(fiff_int_t));
    }
    else {
        printf("Unknown sparse matrix storage type: %d",stor_type);
        return NULL;
    }
    sparse = new INVERSELIB::FiffSparseMatrix;
    sparse->coding = stor_type;
    sparse->m      = nrow;
    sparse->n      = ncol;
    sparse->nz     = nz;
    sparse->data   = (float *)malloc(size);
    sparse->inds   = (int *)(sparse->data+nz);
    sparse->ptrs   = sparse->inds+nz;

    if (stor_type == FIFFTS_MC_RCS) {
        for (j = 0, nz = 0; j < nrow; j++) {
            ptr = -1;
            for (k = 0; k < ncol; k++)
                if (fabs(dense[j][k]) > small) {
                    sparse->data[nz] = dense[j][k];
                    if (ptr < 0)
                        ptr = nz;
                    sparse->inds[nz++] = k;
                }
            sparse->ptrs[j] = ptr;
        }
        sparse->ptrs[nrow] = nz;
        for (j = nrow - 1; j >= 0; j--) /* Take care of the empty rows */
            if (sparse->ptrs[j] < 0)
                sparse->ptrs[j] = sparse->ptrs[j+1];
    }
    else if (stor_type == FIFFTS_MC_CCS) {
        for (k = 0, nz = 0; k < ncol; k++) {
            ptr = -1;
            for (j = 0; j < nrow; j++)
                if (fabs(dense[j][k]) > small) {
                    sparse->data[nz] = dense[j][k];
                    if (ptr < 0)
                        ptr = nz;
                    sparse->inds[nz++] = j;
                }
            sparse->ptrs[k] = ptr;
        }
        sparse->ptrs[ncol] = nz;
        for (k = ncol-1; k >= 0; k--) /* Take care of the empty columns */
            if (sparse->ptrs[k] < 0)
                sparse->ptrs[k] = sparse->ptrs[k+1];
    }
    return sparse;
}




int  mne_sparse_vec_mult2(INVERSELIB::FiffSparseMatrix* mat,     /* The sparse matrix */
                          float           *vector, /* Vector to be multiplied */
                          float           *res)    /* Result of the multiplication */
/*
      * Multiply a vector by a sparse matrix.
      */
{
    int i,j;

    if (mat->coding == FIFFTS_MC_RCS) {
        for (i = 0; i < mat->m; i++) {
            res[i] = 0.0;
            for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                res[i] += mat->data[j]*vector[mat->inds[j]];
        }
        return 0;
    }
    else if (mat->coding == FIFFTS_MC_CCS) {
        for (i = 0; i < mat->m; i++)
            res[i] = 0.0;
        for (i = 0; i < mat->n; i++)
            for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                res[mat->inds[j]] += mat->data[j]*vector[i];
        return 0;
    }
    else {
        printf("mne_sparse_vec_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
}


float *mne_sparse_vec_mult(INVERSELIB::FiffSparseMatrix* mat,
                           float *vector)

{
    float *res = MALLOC(mat->m,float);
    if (mne_sparse_vec_mult2(mat,vector,res) == 0)
        return res;
    else {
        FREE(res);
        return NULL;
    }
}


int  mne_sparse_mat_mult2(INVERSELIB::FiffSparseMatrix* mat,     /* The sparse matrix */
                          float           **mult,  /* Matrix to be multiplied */
                          int             ncol,	   /* How many columns in the above */
                          float           **res)   /* Result of the multiplication */
/*
      * Multiply a dense matrix by a sparse matrix.
      */
{
    int i,j,k;
    float val;

    if (mat->coding == FIFFTS_MC_RCS) {
        for (i = 0; i < mat->m; i++) {
            for (k = 0; k < ncol; k++) {
                val = 0.0;
                for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                    val += mat->data[j]*mult[mat->inds[j]][k];
                res[i][k] = val;
            }
        }
    }
    else if (mat->coding == FIFFTS_MC_CCS) {
        for (k = 0; k < ncol; k++) {
            for (i = 0; i < mat->m; i++)
                res[i][k] = 0.0;
            for (i = 0; i < mat->n; i++)
                for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                    res[mat->inds[j]][k] += mat->data[j]*mult[i][k];
        }
    }
    else {
        printf("mne_sparse_mat_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
    return 0;
}



//============================= mne_named_matrix.c =============================

void mne_free_name_list(char **list, int nlist)
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
        FREE(list[k]);
    }
    FREE(list);
    return;
}


void mne_free_sparse_named_matrix(mneSparseNamedMatrix mat)
/*
      * Free the matrix and all the data from within
      */
{
    if (!mat)
        return;
    mne_free_name_list(mat->rowlist,mat->nrow);
    mne_free_name_list(mat->collist,mat->ncol);
    if(mat->data)
        delete mat->data;
    FREE(mat);
    return;
}


/*
 * Handle matrices whose rows and/or columns are named with a list
 */



int mne_name_list_match(char **list1, int nlist1,
                        char **list2, int nlist2)
/*
 * Check whether two name lists are identical
 */
{
    int k;
    if (list1 == NULL && list2 == NULL)
        return 0;
    if (list1 == NULL || list2 == NULL)
        return 1;
    if (nlist1 != nlist2)
        return 1;
    for (k = 0; k < nlist1; k++)
        if (strcmp(list1[k],list2[k]) != 0)
            return 1;
    return 0;
}


char **mne_dup_name_list(char **list, int nlist)
/*
 * Duplicate a name list
 */
{
    char **res;
    int  k;
    if (list == NULL || nlist == 0)
        return NULL;
    res = MALLOC(nlist,char *);

    for (k = 0; k < nlist; k++)
        res[k] = mne_strdup(list[k]);
    return res;
}

void mne_string_to_name_list(char *s,char ***listp,int *nlistp)
/*
      * Convert a colon-separated list into a string array
      */
{
    char **list = NULL;
    int  nlist  = 0;
    char *one,*now=NULL;

    if (s != NULL && strlen(s) > 0) {
        s = mne_strdup(s);
        //strtok_r linux variant; strtok_s windows varainat
#ifdef __linux__
        for (one = strtok_r(s,":",&now); one != NULL; one = strtok_r(NULL,":",&now)) {
#elif _WIN32
        for (one = strtok_s(s,":",&now); one != NULL; one = strtok_s(NULL,":",&now)) {
#else
        for (one = strtok_r(s,":",&now); one != NULL; one = strtok_r(NULL,":",&now)) {
#endif
            list = REALLOC(list,nlist+1,char *);
            list[nlist++] = mne_strdup(one);
        }
        FREE(s);
    }
    *listp  = list;
    *nlistp = nlist;
    return;
}


char *mne_name_list_to_string(char **list,int nlist)
/*
* Convert a string array to a colon-separated string
*/
{
    int k,len;
    char *res;
    if (nlist == 0 || list == NULL)
        return NULL;
    for (k = len = 0; k < nlist; k++)
        len += strlen(list[k])+1;
    res = MALLOC(len,char);
    res[0] = '\0';
    for (k = len = 0; k < nlist-1; k++) {
        strcat(res,list[k]);
        strcat(res,":");
    }
    strcat(res,list[nlist-1]);
    return res;
}


char *mne_channel_names_to_string(fiffChInfo chs, int nch)
/*
* Make a colon-separated string out of channel names
*/
{
    char **names = MALLOC(nch,char *);
    char *res;
    int  k;

    if (nch <= 0)
        return NULL;
    for (k = 0; k < nch; k++)
        names[k] = chs[k].ch_name;
    res = mne_name_list_to_string(names,nch);
    FREE(names);
    return res;
}


void mne_channel_names_to_name_list(fiffChInfo chs, int nch,
                                    char ***listp, int *nlistp)

{
    char *s = mne_channel_names_to_string(chs,nch);
    mne_string_to_name_list(s,listp,nlistp);
    FREE(s);
    return;
}





//============================= mne_read_evoked.c =============================

#define MAXDATE 100


static FiffDirNode::SPtr find_evoked (const FiffDirNode::SPtr& node)
/*
* Find corresponding FIFFB_EVOKED node
*/
{
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;
    while (tmp_node->type != FIFFB_EVOKED) {
        if (tmp_node->parent == NULL)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    return (tmp_node);
}

static char *get_comment (  FiffStream::SPtr& stream,
                            const FiffDirNode::SPtr& start)

{
    int k;
    FiffTag::SPtr t_pTag;
    QList<FiffDirEntry::SPtr> ent = start->dir;
    for (k = 0; k < start->nent; k++)
        if (ent[k]->kind == FIFF_COMMENT) {
            if (FiffTag::read_tag(stream,t_pTag,ent[k]->pos)) {
                return (mne_strdup((char *)t_pTag->data()));
            }
        }
    return (mne_strdup("No comment"));
}

static void get_aspect_name_type(   FiffStream::SPtr& stream,
                                    const FiffDirNode::SPtr& start,
                                    char **namep, int *typep)

{
    int k;
    FiffTag::SPtr t_pTag;
    QList<FiffDirEntry::SPtr> ent = start->dir;
    const char *res = "unknown";
    int  type = -1;

    for (k = 0; k < start->nent; k++)
        if (ent[k]->kind == FIFF_ASPECT_KIND) {
            if (FiffTag::read_tag(stream,t_pTag,ent[k]->pos)) {
                type = *t_pTag->toInt();
                switch (type) {
                case FIFFV_ASPECT_AVERAGE :
                    res = "average";
                    break;
                case FIFFV_ASPECT_STD_ERR :
                    res = "std.error";
                    break;
                case FIFFV_ASPECT_SINGLE :
                    res = "single trace";
                    break;
                case FIFFV_ASPECT_SAMPLE :
                    res = "sample";
                    break;
                case FIFFV_ASPECT_SUBAVERAGE :
                    res = "subaverage";
                    break;
                case FIFFV_ASPECT_ALTAVERAGE :
                    res = "alt. average";
                    break;
                case FIFFV_ASPECT_POWER_DENSITY :
                    res = "power density spectrum";
                    break;
                case FIFFV_ASPECT_DIPOLE_WAVE :
                    res = "dipole amplitudes";
                    break;
                }
            }
            break;
        }
    if (namep)
        *namep = mne_strdup(res);
    if (typep)
        *typep = type;
    return;
}


static char *get_meas_date (    FiffStream::SPtr& stream,const FiffDirNode::SPtr& node  )
{
    int k;
    FiffTag::SPtr t_pTag;
    char *res = NULL;
    fiff_int_t kind, pos;
    FiffDirNode::SPtr meas_info;

    if (!(meas_info = find_meas_info(node))) {
        return res;
    }
    for (k = 0; k < meas_info->nent;k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        if (kind == FIFF_MEAS_DATE)
        {
            if (FiffTag::read_tag(stream,t_pTag,pos)) {
                fiffTime meas_date = (fiffTime)t_pTag->data();
                time_t   time = meas_date->secs;
                struct   tm *ltime;

                ltime = localtime(&time);
                res = MALLOC(MAXDATE,char);
                (void)strftime(res,MAXDATE,"%x %X",ltime);
                break;
            }
        }
    }
    return res;
}


int mne_find_evoked_types_comments (    FiffStream::SPtr& stream,
                                        QList<FiffDirNode::SPtr>& nodesp,
                                        int         **aspect_typesp,
                                        char        ***commentsp)
/*
* Find all data we are able to process
*/
{
    QList<FiffDirNode::SPtr> evoked;
    QList<FiffDirNode::SPtr> meas;
    QList<FiffDirNode::SPtr> nodes;
    int         evoked_count,count;
    char        *part,*type,*meas_date;
    char        **comments = NULL;
    int         *types = NULL;
    int         j,k,p;

    if (stream == NULL)
        return 0;
    /*
    * First find all measurements
    */
    meas = stream->tree()->dir_tree_find(FIFFB_MEAS);
    /*
    * Process each measurement
    */
    for (count = 0,p = 0; p < meas.size(); p++) {
        evoked = meas[p]->dir_tree_find(FIFFB_EVOKED);
        /*
        * Count the entries
        */
        for (evoked_count = 0, j = 0; j < evoked.size(); j++) {
            for (k = 0; k < evoked[j]->nchild; k++) {
                if (evoked[j]->children[k]->type == FIFFB_ASPECT) {
                    evoked_count++;
                }
            }
        }
        /*
        * Enlarge tables
        */
        comments = REALLOC(comments,count+evoked_count+1,char *);
        types    = REALLOC(types,count+evoked_count+1,int);
        /*
        * Insert node references and compile associated comments...
        */
        for (j = 0; j < evoked.size(); j++)	/* Evoked data */
            for (k = 0; k < evoked[j]->nchild; k++)
                if (evoked[j]->children[k]->type == FIFFB_ASPECT) {
                    meas_date = get_meas_date(stream,evoked[j]);
                    part      = get_comment(stream,evoked[j]);
                    get_aspect_name_type(stream,evoked[j]->children[k],&type,types+count);
                    if (meas_date) {
                        comments[count] = MALLOC(strlen(part)+strlen(type)+strlen(meas_date)+10,char);
                        sprintf(comments[count],"%s>%s>%s",meas_date,part,type);
                    }
                    else {
                        comments[count] = MALLOC(strlen(part)+strlen(type)+10,char);
                        sprintf(comments[count],"%s>%s",part,type);
                    }
                    nodes.append(evoked[j]->children[k]);
                    count++;
                }
    }
    if (count == 0) {   /* Nothing to report */
        FREE(comments);
        nodesp.clear();
        if (commentsp)
            *commentsp = NULL;
        if (aspect_typesp)
            *aspect_typesp = NULL;
        return 0;
    }
    else {              /* Return the appropriate variables */
        comments[count] = NULL;
        types[count]    = -1;
         nodesp = nodes;
        if (commentsp)
            *commentsp = comments;
        else
            mne_free_name_list(comments,count);
        if (aspect_typesp)
            *aspect_typesp = types;
        else
            FREE(types);
        return count;
    }
}


QList<FiffDirNode::SPtr> mne_find_evoked ( FiffStream::SPtr& stream, char ***commentsp)
/* Optionally return the compiled comments here */
{
    QList<FiffDirNode::SPtr> evoked;
    mne_find_evoked_types_comments(stream,evoked,NULL,commentsp);
    return evoked;
}


static int get_meas_info (  FiffStream::SPtr& stream,       /* The stream we are reading */
                            const FiffDirNode::SPtr& node,  /* The directory node containing our data */
                            fiffId *id,                     /* The block id from the nearest FIFFB_MEAS parent */
                            fiffTime *meas_date,            /* Measurement date */
                            int *nchan,                     /* Number of channels */
                            float *sfreq,                   /* Sampling frequency */
                            float *highpass,                /* Highpass filter setting */
                            float *lowpass,                 /* Lowpass filter setting */
                            fiffChInfo *chp,                /* Channel descriptions */
                            fiffCoordTrans *trans)          /* Coordinate transformation (head <-> device) */
/*
* Find channel information from
* nearest FIFFB_MEAS_INFO parent of
* node.
*/
{
    fiffChInfo ch;
    fiffChInfo this_ch;
    fiffCoordTrans t;
    int j,k;
    int to_find = 4;
    QList<FiffDirNode::SPtr> hpi;
    FiffDirNode::SPtr meas;
    FiffDirNode::SPtr meas_info;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;

    *chp     = NULL;
    ch       = NULL;
    *trans   = NULL;
    *id      = NULL;
    /*
    * Find desired parents
    */
    if (!(meas = find_meas(node))) {
        printf ("Meas. block not found!");
        goto bad;
    }
    if (!(meas_info = find_meas_info(node))) {
        printf ("Meas. info not found!");
        goto bad;
    }
    /*
    * Is there a block id is in the FIFFB_MEAS node?
    */
    if (!meas->id.isEmpty()) {
        *id = MALLOC(1,fiffIdRec);
        (*id)->version = meas->id.version;
        (*id)->machid[0] = meas->id.machid[0];
        (*id)->machid[1] = meas->id.machid[1];
        (*id)->time = meas->id.time;
    }
    /*
    * Others from FIFFB_MEAS_INFO
    */
    *lowpass = -1;
    *highpass = -1;
    for (k = 0; k < meas_info->nent; k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        switch (kind) {

        case FIFF_NCHAN :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *nchan = *t_pTag->toInt();
            ch = MALLOC(*nchan,fiffChInfoRec);
            for (j = 0; j < *nchan; j++)
                ch[j].scanNo = -1;
            to_find = to_find + *nchan - 1;
            break;

        case FIFF_SFREQ :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *sfreq = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_MEAS_DATE :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            if (*meas_date)
                FREE(*meas_date);
            *meas_date = MALLOC(1,fiffTimeRec);
            **meas_date = *(fiffTime)t_pTag->data();
            break;

        case FIFF_LOWPASS :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *lowpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_HIGHPASS :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *highpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_CH_INFO : /* Information about one channel */

            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            this_ch = (fiffChInfo)t_pTag->data();
            if (this_ch->scanNo <= 0 || this_ch->scanNo > *nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                memcpy(ch+this_ch->scanNo-1,this_ch,sizeof(fiffChInfoRec));
            to_find--;
            break;

        case FIFF_COORD_TRANS :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
            *t = *(fiffCoordTrans)t_pTag->data();
            /*
            * Require this particular transform!
            */
            if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                *trans = t;
                break;
            }
        }
    }
    /*
    * Search for the coordinate transformation from
    * HPI_RESULT block if it was not previously found
    */

    hpi = meas_info->dir_tree_find(FIFFB_HPI_RESULT);

    if (hpi.size() > 0 && *trans == NULL)
        for (k = 0; k < hpi[0]->nent; k++)
            if (hpi[0]->dir[k]->kind ==  FIFF_COORD_TRANS) {
                if (!FiffTag::read_tag(stream,t_pTag,hpi[0]->dir[k]->pos))
                    goto bad;
                t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
                *t = *(fiffCoordTrans)t_pTag->data();

                /*
                * Require this particular transform!
                */
                if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                    *trans = t;
                    break;
                }
            }
    if (*lowpass < 0) {
        *lowpass = *sfreq/2.0;
        to_find--;
    }
    if (*highpass < 0) {
        *highpass = 0.0;
        to_find--;
    }
    if (to_find != 0) {
        printf ("Not all essential tags were found!");
        goto bad;
    }
    *chp = ch;
    return (0);

bad : {
        FREE (ch);
        return (-1);
    }
}


static int find_between (   FiffStream::SPtr& stream,
                            const FiffDirNode::SPtr& low_node,
                            const FiffDirNode::SPtr& high_node,
                            int kind,
                            fiff_byte_t **data)
{
    FiffTag::SPtr t_pTag;
    FiffDirNode::SPtr node;
    fiffDirEntry dir;
    fiff_int_t kind_1, pos;
    int k;

    *data = NULL;
    node = low_node;
    while (node != NULL) {
        for (k = 0; k < node->nent; k++)
        {
            kind_1 = node->dir[k]->kind;
            pos  = node->dir[k]->pos;
            if (kind_1 == kind) {
                FREE (*data);
                if (!FiffTag::read_tag(stream,t_pTag,pos)) {
                    return (FIFF_FAIL);
                }
                else {
                    fiff_byte_t* tmp =  MALLOC(t_pTag->size(),fiff_byte_t);
                    fiff_byte_t* tmp_current = (fiff_byte_t *)t_pTag->data();

                    for( int k = 0; k < t_pTag->size(); ++k )
                        tmp[k] = tmp_current[k];

                    *data = tmp;
                    return (FIFF_OK);
                }
            }
        }
        if (node == high_node)
            break;
        node = node->parent;
    }
    return (FIFF_OK);
}

static int get_evoked_essentials (FiffStream::SPtr& stream,         /* This is our file */
                                  const FiffDirNode::SPtr& node,    /* The interesting node */
                                  float& sfreq,                     /* Sampling frequency
                                                                     * The value pointed by this is not
                                                                     * modified if individual sampling
                                                                     * frequency is found */
                                  float& tmin,                      /* Time scale minimum */
                                  int& nsamp,                       /* Number of samples */
                                  int& nave,                        /* Number of averaged responses */
                                  int& akind,                       /* Aspect type */
                                  int *& artefs,                /* Artefact removal parameters */
                                  int& nartef)
/*
      * Get the essential info for
      * given evoked response data
      */
{
    FiffTag::SPtr t_pTag;
    int k;
    int to_find = 2;
    int   first = -1;
    int   last = -1;
    int   my_nsamp = -1;
    float my_tmin = -1;
    int   res = -1;
    fiff_int_t kind, pos;

    fiff_byte_t *tempb;

    FiffDirNode::SPtr tmp_node = node;

    /*
    * This is rather difficult...
    */
    if (find_between (stream,tmp_node,tmp_node->parent,FIFF_NAVE,&tempb) == FIFF_FAIL)
        return res;
    if (tempb)
        nave = *(int *)tempb;
    FREE(tempb);
    if (find_between (stream,tmp_node,tmp_node->parent,
                      FIFF_SFREQ,&tempb) == FIFF_FAIL)
        return res;
    if (tempb)
        sfreq = *(float *)tempb;
    FREE(tempb);

    if (find_between (stream,tmp_node,tmp_node->parent,
                      FIFF_ASPECT_KIND,&tempb) == FIFF_FAIL)
        return res;
    if (tempb)
        akind = *(int *)tempb;
    else
        akind = FIFFV_ASPECT_AVERAGE; /* Just a guess */
    FREE(tempb);
    /*
   * Find evoked response descriptive data
   */
    tmp_node = tmp_node->parent;

//    tag.data = NULL;
    for (k = 0; k < tmp_node->dir_tree.size(); k++) {
        kind = tmp_node->dir_tree[k]->kind;
        pos  = tmp_node->dir_tree[k]->pos;
        switch (kind) {

        case FIFF_FIRST_SAMPLE :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            first = *t_pTag->toInt(); to_find--;
            break;

        case FIFF_LAST_SAMPLE :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            last = *t_pTag->toInt(); to_find--;
            break;

        case FIFF_NO_SAMPLES :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            my_nsamp = *t_pTag->toInt(); to_find--;
            break;

        case FIFF_FIRST_TIME :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            my_tmin = *t_pTag->toFloat(); to_find--;
            break;


        case FIFF_ARTEF_REMOVAL :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            qDebug() << "TODO: check whether artefs contains the right stuff -> use MatrixXi instead";
            artefs = t_pTag->toInt();
            nartef = t_pTag->size()/(3*sizeof(int));
            break;
        }
    }
    if (to_find > 0) {
        printf ("Not all essential tags were found!");
        goto out;
    }
    if (first != -1 && last != -1) {
        nsamp = (last)-(first)+1;
        tmin  = (first)/(sfreq);
    }
    else if (my_tmin != -1 && my_nsamp != -1) {
        tmin = my_tmin;
        nsamp = my_nsamp;
    }
    else {
        printf("Not enough data for time scale definition!");
        goto out;
    }
    res = 0;

out : {
        return res;
    }
}


static int get_evoked_optional( FiffStream::SPtr& stream,
                                const FiffDirNode::SPtr& node, /* The directory node containing our data */
                                int *nchan,	 /* Number of channels */
                                fiffChInfo *chp)	 /* Channel descriptions */
/*
* The channel info may have been modified
*/
{
    int res = FIFF_FAIL;
    fiffChInfo   new_ch = NULL;
    int          new_nchan = *nchan;
    int          k,to_find;
    FiffTag::SPtr t_pTag;
    fiff_int_t kind, pos;
    fiffChInfo   this_ch;
    FiffDirNode::SPtr evoked_node;

    if (!(evoked_node = find_evoked(node))) {
        res = FIFF_OK;
        goto out;
    }

    to_find = 0;
    if(evoked_node->find_tag(stream, FIFF_NCHAN, t_pTag))
        new_nchan = *t_pTag->toInt();
    else
        new_nchan = *nchan;

    for (k = 0; k < evoked_node->nent; k++) {
        kind = evoked_node->dir[k]->kind;
        pos  = evoked_node->dir[k]->pos;
        if (kind == FIFF_CH_INFO) {     /* Information about one channel */
            if (new_ch == NULL) {
                new_ch = MALLOC(new_nchan,fiffChInfoRec);
                to_find = new_nchan;
            }
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            this_ch = MALLOC(1,fiffChInfoRec);
            this_ch = (fiffChInfo)t_pTag->data();
            if (this_ch->scanNo <= 0 || this_ch->scanNo > new_nchan) {
                printf ("FIFF_CH_INFO : scan # out of range!");
                goto out;
            }
            else
                new_ch[this_ch->scanNo-1] = *this_ch;
            to_find--;
        }
    }
    if (to_find != 0) {
        printf("All channels were not specified "
               "at the FIFFB_EVOKED level.");
        goto out;
    }
    res = FIFF_OK;
    goto out;

out : {
        if (res == FIFF_OK) {
            *nchan = new_nchan;
            if (new_ch != NULL) {
                FREE(*chp);
                *chp = new_ch;
                new_ch = NULL;
            }
        }
        FREE(new_ch);
        return res;
    }
}







static void unpack_data(double offset,
                        double scale,
                        short *packed,
                        int   nsamp,
                        float *orig)
{
    int k;
    for (k = 0; k < nsamp; k++)
        orig[k] = scale * packed[k] + offset;
    return;
}


static float **get_epochs ( FiffStream::SPtr& stream,       /* This is our file */
                            const FiffDirNode::SPtr& node,  /* The interesting node */
                           int nchan, int nsamp)            /* Number of channels and number of samples to be expected */
/*
* Get the evoked response epochs
*/
{
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;
    int k;
    int ch;
    float **epochs = NULL;
    float offset,scale;
    short *packed;

    for (k = 0, ch = 0; k < node->nent && ch < nchan; k++) {
        kind = node->dir[k]->kind;
        pos  = node->dir[k]->pos;
        if (kind == FIFF_EPOCH) {
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            if (t_pTag->type & FIFFT_MATRIX) {
                if ((t_pTag->type & ~FIFFT_MATRIX) != FIFFT_FLOAT) {
                    printf("Epochs in matrix should be floats!");
                    goto bad;
                }

                qint32 ndim;
                QVector<qint32> dims;
                t_pTag->getMatrixDimensions(ndim, dims);

                if (ndim != 2) {
                    printf("Data matrix dimension should be two!");
                    goto bad;
                }
                if (dims[0] != nsamp) {
                    printf("Incorrect number of samples in data matrix!");
                    goto bad;
                }
                if (dims[1] != nchan) {
                    printf("Incorrect number of channels in data matrix!");
                    goto bad;
                }
                MatrixXf tmp_epochs = t_pTag->toFloatMatrix().transpose();
                epochs = ALLOC_CMATRIX(tmp_epochs.rows(),tmp_epochs.cols());
                fromFloatEigenMatrix(tmp_epochs, epochs);
                ch = nchan;
                break;  /* We have the data */
            }
            else {      /* Individual epochs */
                if (epochs == NULL)
                    epochs = ALLOC_CMATRIX(nchan,nsamp);
                if (t_pTag->type == FIFFT_OLD_PACK) {
                    offset = ((float *)t_pTag->data())[0];
                    scale  = ((float *)t_pTag->data())[1];
                    packed = (short *)(((float *)t_pTag->data())+2);
                    unpack_data(offset,scale,packed,nsamp,epochs[ch++]);
                }
                else if (t_pTag->type == FIFFT_FLOAT)
                    memcpy(epochs[ch++],t_pTag->data(),nsamp*sizeof(float));
                else {
                    printf ("Unknown data packing type!");
                    FREE_CMATRIX (epochs);
                    return (NULL);
                }
            }
            if (ch == nchan)
                return (epochs);
        }
    }
    if (ch < nchan) {
        printf ("All epochs were not found!");
        goto bad;
    }
    return (epochs);

bad : {
        FREE_CMATRIX (epochs);
        return (NULL);
    }
}



static void remove_artefacts (float *resp,
                              int   nsamp,
                              int   *artefs,
                              int   nartef)
/*
* Apply the artefact removal
*/
{
    int   start,end;
    int   j,k;
    float a,b;
    int   remove_jump;

    for (k = 0; k < nartef; k++) {
        if (artefs[3*k] == FIFFV_ARTEF_NONE || artefs[3*k] == FIFFV_ARTEF_KEEP)
            continue;
        remove_jump = (artefs[3*k] == FIFFV_ARTEF_NOJUMP);
        /*
        * Find out the indices for the start and end times
        */
        start = artefs[3*k+1];
        end   = artefs[3*k+2];
        start = MAX(0,MIN(start,nsamp));
        end   = MAX(0,MIN(end,nsamp));
        /*
        * Replace the artefact region with a straight line
        */
        if (start < end) {
            if (remove_jump) {	/* Remove jump... */
                a = resp[end] - resp[start];
                for (j = 0; j <=start; j++)
                    resp[j] = resp[j] + a;
                for (j = start+1 ; j < end; j++)
                    resp[j] = resp[end];
            }
            else {			/* Just connect... */
                a = (resp[end]-resp[start])/(end-start);
                b = (resp[start]*end - resp[end]*start)/(end-start);
                for (j = start+1 ; j < end; j++)
                    resp[j] = a*j+b;
            }
        }
    }
    return;
}


int mne_read_evoked(const QString& name,        /* Name of the file */
                    int        setno,           /* Which data set */
                    int        *nchanp,         /* How many channels */
                    int        *nsampp,         /* Number of time points */
                    float      *tminp,          /* First time point */
                    float      *sfreqp,         /* Sampling frequency */
                    fiffChInfo *chsp,           /* Channel info (this is now optional as well) */
                    float      ***epochsp,      /* Data, channel by channel */
                    /*
                    * Optional items follow
                    */
                    char       **commentp,      /* Comment for these data */
                    float      *highpassp,      /* Highpass frequency */
                    float      *lowpassp,       /* Lowpass frequency */
                    int        *navep,          /* How many averages */
                    int        *aspect_kindp,   /* What kind of an evoked data */
                    fiffCoordTrans *transp,     /* Coordinate transformation */
                    fiffId         *idp,        /* Measurement id */
                    fiffTime       *meas_datep) /* Measurement date */
/*
* Load evoked-response data from a fif file
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FiffDirNode::SPtr> evoked;    /* The evoked data nodes */
    int         nset    = 0;
    int         nchan   = 0;            /* How many channels */
    char        **comments = NULL;      /* The associated comments */
    float       sfreq = 0.0;            /* What sampling frequency */
    FiffDirNode::SPtr start;
    fiffChInfo   chs     = NULL;        /* Channel info */
    int          *artefs = NULL;        /* Artefact limits */
    int           nartef = 0;           /* How many */
    float       **epochs = NULL;        /* The averaged epochs */
    fiffCoordTrans trans = NULL;        /* The coordinate transformation */
    fiffId            id = NULL;        /* Measurement id */
    fiffTime          meas_date = NULL; /* Measurement date */
    int             nave = 1;           /* Number of averaged responses */
    float           tmin = 0;           /* Time scale minimum */
    float           lowpass;            /* Lowpass filter frequency */
    float           highpass = 0.0;     /* Highpass filter frequency */
    int             nsamp = 0;          /* Samples in epoch */
    int             aspect_kind;        /* What kind of data */
    int             res = FAIL;         /* A little bit of pessimism */

    float *epoch;
    int   j,k;

    if (setno < 0) {
        printf ("Evoked response selector must be positive!");
        goto out;
    }

    if(!stream->open())
        goto out;

    /*
    * Select correct data set
    */
    evoked = mne_find_evoked(stream,(commentp == NULL) ? NULL : &comments);
    if (!evoked.size()) {
        printf ("No evoked response data available here");
        goto out;
    }

    nset = evoked.size();

    if (setno < nset) {
        start = evoked[setno];
    }
    else {
        printf ("Too few evoked response data sets (how come?)");
        goto out;
    }
    /*
    * Get various things...
    */
    if (get_meas_info (stream,start,&id,&meas_date,&nchan,&sfreq,&highpass,&lowpass,&chs,&trans) == -1)
        goto out;

    /*
    * sfreq is listed here again because
    * there might be an individual one in the
    * evoked-response data
    */
    if (get_evoked_essentials(stream,start,sfreq,
                              tmin,nsamp,nave,aspect_kind,
                              artefs,nartef) == -1)
        goto out;
    /*
    * Some things may be redefined at a lower level
    */
    if (get_evoked_optional(stream,start,&nchan,&chs) == -1)
        goto out;
    /*
    * Omit nonmagnetic channels
    */
    if ((epochs = get_epochs(stream,start,nchan,nsamp)) == NULL)
        goto out;
    /*
    * Change artefact limits to start from 0
    */
    for (k = 0; k < nartef; k++) {
        qDebug() << "TODO: Artefact Vectors do not contain the right stuff!";
        artefs[2*k+1] = artefs[2*k+1] - sfreq*tmin;
        artefs[2*k+2] = artefs[2*k+2] - sfreq*tmin;
    }
    for (k = 0; k < nchan; k++) {
        epoch = epochs[k];
        for (j = 0; j < nsamp; j++)
            epoch[j] = chs[k].cal*epoch[j];
        remove_artefacts(epoch,nsamp,artefs,nartef);
    }
    /*
   * Ready to go
   */
    if (chsp) {
        *chsp    = chs; chs = NULL;
    }
    *tminp   = tmin;
    *nchanp  = nchan;
    *nsampp  = nsamp;
    *sfreqp  = sfreq;
    *epochsp = epochs; epochs = NULL;
    /*
   * Fill in the optional data
   */
    if (commentp) {
        *commentp = comments[setno];
        comments[setno] = NULL;
    }
    if (highpassp)
        *highpassp = highpass;
    if (lowpassp)
        *lowpassp = lowpass;
    if (transp) {
        *transp = trans;
        trans = NULL;
    }
    if (navep)
        *navep = nave;
    if (aspect_kindp)
        *aspect_kindp = aspect_kind;
    if (idp) {
        *idp = id;
        id = NULL;
    }
    if (meas_datep) {
        *meas_datep = meas_date;
        meas_date = NULL;
    }
    res = OK;
    /*
    * FREE all allocated data on exit
    */
out : {
        mne_free_name_list(comments,nset);
        FREE (chs);
        FREE (artefs);
        FREE (trans);
        FREE (id);
        FREE (meas_date);
        FREE_CMATRIX(epochs);
        stream->close();
        return res;
    }
}


//============================= mne_inverse_io.c =============================

#define MAXBUF 200

char *mne_format_file_id (fiffId id)

{
    char buf[MAXBUF];
    static char s[300];
    struct tm *ltime;
    time_t secs;

    secs = id->time.secs;
    ltime = localtime(&secs);
    (void)strftime(buf,MAXBUF,"%c",ltime);

    sprintf(s,"%d.%d 0x%x%x %s",id->version>>16,id->version & 0xFFFF,id->machid[0],id->machid[1],buf);
    return s;
}


//============================= mne_lin_proj.c =============================

/*
* Handle the linear projection operators
*/
mneProjOp mne_new_proj_op()

{
    mneProjOp new_proj_op = MALLOC(1,mneProjOpRec);

    new_proj_op->items     = NULL;
    new_proj_op->nitems    = 0;
    new_proj_op->names     = NULL;
    new_proj_op->nch       = 0;
    new_proj_op->nvec      = 0;
    new_proj_op->proj_data = NULL;
    return new_proj_op;
}




mneProjItem mne_new_proj_op_item()

{
    mneProjItem new_proj_item = MALLOC(1,mneProjItemRec);

    new_proj_item->vecs        = NULL;
    new_proj_item->kind        = FIFFV_PROJ_ITEM_NONE;
    new_proj_item->desc        = NULL;
    new_proj_item->nvec        = 0;
    new_proj_item->active      = TRUE;
    new_proj_item->active_file = FALSE;
    new_proj_item->has_meg     = FALSE;
    new_proj_item->has_eeg     = FALSE;
    return new_proj_item;
}


void mne_free_proj_op_proj(mneProjOp op)

{
    if (op == NULL)
        return;

    mne_free_name_list(op->names,op->nch);
    FREE_CMATRIX(op->proj_data);

    op->names  = NULL;
    op->nch  = 0;
    op->nvec = 0;
    op->proj_data = NULL;

    return;
}


void mne_free_proj_op_item(mneProjItem it)

{
    if (it == NULL)
        return;

    if(it->vecs)
        delete it->vecs;

    FREE(it->desc);
    FREE(it);
    return;
}


void mne_free_proj_op(mneProjOp op)

{
    int k;

    if (op == NULL)
        return;

    for (k = 0; k < op->nitems; k++)
        mne_free_proj_op_item(op->items[k]);
    FREE(op->items);

    mne_free_proj_op_proj(op);

    FREE(op);
    return;
}


void mne_proj_op_add_item_act(mneProjOp op, MneNamedMatrix* vecs, int kind, const char *desc, int is_active)
/*
* Add a new item to an existing projection operator
*/
{
    mneProjItem new_item;
    int         k;

    op->items = REALLOC(op->items,op->nitems+1,mneProjItem);

    op->items[op->nitems] = new_item = mne_new_proj_op_item();

    new_item->active      = is_active;
    new_item->vecs        = new MneNamedMatrix(*vecs);

    if (kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
        new_item->has_meg = FALSE;
        new_item->has_eeg = TRUE;
    }
    else {
        for (k = 0; k < vecs->ncol; k++) {
            if (strstr(vecs->collist[k],"EEG") == vecs->collist[k])
                new_item->has_eeg = TRUE;
            if (strstr(vecs->collist[k],"MEG") == vecs->collist[k])
                new_item->has_meg = TRUE;
        }
        if (!new_item->has_meg && !new_item->has_eeg) {
            new_item->has_meg = TRUE;
            new_item->has_eeg = FALSE;
        }
        else if (new_item->has_meg && new_item->has_eeg) {
            new_item->has_meg = TRUE;
            new_item->has_eeg = FALSE;
        }
    }
    if (desc != NULL)
        new_item->desc = mne_strdup(desc);
    new_item->kind = kind;
    new_item->nvec = new_item->vecs->nrow;

    op->nitems++;

    mne_free_proj_op_proj(op);  /* These data are not valid any more */
    return;
}


void mne_proj_op_add_item(mneProjOp op, MneNamedMatrix* vecs, int kind, const char *desc)

{
    mne_proj_op_add_item_act(op, vecs, kind, desc, TRUE);
}


mneProjOp mne_dup_proj_op(mneProjOp op)
/*
* Provide a duplicate (item data only)
*/
{
    mneProjOp dup = mne_new_proj_op();
    mneProjItem it;
    int k;

    if (!op)
        return NULL;

    for (k = 0; k < op->nitems; k++) {
        it = op->items[k];
        mne_proj_op_add_item_act(dup,it->vecs,it->kind,it->desc,it->active);
        dup->items[k]->active_file = it->active_file;
    }
    return dup;
}



static char *add_string(char *old,char *add)

{
    char *news = NULL;
    if (!old) {
        if (add || strlen(add) > 0)
            news = mne_strdup(add);
    }
    else {
        old = REALLOC(old,strlen(old) + strlen(add) + 1,char);
        strcat(old,add);
        news = old;
    }
    return news;
}




int mne_proj_op_chs(mneProjOp op, char **list, int nlist)

{
    if (op == NULL)
        return OK;

    mne_free_proj_op_proj(op);  /* These data are not valid any more */

    if (nlist == 0)
        return OK;

    op->names = mne_dup_name_list(list,nlist);
    op->nch   = nlist;

    return OK;
}


void mne_proj_op_report_data(FILE *out,const char *tag, mneProjOp op, int list_data,
                             char **exclude, int nexclude)
/*
* Output info about the projection operator
*/
{
    int j,k,p,q;
    mneProjItem it;
    MneNamedMatrix* vecs;
    int found;

    if (out == NULL)
        return;
    if (op == NULL)
        return;
    if (op->nitems <= 0) {
        fprintf(out,"Empty operator\n");
        return;
    }

    for (k = 0; k < op->nitems; k++) {
        it = op->items[k];
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (tag)
            fprintf(out,"%s",tag);
        fprintf(out,"# %d : %s : %d vecs : %d chs %s %s\n",
                k+1,it->desc,it->nvec,it->vecs->ncol,
                it->has_meg ? "MEG" : "EEG",
                it->active ? "active" : "idle");
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (list_data) {
            vecs = op->items[k]->vecs;

            for (q = 0; q < vecs->ncol; q++) {
                fprintf(out,"%-10s",vecs->collist[q]);
                fprintf(out,q < vecs->ncol-1 ? " " : "\n");
            }
            for (p = 0; p < vecs->nrow; p++)
                for (q = 0; q < vecs->ncol; q++) {
                    for (j = 0, found  = 0; j < nexclude; j++) {
                        if (strcmp(exclude[j],vecs->collist[q]) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    fprintf(out,"%10.5g ",found ? 0.0 : vecs->data[p][q]);
                    fprintf(out,q < vecs->ncol-1 ? " " : "\n");
                }
            if (list_data && tag)
                fprintf(out,"%s\n",tag);
        }
    }
    return;
}


void mne_proj_op_report(FILE *out,const char *tag, mneProjOp op)
{
    mne_proj_op_report_data(out,tag,op, FALSE, NULL, 0);
}


int mne_proj_item_affect(mneProjItem it, char **list, int nlist)
/*
* Does this projection item affect this list of channels?
*/
{
    int k,p,q;

    if (it == NULL || it->vecs == NULL || it->nvec == 0)
        return FALSE;

    for (k = 0; k < nlist; k++)
        for (p = 0; p < it->vecs->ncol; p++)
            if (strcmp(it->vecs->collist[p],list[k]) == 0) {
                for (q = 0; q < it->vecs->nrow; q++) {
                    if (it->vecs->data[q][p] != 0.0)
                        return TRUE;
                }
            }
    return FALSE;
}


int mne_proj_op_affect(mneProjOp op, char **list, int nlist)

{
    int k;
    int naff;

    if (!op)
        return 0;

    for (k = 0, naff = 0; k < op->nitems; k++)
        if (op->items[k]->active && mne_proj_item_affect(op->items[k],list,nlist))
            naff += op->items[k]->nvec;

    return naff;
}


static void clear_these(float *data, char **names, int nnames, const char *start)

{
    int k;
    for (k = 0; k < nnames; k++)
        if (strstr(names[k],start) == names[k])
            data[k] = 0.0;
}


int mne_proj_op_affect_chs(mneProjOp op, fiffChInfo chs, int nch)
{
    char *ch_string;
    int  res;
    char **list;
    int  nlist;


    if (nch == 0)
        return FALSE;
    ch_string = mne_channel_names_to_string(chs,nch);
    mne_string_to_name_list(ch_string,&list,&nlist);
    FREE(ch_string);
    res = mne_proj_op_affect(op,list,nlist);
    mne_free_name_list(list,nlist);
    return res;
}


int mne_proj_op_proj_dvector(mneProjOp op, double *vec, int nch, int do_complement)
/*
* Apply projection operator to a vector (doubles)
* Assume that all dimension checking etc. has been done before
*/
{
    float *pvec;
    double w;
    int k,p;

    if (op->nvec <= 0)
        return OK;

    if (op->nch != nch) {
        qCritical("Data vector size does not match projection operator");
        return FAIL;
    }

    Eigen::VectorXd res = Eigen::VectorXd::Zero(op->nch);

    for (p = 0; p < op->nvec; p++) {
        pvec = op->proj_data[p];
        w = 0.0;
        for (k = 0; k < op->nch; k++)
            w += vec[k]*pvec[k];
        for (k = 0; k < op->nch; k++)
            res[k] = res[k] + w*pvec[k];
    }

    if (do_complement) {
        for (k = 0; k < op->nch; k++)
            vec[k] = vec[k] - res[k];
    }
    else {
        for (k = 0; k < op->nch; k++)
            vec[k] = res[k];
    }

    return OK;
}




int mne_proj_op_apply_cov(mneProjOp op, mneCovMatrix& c)
/*
* Apply the projection operator to a covariance matrix
*/
{
    double **dcov = NULL;
    int j,k,p;
    int do_complement = TRUE;

    if (op == NULL || op->nitems == 0)
        return OK;

    if (mne_name_list_match(op->names,op->nch,c->names,c->ncov) != OK) {
        qCritical("Incompatible data in mne_proj_op_apply_cov");
        return FAIL;
    }

    dcov = ALLOC_DCMATRIX(c->ncov,c->ncov);
    /*
    * Return the appropriate result
    */
    if (c->cov_diag) {  /* Pick the diagonals */
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k < c->ncov; k++)
                dcov[j][k] = (j == k) ? c->cov_diag[j] : 0;
    }
    else {              /* Return full matrix */
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                dcov[j][k] = c->cov[p++];
        for (j = 0; j < c->ncov; j++)
            for (k = j+1; k < c->ncov; k++)
                dcov[j][k] = dcov[k][j];
    }

    /*
    * Project from front and behind
    */
    for (k = 0; k < c->ncov; k++) {
        if (mne_proj_op_proj_dvector(op,dcov[k],c->ncov,do_complement) != OK)
            return FAIL;
    }

    mne_transpose_dsquare(dcov,c->ncov);

    for (k = 0; k < c->ncov; k++)
        if (mne_proj_op_proj_dvector(op,dcov[k],c->ncov,do_complement) != OK)
            return FAIL;

    /*
    * Return the result
    */
    if (c->cov_diag) {  /* Pick the diagonal elements */
        for (j = 0; j < c->ncov; j++) {
            c->cov_diag[j] = dcov[j][j];
        }
        FREE(c->cov); c->cov = NULL;
    }
    else {              /* Put everything back */
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                c->cov[p++] = dcov[j][k];
    }

    FREE_DCMATRIX(dcov);

    c->nproj = mne_proj_op_affect(op,c->names,c->ncov);
    return OK;
}


#define USE_LIMIT   1e-5
#define SMALL_VALUE 1e-4

int mne_proj_op_make_proj_bad(mneProjOp op, char **bad, int nbad)
/*
* Do the channel picking and SVD
* Include a bad list at this phase
* Input to the projection can include the bad channels
* but they are not affected
*/
{
    int   k,p,q,r,nvec;
    float **vv_meg  = NULL;
    float *sing_meg = NULL;
    float **vv_eeg  = NULL;
    float *sing_eeg = NULL;
    float **mat_meg = NULL;
    float **mat_eeg = NULL;
    int   nvec_meg;
    int   nvec_eeg;
    mneNamedVectorRec vec;
    float size;
    int   nzero;
#ifdef DEBUG
    char  name[20];
#endif

    FREE_CMATRIX(op->proj_data);
    op->proj_data = NULL;
    op->nvec      = 0;

    if (op->nch <= 0)
        return OK;
    if (op->nitems <= 0)
        return OK;

    nvec = mne_proj_op_affect(op,op->names,op->nch);
    if (nvec == 0)
        return OK;

    mat_meg = ALLOC_CMATRIX(nvec, op->nch);
    mat_eeg = ALLOC_CMATRIX(nvec, op->nch);

#ifdef DEBUG
    fprintf(stdout,"mne_proj_op_make_proj_bad\n");
#endif
    for (k = 0, nvec_meg = nvec_eeg = 0; k < op->nitems; k++) {
        if (op->items[k]->active && mne_proj_item_affect(op->items[k],op->names,op->nch)) {
            vec.nvec  = op->items[k]->vecs->ncol;
            vec.names = op->items[k]->vecs->collist;
            if (op->items[k]->has_meg) {
                for (p = 0; p < op->items[k]->nvec; p++, nvec_meg++) {
                    vec.data = op->items[k]->vecs->data[p];
                    if (mne_pick_from_named_vector(&vec,op->names,op->nch,FALSE,mat_meg[nvec_meg]) == FAIL)
                        goto bad;
#ifdef DEBUG
                    fprintf(stdout,"Original MEG:\n");
                    mne_print_vector(stdout,op->items[k]->desc,mat_meg[nvec_meg],op->nch);
                    fflush(stdout);
#endif
                }
            }
            else if (op->items[k]->has_eeg) {
                for (p = 0; p < op->items[k]->nvec; p++, nvec_eeg++) {
                    vec.data = op->items[k]->vecs->data[p];
                    if (mne_pick_from_named_vector(&vec,op->names,op->nch,FALSE,mat_eeg[nvec_eeg]) == FAIL)
                        goto bad;
#ifdef DEBUG
                    fprintf (stdout,"Original EEG:\n");
                    mne_print_vector(stdout,op->items[k]->desc,mat_eeg[nvec_eeg],op->nch);
                    fflush(stdout);
#endif
                }
            }
        }
    }
    /*
    * Replace bad channel entries with zeroes
    */
    for (q = 0; q < nbad; q++)
        for (r = 0; r < op->nch; r++)
            if (strcmp(op->names[r],bad[q]) == 0) {
                for (p = 0; p < nvec_meg; p++)
                    mat_meg[p][r] = 0.0;
                for (p = 0; p < nvec_eeg; p++)
                    mat_eeg[p][r] = 0.0;
            }
    /*
    * Scale the rows so that detection of linear dependence becomes easy
    */
    for (p = 0, nzero = 0; p < nvec_meg; p++) {
        size = sqrt(mne_dot_vectors(mat_meg[p],mat_meg[p],op->nch));
        if (size > 0) {
            for (k = 0; k < op->nch; k++)
                mat_meg[p][k] = mat_meg[p][k]/size;
        }
        else
            nzero++;
    }
    if (nzero == nvec_meg) {
        FREE_CMATRIX(mat_meg); mat_meg = NULL; nvec_meg = 0;
    }
    for (p = 0, nzero = 0; p < nvec_eeg; p++) {
        size = sqrt(mne_dot_vectors(mat_eeg[p],mat_eeg[p],op->nch));
        if (size > 0) {
            for (k = 0; k < op->nch; k++)
                mat_eeg[p][k] = mat_eeg[p][k]/size;
        }
        else
            nzero++;
    }
    if (nzero == nvec_eeg) {
        FREE_CMATRIX(mat_eeg); mat_eeg = NULL; nvec_eeg = 0;
    }
    if (nvec_meg + nvec_eeg == 0) {
        fprintf(stderr,"No projection remains after excluding bad channels. Omitting projection.\n");
        return OK;
    }
    /*
    * Proceed to SVD
    */
#ifdef DEBUG
    fprintf(stdout,"Before SVD:\n");
#endif
    if (nvec_meg > 0) {
#ifdef DEBUG
        fprintf(stdout,"---->>\n");
        for (p = 0; p < nvec_meg; p++) {
            sprintf(name,"MEG %02d",p+1);
            mne_print_vector(stdout,name,mat_meg[p],op->nch);
        }
        fprintf(stdout,"---->>\n");
#endif
        sing_meg = MALLOC(nvec_meg+1,float);
        vv_meg   = ALLOC_CMATRIX(nvec_meg,op->nch);
        if (mne_svd(mat_meg,nvec_meg,op->nch,sing_meg,NULL,vv_meg) != OK)
            goto bad;
    }
    if (nvec_eeg > 0) {
#ifdef DEBUG
        fprintf(stdout,"---->>\n");
        for (p = 0; p < nvec_eeg; p++) {
            sprintf(name,"EEG %02d",p+1);
            mne_print_vector(stdout,name,mat_eeg[p],op->nch);
        }
        fprintf(stdout,"---->>\n");
#endif
        sing_eeg = MALLOC(nvec_eeg+1,float);
        vv_eeg   = ALLOC_CMATRIX(nvec_eeg,op->nch);
        if (mne_svd(mat_eeg,nvec_eeg,op->nch,sing_eeg,NULL,vv_eeg) != OK)
            goto bad;
    }
    /*
    * Check for linearly dependent vectors
    */
    for (p = 0, op->nvec = 0; p < nvec_meg; p++, op->nvec++)
        if (sing_meg[p]/sing_meg[0] < USE_LIMIT)
            break;
    for (p = 0; p < nvec_eeg; p++, op->nvec++)
        if (sing_eeg[p]/sing_eeg[0] < USE_LIMIT)
            break;
#ifdef DEBUG
    fprintf(stderr,"Number of linearly independent vectors = %d\n",op->nvec);
#endif
    op->proj_data = ALLOC_CMATRIX(op->nvec,op->nch);
#ifdef DEBUG
    fprintf(stdout,"Final projection data:\n");
#endif
    for (p = 0, op->nvec = 0; p < nvec_meg; p++, op->nvec++) {
        if (sing_meg[p]/sing_meg[0] < USE_LIMIT)
            break;
        for (k = 0; k < op->nch; k++) {
            /*
            * Avoid crosstalk between MEG/EEG
            */
            if (fabs(vv_meg[p][k]) < SMALL_VALUE)
                op->proj_data[op->nvec][k] = 0.0;
            else
                op->proj_data[op->nvec][k] = vv_meg[p][k];

            /*
            * If the above did not work, this will (provided that EEG channels are called EEG*)
            */
            clear_these(op->proj_data[op->nvec],op->names,op->nch,"EEG");
        }
#ifdef DEBUG
        sprintf(name,"MEG %02d",p+1);
        mne_print_vector(stdout,name,op->proj_data[op->nvec],op->nch);
#endif
    }
    for (p = 0; p < nvec_eeg; p++, op->nvec++) {
        if (sing_eeg[p]/sing_eeg[0] < USE_LIMIT)
            break;
        for (k = 0; k < op->nch; k++) {
            /*
            * Avoid crosstalk between MEG/EEG
            */
            if (fabs(vv_eeg[p][k]) < SMALL_VALUE)
                op->proj_data[op->nvec][k] = 0.0;
            else
                op->proj_data[op->nvec][k] = vv_eeg[p][k];
            /*
            * If the above did not work, this will (provided that MEG channels are called MEG*)
            */
            clear_these(op->proj_data[op->nvec],op->names,op->nch,"MEG");
        }
#ifdef DEBUG
        sprintf(name,"EEG %02d",p+1);
        mne_print_vector(stdout,name,op->proj_data[op->nvec],op->nch);
        fflush(stdout);
#endif
    }
    FREE(sing_meg);
    FREE_CMATRIX(vv_meg);
    FREE_CMATRIX(mat_meg);
    FREE(sing_eeg);
    FREE_CMATRIX(vv_eeg);
    FREE_CMATRIX(mat_eeg);
    /*
     * Make sure that the stimulus channels are not modified
     */
    for (k = 0; k < op->nch; k++)
        if (strstr(op->names[k],"STI") == op->names[k]) {
            for (p = 0; p < op->nvec; p++)
                op->proj_data[p][k] = 0.0;
        }

    return OK;

bad : {
        FREE(sing_meg);
        FREE_CMATRIX(vv_meg);
        FREE_CMATRIX(mat_meg);
        FREE(sing_eeg);
        FREE_CMATRIX(vv_eeg);
        FREE_CMATRIX(mat_eeg);
        return FAIL;
    }
}


int mne_proj_op_make_proj(mneProjOp op)
/*
* Do the channel picking and SVD
*/
{
    return mne_proj_op_make_proj_bad(op,NULL,0);
}


int mne_proj_op_proj_vector(mneProjOp op, float *vec, int nvec, int do_complement)
/*
* Apply projection operator to a vector (floats)
* Assume that all dimension checking etc. has been done before
*/
{
    static float *res = NULL;
    int    res_size   = 0;
    float *pvec;
    float  w;
    int k,p;

    if (!op || op->nitems <= 0 || op->nvec <= 0)
        return OK;

    if (op->nch != nvec) {
        printf("Data vector size does not match projection operator");
        return FAIL;
    }

    if (op->nch > res_size) {
        res = REALLOC(res,op->nch,float);
        res_size = op->nch;
    }

    for (k = 0; k < op->nch; k++)
        res[k] = 0.0;

    for (p = 0; p < op->nvec; p++) {
        pvec = op->proj_data[p];
        w = mne_dot_vectors(pvec,vec,op->nch);
        for (k = 0; k < op->nch; k++)
            res[k] = res[k] + w*pvec[k];
    }
    if (do_complement) {
        for (k = 0; k < op->nch; k++)
            vec[k] = vec[k] - res[k];
    }
    else {
        for (k = 0; k < op->nch; k++)
            vec[k] = res[k];
    }
    return OK;
}


mneProjOp mne_proj_op_combine(mneProjOp to, mneProjOp from)
/*
* Copy items from 'from' operator to 'to' operator
*/
{
    int k;
    mneProjItem it;

    if (to == NULL)
        to = mne_new_proj_op();
    if (from) {
        for (k = 0; k < from->nitems; k++) {
            it = from->items[k];
            mne_proj_op_add_item(to,it->vecs,it->kind,it->desc);
            to->items[to->nitems-1]->active_file = it->active_file;
        }
    }
    return to;
}


mneProjOp mne_proj_op_average_eeg_ref(fiffChInfo chs,
                                      int nch)
/*
* Make the projection operator for average electrode reference
*/
{
    int eegcount = 0;
    int k;
    float       **vec_data;
    char        **names;
    MneNamedMatrix* vecs;
    mneProjOp      op;

    for (k = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            eegcount++;
    if (eegcount == 0) {
        qCritical("No EEG channels specified for average reference.");
        return NULL;
    }

    vec_data = ALLOC_CMATRIX(1,eegcount);
    names    = MALLOC(eegcount,char *);

    for (k = 0, eegcount = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            names[eegcount++] = mne_strdup(chs[k].ch_name);

    for (k = 0; k < eegcount; k++)
        vec_data[0][k] = 1.0/sqrt((double)eegcount);

    vecs = MneNamedMatrix::build_named_matrix(1,eegcount,NULL,names,vec_data);

    op = mne_new_proj_op();
    mne_proj_op_add_item(op,vecs,FIFFV_MNE_PROJ_ITEM_EEG_AVREF,"Average EEG reference");

    return op;
}


//============================= mne_lin_proj_io.c =============================

mneProjOp mne_read_proj_op_from_node(//fiffFile in,
                                     FiffStream::SPtr& stream,
                                     const FiffDirNode::SPtr& start)
/*
* Load all the linear projection data
*/
{
    mneProjOp   op     = NULL;
    QList<FiffDirNode::SPtr> proj;
    FiffDirNode::SPtr start_node;
    QList<FiffDirNode::SPtr> items;
    FiffDirNode::SPtr node;
    int         k;
    char        *item_desc,*desc_tag,*lf;
    int         global_nchan,item_nchan,nlist;
    char        **item_names;
    int         item_kind;
    float       **item_vectors = NULL;
    int         item_nvec;
    int         item_active;
    MneNamedMatrix* item;
    FiffTag::SPtr t_pTag;

    if (!stream) {
        qCritical("File not open mne_read_proj_op_from_node");
        goto bad;
    }

    if (!start || start->isEmpty())
        start_node = stream->tree();
    else
        start_node = start;

    op = mne_new_proj_op();
    proj = start_node->dir_tree_find(FIFFB_PROJ);
    if (proj.size() == 0 || proj[0]->isEmpty())   /* The caller must recognize an empty projection */
        goto out;
    /*
    * Only the first projection block is recognized
    */
    items = proj[0]->dir_tree_find(FIFFB_PROJ_ITEM);
    if (items.size() == 0 || items[0]->isEmpty())   /* The caller must recognize an empty projection */
        goto out;
    /*
    * Get a common number of channels
    */
    node = proj[0];
    if(!node->find_tag(stream, FIFF_NCHAN, t_pTag))
        global_nchan = 0;
    else {
        global_nchan = *t_pTag->toInt();
//        TAG_FREE(tag);
    }
    /*
   * Proceess each item
   */
    for (k = 0; k < items.size(); k++) {
        node = items[k];
        /*
        * Complicated procedure for getting the description
        */
        item_desc = NULL;

        if (node->find_tag(stream, FIFF_NAME, t_pTag)) {
            item_desc = add_string(item_desc,(char *)t_pTag->data());
        }

        /*
        * Take the first line of description if it exists
        */
        if (node->find_tag(stream, FIFF_DESCRIPTION, t_pTag)) {
            desc_tag = (char *)t_pTag->data();
            if ((lf = strchr(desc_tag,'\n')) != NULL)
                *lf = '\0';
            printf("###################DEBUG ToDo: item_desc = add_string(item_desc," ");");
            item_desc = add_string(item_desc,(char *)desc_tag);
            FREE(desc_tag);
        }
        /*
        * Possibility to override number of channels here
        */
        if (!node->find_tag(stream, FIFF_NCHAN, t_pTag)) {
            item_nchan = global_nchan;
        }
        else {
            item_nchan = *t_pTag->toInt();
        }
        if (item_nchan <= 0) {
            qCritical("Number of channels incorrectly specified for one of the projection items.");
            goto bad;
        }
        /*
        * Take care of the channel names
        */
        if (!node->find_tag(stream, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag))
            goto bad;

        mne_string_to_name_list((char *)(t_pTag->data()),&item_names,&nlist);
        if (nlist != item_nchan) {
            printf("Channel name list incorrectly specified for proj item # %d",k+1);
            mne_free_name_list(item_names,nlist);
            goto bad;
        }
        /*
        * Kind of item
        */
        if (!node->find_tag(stream, FIFF_PROJ_ITEM_KIND, t_pTag))
            goto bad;
        item_kind = *t_pTag->toInt();
        /*
        * How many vectors
        */
        if (!node->find_tag(stream,FIFF_PROJ_ITEM_NVEC, t_pTag))
            goto bad;
        item_nvec = *t_pTag->toInt();
        /*
        * The projection data
        */
        if (!node->find_tag(stream,FIFF_PROJ_ITEM_VECTORS, t_pTag))
            goto bad;

        MatrixXf tmp_item_vectors = t_pTag->toFloatMatrix().transpose();
        item_vectors = ALLOC_CMATRIX(tmp_item_vectors.rows(),tmp_item_vectors.cols());
        fromFloatEigenMatrix(tmp_item_vectors, item_vectors);

        /*
        * Is this item active?
        */
        if (node->find_tag(stream, FIFF_MNE_PROJ_ITEM_ACTIVE, t_pTag)) {
            item_active = *t_pTag->toInt();
        }
        else
            item_active = FALSE;
        /*
        * Ready to add
        */
        item = MneNamedMatrix::build_named_matrix(item_nvec,item_nchan,NULL,item_names,item_vectors);
        mne_proj_op_add_item_act(op,item,item_kind,item_desc,item_active);
        delete item;
        op->items[op->nitems-1]->active_file = item_active;
    }

out :
    return op;

bad : {
        mne_free_proj_op(op);
        return NULL;
    }
}

mneProjOp mne_read_proj_op(const QString& name)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if(!stream->open())
        return NULL;

    mneProjOp   res = NULL;

    FiffDirNode::SPtr t_default;
    res = mne_read_proj_op_from_node(stream,t_default);

    stream->close();

    return res;
}


//============================= mne_ctf_comp.c =============================

#define MNE_CTFV_COMP_UNKNOWN -1
#define MNE_CTFV_COMP_NONE    0
#define MNE_CTFV_COMP_G1BR    0x47314252
#define MNE_CTFV_COMP_G2BR    0x47324252
#define MNE_CTFV_COMP_G3BR    0x47334252
#define MNE_CTFV_COMP_G2OI    0x47324f49
#define MNE_CTFV_COMP_G3OI    0x47334f49

static struct {
    int grad_comp;
    int ctf_comp;
} compMap[] = { { MNE_CTFV_NOGRAD,       MNE_CTFV_COMP_NONE },
{ MNE_CTFV_GRAD1,        MNE_CTFV_COMP_G1BR },
{ MNE_CTFV_GRAD2,        MNE_CTFV_COMP_G2BR },
{ MNE_CTFV_GRAD3,        MNE_CTFV_COMP_G3BR },
{ MNE_4DV_COMP1,         MNE_4DV_COMP1 },             /* One-to-one mapping for 4D data */
{ MNE_CTFV_COMP_UNKNOWN, MNE_CTFV_COMP_UNKNOWN }};

/*
 * Allocation and freeing of the data structures
 */
mneCTFcompData mne_new_ctf_comp_data()
{
    mneCTFcompData res = MALLOC(1,mneCTFcompDataRec);
    res->kind          = MNE_CTFV_COMP_UNKNOWN;
    res->mne_kind      = MNE_CTFV_COMP_UNKNOWN;
    res->calibrated    = FALSE;
    res->data          = NULL;
    res->presel        = NULL;
    res->postsel       = NULL;
    res->presel_data   = NULL;
    res->comp_data     = NULL;
    res->postsel_data  = NULL;

    return res;
}

mneCTFcompDataSet mne_new_ctf_comp_data_set()
{
    mneCTFcompDataSet res = MALLOC(1,mneCTFcompDataSetRec);

    res->comps   = NULL;
    res->ncomp   = 0;
    res->chs     = NULL;
    res->nch     = 0;
    res->current = NULL;
    res->undo    = NULL;
    return res;
}



void mne_free_ctf_comp_data(mneCTFcompData comp)

{
    if (!comp)
        return;

    if(comp->data)
        delete comp->data;
    if(comp->presel)
        delete comp->presel;
    if(comp->postsel)
        delete comp->postsel;
    FREE(comp->presel_data);
    FREE(comp->postsel_data);
    FREE(comp->comp_data);
    FREE(comp);
    return;
}


void mne_free_ctf_comp_data_set(mneCTFcompDataSet set)

{
    int k;

    if (!set)
        return;

    for (k = 0; k < set->ncomp; k++)
        mne_free_ctf_comp_data(set->comps[k]);
    FREE(set->comps);
    FREE(set->chs);
    mne_free_ctf_comp_data(set->current);
    FREE(set);
    return;
}


mneCTFcompData mne_dup_ctf_comp_data(mneCTFcompData data)
{
    mneCTFcompData res;

    if (!data)
        return NULL;

    res = mne_new_ctf_comp_data();

    res->kind       = data->kind;
    res->mne_kind   = data->mne_kind;
    res->calibrated = data->calibrated;
    res->data       = new MneNamedMatrix(*data->data);

    res->presel     = new FiffSparseMatrix(*data->presel);
    res->postsel    = new FiffSparseMatrix(*data->postsel);

    return res;
}


mneCTFcompDataSet mne_dup_ctf_comp_data_set(mneCTFcompDataSet set)
/*
* Make a verbatim copy of a data set
*/
{
    mneCTFcompDataSet res;
    int  k;

    if (!set)
        return NULL;

    res = mne_new_ctf_comp_data_set();

    if (set->ncomp > 0) {
        res->comps = MALLOC(set->ncomp,mneCTFcompData);
        res->ncomp = set->ncomp;
        for (k = 0; k < res->ncomp; k++)
            res->comps[k] = mne_dup_ctf_comp_data(set->comps[k]);
    }
    res->current = mne_dup_ctf_comp_data(set->current);

    return res;
}


int mne_apply_ctf_comp(mneCTFcompDataSet set,		  /* The compensation data */
                       int               do_it,
                       float             *data,           /* The data to process */
                       int               ndata,
                       float             *compdata,       /* Data containing the compensation channels */
                       int               ncompdata)
/*
* Apply compensation or revert to uncompensated data
*/
{
    mneCTFcompData this_comp;
    float *presel,*comp;
    int   k;

    if (compdata == NULL) {
        compdata  = data;
        ncompdata = ndata;
    }
    if (!set || !set->current)
        return OK;
    this_comp = set->current;
    /*
   * Dimension checks
   */
    if (this_comp->presel) {
        if (this_comp->presel->n != ncompdata) {
            printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->presel->n,ncompdata);
            return FAIL;
        }
    }
    else if (this_comp->data->ncol != ncompdata) {
        printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->ncol,ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->m != ndata) {
            printf("Data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->postsel->m,ndata);
            return FAIL;
        }
    }
    else if (this_comp->data->nrow != ndata) {
        printf("Data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->nrow,ndata);
        return FAIL;
    }
    /*
    * Preselection is optional
    */
    if (this_comp->presel) {
        if (!this_comp->presel_data)
            this_comp->presel_data = MALLOC(this_comp->presel->m,float);
        if (mne_sparse_vec_mult2(this_comp->presel,compdata,this_comp->presel_data) != OK)
            return FAIL;
        presel = this_comp->presel_data;
    }
    else
        presel = compdata;
    /*
    * This always happens
    */
    if (!this_comp->comp_data)
        this_comp->comp_data = MALLOC(this_comp->data->nrow,float);
    mne_mat_vec_mult2(this_comp->data->data,presel,this_comp->comp_data,this_comp->data->nrow,this_comp->data->ncol);
    /*
    * Optional postselection
    */
    if (!this_comp->postsel)
        comp = this_comp->comp_data;
    else {
        if (!this_comp->postsel_data) {
            this_comp->postsel_data = MALLOC(this_comp->postsel->m,float);
        }
        if (mne_sparse_vec_mult2(this_comp->postsel,this_comp->comp_data,this_comp->postsel_data) != OK)
            return FAIL;
        comp = this_comp->postsel_data;
    }
    /*
    * Compensate or revert compensation?
    */
    if (do_it) {
        for (k = 0; k < ndata; k++)
            data[k] = data[k] - comp[k];
    }
    else {
        for (k = 0; k < ndata; k++)
            data[k] = data[k] + comp[k];
    }
    return OK;
}


const char *mne_explain_ctf_comp(int kind)
{
    static struct {
        int kind;
        const char *expl;
    } explain[] = { { MNE_CTFV_COMP_NONE,    "uncompensated" },
    { MNE_CTFV_COMP_G1BR,    "first order gradiometer" },
    { MNE_CTFV_COMP_G2BR,    "second order gradiometer" },
    { MNE_CTFV_COMP_G3BR,    "third order gradiometer" },
    { MNE_4DV_COMP1,         "4D comp 1" },
    { MNE_CTFV_COMP_UNKNOWN, "unknown" } };
    int k;

    for (k = 0; explain[k].kind != MNE_CTFV_COMP_UNKNOWN; k++)
        if (explain[k].kind == kind)
            return explain[k].expl;
    return explain[k].expl;
}




int mne_unmap_ctf_comp_kind(int ctf_comp)

{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (ctf_comp == compMap[k].ctf_comp)
            return compMap[k].grad_comp;
    return ctf_comp;
}


static int mne_calibrate_ctf_comp(mneCTFcompData one,
                                  fiffChInfo     chs,
                                  int            nch,
                                  int            do_it)
/*
* Calibrate or decalibrate a compensation data set
*/
{
    float *col_cals,*row_cals;
    int   j,k,p,found;
    char  *name;
    float **data;

    if (!one)
        return OK;
    if (one->calibrated)
        return OK;

    row_cals = MALLOC(one->data->nrow,float);
    col_cals = MALLOC(one->data->ncol,float);

    for (j = 0; j < one->data->nrow; j++) {
        name = one->data->rowlist[j];
        found = FALSE;
        for (p = 0; p < nch; p++)
            if (strcmp(name,chs[p].ch_name) == 0) {
                row_cals[j] = chs[p].range*chs[p].cal;
                found = TRUE;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name);
            return FAIL;
        }
    }
    for (k = 0; k < one->data->ncol; k++) {
        name = one->data->collist[k];
        found = FALSE;
        for (p = 0; p < nch; p++)
            if (strcmp(name,chs[p].ch_name) == 0) {
                col_cals[k] = chs[p].range*chs[p].cal;
                found = TRUE;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name);
            return FAIL;
        }
    }
    data = one->data->data;
    if (do_it) {
        for (j = 0; j < one->data->nrow; j++)
            for (k = 0; k < one->data->ncol; k++)
                data[j][k] = row_cals[j]*data[j][k]/col_cals[k];
    }
    else {
        for (j = 0; j < one->data->nrow; j++)
            for (k = 0; k < one->data->ncol; k++)
                data[j][k] = col_cals[k]*data[j][k]/row_cals[j];
    }
    return OK;
}


mneCTFcompDataSet mne_read_ctf_comp_data(const QString& name)
/*
* Read all CTF compensation data from a given file
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    mneCTFcompDataSet set = NULL;
    mneCTFcompData    one;
    QList<FiffDirNode::SPtr> nodes;
    QList<FiffDirNode::SPtr> comps;
    int               ncomp;
    MneNamedMatrix*    mat = NULL;
    int               kind,k;
    FiffTag::SPtr t_pTag;
    fiffChInfo        chs = NULL;
    int               nch = 0;
    int               calibrated;
    /*
    * Read the channel information
    */
    {
        fiffChInfo        comp_chs = NULL;
        int               ncompch = 0;

        if (mne_read_meg_comp_eeg_ch_info(name,&chs,&nch,&comp_chs,&ncompch,NULL,NULL,NULL,NULL) == FAIL)
            goto bad;
        if (ncompch > 0) {
            chs = REALLOC(chs,nch+ncompch,fiffChInfoRec);
            for (k = 0; k < ncompch; k++)
                chs[k+nch] = comp_chs[k];
            nch = nch + ncompch;
            FREE(comp_chs);
        }
    }
    /*
    * Read the rest of the stuff
    */
    if(!stream->open())
        goto bad;
    set = mne_new_ctf_comp_data_set();
    /*
    * Locate the compensation data sets
    */
    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_CTF_COMP);
    if (nodes.size() == 0)
        goto good;      /* Nothing more to do */
    comps = nodes[0]->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);
    if (comps.size() == 0)
        goto good;
    ncomp = comps.size();
    /*
    * Set the channel info
    */
    set->chs = chs; chs = NULL;
    set->nch = nch;
    /*
    * Read each data set
    */
    for (k = 0; k < ncomp; k++) {
        mat = MneNamedMatrix::read_named_matrix(stream,comps[k],FIFF_MNE_CTF_COMP_DATA);
        if (!mat)
            goto bad;
        comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_KIND, t_pTag);
        if (t_pTag) {
            kind = *t_pTag->toInt();
        }
        else
            goto bad;
        comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_CALIBRATED, t_pTag);
        if (t_pTag) {
            calibrated = *t_pTag->toInt();
        }
        else
            calibrated = FALSE;
        /*
        * Add these data to the set
        */
        one = mne_new_ctf_comp_data();
        one->data = mat; mat = NULL;
        one->kind                = kind;
        one->mne_kind            = mne_unmap_ctf_comp_kind(one->kind);
        one->calibrated          = calibrated;

        if (mne_calibrate_ctf_comp(one,set->chs,set->nch,TRUE) == FAIL) {
            printf("Warning: Compensation data for '%s' omitted\n", mne_explain_ctf_comp(one->kind));//,err_get_error(),mne_explain_ctf_comp(one->kind));
            mne_free_ctf_comp_data(one);
        }
        else {
            set->comps               = REALLOC(set->comps,set->ncomp+1,mneCTFcompData);
            set->comps[set->ncomp++] = one;
        }
    }
#ifdef DEBUG
    fprintf(stderr,"%d CTF compensation data sets read from %s\n",set->ncomp,name);
#endif
    goto good;

bad : {
        if(mat)
            delete mat;
        stream->close();
        mne_free_ctf_comp_data_set(set);
        return NULL;
    }

good : {
        FREE(chs);
        stream->close();
        return set;
    }
}


/*
 * Mapping from simple integer orders to the mysterious CTF compensation numbers
 */
int mne_map_ctf_comp_kind(int grad)
/*
 * Simple mapping
 */
{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (grad == compMap[k].grad_comp)
            return compMap[k].ctf_comp;
    return grad;
}


int mne_get_ctf_comp(fiffChInfo chs,int nch)
{
    int res = MNE_CTFV_NOGRAD;
    int first_comp,comp;
    int k;

    for (k = 0, first_comp = -1; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comp = chs[k].chpos.coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (first_comp != comp) {
                printf("Non uniform compensation not supported.");
                return FAIL;
            }
        }
    }
    if (first_comp >= 0)
        res = first_comp;
    return res;
}


int mne_make_ctf_comp(mneCTFcompDataSet set,        /* The available compensation data */
                      fiffChInfo        chs,        /* Channels to compensate These may contain channels other than those requiring compensation */
                      int               nch,        /* How many of these */
                      fiffChInfo        compchs,    /* The compensation input channels These may contain channels other than the MEG compensation channels */
                      int               ncomp)      /* How many of these */
/*
* Make compensation data to apply to a set of channels to yield (or uncompensated) compensated data
*/
{
    int *comps = NULL;
    int need_comp;
    int first_comp;
    mneCTFcompData this_comp;
    int  *comp_sel = NULL;
    char **names   = NULL;
    char *name;
    int  j,k,p;

    INVERSELIB::FiffSparseMatrix* presel  = NULL;
    INVERSELIB::FiffSparseMatrix* postsel = NULL;
    MneNamedMatrix*  data    = NULL;

    if (!compchs) {
        compchs = chs;
        ncomp   = nch;
    }
    fprintf(stderr,"Setting up compensation data...\n");
    if (nch == 0)
        return OK;
    if (set) {
        mne_free_ctf_comp_data(set->current);
        set->current = NULL;
    }
    comps = MALLOC(nch,int);
    for (k = 0, need_comp = 0, first_comp = MNE_CTFV_COMP_NONE; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comps[k] = chs[k].chpos.coil_type >> 16;
            if (comps[k] != MNE_CTFV_COMP_NONE) {
                if (first_comp == MNE_CTFV_COMP_NONE)
                    first_comp = comps[k];
                else {
                    if (comps[k] != first_comp) {
                        printf("We do not support nonuniform compensation yet.");
                        goto bad;
                    }
                }
                need_comp++;
            }
        }
        else
            comps[k] = MNE_CTFV_COMP_NONE;
    }
    if (need_comp == 0) {
        fprintf(stderr,"\tNo compensation set. Nothing more to do.\n");
        FREE(comps);
        return OK;
    }
    fprintf(stderr,"\t%d out of %d channels have the compensation set.\n",need_comp,nch);
    if (!set) {
        printf("No compensation data available for the required compensation.");
        return FAIL;
    }
    /*
    * Find the desired compensation data matrix
    */
    for (k = 0, this_comp = NULL; k < set->ncomp; k++) {
        if (set->comps[k]->mne_kind == first_comp) {
            this_comp = set->comps[k];
            break;
        }
    }
    if (!this_comp) {
        printf("Did not find the desired compensation data : %s",
               mne_explain_ctf_comp(mne_map_ctf_comp_kind(first_comp)));
        goto bad;
    }
    fprintf(stderr,"\tDesired compensation data (%s) found.\n",mne_explain_ctf_comp(mne_map_ctf_comp_kind(first_comp)));
    /*
    * Find the compensation channels
    */
    comp_sel = MALLOC(this_comp->data->ncol,int);
    for (k = 0; k < this_comp->data->ncol; k++) {
        comp_sel[k] = -1;
        name = this_comp->data->collist[k];
        for (p = 0; p < ncomp; p++)
            if (strcmp(name,compchs[p].ch_name) == 0) {
                comp_sel[k] = p;
                break;
            }
        if (comp_sel[k] < 0) {
            printf("Compensation channel %s not found",name);
            goto bad;
        }
    }
    fprintf(stderr,"\tAll compensation channels found.\n");
    /*
    * Create the preselector
    */
    {
        float **sel = ALLOC_CMATRIX(this_comp->data->ncol,ncomp);
        for (j = 0; j < this_comp->data->ncol; j++) {
            for (k = 0; k < ncomp; k++)
                sel[j][k] = 0.0;
            sel[j][comp_sel[j]] = 1.0;
        }
        if ((presel = mne_convert_to_sparse(sel,this_comp->data->ncol,ncomp,FIFFTS_MC_RCS,1e-30)) == NULL) {
            FREE_CMATRIX(sel);
            goto bad;
        }
        FREE_CMATRIX(sel);
        fprintf(stderr,"\tPreselector created.\n");
    }
    /*
    * Pick the desired channels
    */
    names = MALLOC(need_comp,char *);
    for (k = 0, p = 0; k < nch; k++) {
        if (comps[k] != MNE_CTFV_COMP_NONE)
            names[p++] = chs[k].ch_name;
    }
    if ((data = this_comp->data->pick_from_named_matrix(names,need_comp,NULL,0)) == NULL)
        goto bad;
    fprintf(stderr,"\tCompensation data matrix created.\n");
    /*
    * Create the postselector
    */
    {
        float **sel = ALLOC_CMATRIX(nch,data->nrow);
        for (j = 0, p = 0; j < nch; j++) {
            for (k = 0; k < data->nrow; k++)
                sel[j][k] = 0.0;
            if (comps[j] != MNE_CTFV_COMP_NONE)
                sel[j][p++] = 1.0;
        }
        if ((postsel = mne_convert_to_sparse(sel,nch,data->nrow,FIFFTS_MC_RCS,1e-30)) == NULL) {
            FREE_CMATRIX(sel);
            goto bad;
        }
        FREE_CMATRIX(sel);
        fprintf(stderr,"\tPostselector created.\n");
    }
    set->current           = mne_new_ctf_comp_data();
    set->current->kind     = this_comp->kind;
    set->current->mne_kind = this_comp->mne_kind;
    set->current->data     = data;
    set->current->presel   = presel;
    set->current->postsel  = postsel;

    fprintf(stderr,"\tCompensation set up.\n");

    FREE(names);
    FREE(comps);
    FREE(comp_sel);

    return OK;

bad : {
        if(presel)
            delete presel;
        if(postsel)
            delete postsel;
        if(data)
            delete data;
        FREE(names);
        FREE(comps);
        FREE(comp_sel);
        return FAIL;
    }
}


int mne_set_ctf_comp(fiffChInfo chs,
                     int        nch,
                     int        comp)
/*
* Set the compensation bits to the desired value
*/
{
    int k;
    int nset;
    for (k = 0, nset = 0; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            chs[k].chpos.coil_type = (chs[k].chpos.coil_type & 0xFFFF) | (comp << 16);
            nset++;
        }
    }
    fprintf(stderr,"A new compensation value (%s) was assigned to %d MEG channels.\n",
            mne_explain_ctf_comp(mne_map_ctf_comp_kind(comp)),nset);
    return nset;
}




int mne_apply_ctf_comp_t(mneCTFcompDataSet set,     /* The compensation data */
                         int               do_it,
                         float             **data,  /* The data to process (channel by channel) */
                         int               ndata,
                         int               ns)      /* Number of samples */
/*
* Apply compensation or revert to uncompensated data
*/
{
    mneCTFcompData this_comp;
    float **presel,**comp;
    float **compdata = data;
    int   ncompdata  = ndata;
    int   k,p;

    if (!set || !set->current)
        return OK;
    this_comp = set->current;
    /*
    * Dimension checks
    */
    if (this_comp->presel) {
        if (this_comp->presel->n != ncompdata) {
            printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->presel->n,ncompdata);
            return FAIL;
        }
    }
    else if (this_comp->data->ncol != ncompdata) {
        printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->ncol,ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->m != ndata) {
            printf("Data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->postsel->m,ndata);
            return FAIL;
        }
    }
    else if (this_comp->data->nrow != ndata) {
        printf("Data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->nrow,ndata);
        return FAIL;
    }
    /*
    * Preselection is optional
    */
    if (this_comp->presel) {
        presel = ALLOC_CMATRIX(this_comp->presel->m,ns);
        if (mne_sparse_mat_mult2(this_comp->presel,compdata,ns,presel) != OK) {
            FREE_CMATRIX(presel);
            return FAIL;
        }
    }
    else
        presel = data;
    /*
    * This always happens
    */
    comp = mne_mat_mat_mult(this_comp->data->data,presel,this_comp->data->nrow,this_comp->data->ncol,ns);
    if (this_comp->presel)
        FREE_CMATRIX(presel);
    /*
    * Optional postselection
    */
    if (this_comp->postsel) {
        float **postsel = ALLOC_CMATRIX(this_comp->postsel->m,ns);
        if (mne_sparse_mat_mult2(this_comp->postsel,comp,ns,postsel) != OK) {
            FREE_CMATRIX(postsel);
            return FAIL;
        }
        FREE_CMATRIX(comp);
        comp = postsel;
    }
    /*
    * Compensate or revert compensation?
    */
    if (do_it) {
        for (k = 0; k < ndata; k++)
            for (p = 0; p < ns; p++)
                data[k][p] = data[k][p] - comp[k][p];
    }
    else {
        for (k = 0; k < ndata; k++)
            for (p = 0; p < ns; p++)
                data[k][p] = data[k][p] + comp[k][p];
    }
    FREE_CMATRIX(comp);
    return OK;
}


int mne_ctf_compensate_to(mneCTFcompDataSet set,            /* The compensation data */
                          int               compensate_to,  /* What is the desired compensation to achieve */
                          fiffChInfo        chs,            /* The channels to compensate */
                          int               nchan,          /* How many? */
                          fiffChInfo        comp_chs,       /* Maybe a different set, defaults to the same */
                          int               ncomp_chan,     /* How many */
                          float             **data,         /* The data in a np x nchan matrix allocated with ALLOC_CMATRIX(np,nchan) */
                          float             **comp_data,    /* The compensation data in a np x ncomp_chan matrix, defaults to data */
                          int               np)             /* How many time points */
/*
* Make data which has the third-order gradient compensation applied
*/
{
    int k;
    int have_comp_chs;
    int comp_was = MNE_CTFV_COMP_UNKNOWN;

    if (!comp_data)
        comp_data = data;
    if (!comp_chs) {
        comp_chs = chs;
        ncomp_chan = nchan;
    }
    if (set) {
        mne_free_ctf_comp_data(set->undo); set->undo = NULL;
        mne_free_ctf_comp_data(set->current); set->current = NULL;
    }
    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
            have_comp_chs++;
    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
        printf("No compensation channels in these data.");
        return FAIL;
    }
    /*
    * Update the 'current' field in 'set' to reflect the compensation possibly present in the data now
    */
    if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
        goto bad;
    /*
    * Are we there already?
    */
    if (set->current && set->current->mne_kind == compensate_to) {
        fprintf(stderr,"The data were already compensated as desired (%s)\n",mne_explain_ctf_comp(set->current->kind));
        return OK;
    }
    /*
    * Undo any previous compensation
    */
    for (k = 0; k < np; k++)
        if (mne_apply_ctf_comp(set,FALSE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
            goto bad;
    if (set->current)
        fprintf(stderr,"The previous compensation (%s) is now undone\n",mne_explain_ctf_comp(set->current->kind));
    /*
    * Set to new gradient compensation
    */
    if (compensate_to == MNE_CTFV_NOGRAD) {
        mne_set_ctf_comp(chs,nchan,compensate_to);
        fprintf(stderr,"No compensation was requested. Original data have been restored.\n");
    }
    else {
        if (mne_set_ctf_comp(chs,nchan,compensate_to) > 0) {
            if (set->current)
                comp_was = set->current->mne_kind;
            if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
                goto bad;
            /*
            * Do the third-order gradient compensation
            */
            for (k = 0; k < np; k++)
                if (mne_apply_ctf_comp(set,TRUE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
                    goto bad;
            if (set->current)
                fprintf(stderr,"The data are now compensated as requested (%s).\n",mne_explain_ctf_comp(set->current->kind));
        }
        else
            fprintf(stderr,"No MEG channels to compensate.\n");
    }
    return OK;

bad : {
        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
            mne_set_ctf_comp(chs,nchan,comp_was);
        return FAIL;
    }
}


int mne_ctf_set_compensation(mneCTFcompDataSet set,            /* The compensation data */
                             int               compensate_to,  /* What is the desired compensation to achieve */
                             fiffChInfo        chs,            /* The channels to compensate */
                             int               nchan,          /* How many? */
                             fiffChInfo        comp_chs,       /* Maybe a different set, defaults to the same */
                             int               ncomp_chan)     /* How many */
/*
* Make data which has the third-order gradient compensation applied
*/
{
    int k;
    int have_comp_chs;
    int comp_was = MNE_CTFV_COMP_UNKNOWN;

    if (!set) {
        if (compensate_to == MNE_CTFV_NOGRAD)
            return OK;
        else {
            printf("Cannot do compensation because compensation data are missing");
            return FAIL;
        }
    }
    if (!comp_chs) {
        comp_chs = chs;
        ncomp_chan = nchan;
    }
    if (set) {
        mne_free_ctf_comp_data(set->undo); set->undo = NULL;
        mne_free_ctf_comp_data(set->current); set->current = NULL;
    }
    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
            have_comp_chs++;
    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
        printf("No compensation channels in these data.");
        return FAIL;
    }
    /*
    * Update the 'current' field in 'set' to reflect the compensation possibly present in the data now
    */
    if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
        goto bad;
    /*
    * Are we there already?
    */
    if (set->current && set->current->mne_kind == compensate_to) {
        fprintf(stderr,"No further compensation necessary (comp = %s)\n",mne_explain_ctf_comp(set->current->kind));
        mne_free_ctf_comp_data(set->current); set->current = NULL;
        return OK;
    }
    set->undo    = set->current;
    set->current = NULL;
    if (compensate_to == MNE_CTFV_NOGRAD) {
        fprintf(stderr,"No compensation was requested.\n");
        mne_set_ctf_comp(chs,nchan,compensate_to);
        return OK;
    }
    if (mne_set_ctf_comp(chs,nchan,compensate_to) > 0) {
        if (set->undo)
            comp_was = set->undo->mne_kind;
        else
            comp_was = MNE_CTFV_NOGRAD;
        if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
            goto bad;
        fprintf(stderr,"Compensation set up as requested (%s -> %s).\n",
                mne_explain_ctf_comp(mne_map_ctf_comp_kind(comp_was)),
                mne_explain_ctf_comp(set->current->kind));
    }
    return OK;

bad : {
        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
            mne_set_ctf_comp(chs,nchan,comp_was);
        return FAIL;
    }
}


//============================= mne_process_bads.c =============================

static int whitespace(char *text)

{
    if (text == NULL || strlen(text) == 0)
        return TRUE;
    if (strspn(text," \t\n\r") == strlen(text))
        return TRUE;
    return FALSE;
}


static char *next_line(char *line, int n, FILE *in)
{
    char *res;

    for (res = fgets(line,n,in); res != NULL; res = fgets(line,n,in))
        if (!whitespace(res))
            if (res[0] != '#')
                break;
    return res;
}

#define MAXLINE 500

int mne_read_bad_channels(const QString& name, char ***listp, int *nlistp)
/*
* Read bad channel names
*/
{
    FILE *in = NULL;
    char **list = NULL;
    int  nlist  = 0;
    char line[MAXLINE+1];
    char *next;


    if (name.isEmpty())
        return OK;

    if ((in = fopen(name.toLatin1().data(),"r")) == NULL) {
        qCritical() << name;
        goto bad;
    }
    while ((next = next_line(line,MAXLINE,in)) != NULL) {
        if (strlen(next) > 0) {
            if (next[strlen(next)-1] == '\n')
                next[strlen(next)-1] = '\0';
            list = REALLOC(list,nlist+1,char *);
            list[nlist++] = mne_strdup(next);
        }
    }
    if (ferror(in))
        goto bad;

    *listp  = list;
    *nlistp = nlist;

    return OK;

bad : {
        mne_free_name_list(list,nlist);
        if (in != NULL)
            fclose(in);
        return FAIL;
    }
}




int mne_read_bad_channel_list_from_node(FiffStream::SPtr& stream,
                                        const FiffDirNode::SPtr& pNode, char ***listp, int *nlistp)
{
    FiffDirNode::SPtr node,bad;
    QList<FiffDirNode::SPtr> temp;
    char **list = NULL;
    int  nlist  = 0;
    FiffTag::SPtr t_pTag;
    char *names;

    if (pNode->isEmpty())
        node = stream->tree();
    else
        node = pNode;

    temp = node->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    if (temp.size() > 0) {
        bad = temp[0];

        bad->find_tag(stream, FIFF_MNE_CH_NAME_LIST, t_pTag);
        if (t_pTag) {
            names = (char *)t_pTag->data();
            mne_string_to_name_list(names,&list,&nlist);
        }
    }
    *listp = list;
    *nlistp = nlist;
    return OK;
}

int mne_read_bad_channel_list(const QString& name, char ***listp, int *nlistp)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int res;

    if(!stream->open())
        return FAIL;

    res = mne_read_bad_channel_list_from_node(stream,stream->tree(),listp,nlistp);

    stream->close();

    return res;
}


//============================= mne_cov_matrix.c =============================

/*
* Routines for handling the covariance matrices
*/
static mneCovMatrix new_cov(int    kind,
                            int    ncov,
                            char   **names,
                            double *cov,
                            double *cov_diag,
                            INVERSELIB::FiffSparseMatrix* cov_sparse)
/*
* Put it together from ingredients
*/
{
    mneCovMatrix new_cov    = MALLOC(1,mneCovMatrixRec);
    new_cov->kind           = kind;
    new_cov->ncov           = ncov;
    new_cov->nproj          = 0;
    new_cov->nzero          = 0;
    new_cov->names          = names;
    new_cov->cov            = cov;
    new_cov->cov_diag       = cov_diag;
    new_cov->cov_sparse     = cov_sparse;
    new_cov->eigen          = NULL;
    new_cov->lambda         = NULL;
    new_cov->chol           = NULL;
    new_cov->inv_lambda     = NULL;
    new_cov->nfree          = 1;
    new_cov->ch_class       = NULL;
    new_cov->proj           = NULL;
    new_cov->sss            = NULL;
    new_cov->bads           = NULL;
    new_cov->nbad           = 0;
    return new_cov;
}

mneCovMatrix mne_new_cov(int    kind,
                         int    ncov,
                         char   **names,
                         double *cov,
                         double *cov_diag)

{
    return new_cov(kind,ncov,names,cov,cov_diag,NULL);
}


mneCovMatrix mne_new_cov_dense(int    kind,
                               int    ncov,
                               char   **names,
                               double *cov)

{
    return new_cov(kind,ncov,names,cov,NULL,NULL);
}


mneCovMatrix mne_new_cov_diag(int    kind,
                              int    ncov,
                              char   **names,
                              double *cov_diag)

{
    return new_cov(kind,ncov,names,NULL,cov_diag,NULL);
}


mneCovMatrix mne_new_cov_sparse(int             kind,
                                int             ncov,
                                char            **names,
                                INVERSELIB::FiffSparseMatrix* cov_sparse)
{
    return new_cov(kind,ncov,names,NULL,NULL,cov_sparse);
}


static int mne_lt_packed_index(int j, int k)

{
    if (j >= k)
        return k + j*(j+1)/2;
    else
        return j + k*(k+1)/2;
}


mneCovMatrix mne_dup_cov(mneCovMatrix c)
{
    double       *vals;
    int          nval;
    int          k;
    mneCovMatrix res;

    if (c->cov_diag)
        nval = c->ncov;
    else
        nval = (c->ncov*(c->ncov+1))/2;

    vals = MALLOC(nval,double);
    if (c->cov_diag) {
        for (k = 0; k < nval; k++)
            vals[k] = c->cov_diag[k];
        res = mne_new_cov(c->kind,c->ncov,mne_dup_name_list(c->names,c->ncov),NULL,vals);
    }
    else {
        for (k = 0; k < nval; k++)
            vals[k] = c->cov[k];
        res = mne_new_cov(c->kind,c->ncov,mne_dup_name_list(c->names,c->ncov),vals,NULL);
    }
    /*
    * Duplicate additional items
    */
    if (c->ch_class) {
        res->ch_class = MALLOC(c->ncov,int);
        for (k = 0; k < c->ncov; k++)
            res->ch_class[k] = c->ch_class[k];
    }
    res->bads = mne_dup_name_list(c->bads,c->nbad);
    res->nbad = c->nbad;
    res->proj = mne_dup_proj_op(c->proj);
    res->sss  = new MneSssData(*(c->sss));

    return res;
}


void mne_free_cov(mneCovMatrix c)
/*
* Free a covariance matrix and all its data
*/
{
    if (c == NULL)
        return;
    FREE(c->cov);
    FREE(c->cov_diag);
    if(c->cov_sparse)
        delete c->cov_sparse;
    mne_free_name_list(c->names,c->ncov);
    FREE_CMATRIX(c->eigen);
    FREE(c->lambda);
    FREE(c->inv_lambda);
    FREE(c->chol);
    FREE(c->ch_class);
    mne_free_proj_op(c->proj);
    if (c->sss)
        delete c->sss;
    mne_free_name_list(c->bads,c->nbad);
    FREE(c);
    return;
}


int mne_is_diag_cov(mneCovMatrix c)

{
    return c->cov_diag != NULL;
}




static int check_cov_data(double *vals, int nval)

{
    int    k;
    double sum;

    for (k = 0, sum = 0.0; k < nval; k++)
        sum += vals[k];
    if (sum == 0.0) {
        qCritical("Sum of covariance matrix elements is zero!");
        return FAIL;
    }
    return OK;
}



mneCovMatrix mne_read_cov(const QString& name,int kind)
/*
* Read a covariance matrix from a fiff
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffTag::SPtr t_pTag;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr    covnode;

    char            **names    = NULL;	/* Optional channel name list */
    int             nnames     = 0;
    double          *cov       = NULL;
    double          *cov_diag  = NULL;
    INVERSELIB::FiffSparseMatrix* cov_sparse = NULL;
    double          *lambda    = NULL;
    float           **eigen    = NULL;
    MatrixXf        tmp_eigen;
    char            **bads     = NULL;
    int             nbad       = 0;
    int             ncov       = 0;
    int             nfree      = 1;
    mneCovMatrix    res        = NULL;

    int            k,p,nn;
    float          *f;
    double         *d;
    mneProjOp      op = NULL;
    MneSssData*     sss = NULL;

    if(!stream->open())
        goto out;

    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_COV);

    if (nodes.size() == 0) {
        printf("No covariance matrix available in %s",name.toLatin1().data());
        goto out;
    }
    /*
    * Locate the desired matrix
    */
    for (k = 0 ; k < nodes.size(); ++k) {
        if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_KIND, t_pTag))
            continue;

        if (*t_pTag->toInt() == kind) {
            covnode = nodes[k];
            break;
        }
    }
    if (covnode->isEmpty()) {
        printf("Desired covariance matrix not found from %s",name.toLatin1().data());
        goto out;
    }
    /*
    * Read the data
    */
    if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_DIM, t_pTag))
        goto out;
    ncov = *t_pTag->toInt();

    if (nodes[k]->find_tag(stream, FIFF_MNE_COV_NFREE, t_pTag)) {
        nfree = *t_pTag->toInt();
    }
    if (covnode->find_tag(stream, FIFF_MNE_ROW_NAMES, t_pTag)) {
        mne_string_to_name_list((char *)(t_pTag->data()),&names,&nnames);
        if (nnames != ncov) {
            qCritical("Incorrect number of channel names for a covariance matrix");
            goto out;
        }
    }
    if (!nodes[k]->find_tag(stream, FIFF_MNE_COV, t_pTag)) {
        if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_DIAG, t_pTag))
            goto out;
        else {
            if (t_pTag->getType() == FIFFT_DOUBLE) {
                cov_diag = MALLOC(ncov,double);
                qDebug() << "ToDo: Check whether cov_diag contains the right stuff!!! - use VectorXd instead";
                d = t_pTag->toDouble();
                for (p = 0; p < ncov; p++)
                    cov_diag[p] = d[p];
                if (check_cov_data(cov_diag,ncov) != OK)
                    goto out;
            }
            else if (t_pTag->getType() == FIFFT_FLOAT) {
                cov_diag = MALLOC(ncov,double);
                qDebug() << "ToDo: Check whether f contains the right stuff!!! - use VectorXf instead";
                f = t_pTag->toFloat();
                for (p = 0; p < ncov; p++)
                    cov_diag[p] = f[p];
            }
            else {
                printf("Illegal data type for covariance matrix");
                goto out;
            }
        }
    }
    else {
        nn = ncov*(ncov+1)/2;
        if (t_pTag->getType() == FIFFT_DOUBLE) {
            qDebug() << "ToDo: Check whether cov contains the right stuff!!! - use VectorXd instead";
//            qDebug() << t_pTag->size() / sizeof(double);
            cov = MALLOC(nn,double);
            d = t_pTag->toDouble();
            for (p = 0; p < nn; p++)
                cov[p] = d[p];
            if (check_cov_data(cov,nn) != OK)
                goto out;
        }
        else if (t_pTag->getType() == FIFFT_FLOAT) {
            cov = MALLOC(nn,double);
            f = t_pTag->toFloat();
            for (p = 0; p < nn; p++)
                cov[p] = f[p];
        }
        else if ((cov_sparse = FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag)) == NULL) {
            goto out;
        }

        if (nodes[k]->find_tag(stream, FIFF_MNE_COV_EIGENVALUES, t_pTag)) {
            lambda = (double *)t_pTag->toDouble();
            if (nodes[k]->find_tag(stream, FIFF_MNE_COV_EIGENVECTORS, t_pTag))
                goto out;

            tmp_eigen = t_pTag->toFloatMatrix().transpose();
            eigen = ALLOC_CMATRIX(tmp_eigen.rows(),tmp_eigen.cols());
            fromFloatEigenMatrix(tmp_eigen, eigen);
        }
        /*
        * Read the optional projection operator
        */
        if ((op = mne_read_proj_op_from_node(stream,nodes[k])) == NULL)
            goto out;
        /*
        * Read the optional SSS data
        */
        if ((sss = MneSssData::read_sss_data_from_node(stream,nodes[k])) == NULL)
            goto out;
        /*
        * Read the optional bad channel list
        */
        if (mne_read_bad_channel_list_from_node(stream,nodes[k],&bads,&nbad) == FAIL)
            goto out;
    }
    if (cov_sparse)
        res = mne_new_cov_sparse(kind,ncov,names,cov_sparse);
    else if (cov)
        res = mne_new_cov_dense(kind,ncov,names,cov);
    else if (cov_diag)
        res = mne_new_cov_diag(kind,ncov,names,cov_diag);
    else {
        qCritical("mne_read_cov : covariance matrix data is not defined. How come?");
        goto out;
    }
    cov         = NULL;
    cov_diag    = NULL;
    cov_sparse  = NULL;
    res->eigen  = eigen;
    res->lambda = lambda;
    res->nfree  = nfree;
    res->bads   = bads;
    res->nbad   = nbad;
    /*
    * Count the non-zero eigenvalues
    */
    if (res->lambda) {
        res->nzero = 0;
        for (k = 0; k < res->ncov; k++, res->nzero++)
            if (res->lambda[k] > 0)
                break;
    }

    if (op && op->nitems > 0) {
        res->proj = op;
        op = NULL;
    }
    if (sss && sss->ncomp > 0 && sss->job != FIFFV_SSS_JOB_NOTHING) {
        res->sss = sss;
        sss = NULL;
    }

out : {
        stream->close();
        mne_free_proj_op(op);
        if(sss)
            delete sss;

        if (!res) {
            mne_free_name_list(names,nnames);
            mne_free_name_list(bads,nbad);
            FREE(cov);
            FREE(cov_diag);
            if(cov_sparse)
                delete cov_sparse;
        }
        return res;
    }

}





mneCovMatrix mne_pick_chs_cov_omit(mneCovMatrix c, char **new_names, int ncov, int omit_meg_eeg, fiffChInfo chs)
/*
* Pick designated channels from a covariance matrix, optionally omit MEG/EEG correlations
*/
{
    int j,k;
    int *pick = NULL;
    double *cov = NULL;
    double *cov_diag = NULL;
    char  **names = NULL;
    int   *is_meg = NULL;
    int   from,to;
    mneCovMatrix res;

    if (ncov == 0) {
        qCritical("No channels specified for picking in mne_pick_chs_cov_omit");
        return NULL;
    }
    if (c->names == NULL) {
        qCritical("No names in covariance matrix. Cannot do picking.");
        return NULL;
    }
    pick = MALLOC(ncov,int);
    for (j = 0; j < ncov; j++)
        pick[j] = -1;
    for (j = 0; j < ncov; j++)
        for (k = 0; k < c->ncov; k++)
            if (strcmp(c->names[k],new_names[j]) == 0) {
                pick[j] = k;
                break;
            }
    for (j = 0; j < ncov; j++) {
        if (pick[j] < 0) {
            printf("All desired channels not found in the covariance matrix (at least missing %s).", new_names[j]);
            FREE(pick);
            return NULL;
        }
    }
    if (omit_meg_eeg) {
        is_meg = MALLOC(ncov,int);
        if (chs) {
            for (j = 0; j < ncov; j++)
                if (chs[j].kind == FIFFV_MEG_CH)
                    is_meg[j] = TRUE;
                else
                    is_meg[j] = FALSE;
        }
        else {
            for (j = 0; j < ncov; j++)
                if (strncmp(new_names[j],"MEG",3) == 0)
                    is_meg[j] = TRUE;
                else
                    is_meg[j] = FALSE;
        }
    }
    names = MALLOC(ncov,char *);
    if (c->cov_diag) {
        cov_diag = MALLOC(ncov,double);
        for (j = 0; j < ncov; j++) {
            cov_diag[j] = c->cov_diag[pick[j]];
            names[j] = mne_strdup(c->names[pick[j]]);
        }
    }
    else {
        cov = MALLOC(ncov*(ncov+1)/2,double);
        for (j = 0; j < ncov; j++) {
            names[j] = mne_strdup(c->names[pick[j]]);
            for (k = 0; k <= j; k++) {
                from = mne_lt_packed_index(pick[j],pick[k]);
                to   = mne_lt_packed_index(j,k);
                if (to < 0 || to > ncov*(ncov+1)/2-1) {
                    printf("Wrong destination index in mne_pick_chs_cov : %d %d %d\n",j,k,to);
                    exit(1);
                }
                if (from < 0 || from > c->ncov*(c->ncov+1)/2-1) {
                    printf("Wrong source index in mne_pick_chs_cov : %d %d %d\n",pick[j],pick[k],from);
                    exit(1);
                }
                cov[to] = c->cov[from];
                if (omit_meg_eeg)
                    if (is_meg[j] != is_meg[k])
                        cov[to] = 0.0;
            }
        }
    }

    res = mne_new_cov(c->kind,ncov,names,cov,cov_diag);

    res->bads = mne_dup_name_list(c->bads,c->nbad);
    res->nbad = c->nbad;
    res->proj = mne_dup_proj_op(c->proj);
    res->sss  = c->sss ? new MneSssData(*(c->sss)) : NULL;

    if (c->ch_class) {
        res->ch_class = MALLOC(res->ncov,int);
        for (k = 0; k < res->ncov; k++)
            res->ch_class[k] = c->ch_class[pick[k]];
    }
    FREE(pick);
    FREE(is_meg);
    return res;
}


void mne_revert_to_diag_cov(mneCovMatrix c)
/*
* Pick the diagonal elements of the full covariance matrix
*/
{
    int k,p;
    if (c->cov == NULL)
        return;
#define REALLY_REVERT
#ifdef REALLY_REVERT
    c->cov_diag = REALLOC(c->cov_diag,c->ncov,double);

    for (k = p = 0; k < c->ncov; k++) {
        c->cov_diag[k] = c->cov[p];
        p = p + k + 2;
    }
    FREE(c->cov);  c->cov = NULL;
#else
    for (j = 0, p = 0; j < c->ncov; j++)
        for (k = 0; k <= j; k++, p++)
            if (j != k)
                c->cov[p] = 0.0;
            else
#endif
                FREE(c->lambda); c->lambda = NULL;
    FREE_CMATRIX(c->eigen); c->eigen = NULL;
    return;

}


int mne_classify_channels_cov(mneCovMatrix cov, fiffChInfo chs, int nchan)
/*
 * Assign channel classes in a covariance matrix with help of channel infos
 */
{
    int k,p;
    fiffChInfo ch;

    if (!chs) {
        qCritical("Channel information not available in mne_classify_channels_cov");
        goto bad;
    }
    cov->ch_class = REALLOC(cov->ch_class,cov->ncov,int);
    for (k = 0; k < cov->ncov; k++) {
        cov->ch_class[k] = MNE_COV_CH_UNKNOWN;
        for (p = 0, ch = NULL; p < nchan; p++) {
            if (strcmp(chs[p].ch_name,cov->names[k]) == 0) {
                ch = chs+p;
                if (ch->kind == FIFFV_MEG_CH) {
                    if (ch->unit == FIFF_UNIT_T)
                        cov->ch_class[k] = MNE_COV_CH_MEG_MAG;
                    else
                        cov->ch_class[k] = MNE_COV_CH_MEG_GRAD;
                }
                else if (ch->kind == FIFFV_EEG_CH)
                    cov->ch_class[k] = MNE_COV_CH_EEG;
                break;
            }
        }
        if (!ch) {
            printf("Could not find channel info for channel %s in mne_classify_channels_cov",cov->names[k]);
            goto bad;
        }
    }
    return OK;

bad : {
        FREE(cov->ch_class);
        cov->ch_class = NULL;
        return FAIL;
    }
}



int mne_add_inv_cov(mneCovMatrix c)
/*
      * Calculate the inverse square roots for whitening
      */
{
    double *src = c->lambda ? c->lambda : c->cov_diag;
    int k;

    if (src == NULL) {
        qCritical("Covariance matrix is not diagonal or not decomposed.");
        return FAIL;
    }
    c->inv_lambda = REALLOC(c->inv_lambda,c->ncov,double);
    for (k = 0; k < c->ncov; k++) {
        if (src[k] <= 0.0)
            c->inv_lambda[k] = 0.0;
        else
            c->inv_lambda[k] = 1.0/sqrt(src[k]);
    }
    return OK;
}




#define SMALL      1e-29
#define MEG_SMALL  1e-29
#define EEG_SMALL  1e-18

typedef struct {
    double lambda;
    int    no;
} *covSort,covSortRec;

static int comp_cov(const void *v1, const void *v2)

{
    covSort s1 = (covSort)v1;
    covSort s2 = (covSort)v2;
    if (s1->lambda < s2->lambda)
        return -1;
    if (s1->lambda > s2->lambda)
        return 1;
    return 0;
}

static int condition_cov(mneCovMatrix c, float rank_threshold, int use_rank)

{
    double *scale  = NULL;
    double *cov    = NULL;
    double *lambda = NULL;
    float  **eigen = NULL;
    double **data1 = NULL;
    double **data2 = NULL;
    double magscale,gradscale,eegscale;
    int    nmag,ngrad,neeg,nok;
    int    j,k;
    int    res = FAIL;

    if (c->cov_diag)
        return OK;
    if (!c->ch_class) {
        qCritical("Channels not classified. Rank cannot be determined.");
        return FAIL;
    }
    magscale = gradscale = eegscale = 0.0;
    nmag = ngrad = neeg = 0;
    for (k = 0; k < c->ncov; k++) {
        if (c->ch_class[k] == MNE_COV_CH_MEG_MAG) {
            magscale += c->cov[mne_lt_packed_index(k,k)]; nmag++;
        }
        else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD) {
            gradscale += c->cov[mne_lt_packed_index(k,k)]; ngrad++;
        }
        else if (c->ch_class[k] == MNE_COV_CH_EEG) {
            eegscale += c->cov[mne_lt_packed_index(k,k)]; neeg++;
        }
#ifdef DEBUG
        fprintf(stdout,"%d ",c->ch_class[k]);
#endif
    }
#ifdef DEBUG
    fprintf(stdout,"\n");
#endif
    if (nmag > 0)
        magscale = magscale > 0.0 ? sqrt(nmag/magscale) : 0.0;
    if (ngrad > 0)
        gradscale = gradscale > 0.0 ? sqrt(ngrad/gradscale) : 0.0;
    if (neeg > 0)
        eegscale = eegscale > 0.0 ? sqrt(neeg/eegscale) : 0.0;
#ifdef DEBUG
    fprintf(stdout,"%d %g\n",nmag,magscale);
    fprintf(stdout,"%d %g\n",ngrad,gradscale);
    fprintf(stdout,"%d %g\n",neeg,eegscale);
#endif
    scale = MALLOC(c->ncov,double);
    for (k = 0; k < c->ncov; k++) {
        if (c->ch_class[k] == MNE_COV_CH_MEG_MAG)
            scale[k] = magscale;
        else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD)
            scale[k] = gradscale;
        else if (c->ch_class[k] == MNE_COV_CH_EEG)
            scale[k] = eegscale;
        else
            scale[k] = 1.0;
    }
    cov    = MALLOC(c->ncov*(c->ncov+1)/2.0,double);
    lambda = MALLOC(c->ncov,double);
    eigen  = ALLOC_CMATRIX(c->ncov,c->ncov);
    for (j = 0; j < c->ncov; j++)
        for (k = 0; k <= j; k++)
            cov[mne_lt_packed_index(j,k)] = c->cov[mne_lt_packed_index(j,k)]*scale[j]*scale[k];
    if (mne_decompose_eigen (cov,lambda,eigen,c->ncov) == 0) {
#ifdef DEBUG
        for (k = 0; k < c->ncov; k++)
            fprintf(stdout,"%g ",lambda[k]/lambda[c->ncov-1]);
        fprintf(stdout,"\n");
#endif
        nok = 0;
        for (k = c->ncov-1; k >= 0; k--) {
            if (lambda[k] >= rank_threshold*lambda[c->ncov-1])
                nok++;
            else
                break;
        }
        printf("\n\tEstimated covariance matrix rank = %d (%g)\n",nok,lambda[c->ncov-nok]/lambda[c->ncov-1]);
        if (use_rank > 0 && use_rank < nok) {
            nok = use_rank;
            fprintf(stderr,"\tUser-selected covariance matrix rank = %d (%g)\n",nok,lambda[c->ncov-nok]/lambda[c->ncov-1]);
        }
        /*
     * Put it back together
     */
        for (j = 0; j < c->ncov-nok; j++)
            lambda[j] = 0.0;
        data1 = ALLOC_DCMATRIX(c->ncov,c->ncov);
        for (j = 0; j < c->ncov; j++) {
#ifdef DEBUG
            mne_print_vector(stdout,NULL,eigen[j],c->ncov);
#endif
            for (k = 0; k < c->ncov; k++)
                data1[j][k] = sqrt(lambda[j])*eigen[j][k];
        }
        data2 = mne_dmatt_dmat_mult2 (data1,data1,c->ncov,c->ncov,c->ncov);
#ifdef DEBUG
        printf(">>>\n");
        for (j = 0; j < c->ncov; j++)
            mne_print_dvector(stdout,NULL,data2[j],c->ncov);
        printf(">>>\n");
#endif
        /*
     * Scale back
     */
        for (k = 0; k < c->ncov; k++)
            if (scale[k] > 0.0)
                scale[k] = 1.0/scale[k];
        for (j = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                if (c->cov[mne_lt_packed_index(j,k)] != 0.0)
                    c->cov[mne_lt_packed_index(j,k)] = scale[j]*scale[k]*data2[j][k];
        res = nok;
    }
    FREE(cov);
    FREE(lambda);
    FREE_CMATRIX(eigen);
    FREE_DCMATRIX(data1);
    FREE_DCMATRIX(data2);
    return res;
}


static int mne_decompose_eigen_cov_small(mneCovMatrix c,float small, int use_rank)
/*
      * Do the eigenvalue decomposition
      */
{
    int   np,k,p,rank;
    float rank_threshold = 1e-6;

    if (small < 0)
        small = 1.0;

    if (!c)
        return OK;
    if (c->cov_diag)
        return mne_add_inv_cov(c);
    if (c->lambda && c->eigen) {
        fprintf(stderr,"\n\tEigenvalue decomposition had been precomputed.\n");
        c->nzero = 0;
        for (k = 0; k < c->ncov; k++, c->nzero++)
            if (c->lambda[k] > 0)
                break;
    }
    else {
        FREE(c->lambda); c->lambda = NULL;
        FREE_CMATRIX(c->eigen); c->eigen = NULL;

        if ((rank = condition_cov(c,rank_threshold,use_rank)) < 0)
            return FAIL;

        np = c->ncov*(c->ncov+1)/2;
        c->lambda = MALLOC(c->ncov,double);
        c->eigen  = ALLOC_CMATRIX(c->ncov,c->ncov);
        if (mne_decompose_eigen (c->cov,c->lambda,c->eigen,c->ncov) != 0)
            goto bad;
        c->nzero = c->ncov - rank;
        for (k = 0; k < c->nzero; k++)
            c->lambda[k] = 0.0;
        /*
     * Find which eigenvectors correspond to EEG/MEG
     */
        {
            float meglike,eeglike;
            int   nmeg,neeg;

            nmeg = neeg = 0;
            for (k = c->nzero; k < c->ncov; k++) {
                meglike = eeglike = 0.0;
                for (p = 0; p < c->ncov; p++)  {
                    if (c->ch_class[p] == MNE_COV_CH_EEG)
                        eeglike += fabs(c->eigen[k][p]);
                    else if (c->ch_class[p] == MNE_COV_CH_MEG_MAG || c->ch_class[p] == MNE_COV_CH_MEG_GRAD)
                        meglike += fabs(c->eigen[k][p]);
                }
                if (meglike > eeglike)
                    nmeg++;
                else
                    neeg++;
            }
            printf("\t%d MEG and %d EEG-like channels remain in the whitened data\n",nmeg,neeg);
        }
    }
    return mne_add_inv_cov(c);

bad : {
        FREE(c->lambda); c->lambda = NULL;
        FREE_CMATRIX(c->eigen); c->eigen = NULL;
        return FAIL;
    }
}


int mne_decompose_eigen_cov(mneCovMatrix c)

{
    return mne_decompose_eigen_cov_small(c,-1.0,-1);
}


void mne_regularize_cov(mneCovMatrix c,       /* The matrix to regularize */
                        float        *regs)   /* Regularization values to apply (fractions of the
                           * average diagonal values for each class */
/*
 * Regularize different parts of the noise covariance matrix differently
 */
{
    int    j;
    float  sums[3],nn[3];
    int    nkind = 3;

    if (!c->cov || !c->ch_class)
        return;

    for (j = 0; j < nkind; j++) {
        sums[j] = 0.0;
        nn[j]   = 0;
    }
    /*
   * Compute the averages over the diagonal elements for each class
   */
    for (j = 0; j < c->ncov; j++) {
        if (c->ch_class[j] >= 0) {
            sums[c->ch_class[j]] += c->cov[mne_lt_packed_index(j,j)];
            nn[c->ch_class[j]]++;
        }
    }
    fprintf(stderr,"Average noise-covariance matrix diagonals:\n");
    for (j = 0; j < nkind; j++) {
        if (nn[j] > 0) {
            sums[j] = sums[j]/nn[j];
            if (j == MNE_COV_CH_MEG_MAG)
                fprintf(stderr,"\tMagnetometers       : %-7.2f fT    reg = %-6.2f\n",1e15*sqrt(sums[j]),regs[j]);
            else if (j == MNE_COV_CH_MEG_GRAD)
                fprintf(stderr,"\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f\n",1e13*sqrt(sums[j]),regs[j]);
            else
                fprintf(stderr,"\tEEG                 : %-7.2f uV    reg = %-6.2f\n",1e6*sqrt(sums[j]),regs[j]);
            sums[j] = regs[j]*sums[j];
        }
    }
    /*
   * Add thee proper amount to the diagonal
   */
    for (j = 0; j < c->ncov; j++)
        if (c->ch_class[j] >= 0)
            c->cov[mne_lt_packed_index(j,j)] += sums[c->ch_class[j]];

    fprintf(stderr,"Noise-covariance regularized as requested.\n");
    return;
}



//============================= mne_source_space.c =============================

void mne_free_patch(mnePatchInfo p)

{
    if (!p)
        return;
    FREE(p->memb_vert);
    FREE(p);
    return;
}




//============================= make_filter_source_sapces.c =============================

static double solid_angle (float       *from,	/* From this point... */
                           mneTriangle tri)	/* ...to this triangle */
/*
 * Compute the solid angle according to van Oosterom's
 * formula
 */
{
    double v1[3],v2[3],v3[3];
    double l1,l2,l3,s,triple;
    double cross[3];

    VEC_DIFF (from,tri->r1,v1);
    VEC_DIFF (from,tri->r2,v2);
    VEC_DIFF (from,tri->r3,v3);

    CROSS_PRODUCT(v1,v2,cross);
    triple = VEC_DOT(cross,v3);

    l1 = VEC_LEN(v1);
    l2 = VEC_LEN(v2);
    l3 = VEC_LEN(v3);
    s = (l1*l2*l3+VEC_DOT(v1,v2)*l3+VEC_DOT(v1,v3)*l2+VEC_DOT(v2,v3)*l1);

    return (2.0*atan2(triple,s));
}






//============================= fwd_bem_model.c =============================

static struct {
    int  kind;
    const char *name;
} surf_expl[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };

static struct {
    int  method;
    const char *name;
} method_expl[] = { { FWD_BEM_CONSTANT_COLL , "constant collocation" },
{ FWD_BEM_LINEAR_COLL   , "linear collocation" },
{ -1                    , "unknown" } };


fwdBemModel fwd_bem_new_model()

{
    fwdBemModel m = MALLOC(1,fwdBemModelRec);

    m->surf_name   = NULL;
    m->surfs       = NULL;
    m->nsurf       = 0;
    m->ntri        = NULL;
    m->np          = NULL;
    m->sigma       = NULL;
    m->gamma       = NULL;
    m->source_mult = NULL;
    m->field_mult  = NULL;
    m->bem_method  = FWD_BEM_UNKNOWN;
    m->head_mri_t  = NULL;
    m->sol_name    = NULL;
    m->solution    = NULL;
    m->nsol        = 0;
    m->v0          = NULL;
    m->use_ip_approach = FALSE;
    m->ip_approach_limit = FWD_BEM_IP_APPROACH_LIMIT;
    return m;
}

void fwd_bem_free_solution(fwdBemModel m)

{
    if (!m)
        return;
    FREE_CMATRIX(m->solution); m->solution = NULL;
    FREE(m->sol_name); m->sol_name = NULL;
    FREE(m->v0); m->v0 = NULL;
    m->bem_method = FWD_BEM_UNKNOWN;
    m->nsol       = 0;

    return;
}

void fwd_bem_free_coil_solution(void *user)

{
    fwdBemSolution sol = (fwdBemSolution)user;

    if (!sol)
        return;
    FREE_CMATRIX(sol->solution);
    FREE(sol);
    return;
}

fwdBemSolution fwd_bem_new_coil_solution()

{
    fwdBemSolution sol = MALLOC(1,fwdBemSolutionRec);

    sol->solution = NULL;
    sol->ncoil    = 0;
    sol->np       = 0;

    return sol;
}

void fwd_bem_free_model(fwdBemModel m)

{
    int k;

    if (!m)
        return;

    FREE(m->surf_name);
    for (k = 0; k < m->nsurf; k++)
        delete m->surfs[k];
    FREE(m->surfs);
    FREE(m->ntri);
    FREE(m->np);
    FREE(m->sigma);
    FREE(m->source_mult);
    FREE(m->field_mult);
    FREE_CMATRIX(m->gamma);
    FREE(m->head_mri_t);
    fwd_bem_free_solution(m);

    FREE(m);
    return;
}

const char *fwd_bem_explain_surface(int kind)

{
    int k;

    for (k = 0; surf_expl[k].kind >= 0; k++)
        if (surf_expl[k].kind == kind)
            return surf_expl[k].name;

    return surf_expl[k].name;
}







//============================= fwd_bem_solution.c =============================

float **fwd_bem_multi_solution (float **solids,    /* The solid-angle matrix */
                                float **gamma,     /* The conductivity multipliers */
                                int   nsurf,       /* Number of surfaces */
                                int   *ntri)       /* Number of triangles or nodes on each surface */
/*
      * Invert I - solids/(2*M_PI)
      * Take deflation into account
      * The matrix is destroyed after inversion
      * This is the general multilayer case
      */
{
    int j,k,p,q;
    float defl;
    float pi2 = 1.0/(2*M_PI);
    float mult;
    int   joff,koff,jup,kup,ntot;

    for (j = 0,ntot = 0; j < nsurf; j++)
        ntot += ntri[j];
    defl = 1.0/ntot;
    /*
   * Modify the matrix
   */
    for (p = 0, joff = 0; p < nsurf; p++) {
        jup = ntri[p] + joff;
        for (q = 0, koff = 0; q < nsurf; q++) {
            kup = ntri[q] + koff;
            mult = (gamma == NULL) ? pi2 : pi2*gamma[p][q];
            for (j = joff; j < jup; j++)
                for (k = koff; k < kup; k++)
                    solids[j][k] = defl - solids[j][k]*mult;
            koff = kup;
        }
        joff = jup;
    }
    for (k = 0; k < ntot; k++)
        solids[k][k] = solids[k][k] + 1.0;

    return (mne_lu_invert(solids,ntot));
}



float **fwd_bem_homog_solution (float **solids,int ntri)
/*
      * Invert I - solids/(2*M_PI)
      * Take deflation into account
      * The matrix is destroyed after inversion
      * This is the homogeneous model case
      */
{
    return fwd_bem_multi_solution (solids,NULL,1,&ntri);
}


void fwd_bem_ip_modify_solution(float **solution,           /* The original solution */
                                float **ip_solution,	    /* The isolated problem solution */
                                float ip_mult,		    /* Conductivity ratio */
                                int nsurf,		    /* Number of surfaces */
                                int *ntri)		    /* Number of triangles (nodes) on each surface */
/*
      * Modify the solution according to the IP approach
      */
{
    int s;
    int j,k,joff,koff,ntot,nlast;
    float mult;
    float *row = NULL;
    float **sub = NULL;

    for (s = 0, koff = 0; s < nsurf-1; s++)
        koff = koff + ntri[s];
    nlast = ntri[nsurf-1];
    ntot  = koff + nlast;

    row = MALLOC(nlast,float);
    sub = MALLOC(ntot,float *);
    mult = (1.0 + ip_mult)/ip_mult;

    fprintf(stderr,"\t\tCombining...");
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square(ip_solution,nlast);
#endif
    for (s = 0, joff = 0; s < nsurf; s++) {
        fprintf(stderr,"%d3 ",s+1);
        /*
     * Pick the correct submatrix
     */
        for (j = 0; j < ntri[s]; j++)
            sub[j] = solution[j+joff]+koff;
        /*
     * Multiply
     */
#ifdef OLD
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++) {
                res = mne_dot_vectors_skip_skip(sub[j],1,ip_solution[0]+k,nlast,nlast);
                row[k] = sub[j][k] - 2.0*res;
            }
            for (k = 0; k < nlast; k++)
                sub[j][k] = row[k];
        }
#else
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++)
                row[k] = mne_dot_vectors(sub[j],ip_solution[k],nlast);
            mne_add_scaled_vector_to(row,-2.0,sub[j],nlast);
        }
#endif
        joff = joff+ntri[s];
    }
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square(ip_solution,nlast);
#endif
    fprintf(stderr,"33 ");
    /*
   * The lower right corner is a special case
   */
    for (j = 0; j < nlast; j++)
        for (k = 0; k < nlast; k++)
            sub[j][k] = sub[j][k] + mult*ip_solution[j][k];
    /*
   * Final scaling
   */
    fprintf(stderr,"done.\n\t\tScaling...");
    mne_scale_vector(ip_mult,solution[0],ntot*ntot);
    fprintf(stderr,"done.\n");
    FREE(row); FREE(sub);
    return;
}



//============================= fwd_bem_linear_collocation.c =============================



/*
 * The following approach is based on:
 *
 * de Munck JC: "A linear discretization of the volume conductor boundary integral equation using analytically integrated elements",
 * IEEE Trans Biomed Eng. 1992 39(9) : 986 - 990
 *
 */

static double calc_beta (double *rk,double *rk1)

{
    double rkk1[3];
    double size;
    double res;

    VEC_DIFF (rk,rk1,rkk1);
    size = VEC_LEN(rkk1);

    res = log((VEC_LEN(rk)*size + VEC_DOT(rk,rkk1))/
              (VEC_LEN(rk1)*size + VEC_DOT(rk1,rkk1)))/size;
    return (res);
}

static void lin_pot_coeff (float  *from,	/* Origin */
                           mneTriangle to,	/* The destination triangle */
                           double omega[3])	/* The final result */
/*
      * The linear potential matrix element computations
      */
{
    double y1[3],y2[3],y3[3];	/* Corners with origin at from */
    double *y[5];
    double **yy;
    double l1,l2,l3;		/* Lengths of y1, y2, and y3 */
    double solid;			/* The standard solid angle */
    double vec_omega[3];		/* The cross-product integral */
    double cross[3];		/* y1 x y2 */
    double triple;		/* VEC_DOT(y1 x y2,y3) */
    double ss;
    double beta[3],bbeta[3];
    int   j,k;
    double z[3];
    double n2,area2;
    double diff[3];
    static const double solid_eps = 4.0*M_PI/1.0E6;
    /*
   * This circularity makes things easy for us...
   */
    y[0] = y3;
    y[1] = y1;
    y[2] = y2;
    y[3] = y3;
    y[4] = y1;
    yy = y + 1;			/* yy can have index -1! */
    /*
   * The standard solid angle computation
   */
    VEC_DIFF (from,to->r1,y1);
    VEC_DIFF (from,to->r2,y2);
    VEC_DIFF (from,to->r3,y3);

    CROSS_PRODUCT(y1,y2,cross);
    triple = VEC_DOT(cross,y3);

    l1 = VEC_LEN(y1);
    l2 = VEC_LEN(y2);
    l3 = VEC_LEN(y3);
    ss = (l1*l2*l3+VEC_DOT(y1,y2)*l3+VEC_DOT(y1,y3)*l2+VEC_DOT(y2,y3)*l1);
    solid  = 2.0*atan2(triple,ss);
    if (fabs(solid) < solid_eps) {
        for (k = 0; k < 3; k++)
            omega[k] = 0.0;
    }
    else {
        /*
     * Calculate the magic vector vec_omega
     */
        for (j = 0; j < 3; j++)
            beta[j] = calc_beta(yy[j],yy[j+1]);
        bbeta[0] = beta[2] - beta[0];
        bbeta[1] = beta[0] - beta[1];
        bbeta[2] = beta[1] - beta[2];

        for (j = 0; j < 3; j++)
            vec_omega[j] = 0.0;
        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                vec_omega[k] = vec_omega[k] + bbeta[j]*yy[j][k];
        /*
     * Put it all together...
     */
        area2 = 2.0*to->area;
        n2 = 1.0/(area2*area2);
        for (k = 0; k < 3; k++) {
            CROSS_PRODUCT (yy[k+1],yy[k-1],z);
            VEC_DIFF (yy[k+1],yy[k-1],diff);
            omega[k] = n2*(-area2*VEC_DOT(z,to->nn)*solid +
                           triple*VEC_DOT(diff,vec_omega));
        }
    }
#ifdef CHECK
    /*
   * Check it out!
   *
   * omega1 + omega2 + omega3 = solid
   */
    rel1 = (solid + omega[X]+omega[Y]+omega[Z])/solid;
    /*
   * The other way of evaluating...
   */
    for (j = 0; j < 3; j++)
        check[j] = 0;
    for (k = 0; k < 3; k++) {
        CROSS_PRODUCT (to->nn[to],yy[k],z);
        for (j = 0; j < 3; j++)
            check[j] = check[j] + omega[k]*z[j];
    }
    for (j = 0; j < 3; j++)
        check[j] = -area2*check[j]/triple;
    fprintf (stderr,"(%g,%g,%g) =? (%g,%g,%g)\n",
             check[X],check[Y],check[Z],
             vec_omega[X],vec_omega[Y],vec_omega[Z]);
    for (j = 0; j < 3; j++)
        check[j] = check[j] - vec_omega[j];
    rel2 = sqrt(VEC_DOT(check,check)/VEC_DOT(vec_omega,vec_omega));
    fprintf (stderr,"err1 = %g, err2 = %g\n",100*rel1,100*rel2);
#endif
    return;
}



static void correct_auto_elements (MneSurfaceOrVolume::MneCSurface* surf,
                                   float      **mat)
/*
      * Improve auto-element approximation...
      */
{
    float *row;
    float sum,miss;
    int   nnode = surf->np;
    int   ntri  = surf->ntri;
    int   nmemb;
    int   j,k;
    float pi2 = 2.0*M_PI;
    mneTriangle   tri;

#ifdef SIMPLE
    for (j = 0; j < nnode; j++) {
        row = mat[j];
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + row[k];
        fprintf (stderr,"row %d sum = %g missing = %g\n",j+1,sum/pi2,
                 1.0-sum/pi2);
        row[j] = pi2 - sum;
    }
#else
    for (j = 0; j < nnode; j++) {
        /*
     * How much is missing?
     */
        row = mat[j];
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + row[k];
        miss  = pi2-sum;
        nmemb = surf->nneighbor_tri[j];
        /*
     * The node itself receives one half
     */
        row[j] = miss/2.0;
        /*
     * The rest is divided evenly among the member nodes...
     */
        miss = miss/(4.0*nmemb);
        for (k = 0,tri = surf->tris; k < ntri; k++,tri++) {
            if (tri->vert[0] == j) {
                row[tri->vert[1]] = row[tri->vert[1]] + miss;
                row[tri->vert[2]] = row[tri->vert[2]] + miss;
            }
            else if (tri->vert[1] == j) {
                row[tri->vert[0]] = row[tri->vert[0]] + miss;
                row[tri->vert[2]] = row[tri->vert[2]] + miss;
            }
            else if (tri->vert[2] == j) {
                row[tri->vert[0]] = row[tri->vert[0]] + miss;
                row[tri->vert[1]] = row[tri->vert[1]] + miss;
            }
        }
        /*
     * Just check it it out...
     *
    for (k = 0, sum = 0; k < nnode; k++)
      sum = sum + row[k];
    fprintf (stderr,"row %d sum = %g\n",j+1,sum/pi2);
    */
    }
#endif
    return;
}


static float **fwd_bem_lin_pot_coeff (MneSurfaceOrVolume::MneCSurface* *surfs,int nsurf)
/*
      * Calculate the coefficients for linear collocation approach
      */
{
    float **mat = NULL;
    float **sub_mat = NULL;
    int   np1,np2,ntri,np_tot,np_max;
    float **nodes;
    mneTriangle   tri;
    double omega[3];
    double *row = NULL;
    int    j,k,p,q,c;
    int    joff,koff;
    MneSurfaceOrVolume::MneCSurface* surf1;
    MneSurfaceOrVolume::MneCSurface* surf2;

    for (p = 0, np_tot = np_max = 0; p < nsurf; p++) {
        np_tot += surfs[p]->np;
        if (surfs[p]->np > np_max)
            np_max = surfs[p]->np;
    }

    mat = ALLOC_CMATRIX(np_tot,np_tot);
    for (j = 0; j < np_tot; j++)
        for (k = 0; k < np_tot; k++)
            mat[j][k] = 0.0;
    row        = MALLOC(np_max,double);
    sub_mat = MALLOC(np_max,float *);
    for (p = 0, joff = 0; p < nsurf; p++, joff = joff + np1) {
        surf1 = surfs[p];
        np1   = surf1->np;
        nodes = surf1->rr;
        for (q = 0, koff = 0; q < nsurf; q++, koff = koff + np2) {
            surf2 = surfs[q];
            np2   = surf2->np;
            ntri  = surf2->ntri;

            fprintf(stderr,"\t\t%s (%d) -> %s (%d) ... ",
                    fwd_bem_explain_surface(surf1->id),np1,
                    fwd_bem_explain_surface(surf2->id),np2);

            for (j = 0; j < np1; j++) {
                for (k = 0; k < np2; k++)
                    row[k] = 0.0;
                for (k = 0, tri = surf2->tris; k < ntri; k++,tri++) {
                    /*
           * No contribution from a triangle that
           * this vertex belongs to
           */
                    if (p == q && (tri->vert[0] == j || tri->vert[1] == j || tri->vert[2] == j))
                        continue;
                    /*
           * Otherwise do the hard job
           */
                    lin_pot_coeff (nodes[j],tri,omega);
                    for (c = 0; c < 3; c++)
                        row[tri->vert[c]] = row[tri->vert[c]] - omega[c];
                }
                for (k = 0; k < np2; k++)
                    mat[j+joff][k+koff] = row[k];
            }
            if (p == q) {
                for (j = 0; j < np1; j++)
                    sub_mat[j] = mat[j+joff]+koff;
                correct_auto_elements (surf1,sub_mat);
            }
            fprintf(stderr,"[done]\n");
        }
    }
    FREE(row);
    FREE(sub_mat);
    return(mat);
}


int fwd_bem_linear_collocation_solution(fwdBemModel m)
/*
 * Compute the linear collocation potential solution
 */
{
    float **coeff = NULL;
    float ip_mult;
    int k;

    fwd_bem_free_solution(m);

    fprintf(stderr,"\nComputing the linear collocation solution...\n");
    fprintf (stderr,"\tMatrix coefficients...\n");
    if ((coeff = fwd_bem_lin_pot_coeff (m->surfs,m->nsurf)) == NULL)
        goto bad;

    for (k = 0, m->nsol = 0; k < m->nsurf; k++)
        m->nsol += m->surfs[k]->np;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    if ((m->solution = fwd_bem_multi_solution (coeff,m->gamma,m->nsurf,m->np)) == NULL)
        goto bad;

    /*
   * IP approach?
   */
    if ((m->nsurf == 3) &&
            (ip_mult = m->sigma[m->nsurf-2]/m->sigma[m->nsurf-1]) <= m->ip_approach_limit) {
        float **ip_solution = NULL;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tMatrix coefficients (homog)...\n");
        if ((coeff = fwd_bem_lin_pot_coeff (m->surfs+m->nsurf-1,1)) == NULL)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        if ((ip_solution = fwd_bem_homog_solution (coeff,m->surfs[m->nsurf-1]->np)) == NULL)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");

        fwd_bem_ip_modify_solution(m->solution,ip_solution,ip_mult,m->nsurf,m->np);
        FREE_CMATRIX(ip_solution);

    }
    m->bem_method = FWD_BEM_LINEAR_COLL;
    fprintf(stderr,"Solution ready.\n");
    return OK;

bad : {
        fwd_bem_free_solution(m);
        FREE_CMATRIX(coeff);
        return FAIL;
    }
}




//============================= fwd_bem_constant_collocation.c =============================


static int fwd_bem_check_solids (float **angles,int ntri1,int ntri2, float desired)
/*
 * Check the angle computations
 */
{
    float *sums = MALLOC(ntri1,float);
    float sum;
    int j,k;
    int res = 0;

    for (j = 0; j < ntri1; j++) {
        sum = 0;
        for (k = 0; k < ntri2; k++)
            sum = sum + angles[j][k];
        sums[j] = sum/(2*M_PI);
    }
    for (j = 0; j < ntri1; j++)
        /*
     * Three cases:
     * same surface: sum = 2*pi
     * to outer:     sum = 4*pi
     * to inner:     sum = 0*pi;
     */
        if (fabs(sums[j]-desired) > 1e-4) {
            printf("solid angle matrix: rowsum[%d] = 2PI*%g",
                   j+1,sums[j]);
            res = -1;
            break;
        }
    FREE(sums);
    return res;
}


static float **fwd_bem_solid_angles (MneSurfaceOrVolume::MneCSurface* *surfs, int nsurf)
/*
      * Compute the solid angle matrix
      */
{
    MneSurfaceOrVolume::MneCSurface* surf1;
    MneSurfaceOrVolume::MneCSurface* surf2;
    mneTriangle tri;
    int ntri1,ntri2,ntri_tot;
    int j,k,p,q;
    int joff,koff;
    float **solids;
    float result;
    float **sub_solids = NULL;
    float desired;

    for (p = 0,ntri_tot = 0; p < nsurf; p++)
        ntri_tot += surfs[p]->ntri;

    sub_solids = MALLOC(ntri_tot,float *);
    solids = ALLOC_CMATRIX(ntri_tot,ntri_tot);
    for (p = 0, joff = 0; p < nsurf; p++, joff = joff + ntri1) {
        surf1 = surfs[p];
        ntri1 = surf1->ntri;
        for (q = 0, koff = 0; q < nsurf; q++, koff = koff + ntri2) {
            surf2 = surfs[q];
            ntri2 = surf2->ntri;
            fprintf(stderr,"\t\t%s (%d) -> %s (%d) ... ",fwd_bem_explain_surface(surf1->id),ntri1,fwd_bem_explain_surface(surf2->id),ntri2);
            for (j = 0; j < ntri1; j++)
                for (k = 0, tri = surf2->tris; k < ntri2; k++, tri++) {
                    if (p == q && j == k)
                        result = 0.0;
                    else
                        result = solid_angle (surf1->tris[j].cent,tri);
                    solids[j+joff][k+koff] = result;
                }
            for (j = 0; j < ntri1; j++)
                sub_solids[j] = solids[j+joff]+koff;
            fprintf(stderr,"[done]\n");
            if (p == q)
                desired = 1;
            else if (p < q)
                desired = 0;
            else
                desired = 2;
            if (fwd_bem_check_solids(sub_solids,ntri1,ntri2,desired) == FAIL) {
                FREE_CMATRIX(solids);
                FREE(sub_solids);
                return NULL;
            }
        }
    }
    FREE(sub_solids);
    return (solids);
}



int fwd_bem_constant_collocation_solution(fwdBemModel m)
/*
 * Compute the solution for the constant collocation approach
 */
{
    float  **solids = NULL;
    int    k;
    float  ip_mult;

    fwd_bem_free_solution(m);

    fprintf(stderr,"\nComputing the constant collocation solution...\n");
    fprintf(stderr,"\tSolid angles...\n");
    if ((solids = fwd_bem_solid_angles(m->surfs,m->nsurf)) == NULL)
        goto bad;

    for (k = 0, m->nsol = 0; k < m->nsurf; k++)
        m->nsol += m->surfs[k]->ntri;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    if ((m->solution = fwd_bem_multi_solution (solids,m->gamma,m->nsurf,m->ntri)) == NULL)
        goto bad;
    /*
   * IP approach?
   */
    if ((m->nsurf == 3) &&
            (ip_mult = m->sigma[m->nsurf-2]/m->sigma[m->nsurf-1]) <= m->ip_approach_limit) {
        float **ip_solution = NULL;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tSolid angles (homog)...\n");
        if ((solids = fwd_bem_solid_angles (m->surfs+m->nsurf-1,1)) == NULL)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        if ((ip_solution = fwd_bem_homog_solution (solids,m->surfs[m->nsurf-1]->ntri)) == NULL)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");
        fwd_bem_ip_modify_solution(m->solution,ip_solution,ip_mult,m->nsurf,m->ntri);
        FREE_CMATRIX(ip_solution);
    }
    m->bem_method = FWD_BEM_CONSTANT_COLL;
    fprintf (stderr,"Solution ready.\n");

    return OK;

bad : {
        fwd_bem_free_solution(m);
        FREE_CMATRIX(solids);
        return FAIL;
    }
}






//============================= fwd_bem_model.c =============================


const char *fwd_bem_explain_method(int method)

{
    int k;

    for (k = 0; method_expl[k].method >= 0; k++)
        if (method_expl[k].method == method)
            return method_expl[k].name;

    return method_expl[k].name;
}

MneSurfaceOrVolume::MneCSurface* fwd_bem_find_surface(fwdBemModel model, int kind)
/*
 * Return a pointer to a specific surface in a BEM
 */
{
    int k;
    if (!model) {
        printf("No model specified for fwd_bem_find_surface");
        return NULL;
    }
    for (k = 0; k < model->nsurf; k++)
        if (model->surfs[k]->id == kind)
            return model->surfs[k];
    printf("Desired surface (%d = %s) not found.",
           kind,fwd_bem_explain_surface(kind));
    return NULL;
}

fwdBemModel fwd_bem_load_surfaces(char *name,
                                  int  *kinds,
                                  int  nkind)
/*
 * Load a set of surfaces
 */
{
    MneSurfaceOrVolume::MneCSurface* *surfs = NULL;
    float      *sigma = NULL;
    float      *sigma1;
    fwdBemModel m = NULL;
    int         j,k;

    if (nkind <= 0) {
        printf("No surfaces specified to fwd_bem_load_surfaces");
        return NULL;
    }

    surfs = MALLOC(nkind,MneSurfaceOrVolume::MneCSurface*);
    sigma = MALLOC(nkind,float);
    for (k = 0; k < nkind; k++)
        surfs[k] = NULL;

    for (k = 0; k < nkind; k++) {
        if ((surfs[k] = MneSurfaceOrVolume::MneCSurface::read_bem_surface(name,kinds[k],TRUE,sigma+k)) == NULL)
            goto bad;
        if (sigma[k] < 0.0) {
            printf("No conductivity available for surface %s",fwd_bem_explain_surface(kinds[k]));
            goto bad;
        }
        if (surfs[k]->coord_frame != FIFFV_COORD_MRI) { /* We make our life much easier with this */
            printf("Surface %s not specified in MRI coordinates.",fwd_bem_explain_surface(kinds[k]));
            goto bad;
        }
    }
    m = fwd_bem_new_model();

    m->surf_name = mne_strdup(name);
    m->nsurf     = nkind;
    m->surfs     = surfs;
    m->sigma     = sigma;
    m->ntri      = MALLOC(nkind,int);
    m->np        = MALLOC(nkind,int);
    m->gamma = ALLOC_CMATRIX(nkind,nkind);
    m->source_mult = MALLOC(nkind,float);
    m->field_mult  = MALLOC(nkind,float);
    /*
   * Dirty trick for the zero conductivity outside
   */
    sigma1 = MALLOC(nkind+1,float);
    sigma1[0] = 0.0;
    sigma  = sigma1+1;
    for (k = 0; k < m->nsurf; k++)
        sigma[k] = m->sigma[k];
    /*
   * Gamma factors and multipliers
   */
    for (j = 0; j < m->nsurf; j++) {
        m->ntri[j] = m->surfs[j]->ntri;
        m->np[j]   = m->surfs[j]->np;
        m->source_mult[j] = 2.0/(sigma[j]+sigma[j-1]);
        m->field_mult[j] = sigma[j]-sigma[j-1];
        for (k = 0; k < m->nsurf; k++)
            m->gamma[j][k] = (sigma[k]-sigma[k-1])/(sigma[j]+sigma[j-1]);
    }
    FREE(sigma1);

    return m;

bad : {
        FREE(sigma);
        for (k = 0; k < nkind; k++)
            delete surfs[k];
        FREE(surfs);
        return NULL;
    }
}

fwdBemModel fwd_bem_load_homog_surface(char *name)
/*
 * Load surfaces for the homogeneous model
 */
{
    int kinds[] = { FIFFV_BEM_SURF_ID_BRAIN };
    int nkind   = 1;

    return fwd_bem_load_surfaces(name,kinds,nkind);
}

fwdBemModel fwd_bem_load_three_layer_surfaces(char *name)
/*
 * Load surfaces for three-layer model
 */
{
    int kinds[] = { FIFFV_BEM_SURF_ID_HEAD, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_BRAIN };
    int nkind   = 3;

    return fwd_bem_load_surfaces(name,kinds,nkind);
}

static int get_int( FiffStream::SPtr& stream, const FiffDirNode::SPtr& node,int what,int *res)
/*
* Wrapper to get int's
*/
{
    FiffTag::SPtr t_pTag;
    if(node->find_tag(stream, what, t_pTag)) {
        if (t_pTag->getType() != FIFFT_INT) {
            printf("Expected an integer tag : %d (found data type %d instead)\n",what,t_pTag->getType() );
            return FAIL;
        }
        *res = *t_pTag->toInt();
        return OK;
    }
    return FAIL;
}


int fwd_bem_load_solution(char *name, int bem_method, fwdBemModel m)
/*
* Load the potential solution matrix and attach it to the model:
*
* return values:
*
*       TRUE   found a suitable solution
*       FALSE  did not find a suitable solution
*       FAIL   error in reading the solution
*
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    float       **sol = NULL;
    FiffDirNode::SPtr bem_node;
    int         method;
    FiffTag::SPtr t_pTag;
    int         nsol;

    if(!stream->open())
        goto not_found;

    /*
   * Find the BEM data
   */
    {
        QList<FiffDirNode::SPtr> nodes = stream->tree()->dir_tree_find(FIFFB_BEM);

        if (nodes.size() == 0) {
            printf ("No BEM data in %s",name);
            goto not_found;
        }
        bem_node = nodes[0];
    }
    /*
    * Approximation method
    */
    if (get_int(stream,bem_node,FIFF_BEM_APPROX,&method) != OK)
        goto not_found;
    if (method == FIFFV_BEM_APPROX_CONST)
        method = FWD_BEM_CONSTANT_COLL;
    else if (method == FIFFV_BEM_APPROX_LINEAR)
        method = FWD_BEM_LINEAR_COLL;
    else {
        printf ("Cannot handle BEM approximation method : %d",method);
        goto bad;
    }
    if (bem_method != FWD_BEM_UNKNOWN && method != bem_method) {
        printf("Approximation method in file : %d desired : %d",method,bem_method);
        goto not_found;
    }
    {
        int         dim,k;

        if (!bem_node->find_tag(stream, FIFF_BEM_POT_SOLUTION, t_pTag))
            goto bad;
        qint32 ndim;
        QVector<qint32> dims;
        t_pTag->getMatrixDimensions(ndim, dims);

        if (ndim != 2) {
            printf("Expected a two-dimensional solution matrix instead of a %d dimensional one",ndim);
            goto bad;
        }
        for (k = 0, dim = 0; k < m->nsurf; k++)
            dim = dim + ((method == FWD_BEM_LINEAR_COLL) ? m->surfs[k]->np : m->surfs[k]->ntri);
        if (dims[0] != dim || dims[1] != dim) {
            printf("Expected a %d x %d solution matrix instead of a %d x %d  one",dim,dim,dims[0],dims[1]);
            goto not_found;
        }

        MatrixXf tmp_sol = t_pTag->toFloatMatrix().transpose();
        sol = ALLOC_CMATRIX(tmp_sol.rows(),tmp_sol.cols());
        fromFloatEigenMatrix(tmp_sol, sol);
        nsol = dims[1];
    }
    fwd_bem_free_solution(m);
    m->sol_name = mne_strdup(name);
    m->solution = sol;
    m->nsol     = nsol;
    m->bem_method = method;
    stream->close();

    return TRUE;

bad : {
        stream->close();
        FREE_CMATRIX(sol);
        return FAIL;
    }

not_found : {
        stream->close();
        FREE_CMATRIX(sol);
        return FALSE;
    }
}


int fwd_bem_compute_solution(fwdBemModel m,
                             int         bem_method)
/*
* Compute the solution
*/
{
    /*
    * Compute the solution
    */
    if (bem_method == FWD_BEM_LINEAR_COLL)
        return fwd_bem_linear_collocation_solution(m);
    else if (bem_method == FWD_BEM_CONSTANT_COLL)
        return fwd_bem_constant_collocation_solution(m);

    fwd_bem_free_solution(m);
    printf ("Unknown BEM method: %d\n",bem_method);
    return FAIL;
}

int fwd_bem_load_recompute_solution(char        *name,
                                    int         bem_method,
                                    int         force_recompute,
                                    fwdBemModel m)
/*
* Load or recompute the potential solution matrix
*/
{
    int solres;

    if (!m) {
        printf ("No model specified for fwd_bem_load_recompute_solution");
        return FAIL;
    }

    if (!force_recompute) {
        fwd_bem_free_solution(m);
        solres = fwd_bem_load_solution(name,bem_method,m);
        if (solres == TRUE) {
            fprintf(stderr,"\nLoaded %s BEM solution from %s\n",fwd_bem_explain_method(m->bem_method),name);
            return OK;
        }
        else if (solres == FAIL)
            return FAIL;
#ifdef DEBUG
        else
            fprintf(stderr,"Desired BEM  solution not available in %s (%s)\n",name,err_get_error());
#endif
    }
    if (bem_method == FWD_BEM_UNKNOWN)
        bem_method = FWD_BEM_LINEAR_COLL;
    return fwd_bem_compute_solution(m,bem_method);
}

int fwd_bem_set_head_mri_t(fwdBemModel m, fiffCoordTrans t)
/*
* Set the coordinate transformation
*/
{
    if (t->from == FIFFV_COORD_HEAD && t->to == FIFFV_COORD_MRI) {
        FREE(m->head_mri_t);
        m->head_mri_t = fiff_dup_transform(t);
        return OK;
    }
    else if (t->from == FIFFV_COORD_MRI && t->to == FIFFV_COORD_HEAD) {
        FREE(m->head_mri_t);
        m->head_mri_t = fiff_invert_transform(t);
        return OK;
    }
    else {
        printf ("Improper coordinate transform delivered to fwd_bem_set_head_mri_t");
        return FAIL;
    }
}

/*
* Some filename utilities follow
*/
static char *ends_with(char *s, char *suffix)
/*
* Does a string end with the given suffix?
*/
{
    char *p;

    if (!s)
        return NULL;

    for (p = strstr(s,suffix); p ; s = p + strlen(suffix), p = strstr(s,suffix))
        if (p == s + strlen(s) - strlen(suffix))
            return p;
    return NULL;
}

static char *strip_from(char *s, char *suffix)

{
    char *p = ends_with(s,suffix);
    char c;
    char *res;

    if (p) {
        c = *p;
        *p = '\0';
        res = mne_strdup(s);
        *p = c;
    }
    else
        res = mne_strdup(s);
    return res;
}

#define BEM_SUFFIX     "-bem.fif"
#define BEM_SOL_SUFFIX "-bem-sol.fif"

char *fwd_bem_make_bem_name(char *name)
/*
 * Make a standard BEM file name
 */
{
    char *s1,*s2;

    s1 = strip_from(name,(char*)(".fif"));
    s2 = strip_from(s1,(char*)("-sol"));
    FREE(s1);
    s1 = strip_from(s2,(char*)("-bem"));
    FREE(s2);
    s2 = MALLOC(strlen(s1)+strlen(BEM_SUFFIX)+1,char);
    sprintf(s2,"%s%s",s1,BEM_SUFFIX);
    FREE(s1);
    return s2;
}

char *fwd_bem_make_bem_sol_name(char *name)
/*
 * Make a standard BEM solution file name
 */
{
    char *s1,*s2;

    s1 = strip_from(name,(char*)(".fif"));
    s2 = strip_from(s1,(char*)("-sol"));
    FREE(s1);
    s1 = strip_from(s2,(char*)("-bem"));
    FREE(s2);
    s2 = MALLOC(strlen(s1)+strlen(BEM_SOL_SUFFIX)+1,char);
    sprintf(s2,"%s%s",s1,BEM_SOL_SUFFIX);
    FREE(s1);
    return s2;
}



//============================= mne_project_to_surface.c =============================


typedef struct {
    float *a;
    float *b;
    float *c;
    int   *act;
    int   nactive;
} *projData,projDataRec;




void mne_triangle_coords(float       *r,       /* Location of a point */
                         MneSurfaceOrVolume::MneCSurface*  s,	       /* The surface */
                         int         tri,      /* Which triangle */
                         float       *x,       /* Coordinates of the point on the triangle */
                         float       *y,
                         float       *z)
/*
      * Compute the coordinates of a point within a triangle
      */
{
    double rr[3];			/* Vector from triangle corner #1 to r */
    double a,b,c,v1,v2,det;
    mneTriangle this_tri;

    this_tri = s->tris+tri;

    VEC_DIFF(this_tri->r1,r,rr);
    *z = VEC_DOT(rr,this_tri->nn);

    a =  VEC_DOT(this_tri->r12,this_tri->r12);
    b =  VEC_DOT(this_tri->r13,this_tri->r13);
    c =  VEC_DOT(this_tri->r12,this_tri->r13);

    v1 = VEC_DOT(rr,this_tri->r12);
    v2 = VEC_DOT(rr,this_tri->r13);

    det = a*b - c*c;

    *x = (b*v1 - c*v2)/det;
    *y = (a*v2 - c*v1)/det;

    return;
}


static int nearest_triangle_point(float       *r,    /* Location of a point */
                                  MneSurfaceOrVolume::MneCSurface*  s,     /* The surface */
                                  void        *user, /* Something precomputed */
                                  int         tri,   /* Which triangle */
                                  float       *x,    /* Coordinates of the point on the triangle */
                                  float       *y,
                                  float       *z)
/*
      * Find the nearest point from a triangle
      */
{

    double p,q,p0,q0,t0;
    double rr[3];			/* Vector from triangle corner #1 to r */
    double a,b,c,v1,v2,det;
    double best,dist,dist0;
    projData    pd = (projData)user;
    mneTriangle this_tri;

    this_tri = s->tris+tri;
    VEC_DIFF(this_tri->r1,r,rr);
    dist  = VEC_DOT(rr,this_tri->nn);

    if (pd) {
        if (!pd->act[tri])
            return FALSE;
        a = pd->a[tri];
        b = pd->b[tri];
        c = pd->c[tri];
    }
    else {
        a =  VEC_DOT(this_tri->r12,this_tri->r12);
        b =  VEC_DOT(this_tri->r13,this_tri->r13);
        c =  VEC_DOT(this_tri->r12,this_tri->r13);
    }

    v1 = VEC_DOT(rr,this_tri->r12);
    v2 = VEC_DOT(rr,this_tri->r13);

    det = a*b - c*c;

    p = (b*v1 - c*v2)/det;
    q = (a*v2 - c*v1)/det;
    /*
   * If the point projects into the triangle we are done
   */
    if (p >= 0.0 && p <= 1.0 &&
            q >= 0.0 && q <= 1.0 &&
            q <= 1.0 - p) {
        *x = p;
        *y = q;
        *z = dist;
        return TRUE;
    }
    /*
   * Tough: must investigate the sides
   * We might do something intelligent here. However, for now it is ok
   * to do it in the hard way
   */
    /*
   * Side 1 -> 2
   */
    p0 = p + 0.5*(q * c)/a;
    if (p0 < 0.0)
        p0 = 0.0;
    else if (p0 > 1.0)
        p0 = 1.0;
    q0 = 0.0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    best = dist0;
    *x = p0;
    *y = q0;
    *z = dist0;
    /*
   * Side 2 -> 3
   */
    t0 = 0.5*((2.0*a-c)*(1.0-p) + (2.0*b-c)*q)/(a+b-c);
    if (t0 < 0.0)
        t0 = 0.0;
    else if (t0 > 1.0)
        t0 = 1.0;
    p0 = 1.0 - t0;
    q0 = t0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    if (dist0 < best) {
        best = dist0;
        *x = p0;
        *y = q0;
        *z = dist0;
    }
    /*
   * Side 1 -> 3
   */
    p0 = 0.0;
    q0 = q + 0.5*(p * c)/b;
    if (q0 < 0.0)
        q0 = 0.0;
    else if (q0 > 1.0)
        q0 = 1.0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    if (dist0 < best) {
        best = dist0;
        *x = p0;
        *y = q0;
        *z = dist0;
    }
    return TRUE;
}

static void project_to_triangle(MneSurfaceOrVolume::MneCSurface* s,
                                int        tri,
                                float      p,
                                float      q,
                                float      *r)

{
    int   k;
    mneTriangle this_tri;

    this_tri = s->tris+tri;

    for (k = 0; k < 3; k++)
        r[k] = this_tri->r1[k] + p*this_tri->r12[k] + q*this_tri->r13[k];

    return;
}

int mne_nearest_triangle_point(float       *r,    /* Location of a point */
                               MneSurfaceOrVolume::MneCSurface*  s,     /* The surface */
                               int         tri,   /* Which triangle */
                               float       *x,    /* Coordinates of the point on the triangle */
                               float       *y,
                               float       *z)
/*
 * This is for external use
 */
{
    return nearest_triangle_point(r,s,NULL,tri,x,y,z);
}



int mne_project_to_surface(MneSurfaceOrVolume::MneCSurface* s, void *proj_data, float *r, int project_it, float *distp)
/*
      * Project the point onto the closest point on the surface
      */
{
    float dist;			/* Distance to the triangle */
    float p,q;			/* Coordinates on the triangle */
    float p0,q0,dist0;
    int   best;
    int   k;

    p0 = q0 = 0.0;
    dist0 = 0.0;
    for (best = -1, k = 0; k < s->ntri; k++) {
        if (nearest_triangle_point(r,s,proj_data,k,&p,&q,&dist)) {
            if (best < 0 || fabs(dist) < fabs(dist0)) {
                dist0 = dist;
                best = k;
                p0 = p;
                q0 = q;
            }
        }
    }
    if (best >= 0 && project_it)
        project_to_triangle(s,best,p0,q0,r);
    if (distp)
        *distp = dist0;
    return best;
}








//============================= fwd_bem_pot.c =============================

static float fwd_bem_inf_field(float *rd,      /* Dipole position */
                               float *Q,       /* Dipole moment */
                               float *rp,      /* Field point */
                               float *dir)     /* Which field component */
/*
 * Infinite-medium magnetic field
 * (without \mu_0/4\pi)
 */
{
    float diff[3],diff2,cross[3];

    VEC_DIFF (rd,rp,diff);
    diff2 = VEC_DOT(diff,diff);
    CROSS_PRODUCT (Q,diff,cross);

    return (VEC_DOT(cross,dir)/(diff2*sqrt(diff2)));
}


float fwd_bem_inf_pot (float *rd,	/* Dipole position */
                       float *Q,	/* Dipole moment */
                       float *rp)	/* Potential point */
/*
 * The infinite medium potential
 */
{
    float diff[3];
    float diff2;
    VEC_DIFF(rd,rp,diff);
    diff2 = VEC_DOT(diff,diff);
    return (VEC_DOT(Q,diff)/(4.0*M_PI*diff2*sqrt(diff2)));
}




int fwd_bem_specify_els(fwdBemModel m,
                        FwdCoilSet*  els)
/*
 * Set up for computing the solution at a set of electrodes
 */
{
    FwdCoil*     el;
    MneSurfaceOrVolume::MneCSurface*  scalp;
    int         k,p,q,v;
    float       *one_sol,*pick_sol;
    float       r[3],w[3],dist;
    int         best;
    mneTriangle tri;
    float       x,y,z;
    fwdBemSolution sol;

    extern fwdBemSolution fwd_bem_new_coil_solution();
    extern void fwd_bem_free_coil_solution(void *user);

    if (!m) {
        printf("Model missing in fwd_bem_specify_els");
        goto bad;
    }
    if (!m->solution) {
        printf("Solution not computed in fwd_bem_specify_els");
        goto bad;
    }
    if (!els || els->ncoil == 0)
        return OK;
    els->fwd_free_coil_set_user_data();
    /*
   * Hard work follows
   */
    els->user_data = sol = fwd_bem_new_coil_solution();
    els->user_data_free = fwd_bem_free_coil_solution;

    sol->ncoil = els->ncoil;
    sol->np    = m->nsol;
    sol->solution  = ALLOC_CMATRIX(sol->ncoil,sol->np);
    /*
   * Go through all coils
   */
    for (k = 0; k < els->ncoil; k++) {
        el = els->coils[k];
        one_sol = sol->solution[k];
        for (q = 0; q < m->nsol; q++)
            one_sol[q] = 0.0;
        scalp = m->surfs[0];
        /*
     * Go through all 'integration points'
     */
        for (p = 0; p < el->np; p++) {
            VEC_COPY(r,el->rmag[p]);
            if (m->head_mri_t != NULL)
                fiff_coord_trans(r,m->head_mri_t,FIFFV_MOVE);
            best = mne_project_to_surface(scalp,NULL,r,FALSE,&dist);
            if (best < 0) {
                printf("One of the electrodes could not be projected onto the scalp surface. How come?");
                goto bad;
            }
            if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
                /*
         * Simply pick the value at the triangle
         */
                pick_sol = m->solution[best];
                for (q = 0; q < m->nsol; q++)
                    one_sol[q] += el->w[p]*pick_sol[q];
            }
            else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
                /*
         * Calculate a linear interpolation between the vertex values
         */
                tri = scalp->tris+best;
                mne_triangle_coords(r,scalp,best,&x,&y,&z);

                w[X] = el->w[p]*(1.0 - x - y);
                w[Y] = el->w[p]*x;
                w[Z] = el->w[p]*y;
                for (v = 0; v < 3; v++) {
                    pick_sol = m->solution[tri->vert[v]];
                    for (q = 0; q < m->nsol; q++)
                        one_sol[q] += w[v]*pick_sol[q];
                }
            }
            else {
                printf("Unknown BEM approximation method : %d\n",m->bem_method);
                goto bad;
            }
        }
    }
    return OK;

bad : {
        els->fwd_free_coil_set_user_data();
        return FAIL;
    }
}




static void fwd_bem_lin_pot_calc (float       *rd,		/* Dipole position */
                                  float       *Q,		/* Dipole orientation */
                                  fwdBemModel m,		/* The model */
                                  FwdCoilSet*  els,              /* Use this electrode set if available */
                                  int         all_surfs,	/* Compute on all surfaces? */
                                  float      *pot)              /* Put the result here */
/*
      * Compute the potentials due to a current dipole
      * using the linear potential approximation
      */
{
    float **rr;
    int   np;
    int   s,k,p,nsol;
    float mult,mri_rd[3],mri_Q[3];

    float *v0;
    float **solution;

    if (!m->v0)
        m->v0 = MALLOC(m->nsol,float);
    v0 = m->v0;

    VEC_COPY(mri_rd,rd);
    VEC_COPY(mri_Q,Q);
    if (m->head_mri_t) {
        fiff_coord_trans(mri_rd,m->head_mri_t,FIFFV_MOVE);
        fiff_coord_trans(mri_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < m->nsurf; s++) {
        np     = m->surfs[s]->np;
        rr     = m->surfs[s]->rr;
        mult   = m->source_mult[s];
        for (k = 0; k < np; k++)
            v0[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,rr[k]);
    }
    if (els) {
        fwdBemSolution sol = (fwdBemSolution)els->user_data;
        solution = sol->solution;
        nsol     = sol->ncoil;
    }
    else {
        solution = m->solution;
        nsol     = all_surfs ? m->nsol : m->surfs[0]->np;
    }
    for (k = 0; k < nsol; k++)
        pot[k] = mne_dot_vectors(solution[k],v0,m->nsol);
    return;
}



static void fwd_bem_pot_calc (float       *rd,	     /* Dipole position */
                              float       *Q,        /* Dipole orientation */
                              fwdBemModel m,	     /* The model */
                              FwdCoilSet*  els,       /* Use this electrode set if available */
                              int         all_surfs, /* Compute solution on all surfaces? */
                              float       *pot)
/*
      * Compute the potentials due to a current dipole
      */
{
    mneTriangle tri;
    int         ntri;
    int         s,k,p,nsol;
    float       mult;
    float       *v0;
    float       **solution;
    float       mri_rd[3],mri_Q[3];

    if (!m->v0)
        m->v0 = MALLOC(m->nsol,float);
    v0 = m->v0;

    VEC_COPY(mri_rd,rd);
    VEC_COPY(mri_Q,Q);
    if (m->head_mri_t) {
        fiff_coord_trans(mri_rd,m->head_mri_t,FIFFV_MOVE);
        fiff_coord_trans(mri_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < m->nsurf; s++) {
        ntri = m->surfs[s]->ntri;
        tri  = m->surfs[s]->tris;
        mult = m->source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,tri->cent);
    }
    if (els) {
        fwdBemSolution sol = (fwdBemSolution)els->user_data;
        solution = sol->solution;
        nsol     = sol->ncoil;
    }
    else {
        solution = m->solution;
        nsol     = all_surfs ? m->nsol : m->surfs[0]->ntri;
    }
    for (k = 0; k < nsol; k++)
        pot[k] = mne_dot_vectors(solution[k],v0,m->nsol);
    return;
}


int fwd_bem_pot_els (float       *rd,	  /* Dipole position */
                     float       *Q,	  /* Dipole orientation */
                     FwdCoilSet*  els,     /* Electrode descriptors */
                     float       *pot,    /* Result */
                     void        *client) /* The model */
/*
 * This version calculates the potential on all surfaces
 */
{
    fwdBemModel    m = (fwdBemModel)client;
    fwdBemSolution sol = (fwdBemSolution)els->user_data;

    if (!m) {
        printf("No BEM model specified to fwd_bem_pot_els");
        return FAIL;
    }
    if (!m->solution) {
        printf("No solution available for fwd_bem_pot_els");
        return FAIL;
    }
    if (!sol || sol->ncoil != els->ncoil) {
        printf("No appropriate electrode-specific data available in fwd_bem_pot_coils");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        fwd_bem_pot_calc(rd,Q,m,els,FALSE,pot);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        fwd_bem_lin_pot_calc(rd,Q,m,els,FALSE,pot);
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}






//============================= fwd_bem_field.c =============================


/*
 * These are some of the integration formulas listed in
 *
 * L. Urankar, Common compact analytical formulas for computation of
 * geometry integrals on a basic Cartesian sub-domain in boundary and
 * volume integral methods, Engineering Analysis with Boundary Elements,
 * 7 (3), 1990, 124 - 129.
 *
 */
#define ARSINH(x) log((x) + sqrt(1.0+(x)*(x)))


static void calc_f (double *xx,
                    double *yy,		/* Corner coordinates */
                    double *f0,
                    double *fx,
                    double *fy)	        /* The weights in the linear approximation */

{
    double det = -xx[Y]*yy[X] + xx[Z]*yy[X] +
            xx[X]*yy[Y] - xx[Z]*yy[Y] - xx[X]*yy[Z] + xx[Y]*yy[Z];
    int k;

    f0[X] = -xx[Z]*yy[Y] + xx[Y]*yy[Z];
    f0[Y] = xx[Z]*yy[X] - xx[X]*yy[Z];
    f0[Z] = -xx[Y]*yy[X] + xx[X]*yy[Y];

    fx[X] =  yy[Y] - yy[Z];
    fx[Y] = -yy[X] + yy[Z];
    fx[Z] = yy[X] - yy[Y];

    fy[X] = -xx[Y] + xx[Z];
    fy[Y] = xx[X] - xx[Z];
    fy[Z] = -xx[X] + xx[Y];

    for (k = 0; k < 3; k++) {
        f0[k] = f0[k]/det;
        fx[k] = fx[k]/det;
        fy[k] = fy[k]/det;
    }
}



static void calc_magic (double u,double z,
                        double A,
                        double B,
                        double *beta,
                        double *D)
/*
      * Calculate Urankar's magic numbers
      */
{
    double B2 = 1.0 + B*B;
    double ABu = A + B*u;
    *D = sqrt(u*u + z*z + ABu*ABu);
    beta[0] = ABu/sqrt(u*u + z*z);
    beta[1] = (A*B + B2*u)/sqrt(A*A + B2*z*z);
    beta[2] = (B*z*z - A*u)/(z*(*D));
}


static void field_integrals (float *from,
                             mneTriangle to,
                             double *I1p,
                             double *T,double *S1,double *S2,
                             double *f0,double *fx,double *fy)

{
    double y1[3],y2[3],y3[3];
    double xx[4],yy[4];
    double A,B,z,dx;
    double beta[3],I1,Tx,Ty,Txx,Tyy,Sxx,mult;
    double S1x,S1y,S2x,S2y;
    double D1,B2;
    int k;
    /*
   * Preliminaries...
   *
   * 1. Move origin to viewpoint...
   *
   */
    VEC_DIFF (from,to->r1,y1);
    VEC_DIFF (from,to->r2,y2);
    VEC_DIFF (from,to->r3,y3);
    /*
   * 2. Calculate local xy coordinates...
   */
    xx[0] = VEC_DOT(y1,to->ex);
    xx[1] = VEC_DOT(y2,to->ex);
    xx[2] = VEC_DOT(y3,to->ex);
    xx[3] = xx[0];

    yy[0] = VEC_DOT(y1,to->ey);
    yy[1] = VEC_DOT(y2,to->ey);
    yy[2] = VEC_DOT(y3,to->ey);
    yy[3] = yy[0];

    calc_f (xx,yy,f0,fx,fy);
    /*
   * 3. Distance of the plane from origin...
   */
    z = VEC_DOT(y1,to->nn);
    /*
   * Put together the line integral...
   * We use the convention where the local y-axis
   * is parallel to the last side and, therefore, dx = 0
   * on that side. We can thus omit the last side from this
   * computation in some cases.
   */
    I1 = 0.0;
    Tx = 0.0;
    Ty = 0.0;
    S1x = 0.0;
    S1y = 0.0;
    S2x = 0.0;
    S2y = 0.0;
    for (k = 0; k < 2; k++) {
        dx = xx[k+1] - xx[k];
        A = (yy[k]*xx[k+1] - yy[k+1]*xx[k])/dx;
        B = (yy[k+1]-yy[k])/dx;
        B2 = (1.0 + B*B);
        /*
     * Upper limit
     */
        calc_magic (xx[k+1],z,A,B,beta,&D1);
        I1 = I1 - xx[k+1]*ARSINH(beta[0]) - (A/sqrt(1.0+B*B))*ARSINH(beta[1])
                - z*atan(beta[2]);
        Txx = ARSINH(beta[1])/sqrt(B2);
        Tx = Tx + Txx;
        Ty = Ty + B*Txx;
        Sxx = (D1 - A*B*Txx)/B2;
        S1x = S1x + Sxx;
        S1y = S1y + B*Sxx;
        Sxx = (B*D1 + A*Txx)/B2;
        S2x = S2x + Sxx;
        /*
     * Lower limit
     */
        calc_magic (xx[k],z,A,B,beta,&D1);
        I1 = I1 + xx[k]*ARSINH(beta[0]) + (A/sqrt(1.0+B*B))*ARSINH(beta[1])
                + z*atan(beta[2]);
        Txx = ARSINH(beta[1])/sqrt(B2);
        Tx = Tx - Txx;
        Ty = Ty - B*Txx;
        Sxx = (D1 - A*B*Txx)/B2;
        S1x = S1x - Sxx;
        S1y = S1y - B*Sxx;
        Sxx = (B*D1 + A*Txx)/B2;
        S2x = S2x - Sxx;
    }
    /*
   * Handle last side (dx = 0) in a special way;
   */
    mult = 1.0/sqrt(xx[k]*xx[k]+z*z);
    /*
   * Upper...
   */
    Tyy = ARSINH(mult*yy[k+1]);
    Ty = Ty + Tyy;
    S1y = S1y + xx[k]*Tyy;
    /*
   * Lower...
   */
    Tyy = ARSINH(mult*yy[k]);
    Ty = Ty - Tyy;
    S1y = S1y - xx[k]*Tyy;
    /*
   * Set return values
   */
    *I1p = I1;
    T[X] = Tx;
    T[Y] = Ty;
    S1[X] = S1x;
    S1[Y] = S1y;
    S2[X] = S2x;
    S2[Y] = -S1x;
    return;
}


static double one_field_coeff (float       *dest,	/* The destination field point */
                               float       *normal,	/* The field direction we are interested in */
                               mneTriangle tri)
/*
 * Compute the integral over one triangle.
 * This looks magical but it is not.
 */
{
    double *yy[4];
    double y1[3],y2[3],y3[3];
    double beta[3];
    double bbeta[3];
    double coeff[3];
    int   j,k;

    yy[0] = y1;
    yy[1] = y2;
    yy[2] = y3;
    yy[3] = y1;
    VEC_DIFF (dest,tri->r1,y1);
    VEC_DIFF (dest,tri->r2,y2);
    VEC_DIFF (dest,tri->r3,y3);
    for (j = 0; j < 3; j++)
        beta[j] = calc_beta(yy[j],yy[j+1]);
    bbeta[0] = beta[2] - beta[0];
    bbeta[1] = beta[0] - beta[1];
    bbeta[2] = beta[1] - beta[2];

    for (j = 0; j < 3; j++)
        coeff[j] = 0.0;
    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            coeff[k] = coeff[k] + yy[j][k]*bbeta[j];
    return (VEC_DOT(coeff,normal));
}



float **fwd_bem_field_coeff(fwdBemModel m,	/* The model */
                            FwdCoilSet*  coils)	/* Gradiometer coil positions */
/*
 * Compute the weighting factors to obtain the magnetic field
 */
{
    MneSurfaceOrVolume::MneCSurface*     surf;
    mneTriangle    tri;
    FwdCoil*        coil;
    FwdCoilSet*     tcoils = NULL;
    int            ntri;
    float          **coeff = NULL;
    int            j,k,p,s,off;
    double         res;
    double         mult;

    if (m->solution == NULL) {
        printf("Solution matrix missing in fwd_bem_field_coeff");
        return NULL;
    }
    if (m->bem_method != FWD_BEM_CONSTANT_COLL) {
        printf("BEM method should be constant collocation for fwd_bem_field_coeff");
        return NULL;
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (!m->head_mri_t) {
                printf("head -> mri coordinate transform missing in fwd_bem_field_coeff");
                return NULL;
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return NULL;
                }
                /*
                * Make a transformed duplicate
                */
                if ((tcoils = coils->dup_coil_set(m->head_mri_t)) == NULL)
                    return NULL;
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return NULL;
        }
    }
    ntri  = m->nsol;
    coeff = ALLOC_CMATRIX(coils->ncoil,ntri);

    for (s = 0, off = 0; s < m->nsurf; s++) {
        surf = m->surfs[s];
        ntri = surf->ntri;
        tri  = surf->tris;
        mult = m->field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil; j++) {
                coil = coils->coils[j];
                res = 0.0;
                for (p = 0; p < coil->np; p++)
                    res = res + coil->w[p]*one_field_coeff(coil->rmag[p],coil->cosmag[p],tri);
                coeff[j][k+off] = mult*res;
            }
        }
        off = off + ntri;
    }
    delete tcoils;
    return coeff;
}



/*
 * These are the formulas from Ferguson et al
 * A Complete Linear Discretization for Calculating the Magnetic Field
 * Using the Boundary-Element Method, IEEE Trans. Biomed. Eng., submitted
 */

static double calc_gamma (double *rk,double *rk1)

{
    double rkk1[3];
    double size;
    double res;

    VEC_DIFF (rk,rk1,rkk1);
    size = VEC_LEN(rkk1);

    res = log((VEC_LEN(rk1)*size + VEC_DOT(rk1,rkk1))/
              (VEC_LEN(rk)*size + VEC_DOT(rk,rkk1)))/size;
    return (res);
}




void fwd_bem_one_lin_field_coeff_ferg (float *dest,	/* The field point */
                                       float *dir,	/* The interesting direction */
                                       mneTriangle tri,	/* The destination triangle */
                                       double *res)	/* The results */

{
    double c[3];			/* Component of dest vector normal to
                                 * the triangle plane */
    double A[3];			/* Projection of dest onto the triangle */
    double c1[3],c2[3],c3[3];
    double y1[3],y2[3],y3[3];
    double *yy[4],*cc[4];
    double rjk[3][3];
    double cross[3],triple,l1,l2,l3,solid,clen;
    double common,sum,beta,gamma;
    int    k;

    yy[0] = y1;   cc[0] = c1;
    yy[1] = y2;   cc[1] = c2;
    yy[2] = y3;   cc[2] = c3;
    yy[3] = y1;   cc[3] = c1;

    VEC_DIFF(tri->r2,tri->r3,rjk[0]);
    VEC_DIFF(tri->r3,tri->r1,rjk[1]);
    VEC_DIFF(tri->r1,tri->r2,rjk[2]);

    for (k = 0; k < 3; k++) {
        y1[k] = tri->r1[k] - dest[k];
        y2[k] = tri->r2[k] - dest[k];
        y3[k] = tri->r3[k] - dest[k];
    }
    clen  = VEC_DOT(y1,tri->nn);
    for (k = 0; k < 3; k++) {
        c[k]  = clen*tri->nn[k];
        A[k]  = dest[k] + c[k];
        c1[k] = tri->r1[k] - A[k];
        c2[k] = tri->r2[k] - A[k];
        c3[k] = tri->r3[k] - A[k];
    }
    /*
   * beta and gamma...
   */
    for (sum = 0.0, k = 0; k < 3; k++) {
        CROSS_PRODUCT(cc[k],cc[k+1],cross);
        beta  = VEC_DOT(cross,tri->nn);
        gamma = calc_gamma (yy[k],yy[k+1]);
        sum = sum + beta*gamma;
    }
    /*
   * Solid angle...
   */
    CROSS_PRODUCT(y1,y2,cross);
    triple = VEC_DOT(cross,y3);

    l1 = VEC_LEN(y1);
    l2 = VEC_LEN(y2);
    l3 = VEC_LEN(y3);
    solid = 2.0*atan2(triple,
                      (l1*l2*l3+
                       VEC_DOT(y1,y2)*l3+
                       VEC_DOT(y1,y3)*l2+
                       VEC_DOT(y2,y3)*l1));
    /*
   * Now we are ready to assemble it all together
   */
    common = (sum-clen*solid)/(2.0*tri->area);
    for (k = 0; k < 3; k++)
        res[k] = -VEC_DOT(rjk[k],dir)*common;
    return;
}





void fwd_bem_one_lin_field_coeff_uran(float *dest,	/* The field point */
                                      float *dir,	/* The interesting direction */
                                      mneTriangle tri,	/* The destination triangle */
                                      double *res)	/* The results */

{
    double      I1,T[2],S1[2],S2[2];
    double      f0[3],fx[3],fy[3];
    double      res_x,res_y;
    double      x_fac,y_fac;
    int         k;
    double      len;
    /*
   * Compute the component integrals
   */
    field_integrals (dest,tri,&I1,T,S1,S2,f0,fx,fy);
    /*
   * Compute the coefficient for each node...
   */
    len = VEC_LEN(dir);
    dir[X] = dir[X]/len;
    dir[Y] = dir[Y]/len;
    dir[Z] = dir[Z]/len;

    x_fac = -VEC_DOT(dir,tri->ex);
    y_fac = -VEC_DOT(dir,tri->ey);
    for (k = 0; k < 3; k++) {
        res_x = f0[k]*T[X] + fx[k]*S1[X] + fy[k]*S2[X] + fy[k]*I1;
        res_y = f0[k]*T[Y] + fx[k]*S1[Y] + fy[k]*S2[Y] - fx[k]*I1;
        res[k] = x_fac*res_x + y_fac*res_y;
    }
    return;
}


void fwd_bem_one_lin_field_coeff_simple (float       *dest,    /* The destination field point */
                                         float       *normal,  /* The field direction we are interested in */
                                         mneTriangle source,   /* The source triangle */
                                         double      *res)     /* The result for each triangle node */
/*
      * Simple version...
      */
{
    float diff[3];
    float vec_result[3];
    float dl;
    int   k;
    float *rr[3];


    rr[0] = source->r1;
    rr[1] = source->r2;
    rr[2] = source->r3;

    for (k = 0; k < 3; k++) {
        VEC_DIFF (rr[k],dest,diff);
        dl = VEC_DOT(diff,diff);
        CROSS_PRODUCT (diff,source->nn,vec_result);
        res[k] = source->area*VEC_DOT(vec_result,normal)/(3.0*dl*sqrt(dl));
    }
    return;
}

typedef void (* linFieldIntFunc)(float *dest,float *dir,mneTriangle tri, double *res);

float **fwd_bem_lin_field_coeff (fwdBemModel m,	        /* The model */
                                 FwdCoilSet*  coils,	/* Coil information */
                                 int         method)	/* Which integration formula to use */
/*
      * Compute the weighting factors to obtain the magnetic field
      * in the linear potential approximation
      */
{
    MneSurfaceOrVolume::MneCSurface*  surf;
    mneTriangle tri;
    FwdCoil*     coil;
    FwdCoilSet*  tcoils = NULL;
    int         ntri;
    float       **coeff  = NULL;
    int         j,k,p,pp,off,s;
    double      res[3],one[3];
    float       mult;
    linFieldIntFunc func;

    if (m->solution == NULL) {
        printf("Solution matrix missing in fwd_bem_lin_field_coeff");
        return NULL;
    }
    if (m->bem_method != FWD_BEM_LINEAR_COLL) {
        printf("BEM method should be linear collocation for fwd_bem_lin_field_coeff");
        return NULL;
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (!m->head_mri_t) {
                printf("head -> mri coordinate transform missing in fwd_bem_lin_field_coeff");
                return NULL;
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return NULL;
                }
                /*
                * Make a transformed duplicate
                */
                if ((tcoils = coils->dup_coil_set(m->head_mri_t)) == NULL)
                    return NULL;
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return NULL;
        }
    }
    if (method == FWD_BEM_LIN_FIELD_FERGUSON)
        func = fwd_bem_one_lin_field_coeff_ferg;
    else if (method == FWD_BEM_LIN_FIELD_URANKAR)
        func = fwd_bem_one_lin_field_coeff_uran;
    else
        func = fwd_bem_one_lin_field_coeff_simple;

    coeff = ALLOC_CMATRIX(coils->ncoil,m->nsol);
    for (k = 0; k < m->nsol; k++)
        for (j = 0; j < coils->ncoil; j++)
            coeff[j][k] = 0.0;
    /*
   * Process each of the surfaces
   */
    for (s = 0, off = 0; s < m->nsurf; s++) {
        surf = m->surfs[s];
        ntri = surf->ntri;
        tri  = surf->tris;
        mult = m->field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil; j++) {
                coil = coils->coils[j];
                for (pp = 0; pp < 3; pp++)
                    res[pp] = 0;
                /*
         * Accumulate the coefficients for each triangle node...
         */
                for (p = 0; p < coil->np; p++) {
                    func(coil->rmag[p],coil->cosmag[p],tri,one);
                    for (pp = 0; pp < 3; pp++)
                        res[pp] = res[pp] + coil->w[p]*one[pp];
                }
                /*
         * Add these to the corresponding coefficient matrix
         * elements...
         */
                for (pp = 0; pp < 3; pp++)
                    coeff[j][tri->vert[pp]+off] = coeff[j][tri->vert[pp]+off] + mult*res[pp];
            }
        }
        off = off + surf->np;
    }
    /*
   * Discard the duplicate
   */
    delete tcoils;
    return (coeff);
}



int fwd_bem_specify_coils(fwdBemModel m,
                          FwdCoilSet*  coils)
/*
 * Set up for computing the solution at a set of coils
  */
{
    float **sol = NULL;
    fwdBemSolution csol;

    if (!m) {
        printf("Model missing in fwd_bem_specify_coils");
        goto bad;
    }
    if (!m->solution) {
        printf("Solution not computed in fwd_bem_specify_coils");
        goto bad;
    }
    if(coils)
        coils->fwd_free_coil_set_user_data();
    if (!coils || coils->ncoil == 0)
        return OK;
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        sol = fwd_bem_field_coeff(m,coils);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        sol = fwd_bem_lin_field_coeff(m,coils,FWD_BEM_LIN_FIELD_SIMPLE);
    else {
        printf("Unknown BEM method in fwd_bem_specify_coils : %d",m->bem_method);
        goto bad;
    }
    coils->user_data = csol = fwd_bem_new_coil_solution();
    coils->user_data_free   = fwd_bem_free_coil_solution;

    csol->ncoil     = coils->ncoil;
    csol->np        = m->nsol;
    csol->solution  = mne_mat_mat_mult(sol,m->solution,coils->ncoil,m->nsol,m->nsol);

    FREE_CMATRIX(sol);
    return OK;

bad : {
        FREE_CMATRIX(sol);
        return FAIL;

    }
}




#define MAG_FACTOR 1e-7		/* \mu_0/4\pi */




static void fwd_bem_lin_field_calc(float       *rd,
                                   float       *Q,
                                   FwdCoilSet*  coils,
                                   fwdBemModel m,
                                   float       *B)
/*
 * Calculate the magnetic field in a set of coils
 */
{
    float *v0;
    int   s,k,p,np;
    FwdCoil* coil;
    float  mult;
    float  **rr;
    float  my_rd[3],my_Q[3];
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;
    /*
   * Infinite-medium potentials
   */
    if (!m->v0)
        m->v0 = MALLOC(m->nsol,float);
    v0 = m->v0;
    /*
   * The dipole location and orientation must be transformed
   */
    VEC_COPY(my_rd,rd);
    VEC_COPY(my_Q,Q);
    if (m->head_mri_t) {
        fiff_coord_trans(my_rd,m->head_mri_t,FIFFV_MOVE);
        fiff_coord_trans(my_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    /*
   * Compute the inifinite-medium potentials at the vertices
   */
    for (s = 0, p = 0; s < m->nsurf; s++) {
        np     = m->surfs[s]->np;
        rr     = m->surfs[s]->rr;
        mult   = m->source_mult[s];
        for (k = 0; k < np; k++)
            v0[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,rr[k]);
    }
    /*
   * Primary current contribution
   * (can be calculated in the coil/dipole coordinates)
   */
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(rd,Q,coil->rmag[p],coil->cosmag[p]);
    }
    /*
   * Volume current contribution
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = B[k] + mne_dot_vectors(sol->solution[k],v0,m->nsol);
    /*
   * Scale correctly
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}


static void fwd_bem_field_calc(float       *rd,
                               float       *Q,
                               FwdCoilSet*  coils,
                               fwdBemModel m,
                               float       *B)
/*
 * Calculate the magnetic field in a set of coils
 */
{
    float *v0;
    int   s,k,p,ntri;
    FwdCoil* coil;
    mneTriangle tri;
    float   mult;
    float  my_rd[3],my_Q[3];
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;
    /*
   * Infinite-medium potentials
   */
    if (!m->v0)
        m->v0 = MALLOC(m->nsol,float);
    v0 = m->v0;
    /*
   * The dipole location and orientation must be transformed
   */
    VEC_COPY(my_rd,rd);
    VEC_COPY(my_Q,Q);
    if (m->head_mri_t) {
        fiff_coord_trans(my_rd,m->head_mri_t,FIFFV_MOVE);
        fiff_coord_trans(my_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    /*
   * Compute the inifinite-medium potentials at the centers of the triangles
   */
    for (s = 0, p = 0; s < m->nsurf; s++) {
        ntri = m->surfs[s]->ntri;
        tri  = m->surfs[s]->tris;
        mult = m->source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,tri->cent);
    }
    /*
   * Primary current contribution
   * (can be calculated in the coil/dipole coordinates)
   */
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(rd,Q,coil->rmag[p],coil->cosmag[p]);
    }
    /*
   * Volume current contribution
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = B[k] + mne_dot_vectors(sol->solution[k],v0,m->nsol);
    /*
   * Scale correctly
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}




int fwd_bem_field(float       *rd,	/* Dipole position */
                  float       *Q,	/* Dipole orientation */
                  FwdCoilSet*  coils,    /* Coil descriptors */
                  float       *B,       /* Result */
                  void        *client)  /* The model */
/*
 * This version calculates the magnetic field in a set of coils
 * Call fwd_bem_specify_coils first to establish the coil-specific
 * solution matrix
 */
{
    fwdBemModel m = (fwdBemModel)client;
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;

    if (!m) {
        printf("No BEM model specified to fwd_bem_field");
        return FAIL;
    }
    if (!sol || !sol->solution || sol->ncoil != coils->ncoil) {
        printf("No appropriate coil-specific data available in fwd_bem_field");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        fwd_bem_field_calc(rd,Q,coils,m,B);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        fwd_bem_lin_field_calc(rd,Q,coils,m,B);
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}

\
//============================= fwd_comp.c =============================




int fwd_comp_field(float *rd,float *Q, FwdCoilSet* coils, float *res, void *client)
/*
      * Calculate the compensated field (one dipole component)
      */
{
    fwdCompData comp = (fwdCompData)client;

    if (!comp->field) {
        printf("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
   * First compute the field in the primary set of coils
   */
    if (comp->field(rd,Q,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
   * Compensation needed?
   */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
   * Workspace needed?
   */
    if (!comp->work)
        comp->work = MALLOC(comp->comp_coils->ncoil,float);
    /*
   * Compute the field in the compensation coils
   */
    if (comp->field(rd,Q,comp->comp_coils,comp->work,comp->client) == FAIL)
        return FAIL;
    /*
   * Compute the compensated field
   */
    return mne_apply_ctf_comp(comp->set,TRUE,res,coils->ncoil,comp->work,comp->comp_coils->ncoil);
}



/*
 * Routines to implement the reference channel compensation in field computations
 */

void fwd_free_comp_data(void *d)

{
    fwdCompData comp = (fwdCompData)d;

    if (!comp)
        return;
    delete comp->comp_coils;
    mne_free_ctf_comp_data_set(comp->set);
    FREE(comp->work);
    FREE_CMATRIX(comp->vec_work);

    if (comp->client_free && comp->client)
        comp->client_free(comp->client);

    FREE(comp);
    return;
}




fwdCompData fwd_new_comp_data()

{
    fwdCompData comp = MALLOC(1,fwdCompDataRec);

    comp->comp_coils  = NULL;
    comp->field       = NULL;
    comp->vec_field   = NULL;
    comp->field_grad  = NULL;
    comp->client      = NULL;
    comp->client_free = NULL;
    comp->set         = NULL;
    comp->work        = NULL;
    comp->vec_work    = NULL;
    return comp;
}




static int fwd_make_ctf_comp_coils(mneCTFcompDataSet set,          /* The available compensation data */
                                   FwdCoilSet*        coils,        /* The main coil set */
                                   FwdCoilSet*        comp_coils)   /* The compensation coil set */
/*
      * Call mne_make_ctf_comp using the information in the coil sets
      */
{
    fiffChInfo chs     = NULL;
    fiffChInfo compchs = NULL;
    int        nchan   = 0;
    int        ncomp   = 0;
    FwdCoil* coil;
    int k,res;

    if (!coils || coils->ncoil <= 0) {
        printf("Coil data missing in fwd_make_ctf_comp_coils");
        return FAIL;
    }
    /*
   * Create the fake channel info which contain just enough information
   * for mne_make_ctf_comp
   */
    chs = MALLOC(coils->ncoil,fiffChInfoRec);
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        strcpy(chs[k].ch_name,coil->chname);
        chs[k].chpos.coil_type = coil->type;
        chs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
    }
    nchan = coils->ncoil;
    if (comp_coils && comp_coils->ncoil > 0) {
        compchs = MALLOC(comp_coils->ncoil,fiffChInfoRec);
        for (k = 0; k < comp_coils->ncoil; k++) {
            coil = comp_coils->coils[k];
            strcpy(compchs[k].ch_name,coil->chname);
            compchs[k].chpos.coil_type = coil->type;
            compchs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
        }
        ncomp = comp_coils->ncoil;
    }
    res = mne_make_ctf_comp(set,chs,nchan,compchs,ncomp);

    FREE(chs);
    FREE(compchs);

    return res;
}



fwdCompData fwd_make_comp_data(mneCTFcompDataSet set,           /* The CTF compensation data read from the file */
                               FwdCoilSet*        coils,         /* The principal set of coils */
                               FwdCoilSet*        comp_coils,    /* The compensation coils */
                               fwdFieldFunc      field,	        /* The field computation functions */
                               fwdVecFieldFunc   vec_field,
                               fwdFieldGradFunc  field_grad,    /* The field and gradient computation function */
                               void              *client,       /* Client data to be passed to the above */
                               fwdUserFreeFunc   client_free)
/*
      * Compose a compensation data set
      */
{
    fwdCompData comp = fwd_new_comp_data();

    comp->set = mne_dup_ctf_comp_data_set(set);

    if (comp_coils) {
        comp->comp_coils = comp_coils->dup_coil_set(NULL);
    }
    else {
        qWarning("No coils to duplicate");
        comp->comp_coils = NULL;
    }
    comp->field       = field;
    comp->vec_field   = vec_field;
    comp->field_grad  = field_grad;
    comp->client      = client;
    comp->client_free = client_free;

    if (fwd_make_ctf_comp_coils(comp->set,
                                coils,
                                comp->comp_coils) != OK) {
        fwd_free_comp_data(comp);
        return NULL;
    }
    else {
        return comp;
    }
}



int fwd_comp_field_vec(float *rd, FwdCoilSet* coils, float **res, void *client)
/*
      * Calculate the compensated field (all dipole components)
      */
{
    fwdCompData comp = (fwdCompData)client;
    int k;

    if (!comp->vec_field) {
        printf("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
   * First compute the field in the primary set of coils
   */
    if (comp->vec_field(rd,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
   * Compensation needed?
   */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
   * Need workspace?
   */
    if (!comp->vec_work)
        comp->vec_work = ALLOC_CMATRIX(3,comp->comp_coils->ncoil);
    /*
   * Compute the field at the compensation sensors
   */
    if (comp->vec_field(rd,comp->comp_coils,comp->vec_work,comp->client) == FAIL)
        return FAIL;
    /*
   * Compute the compensated field of three orthogonal dipoles
   */
    for (k = 0; k < 3; k++) {
        if (mne_apply_ctf_comp(comp->set,TRUE,res[k],coils->ncoil,comp->vec_work[k],comp->comp_coils->ncoil) == FAIL)
            return FAIL;
    }
    return OK;
}



//============================= fwd_fit_berg_scherg.c =============================





//============================= fwd_eeg_sphere_models.c =============================
















#define SEP ":\n\r"

#ifndef R_OK
#define R_OK    4       /* Test for read permission.  */
#endif

#ifndef W_OK
#define W_OK    2       /* Test for write permission.  */
#endif
//#define   X_OK    1       /* execute permission - unsupported in windows*/
#ifndef F_OK
#define F_OK    0       /* Test for existence.  */
#endif













//============================= fwd_spherefield.c =============================

#define EPS   1e-5		/* Points closer to origin than this many
    meters are considered to be at the
    origin */
#define CEPS       1e-5




int fwd_sphere_field(float        *rd,	        /* The dipole location */
                     float        Q[],	        /* The dipole components (xyz) */
                     FwdCoilSet*   coils,	/* The coil definitions */
                     float        Bval[],	/* Results */
                     void         *client)	/* Client data will be the sphere model origin */

{
    /* This version uses Jukka Sarvas' field computation
     for details, see

     Jukka Sarvas:

     Basic mathematical and electromagnetic concepts
     of the biomagnetic inverse problem,

     Phys. Med. Biol. 1987, Vol. 32, 1, 11-22

     The formulas have been manipulated for efficient computation
     by Matti Hamalainen, February 1990

  */
    float *r0 = (float *)client;      /* The sphere model origin */
    float v[3],a_vec[3];
    float a,a2,r,r2;
    float ar,ar0,rr0;
    float vr,ve,re,r0e;
    float F,g0,gr,result,sum;
    int   j,k,p;
    FwdCoil* this_coil;
    float *this_pos,*this_dir;	/* These point to the coil structure! */
    int   np;
    float myrd[3];
    float pos[3];
    /*
   * Shift to the sphere model coordinates
   */
    for (p = 0; p < 3; p++)
        myrd[p] = rd[p] - r0[p];
    rd = myrd;
    /*
   * Check for a dipole at the origin
   */
    for (k = 0 ; k < coils->ncoil ; k++)
        if (FWD_IS_MEG_COIL(coils->coils[k]->coil_class))
            Bval[k] = 0.0;
    r = VEC_LEN(rd);
    if (r > EPS)	{		/* The hard job */

        CROSS_PRODUCT(Q,rd,v);

        for (k = 0; k < coils->ncoil; k++) {
            this_coil = coils->coils[k];
            if (FWD_IS_MEG_COIL(this_coil->type)) {

                np = this_coil->np;

                for (j = 0, sum = 0.0; j < np; j++) {

                    this_pos = this_coil->rmag[j];
                    this_dir = this_coil->cosmag[j];

                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;
                    result = 0.0;

                    /* Vector from dipole to the field point */

                    VEC_DIFF (rd,this_pos,a_vec);

                    /* Compute the dot products needed */

                    a2  = VEC_DOT(a_vec,a_vec);       a = sqrt(a2);

                    if (a > 0.0) {
                        r2  = VEC_DOT(this_pos,this_pos); r = sqrt(r2);
                        if (r > 0.0) {
                            rr0 = VEC_DOT(this_pos,rd);
                            ar = (r2-rr0);
                            if (fabs(ar/(a*r)+1.0) > CEPS) { /* There is a problem on the negative 'z' axis if the dipole location
                                                * and the field point are on the same line */
                                ar0  = ar/a;

                                ve = VEC_DOT(v,this_dir); vr = VEC_DOT(v,this_pos);
                                re = VEC_DOT(this_pos,this_dir); r0e = VEC_DOT(rd,this_dir);

                                /* The main ingredients */

                                F  = a*(r*a + ar);
                                gr = a2/r + ar0 + 2.0*(a+r);
                                g0 = a + 2*r + ar0;

                                /* Mix them together... */

                                sum = sum + this_coil->w[j]*(ve*F + vr*(g0*r0e - gr*re))/(F*F);
                            }
                        }
                    }
                }				/* All points done */
                Bval[k] = MAG_FACTOR*sum;
            }
        }
    }
    return OK;			/* Happy conclusion: this works always */
}


int fwd_sphere_field_vec(float        *rd,	/* The dipole location */
                         FwdCoilSet*   coils,	/* The coil definitions */
                         float        **Bval,  /* Results: rows are the fields of the x,y, and z direction dipoles */
                         void         *client)	/* Client data will be the sphere model origin */

{
    /* This version uses Jukka Sarvas' field computation
     for details, see

     Jukka Sarvas:

     Basic mathematical and electromagnetic concepts
     of the biomagnetic inverse problem,

     Phys. Med. Biol. 1987, Vol. 32, 1, 11-22

     The formulas have been manipulated for efficient computation
     by Matti Hamalainen, February 1990

     The idea of matrix kernels is from

     Mosher, Leahy, and Lewis: EEG and MEG: Forward Solutions for Inverse Methods

     which has been simplified here using standard vector notation

  */
    float *r0 = (float *)client;      /* The sphere model origin */
    float a_vec[3],v1[3],v2[3];
    float a,a2,r,r2;
    float ar,ar0,rr0;
    float re,r0e;
    float F,g0,gr,g,sum[3];
    int   j,k,p;
    FwdCoil* this_coil;
    float *this_pos,*this_dir;	/* These point to the coil structure! */
    int   np;
    float myrd[3];
    float pos[3];
    /*
   * Shift to the sphere model coordinates
   */
    for (p = 0; p < 3; p++)
        myrd[p] = rd[p] - r0[p];
    rd = myrd;
    /*
   * Check for a dipole at the origin
   */
    r = VEC_LEN(rd);
    for (k = 0; k < coils->ncoil; k++) {
        this_coil = coils->coils[k];
        if (FWD_IS_MEG_COIL(this_coil->coil_class)) {
            if (r < EPS) {
                Bval[0][k] = Bval[1][k] = Bval[2][k] = 0.0;
            }
            else { 	/* The hard job */

                np = this_coil->np;
                sum[0] = sum[1] = sum[2] = 0.0;

                for (j = 0; j < np; j++) {

                    this_pos = this_coil->rmag[j];
                    this_dir = this_coil->cosmag[j];

                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;

                    /* Vector from dipole to the field point */

                    VEC_DIFF (rd,this_pos,a_vec);

                    /* Compute the dot products needed */

                    a2  = VEC_DOT(a_vec,a_vec);       a = sqrt(a2);

                    if (a > 0.0) {
                        r2  = VEC_DOT(this_pos,this_pos); r = sqrt(r2);
                        if (r > 0.0) {
                            rr0 = VEC_DOT(this_pos,rd);
                            ar = (r2-rr0);
                            if (fabs(ar/(a*r)+1.0) > CEPS) { /* There is a problem on the negative 'z' axis if the dipole location
                                                * and the field point are on the same line */

                                /* The main ingredients */

                                ar0  = ar/a;
                                F  = a*(r*a + ar);
                                gr = a2/r + ar0 + 2.0*(a+r);
                                g0 = a + 2*r + ar0;

                                re = VEC_DOT(this_pos,this_dir); r0e = VEC_DOT(rd,this_dir);
                                CROSS_PRODUCT(rd,this_dir,v1);
                                CROSS_PRODUCT(rd,this_pos,v2);

                                g = (g0*r0e - gr*re)/(F*F);
                                /*
                 * Mix them together...
                 */
                                for (p = 0; p < 3; p++)
                                    sum[p] = sum[p] + this_coil->w[j]*(v1[p]/F + v2[p]*g);
                            }
                        }
                    }
                }				/* All points done */
                for (p = 0; p < 3; p++)
                    Bval[p][k] = MAG_FACTOR*sum[p];
            }
        }
    }
    return OK;			/* Happy conclusion: this works always */
}


//============================= mne_simplex_fit.c =============================

/*
 * This routine comes from Numerical recipes
 */

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0

static float tryit (float **p,
                    float *y,
                    float *psum,
                    int   ndim,
                    float (*func)(float *x,int npar,void *user_data),	  /* The function to be evaluated */
                    void  *user_data,				          /* Data to be passed to the above function in each evaluation */
                    int   ihi,
                    int   *neval,
                    float fac)

{
    int j;
    float fac1,fac2,ytry,*ptry;

    ptry = ALLOC_FLOAT(ndim);
    fac1 = (1.0-fac)/ndim;
    fac2 = fac1-fac;
    for (j = 0; j < ndim; j++)
        ptry[j] = psum[j]*fac1-p[ihi][j]*fac2;
    ytry = (*func)(ptry,ndim,user_data);
    ++(*neval);
    if (ytry < y[ihi]) {
        y[ihi] = ytry;
        for (j = 0; j < ndim; j++) {
            psum[j] +=  ptry[j]-p[ihi][j];
            p[ihi][j] = ptry[j];
        }
    }
    FREE(ptry);
    return ytry;
}

int mne_simplex_minimize(float **p,		                              /* The initial simplex */
                         float *y,		                              /* Function values at the vertices */
                         int   ndim,	                                      /* Number of variables */
                         float ftol,	                                      /* Relative convergence tolerance */
                         float (*func)(float *x,int npar,void *user_data),    /* The function to be evaluated */
                         void  *user_data,				      /* Data to be passed to the above function in each evaluation */
                         int   max_eval,	                              /* Maximum number of function evaluations */
                         int   *neval,	                                      /* Number of function evaluations */
                         int   report,                                        /* How often to report (-1 = no_reporting) */
                         int   (*report_func)(int loop,
                                              float *fitpar, int npar,
                                              double fval))                   /* The function to be called when reporting */

/*
      * Minimization with the simplex algorithm
      * Modified from Numerical recipes
      */

{
    int   i,j,ilo,ihi,inhi;
    int   mpts = ndim+1;
    float ytry,ysave,sum,rtol,*psum;
    int   result = 0;
    int   count = 0;
    int   loop  = 1;

    psum = ALLOC_FLOAT(ndim);
    *neval = 0;
    for (j = 0; j < ndim; j++) {
        for (i = 0,sum = 0.0; i<mpts; i++)
            sum +=  p[i][j];
        psum[j] = sum;
    }
    if (report_func != NULL && report > 0)
        (void)report_func (0,p[0],ndim,-1.0);

    for (;;count++,loop++) {
        ilo = 1;
        ihi  =  y[1]>y[2] ? (inhi = 2,1) : (inhi = 1,2);
        for (i = 0; i < mpts; i++) {
            if (y[i]  <  y[ilo]) ilo = i;
            if (y[i] > y[ihi]) {
                inhi = ihi;
                ihi = i;
            } else if (y[i] > y[inhi])
                if (i !=  ihi) inhi = i;
        }
        rtol = 2.0*fabs(y[ihi]-y[ilo])/(fabs(y[ihi])+fabs(y[ilo]));
        /*
     * Report that we are proceeding...
     */
        if (count == report && report_func != NULL) {
            if (report_func (loop,p[ilo],ndim,y[ilo])) {
                qWarning("Interation interrupted.");
                result = -1;
                break;
            }
            count = 0;
        }
        if (rtol < ftol) break;
        if (*neval >=  max_eval) {
            qWarning("Maximum number of evaluations exceeded.");
            result  =  -1;
            break;
        }
        ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,BETA);
            if (ytry >= ysave) {
                for (i = 0; i < mpts; i++) {
                    if (i !=  ilo) {
                        for (j = 0; j < ndim; j++) {
                            psum[j] = 0.5*(p[i][j]+p[ilo][j]);
                            p[i][j] = psum[j];
                        }
                        y[i] = (*func)(psum,ndim,user_data);
                    }
                }
                *neval +=  ndim;
                for (j = 0; j < ndim; j++) {
                    for (i = 0,sum = 0.0; i < mpts; i++)
                        sum +=  p[i][j];
                    psum[j] = sum;
                }
            }
        }
    }
    FREE (psum);
    return (result);
}

#undef ALPHA
#undef BETA
#undef GAMMA



//============================= fit_sphere.c =============================


typedef struct {
    float **rr;
    int   np;
    int   report;
} *fitSphereUser,fitSphereUserRec;

static int report_func(int     loop,
                       float   *fitpar,
                       int     npar,
                       double  fval)
/*
      * Report periodically
      */
{
    float *r0 = fitpar;

    fprintf(stderr,"loop %d r0 %7.1f %7.1f %7.1f fval %g\n",
            loop,1000*r0[0],1000*r0[1],1000*r0[2],fval);

    return OK;
}

static float fit_sphere_eval(float *fitpar,
                             int   npar,
                             void  *user_data)
/*
      * Calculate the cost function value
      * Optimize for the radius inside here
      */
{
    fitSphereUser user = (fitSphereUser)user_data;
    float *r0 = fitpar;
    float diff[3];
    int   k;
    float sum,sum2,one,F;

    for (k = 0, sum = sum2 = 0.0; k < user->np; k++) {
        VEC_DIFF(r0,user->rr[k],diff);
        one = VEC_LEN(diff);
        sum  += one;
        sum2 += one*one;
    }
    F = sum2 - sum*sum/user->np;

    if (user->report)
        fprintf(stderr,"r0 %7.1f %7.1f %7.1f R %7.1f fval %g\n",
                1000*r0[0],1000*r0[1],1000*r0[2],1000*sum/user->np,F);

    return F;
}

static float opt_rad(float *r0,fitSphereUser user)

{
    float sum, diff[3], one;
    int   k;

    for (k = 0, sum = 0.0; k < user->np; k++) {
        VEC_DIFF(r0,user->rr[k],diff);
        one = VEC_LEN(diff);
        sum  += one;
    }
    return sum/user->np;
}


static void calculate_cm_ave_dist(float **rr, int np, float *cm, float *avep)

{
    int k,q;
    float ave,diff[3];

    for (q = 0; q < 3; q++)
        cm[q] = 0.0;

    for (k = 0; k < np; k++)
        for (q = 0; q < 3; q++)
            cm[q] += rr[k][q];

    if (np > 0) {
        for (q = 0; q < 3; q++)
            cm[q] = cm[q]/np;

        for (k = 0, ave = 0.0; k < np; k++) {
            for (q = 0; q < 3; q++)
                diff[q] = rr[k][q] - cm[q];
            ave += VEC_LEN(diff);
        }
        *avep = ave/np;
    }
    return;
}

static float **make_initial_simplex(float  *pars,
                                    int    npar,
                                    float  size)
/*
      * Make the initial tetrahedron
      */
{
    float **simplex = ALLOC_CMATRIX(npar+1,npar);
    int k;

    for (k = 0; k < npar+1; k++)
        memcpy (simplex[k],pars,npar*sizeof(float));

    for (k = 1; k < npar+1; k++)
        simplex[k][k-1] = simplex[k][k-1] + size;
    return (simplex);
}


int fit_sphere_to_points(float **rr,
                         int   np,
                         float simplex_size,
                         float *r0,
                         float *R)
/*
      * Find the optimal sphere origin
      */
{
    fitSphereUserRec user;
    float      ftol            = 1e-5;
    int        max_eval        = 500;
    int        report_interval = -1;
    int        neval = 0;
    float      **init_simplex  = NULL;
    float      *init_vals      = NULL;

    float      cm[3],R0;
    int        k;

    int        res = FAIL;

    user.rr = rr;
    user.np = np;

    calculate_cm_ave_dist(rr,np,cm,&R0);

#ifdef DEBUG
    fprintf(stderr,"cm %7.1f %7.1f %7.1f R %7.1f\n",
            1000*cm[0],1000*cm[1],1000*cm[2],1000*R0);
#endif

    init_simplex = make_initial_simplex(cm,3,simplex_size);

    init_vals = MALLOC(4,float);

#ifdef DEBUG
    user.report = TRUE;
#else
    user.report = FALSE;
#endif

    for (k = 0; k < 4; k++)
        init_vals[k] = fit_sphere_eval(init_simplex[k],3,&user);

    if (mne_simplex_minimize(init_simplex,		                  /* The initial simplex */
                             init_vals,		                          /* Function values at the vertices */
                             3,    	                                  /* Number of variables */
                             ftol,	                                  /* Relative convergence tolerance */
                             fit_sphere_eval,                                      /* The function to be evaluated */
                             &user,	  			          /* Data to be passed to the above function in each evaluation */
                             max_eval,	                                  /* Maximum number of function evaluations */
                             &neval,	                                  /* Number of function evaluations */
                             report_interval,	                          /* How often to report (-1 = no_reporting) */
                             report_func) != OK)                            /* The function to be called when reporting */
        goto out;

    r0[X] = init_simplex[0][X];
    r0[Y] = init_simplex[0][Y];
    r0[Z] = init_simplex[0][Z];
    *R    = opt_rad(r0,&user);

    res = OK;
    goto out;

out : {
        FREE(init_vals);
        FREE_CMATRIX(init_simplex);
        return res;
    }
}




//============================= fwd_mag_dipole_field.c =============================


#define MAG_FACTOR 1e-7
#define EPS 1e-5

#ifndef OK
#define OK 0
#endif

/*
 * Compute the field of a magnetic dipole
 */


int fwd_mag_dipole_field(float        *rm,      /* The dipole location in the same coordinate system as the coils */
                         float        M[],	/* The dipole components (xyz) */
                         FwdCoilSet*   coils,	/* The coil definitions */
                         float        Bval[],	/* Results */
                         void         *client)	/* Client data will be the sphere model origin */
/*
 * This is for a specific dipole component
 */
{
    int     j,k,np;
    FwdCoil* this_coil;
    float   sum,diff[3],dist,dist2,dist5,*dir;


    for (k = 0; k < coils->ncoil; k++) {
        this_coil = coils->coils[k];
        if (FWD_IS_MEG_COIL(this_coil->type)) {
            np = this_coil->np;
            /*
       * Go through all points
       */
            for (j = 0, sum = 0.0; j < np; j++) {
                dir = this_coil->cosmag[j];
                VEC_DIFF(rm,this_coil->rmag[j],diff);
                dist = VEC_LEN(diff);
                if (dist > EPS) {
                    dist2 = dist*dist;
                    dist5 = dist2*dist2*dist;
                    sum = sum + this_coil->w[j]*(3*VEC_DOT(M,diff)*VEC_DOT(diff,dir) - dist2*VEC_DOT(M,dir))/dist5;
                }
            }				/* All points done */
            Bval[k] = MAG_FACTOR*sum;
        }
        else if (this_coil->type == FWD_COILC_EEG)
            Bval[k] = 0.0;
    }
    return OK;
}

int fwd_mag_dipole_field_vec(float        *rm,	        /* The dipole location */
                             FwdCoilSet*   coils,	/* The coil definitions */
                             float        **Bval,       /* Results: rows are the fields of the x,y, and z direction dipoles */
                             void         *client)	/* Client data will be the sphere model origin */
/*
 * This is for all dipole components
 * For EEG this produces a zero result
 */
{
    int     j,k,p,np;
    FwdCoil* this_coil;
    float   sum[3],diff[3],dist,dist2,dist5,*dir;


    for (k = 0; k < coils->ncoil; k++) {
        this_coil = coils->coils[k];
        if (FWD_IS_MEG_COIL(this_coil->type)) {
            np = this_coil->np;
            sum[0] = sum[1] = sum[2] = 0.0;
            /*
       * Go through all points
       */
            for (j = 0; j < np; j++) {
                dir = this_coil->cosmag[j];
                VEC_DIFF(rm,this_coil->rmag[j],diff);
                dist = VEC_LEN(diff);
                if (dist > EPS) {
                    dist2 = dist*dist;
                    dist5 = dist2*dist2*dist;
                    for (p = 0; p < 3; p++)
                        sum[p] = sum[p] + this_coil->w[j]*(3*diff[p]*VEC_DOT(diff,dir) - dist2*dir[p])/dist5;
                }
            }				/* All points done */
            for (p = 0; p < 3; p++)
                Bval[p][k] = MAG_FACTOR*sum[p];
        }
        else if (this_coil->type == FWD_COILC_EEG) {
            for (p = 0; p < 3; p++)
                Bval[p][k] = 0.0;
        }
    }
    return OK;
}




//============================= dipole_fit_setup.c =============================


static dipoleFitFuncs new_dipole_fit_funcs()

{
    dipoleFitFuncs f = MALLOC(1,dipoleFitFuncsRec);

    f->meg_field     = NULL;
    f->eeg_pot       = NULL;
    f->meg_vec_field = NULL;
    f->eeg_vec_pot   = NULL;
    f->meg_client      = NULL;
    f->meg_client_free = NULL;
    f->eeg_client      = NULL;
    f->eeg_client_free = NULL;

    return f;
}

static void free_dipole_fit_funcs(dipoleFitFuncs f)

{
    if (!f)
        return;

    if (f->meg_client_free && f->meg_client)
        f->meg_client_free(f->meg_client);
    if (f->eeg_client_free && f->eeg_client)
        f->eeg_client_free(f->eeg_client);

    FREE(f);
    return;
}


DipoleFitData* new_dipole_fit_data()

{
    DipoleFitData* res = MALLOC(1,DipoleFitData);

    res->mri_head_t    = NULL;
    res->meg_head_t    = NULL;
    res->chs           = NULL;
    res->meg_coils     = NULL;
    res->eeg_els       = NULL;
    res->nmeg          = 0;
    res->neeg          = 0;
    res->ch_names      = NULL;
    res->pick          = NULL;
    res->r0[0]         = 0.0;
    res->r0[1]         = 0.0;
    res->r0[2]         = 0.0;
    res->bemname       = NULL;
    res->bem_model     = NULL;
    res->eeg_model     = NULL;
    res->fixed_noise   = FALSE;
    res->noise         = NULL;
    res->noise_orig    = NULL;
    res->nave          = 1;
    res->user          = NULL;
    res->user_free     = NULL;
    res->proj          = NULL;

    res->sphere_funcs     = NULL;
    res->bem_funcs        = NULL;
    res->mag_dipole_funcs = NULL;
    res->funcs            = NULL;
    res->column_norm      = COLUMN_NORM_NONE;
    res->fit_mag_dipoles  = FALSE;

    return res;
}


void free_dipole_fit_data(DipoleFitData* d)

{
    if (!d)
        return;

    FREE(d->mri_head_t);
    FREE(d->meg_head_t);
    FREE(d->chs);
    delete d->meg_coils;
    delete d->eeg_els;
    FREE(d->bemname);
    mne_free_cov(d->noise);
    mne_free_cov(d->noise_orig);
    mne_free_name_list(d->ch_names,d->nmeg+d->neeg);
    if(d->pick)
        d->pick;
    fwd_bem_free_model(d->bem_model);
    delete d->eeg_model;
    if (d->user_free)
        d->user_free(d->user);

    mne_free_proj_op(d->proj);

    free_dipole_fit_funcs(d->sphere_funcs);
    free_dipole_fit_funcs(d->bem_funcs);
    free_dipole_fit_funcs(d->mag_dipole_funcs);

    FREE(d);
    return;
}


static int setup_forward_model(DipoleFitData* d, mneCTFcompDataSet comp_data, FwdCoilSet* comp_coils)
/*
 * Take care of some hairy details
 */
{
    fwdCompData comp;
    dipoleFitFuncs f;
    int fit_sphere_to_bem = TRUE;

    if (d->bemname) {
        /*
     * Set up the boundary-element model
     */
        char *bemsolname = fwd_bem_make_bem_sol_name(d->bemname);
        FREE(d->bemname); d->bemname = bemsolname;

        printf("\nSetting up the BEM model using %s...\n",d->bemname);
        printf("\nLoading surfaces...\n");
        d->bem_model = fwd_bem_load_three_layer_surfaces(d->bemname);
        if (d->bem_model) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            d->bem_model = fwd_bem_load_homog_surface(d->bemname);
            if (!d->bem_model)
                goto out;
            printf("Homogeneous model surface loaded.\n");
        }
        if (d->neeg > 0 && d->bem_model->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            goto out;
        }
        printf("\nLoading the solution matrix...\n");
        if (fwd_bem_load_recompute_solution(d->bemname,FWD_BEM_UNKNOWN,FALSE,d->bem_model) == FAIL)
            goto out;
        printf("Employing the head->MRI coordinate transform with the BEM model.\n");
        if (fwd_bem_set_head_mri_t(d->bem_model,d->mri_head_t) == FAIL)
            goto out;
        printf("BEM model %s is now set up\n",d->bem_model->sol_name);
        /*
     * Find the best-fitting sphere
     */
        if (fit_sphere_to_bem) {
            MneSurfaceOrVolume::MneCSurface* inner_skull;
            float      simplex_size = 2e-2;
            float      R;

            if ((inner_skull = fwd_bem_find_surface(d->bem_model,FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
                goto out;

            if (fit_sphere_to_points(inner_skull->rr,inner_skull->np,simplex_size,d->r0,&R) == FAIL)
                goto out;

            fiff_coord_trans(d->r0,d->mri_head_t,TRUE);
            printf("Fitted sphere model origin : %6.1f %6.1f %6.1f mm rad = %6.1f mm.\n",
                   1000*d->r0[X],1000*d->r0[Y],1000*d->r0[Z],1000*R);
        }
        d->bem_funcs = f = new_dipole_fit_funcs();
        if (d->nmeg > 0) {
            /*
       * Use the new compensated field computation
       * It works the same way independent of whether or not the compensation is in effect
       */
            comp = fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                      fwd_bem_field,NULL,NULL,d->bem_model,NULL);
            if (!comp)
                goto out;
            printf("Compensation setup done.\n");

            printf("MEG solution matrix...");
            if (fwd_bem_specify_coils(d->bem_model,d->meg_coils) == FAIL)
                goto out;
            if (fwd_bem_specify_coils(d->bem_model,comp->comp_coils) == FAIL)
                goto out;
            printf("[done]\n");

            f->meg_field       = fwd_comp_field;
            f->meg_vec_field   = NULL;
            f->meg_client      = comp;
            f->meg_client_free = fwd_free_comp_data;
        }
        if (d->neeg > 0) {
            printf("\tEEG solution matrix...");
            if (fwd_bem_specify_els(d->bem_model,d->eeg_els) == FAIL)
                goto out;
            printf("[done]\n");
            f->eeg_pot     = fwd_bem_pot_els;
            f->eeg_vec_pot = NULL;
            f->eeg_client  = d->bem_model;
        }
    }
    if (d->neeg > 0 && !d->eeg_model) {
        qCritical("EEG sphere model not defined.");
        goto out;
    }
    d->sphere_funcs = f = new_dipole_fit_funcs();
    if (d->neeg > 0) {
        VEC_COPY(d->eeg_model->r0,d->r0);
        f->eeg_pot     = FwdEegSphereModel::fwd_eeg_spherepot_coil;
        f->eeg_vec_pot = FwdEegSphereModel::fwd_eeg_spherepot_coil_vec;
        f->eeg_client  = d->eeg_model;
    }
    if (d->nmeg > 0) {
        /*
     * Use the new compensated field computation
     * It works the same way independent of whether or not the compensation is in effect
     */
        comp = fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                  fwd_sphere_field,
                                  fwd_sphere_field_vec,
                                  NULL,
                                  d->r0,NULL);
        if (!comp)
            goto out;
        f->meg_field       = fwd_comp_field;
        f->meg_vec_field   = fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = fwd_free_comp_data;
    }
    printf("Sphere model origin : %6.1f %6.1f %6.1f mm.\n",
           1000*d->r0[X],1000*d->r0[Y],1000*d->r0[Z]);
    /*
   * Finally add the magnetic dipole fitting functions (for special purposes)
   */
    d->mag_dipole_funcs = f = new_dipole_fit_funcs();
    if (d->nmeg > 0) {
        /*
     * Use the new compensated field computation
     * It works the same way independent of whether or not the compensation is in effect
     */
        comp = fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                  fwd_mag_dipole_field,
                                  fwd_mag_dipole_field_vec,
                                  NULL,
                                  NULL,NULL);
        if (!comp)
            goto out;
        f->meg_field       = fwd_comp_field;
        f->meg_vec_field   = fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = fwd_free_comp_data;
    }
    f->eeg_pot     = fwd_mag_dipole_field;
    f->eeg_vec_pot = fwd_mag_dipole_field_vec;
    /*
    * Select the appropriate fitting function
    */
    d->funcs = d->bemname ? d->bem_funcs : d->sphere_funcs;

    fprintf (stderr,"\n");
    return OK;

out :
    return FAIL;
}



static mneCovMatrix ad_hoc_noise(FwdCoilSet* meg,          /* Channel name lists to define which channels are gradiometers */
                                 FwdCoilSet* eeg,
                                 float      grad_std,
                                 float      mag_std,
                                 float      eeg_std)
/*
 * Specify constant noise values
 */
{
    int    nchan;
    double *stds;
    char  **names,**ch_names;
    int   k,n;

    printf("Using standard noise values "
           "(MEG grad : %6.1f fT/cm MEG mag : %6.1f fT EEG : %6.1f uV)\n",
           1e13*grad_std,1e15*mag_std,1e6*eeg_std);

    nchan = 0;
    if (meg)
        nchan = nchan + meg->ncoil;
    if (eeg)
        nchan = nchan + eeg->ncoil;

    stds = MALLOC(nchan,double);
    ch_names = MALLOC(nchan,char *);

    n = 0;
    if (meg) {
        for (k = 0; k < meg->ncoil; k++, n++) {
            if (meg->coils[k]->is_axial_coil()) {
                stds[n] = mag_std*mag_std;
#ifdef TEST_REF
                if (meg->coils[k]->type == FIFFV_COIL_CTF_REF_MAG ||
                        meg->coils[k]->type == FIFFV_COIL_CTF_REF_GRAD ||
                        meg->coils[k]->type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD)
                    stds[n] = 1e6*stds[n];
#endif
            }
            else
                stds[n] = grad_std*grad_std;
            ch_names[n] = meg->coils[k]->chname;
        }
    }
    if (eeg) {
        for (k = 0; k < eeg->ncoil; k++, n++) {
            stds[n]     = eeg_std*eeg_std;
            ch_names[n] = eeg->coils[k]->chname;
        }
    }
    names = mne_dup_name_list(ch_names,nchan);
    FREE(ch_names);
    return mne_new_cov(FIFFV_MNE_NOISE_COV,nchan,names,NULL,stds);
}

static int make_projection(const QList<QString>& projnames,
                           fiffChInfo chs,
                           int        nch,
                           mneProjOp  *res)
/*
      * Process the projection data
      */
{
    mneProjOp all  = NULL;
    mneProjOp one  = NULL;
    int       k,found;
    int       neeg;

    for (k = 0, neeg = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;

    if (projnames.size() == 0 && neeg == 0)
        return OK;

    for (k = 0; k < projnames.size(); k++) {
        if ((one = mne_read_proj_op(projnames[k])) == NULL)
            goto bad;
        if (one->nitems == 0) {
            printf("No linear projection information in %s.\n",projnames[k].toLatin1().data());
            mne_free_proj_op(one); one = NULL;
        }
        else {
            printf("Loaded projection from %s:\n",projnames[k].toLatin1().data());
            mne_proj_op_report(stderr,"\t",one);
            all = mne_proj_op_combine(all,one);
            mne_free_proj_op(one); one = NULL;
        }
    }

    if (neeg > 0) {
        found = FALSE;
        if (all) {
            for (k = 0; k < all->nitems; k++)
                if (all->items[k]->kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
                    found = TRUE;
                    break;
                }
        }
        if (!found) {
            if ((one = mne_proj_op_average_eeg_ref(chs,nch)) != NULL) {
                printf("Average EEG reference projection added:\n");
                mne_proj_op_report(stderr,"\t",one);
                all = mne_proj_op_combine(all,one);
                mne_free_proj_op(one); one = NULL;
            }
        }
    }
    if (all && mne_proj_op_affect_chs(all,chs,nch) == 0) {
        printf("Projection will not have any effect on selected channels. Projection omitted.\n");
        mne_free_proj_op(all);
        all = NULL;
    }
    *res = all;
    return OK;

bad :
    return FAIL;
}






int scale_noise_cov(DipoleFitData* f,int nave)

{
    float nave_ratio = ((float)f->nave)/(float)nave;
    int   k;

    if (!f->noise)
        return OK;

    if (f->noise->cov != NULL) {
        fprintf(stderr,"Decomposing the sensor noise covariance matrix...\n");
        if (mne_decompose_eigen_cov(f->noise) == FAIL)
            goto bad;

        for (k = 0; k < f->noise->ncov*(f->noise->ncov+1)/2; k++)
            f->noise->cov[k] = nave_ratio*f->noise->cov[k];
        for (k = 0; k < f->noise->ncov; k++) {
            f->noise->lambda[k] = nave_ratio*f->noise->lambda[k];
            if (f->noise->lambda[k] < 0.0)
                f->noise->lambda[k] = 0.0;
        }
        if (mne_add_inv_cov(f->noise) == FAIL)
            goto bad;
    }
    else {
        for (k = 0; k < f->noise->ncov; k++)
            f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
        fprintf(stderr,"Decomposition not needed for a diagonal noise covariance matrix.\n");
        if (mne_add_inv_cov(f->noise) == FAIL)
            goto bad;
    }
    fprintf(stderr,"Effective nave is now %d\n",nave);
    f->nave = nave;
    return OK;

bad :
    return FAIL;
}


static void regularize_cov(mneCovMatrix c,       /* The matrix to regularize */
                           float        *regs,   /* Regularization values to apply (fractions of the
                                                     * average diagonal values for each class */
                           int          *active) /* Which of the channels are 'active' */
/*
      * Regularize different parts of the noise covariance matrix differently
      */
{
    int    j;
    float  sums[3],nn[3];
    int    nkind = 3;

    if (!c->cov || !c->ch_class)
        return;

    for (j = 0; j < nkind; j++) {
        sums[j] = 0.0;
        nn[j]   = 0;
    }
    /*
   * Compute the averages over the diagonal elements for each class
   */
    for (j = 0; j < c->ncov; j++) {
        if (c->ch_class[j] >= 0) {
            if (!active || active[j]) {
                sums[c->ch_class[j]] += c->cov[mne_lt_packed_index(j,j)];
                nn[c->ch_class[j]]++;
            }
        }
    }
    printf("Average noise-covariance matrix diagonals:\n");
    for (j = 0; j < nkind; j++) {
        if (nn[j] > 0) {
            sums[j] = sums[j]/nn[j];
            if (j == MNE_COV_CH_MEG_MAG)
                printf("\tMagnetometers       : %-7.2f fT    reg = %-6.2f\n",1e15*sqrt(sums[j]),regs[j]);
            else if (j == MNE_COV_CH_MEG_GRAD)
                printf("\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f\n",1e13*sqrt(sums[j]),regs[j]);
            else
                printf("\tEEG                 : %-7.2f uV    reg = %-6.2f\n",1e6*sqrt(sums[j]),regs[j]);
            sums[j] = regs[j]*sums[j];
        }
    }
    /*
   * Add thee proper amount to the diagonal
   */
    for (j = 0; j < c->ncov; j++)
        if (c->ch_class[j] >= 0)
            c->cov[mne_lt_packed_index(j,j)] += sums[c->ch_class[j]];

    printf("Noise-covariance regularized as requested.\n");
    return;
}



static int scale_dipole_fit_noise_cov(DipoleFitData* f,int nave)

{
    float nave_ratio = ((float)f->nave)/(float)nave;
    int   k;

    if (!f->noise)
        return OK;
    if (f->fixed_noise)
        return OK;

    if (f->noise->cov) {
        /*
     * Do the decomposition and check that the matrix is positive definite
     */
        fprintf(stderr,"Decomposing the noise covariance...");
        if (f->noise->cov) {
            if (mne_decompose_eigen_cov(f->noise) == FAIL)
                goto bad;
            for (k = 0; k < f->noise->ncov; k++) {
                if (f->noise->lambda[k] < 0.0)
                    f->noise->lambda[k] = 0.0;
            }
        }
        for (k = 0; k < f->noise->ncov*(f->noise->ncov+1)/2; k++)
            f->noise->cov[k] = nave_ratio*f->noise->cov[k];
        for (k = 0; k < f->noise->ncov; k++) {
            f->noise->lambda[k] = nave_ratio*f->noise->lambda[k];
            if (f->noise->lambda[k] < 0.0)
                f->noise->lambda[k] = 0.0;
        }
        if (mne_add_inv_cov(f->noise) == FAIL)
            goto bad;
    }
    else {
        for (k = 0; k < f->noise->ncov; k++)
            f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
        fprintf(stderr,"Decomposition not needed for a diagonal noise covariance matrix.\n");
        if (mne_add_inv_cov(f->noise) == FAIL)
            goto bad;
    }
    fprintf(stderr,"Effective nave is now %d\n",nave);
    f->nave = nave;
    return OK;

bad :
    return FAIL;
}



int select_dipole_fit_noise_cov(DipoleFitData* f, mshMegEegData d)
/*
 * Do the channel selection and scale with nave
 */
{
    int   nave,j,k;
    float nonsel_w  = 30;
    int   min_nchan = 20;

    if (!f || !f->noise_orig)
        return OK;
    if (!d)
        nave = 1;
    else {
        if (d->nave < 0)
            nave = d->meas->current->nave;
        else
            nave = d->nave;
    }
    /*
   * Channel selection
   */
    if (d) {
        float  *w    = MALLOC(f->noise_orig->ncov,float);
        int    nomit_meg,nomit_eeg,nmeg,neeg;
        double *val;

        nmeg = neeg = 0;
        nomit_meg = nomit_eeg = 0;
        for (k = 0; k < f->noise_orig->ncov; k++) {
            if (f->noise_orig->ch_class[k] == MNE_COV_CH_EEG)
                neeg++;
            else
                nmeg++;
            if (is_selected_in_data(d,f->noise_orig->names[k]))
                w[k] = 1.0;
            else {
                w[k] = nonsel_w;
                if (f->noise_orig->ch_class[k] == MNE_COV_CH_EEG)
                    nomit_eeg++;
                else
                    nomit_meg++;
            }
        }
        mne_free_cov(f->noise); f->noise = NULL;
        if (nmeg > 0 && nmeg-nomit_meg > 0 && nmeg-nomit_meg < min_nchan) {
            qCritical("Too few MEG channels remaining");
            return FAIL;
        }
        if (neeg > 0 && neeg-nomit_eeg > 0 && neeg-nomit_eeg < min_nchan) {
            qCritical("Too few EEG channels remaining");
            return FAIL;
        }
        f->noise = mne_dup_cov(f->noise_orig);
        if (nomit_meg+nomit_eeg > 0) {
            if (f->noise->cov) {
                for (j = 0; j < f->noise->ncov; j++)
                    for (k = 0; k <= j; k++) {
                        val = f->noise->cov+mne_lt_packed_index(j,k);
                        *val = w[j]*w[k]*(*val);
                    }
            }
            else {
                for (j = 0; j < f->noise->ncov; j++) {
                    val  = f->noise->cov_diag+j;
                    *val = w[j]*w[j]*(*val);
                }
            }
        }
        FREE(w);
    }
    else {
        if (f->noise && f->nave == nave)
            return OK;
        f->noise = mne_dup_cov(f->noise_orig);
    }

    return scale_dipole_fit_noise_cov(f,nave);
}



DipoleFitData* setup_dipole_fit_data(   const QString& mriname,         /**< This gives the MRI/head transform */
                                        const QString& measname,        /**< This gives the MEG/head transform and sensor locations */
                                        char  *bemname,                 /**< BEM model */
                                        Eigen::Vector3f *r0,            /**< Sphere model origin in head coordinates (optional) */
                                        FwdEegSphereModel* eeg_model,   /**< EEG sphere model definition */
                                        int   accurate_coils,           /**< Use accurate coil definitions? */
                                        const QString& badname,         /**< Bad channels list */
                                        const QString& noisename,               /**< Noise covariance matrix */
                                        float grad_std,                 /**< Standard deviations for the ad-hoc noise cov (planar gradiometers) */
                                        float mag_std,                  /**< Ditto for magnetometers */
                                        float eeg_std,                  /**< Ditto for EEG */
                                        float mag_reg,                  /**< Noise-covariance regularization factors */
                                        float grad_reg,
                                        float eeg_reg,
                                        int   diagnoise,                /**< Use only the diagonal elements of the noise-covariance matrix */
                                        const QList<QString>& projnames,/**< SSP file names */
                                        int   include_meg,              /**< Include MEG in the fitting? */
                                        int   include_eeg)              /**< Include EEG in the fitting? */
/*
      * Background work for modelling
      */
{
    DipoleFitData*  res = new_dipole_fit_data();
    int            k;
    char           **badlist = NULL;
    int            nbad      = 0;
    char           **file_bads;
    int            file_nbad;
    int            coord_frame = FIFFV_COORD_HEAD;
    mneCovMatrix cov;
    FwdCoilSet*     templates = NULL;
    mneCTFcompDataSet comp_data  = NULL;
    FwdCoilSet*        comp_coils = NULL;

    /*
   * Read the coordinate transformations
   */
    if (!mriname.isEmpty()) {
        if ((res->mri_head_t = mne_read_mri_transform(mriname)) == NULL)
            goto bad;
    }
    else if (bemname) {
        qWarning("Source of MRI / head transform required for the BEM model is missing");
        goto bad;
    }
    else {
        float move[] = { 0.0, 0.0, 0.0 };
        float rot[3][3] = { { 1.0, 0.0, 0.0 },
                            { 0.0, 1.0, 0.0 },
                            { 0.0, 0.0, 1.0 } };
        res->mri_head_t = fiff_make_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD,rot,move);
    }

    mne_print_coord_transform(stderr,res->mri_head_t);
    if ((res->meg_head_t = mne_read_meas_transform(measname)) == NULL)
        goto bad;
    mne_print_coord_transform(stderr,res->meg_head_t);
    /*
   * Read the bad channel lists
   */
    if (!badname.isEmpty()) {
        if (mne_read_bad_channels(badname,&badlist,&nbad) != OK)
            goto bad;
        printf("%d bad channels read from %s.\n",nbad,badname.toLatin1().data());
    }
    if (mne_read_bad_channel_list(measname,&file_bads,&file_nbad) == OK && file_nbad > 0) {
        if (!badlist)
            nbad = 0;
        badlist = REALLOC(badlist,nbad+file_nbad,char *);
        for (k = 0; k < file_nbad; k++)
            badlist[nbad++] = file_bads[k];
        FREE(file_bads);
        printf("%d bad channels read from the data file.\n",file_nbad);
    }
    printf("%d bad channels total.\n",nbad);
    /*
   * Read the channel information
   */
    if (read_meg_eeg_ch_info(measname,include_meg,include_eeg,badlist,nbad,
                             &res->chs,&res->nmeg,&res->neeg) != OK)
        goto bad;

    if (res->nmeg > 0)
        printf("Will use %3d MEG channels from %s\n",res->nmeg,measname.toLatin1().data());
    if (res->neeg > 0)
        printf("Will use %3d EEG channels from %s\n",res->neeg,measname.toLatin1().data());
    {
        char *s = mne_channel_names_to_string(res->chs,res->nmeg+res->neeg);
        int  n;
        mne_string_to_name_list(s,&res->ch_names,&n);
    }
    /*
   * Make coil definitions
   */
    res->coord_frame = coord_frame;
    if (coord_frame == FIFFV_COORD_HEAD) {
#ifdef USE_SHARE_PATH
        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
#else
        //    const char *path = "setup/mne";
        //    const char *filename = "coil_def.dat";
        //    const char *coilfile = mne_compose_mne_name(path,filename);

        //    QString qPath("/usr/pubsw/packages/mne/stable/share/mne/coil_def.dat");

        QString qPath = QString("./resources/coilDefinitions/coil_def.dat");
        QFile file(qPath);
        if ( !QCoreApplication::startingUp() )
            qPath = QCoreApplication::applicationDirPath() + QString("/resources/coilDefinitions/coil_def.dat");
        else if (!file.exists())
            qPath = "./bin/resources/coilDefinitions/coil_def.dat";

        char *coilfile = MALLOC(strlen(qPath.toLatin1().data())+1,char);
        strcpy(coilfile,qPath.toLatin1().data());
#endif

        if (!coilfile)
            goto bad;
        if ((templates = FwdCoilSet::read_coil_defs(coilfile)) == NULL) {
            FREE(coilfile);
            goto bad;
        }

        if ((res->meg_coils = templates->create_meg_coils(res->chs,res->nmeg,
                                                   accurate_coils ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                   res->meg_head_t)) == NULL)
            goto bad;
        if ((res->eeg_els = FwdCoilSet::create_eeg_els(res->chs+res->nmeg,res->neeg,NULL)) == NULL)
            goto bad;
        printf("Head coordinate coil definitions created.\n");
    }
    else {
        qWarning("Cannot handle computations in %s coordinates",mne_coord_frame_name(coord_frame));
        goto bad;
    }
    /*
   * Forward model setup
   */
    res->bemname   = mne_strdup(bemname);
    if (r0) {
        res->r0[0]     = (*r0)[0];
        res->r0[1]     = (*r0)[1];
        res->r0[2]     = (*r0)[2];
    }
    res->eeg_model = eeg_model;
    /*
   * Compensation data
   */
    if ((comp_data = mne_read_ctf_comp_data(measname)) == NULL)
        goto bad;
    if (comp_data->ncomp > 0) {	/* Compensation channel information may be needed */
        fiffChInfo comp_chs = NULL;
        int        ncomp    = 0;

        printf("%d compensation data sets in %s\n",comp_data->ncomp,measname.toLatin1().data());
        if (mne_read_meg_comp_eeg_ch_info(measname,NULL,0,&comp_chs,&ncomp,NULL,NULL,NULL,NULL) == FAIL)
            goto bad;
        if (ncomp > 0) {
            if ((comp_coils = templates->create_meg_coils(comp_chs,ncomp,
                                                   FWD_COIL_ACCURACY_NORMAL,res->meg_head_t)) == NULL) {
                FREE(comp_chs);
                goto bad;
            }
            printf("%d compensation channels in %s\n",comp_coils->ncoil,measname.toLatin1().data());
        }
        FREE(comp_chs);
    }
    else {			/* Get rid of the empty data set */
        mne_free_ctf_comp_data_set(comp_data);
        comp_data = NULL;
    }
    /*
   * Ready to set up the forward model
   */
    if (setup_forward_model(res,comp_data,comp_coils) == FAIL)
        goto bad;
    res->column_norm = COLUMN_NORM_LOC;
    /*
   * Projection data should go here
   */
    if (make_projection(projnames,res->chs,res->nmeg+res->neeg,&res->proj) == FAIL)
        goto bad;
    if (res->proj && res->proj->nitems > 0) {
        fprintf(stderr,"Final projection operator is:\n");
        mne_proj_op_report(stderr,"\t",res->proj);

        if (mne_proj_op_chs(res->proj,res->ch_names,res->nmeg+res->neeg) == FAIL)
            goto bad;
        if (mne_proj_op_make_proj(res->proj) == FAIL)
            goto bad;
    }
    else
        printf("No projection will be applied to the data.\n");

    /*
    * Noise covariance
    */
    if (!noisename.isEmpty()) {
        if ((cov = mne_read_cov(noisename,FIFFV_MNE_SENSOR_COV)) == NULL)
            goto bad;
        printf("Read a %s noise-covariance matrix from %s\n",
               cov->cov_diag ? "diagonal" : "full", noisename.toLatin1().data());
    }
    else {
        if ((cov = ad_hoc_noise(res->meg_coils,res->eeg_els,grad_std,mag_std,eeg_std)) == NULL)
            goto bad;
    }
    res->noise = mne_pick_chs_cov_omit(cov,res->ch_names,res->nmeg+res->neeg,TRUE,res->chs);
    if (res->noise == NULL) {
        mne_free_cov(cov);
        goto bad;
    }

    printf("Picked appropriate channels from the noise-covariance matrix.\n");
    mne_free_cov(cov);

    /*
   * Apply the projection operator to the noise-covariance matrix
   */
    if (res->proj && res->proj->nitems > 0 && res->proj->nvec > 0) {
        if (mne_proj_op_apply_cov(res->proj,res->noise) == FAIL)
            goto bad;
        printf("Projection applied to the covariance matrix.\n");
    }

    /*
   * Force diagonal noise covariance?
   */
    if (diagnoise) {
        mne_revert_to_diag_cov(res->noise);
        fprintf(stderr,"Using only the main diagonal of the noise-covariance matrix.\n");
    }

    /*
   * Regularize the possibly deficient noise-covariance matrix
   */
    if (res->noise->cov) {
        float regs[3];
        int   do_it;

        regs[MNE_COV_CH_MEG_MAG]  = mag_reg;
        regs[MNE_COV_CH_MEG_GRAD] = grad_reg;
        regs[MNE_COV_CH_EEG]      = eeg_reg;
        /*
     * Classify the channels
     */
        if (mne_classify_channels_cov(res->noise,res->chs,res->nmeg+res->neeg) == FAIL)
            goto bad;
        /*
     * Do we need to do anything?
     */
        for (k = 0, do_it = 0; k < res->noise->ncov; k++) {
            if (res->noise->ch_class[k] != MNE_COV_CH_UNKNOWN &&
                    regs[res->noise->ch_class[k]] > 0.0)
                do_it++;
        }
        /*
     * Apply regularization if necessary
     */
        if (do_it > 0)
            mne_regularize_cov(res->noise,regs);
        else
            printf("No regularization applied to the noise-covariance matrix\n");
    }

    /*
   * Do the decomposition and check that the matrix is positive definite
   */
    fprintf(stderr,"Decomposing the noise covariance...\n");
    if (res->noise->cov) {
        if (mne_decompose_eigen_cov(res->noise) == FAIL)
            goto bad;
        fprintf(stderr,"Eigenvalue decomposition done.\n");
        for (k = 0; k < res->noise->ncov; k++) {
            if (res->noise->lambda[k] < 0.0)
                res->noise->lambda[k] = 0.0;
        }
    }
    else {
        printf("Decomposition not needed for a diagonal covariance matrix.\n");
        if (mne_add_inv_cov(res->noise) == FAIL)
            goto bad;
    }

    mne_free_name_list(badlist,nbad);
    delete templates;
    delete comp_coils;
    mne_free_ctf_comp_data_set(comp_data);
    return res;


bad : {
        mne_free_name_list(badlist,nbad);
        delete templates;
        delete comp_coils;
        mne_free_ctf_comp_data_set(comp_data);
        free_dipole_fit_data(res);
        return NULL;
    }
}


//============================= mne_ringbuffer.c =============================

typedef struct {
    int   size;		        /* Size of this buffer in floats */
    float *data;			/* The allocated buffer */
    float ***datap;		/* The matrix which uses this */
} *ringBufBuf,ringBufBufRec;

typedef struct {
    ringBufBuf *bufs;
    int        nbuf;
    int        next;
} *ringBuf,ringBufRec;


void mne_free_ring_buffer(void *thisp)

{
    int k;
    ringBuf this_buf = (ringBuf)thisp;

    if (!this_buf)
        return;

    for (k = 0; k < this_buf->nbuf; k++)
        FREE(this_buf->bufs[k]->data);
    FREE(this_buf->bufs);
    FREE(this_buf);
    return;
}

void *mne_initialize_ring(int nbuf)

{
    int k;
    ringBuf ring;

    ring = MALLOC(1,ringBufRec);
    ring->bufs = MALLOC(nbuf,ringBufBuf);
    ring->nbuf = nbuf;

    for (k = 0; k < nbuf; k++) {
        ring->bufs[k] = MALLOC(1,ringBufBufRec);
        ring->bufs[k]->size  = 0;
        ring->bufs[k]->data  = NULL;
        ring->bufs[k]->datap = NULL;
    }
    ring->next = 0;

#ifdef DEBUG
    fprintf(stderr,"Ring buffer structure with %d entries initialized\n",ring->nbuf);
#endif

    return ring;
}

void mne_allocate_from_ring(void *ringp, int nrow, int ncol, float ***res)
/*
 * Get a new buffer
 */
{
    float **mat;
    int   j;
    ringBufBuf buf;
    ringBuf    ring = (ringBuf)ringp;

    if (ring->next > ring->nbuf-1)
        ring->next = 0;

#ifdef DEBUG
    fprintf(stderr,"Allocating buf # %d\n",ring->next);
#endif

    buf = ring->bufs[ring->next++];

    if (buf->datap) {		/* Clear the reference */
        FREE(*buf->datap);
        *buf->datap = NULL;
    }
    *res = mat = MALLOC(nrow,float *);
    if (buf->size < nrow*ncol)
        buf->data = REALLOC(buf->data,nrow*ncol,float);

    for (j = 0; j < nrow; j++)
        mat[j] = buf->data + j*ncol;

    buf->datap = res;

    return;
}


//============================= mne_raw_routines.c =============================

void mne_free_raw_info(mneRawInfo info)

{
    if (!info)
        return;
    FREE(info->filename);
    FREE(info->chInfo);
    FREE(info->trans);
//    FREE(info->rawDir);
    FREE(info->id);
    FREE(info);
    return;
}


int mne_read_raw_buffer_t(//fiffFile     in,        /* Input file */
                          FiffStream::SPtr& stream,
                          const FiffDirEntry::SPtr& ent,         /* The directory entry to read */
                          float        **data,      /* Allocated for npick x nsamp samples */
                          int          nchan,       /* Number of channels in the data */
                          int          nsamp,       /* Expected number of samples */
                          fiffChInfo   chs,         /* Channel info for ALL channels */
                          int          *pickno,     /* Which channels to pick */
                          int          npick)       /* How many */

{
    FiffTag::SPtr t_pTag;
//    fiffTagRec   tag;
    fiff_short_t *this_samples;
    fiff_float_t *this_samplef;
    fiff_int_t   *this_sample;

    int s,c;
    int do_all;
    float *mult;

//    tag.data = NULL;

    if (npick == 0) {
        pickno = MALLOC(nchan, int);
        for (c = 0; c < nchan; c++)
            pickno[c] = c;
        do_all = TRUE;
        npick = nchan;
    }
    else
        do_all = FALSE;

    mult = MALLOC(npick,float);
    for (c = 0; c < npick; c++)
        mult[c] = chs[pickno[c]].cal*chs[pickno[c]].range;

//    if (fiff_read_this_tag(in->fd,ent->pos,&tag) ==  FIFF_FAIL)
//        goto bad;
    if (!FiffTag::read_tag(stream,t_pTag,ent->pos))
        goto bad;

    if (ent->type == FIFFT_FLOAT) {
        if ((int)(t_pTag->size()/(sizeof(fiff_float_t)*nchan)) != nsamp) {
            printf("Incorrect number of samples in buffer.");
            goto bad;
        }
        qDebug() << "ToDo: Check whether this_samplef contains the right stuff!!! - use VectorXf instead";
        this_samplef = t_pTag->toFloat();
        for (s = 0; s < nsamp; s++, this_samplef += nchan) {
            for (c = 0; c < npick; c++)
                data[c][s] = mult[c]*this_samplef[pickno[c]];
        }
    }
    else if (ent->type == FIFFT_SHORT || ent->type == FIFFT_DAU_PACK16) {
        if ((int)(t_pTag->size()/(sizeof(fiff_short_t)*nchan)) != nsamp) {
            printf("Incorrect number of samples in buffer.");
            goto bad;
        }
        qDebug() << "ToDo: Check whether this_samples contains the right stuff!!! - use VectorXi instead";
        this_samples = (fiff_short_t *)t_pTag->data();
        for (s = 0; s < nsamp; s++, this_samples += nchan) {
            for (c = 0; c < npick; c++)
                data[c][s] = mult[c]*this_samples[pickno[c]];
        }
    }
    else if (ent->type == FIFFT_INT) {
        if ((int)(t_pTag->size()/(sizeof(fiff_int_t)*nchan)) != nsamp) {
            printf("Incorrect number of samples in buffer.");
            goto bad;
        }
        qDebug() << "ToDo: Check whether this_sample contains the right stuff!!! - use VectorXi instead";
        this_sample = t_pTag->toInt();
        for (s = 0; s < nsamp; s++, this_sample += nchan) {
            for (c = 0; c < npick; c++)
                data[c][s] = mult[c]*this_sample[pickno[c]];
        }
    }
    else {
        printf("We are not prepared to handle raw data type: %d",ent->type);
        goto bad;
    }
    if (do_all)
        FREE(pickno);
    FREE(mult);
//    FREE(tag.data);
    return OK;

bad : {
        if (do_all)
            FREE(pickno);
//        FREE(tag.data);
        return FAIL;
    }
}



//============================= mne_fft.c =============================


void mne_fft_ana(float *data,int np, float **precalcp)
/*
      * FFT analysis for real data
      */
{
    float *precalc;

    printf("##################### DEBUG Error: FFT analysis needs to be implemented");

    //  if (precalcp && *precalcp)
    //    precalc = *precalcp;
    //  else {
    //    precalc = MALLOC(2*np+15,float);
    //    rffti(&np,precalc);
    //    if (precalcp)
    //      *precalcp = precalc;
    //  }
    //  rfftf(&np,data,precalc);
    if (!precalcp)
        FREE(precalc);
    return;
}


void mne_fft_syn(float *data,int np, float **precalcp)
/*
      * FFT synthesis for real data
      */
{
    float *precalc;
    float mult;

    printf("##################### DEBUG Error: FFT synthesis needs to be implemented");

    //  if (precalcp && *precalcp)
    //    precalc = *precalcp;
    //  else {
    //    precalc = MALLOC(2*np+15,float);
    //    rffti(&np,precalc);
    //    if (precalcp)
    //      *precalcp = precalc;
    //  }
    //  rfftb(&np,data,precalc);
    //  /*
    //   * Normalization
    //   */
    //  mult = 1.0/np;
    //  mne_scale_vector(mult,data,np);

    if (!precalcp)
        FREE(precalc);
    return;
}


//============================= mne_apply_filter.c =============================


typedef struct {
    float *freq_resp;		/* Frequency response */
    float *eog_freq_resp;		/* Frequency response (EOG) */
    float *precalc;		/* Precalculated data for FFT */
    int   np;			/* Length */
    float nprec;
} *filterData,filterDataRec;

static void filter_data_free(void *datap)

{
    filterData data = (filterData)datap;
    if (!data)
        return;
    FREE(data->freq_resp);
    FREE(data->eog_freq_resp);
    FREE(data->precalc);
    FREE(data);
    return;
}

static filterData new_filter_data()

{
    filterData data = MALLOC(1,filterDataRec);

    data->freq_resp     = NULL;
    data->eog_freq_resp = NULL;
    data->precalc       = NULL;
    data->np            = 0;
    return data;
}

int mne_compare_filters(mneFilterDef f1,
                        mneFilterDef f2)
/*
      * Return 0 if the two filter definitions are same, 1 otherwise
      */
{
    if (f1->filter_on != f2->filter_on ||
            fabs(f1->lowpass-f2->lowpass) > 0.1 ||
            fabs(f1->lowpass_width-f2->lowpass_width) > 0.1 ||
            fabs(f1->highpass-f2->highpass) > 0.1 ||
            fabs(f1->highpass_width-f2->highpass_width) > 0.1 ||
            fabs(f1->eog_lowpass-f2->eog_lowpass) > 0.1 ||
            fabs(f1->eog_lowpass_width-f2->eog_lowpass_width) > 0.1 ||
            fabs(f1->eog_highpass-f2->eog_highpass) > 0.1 ||
            fabs(f1->eog_highpass_width-f2->eog_highpass_width) > 0.1)
        return 1;
    else
        return 0;
}


void mne_create_filter_response(mneFilterDef    filter,
                                float           sfreq,
                                void            **filter_datap,
                                mneUserFreeFunc *filter_data_freep,
                                int             *highpass_effective)
/*
      * Create a frequency response and return also the function to free it
      */
{
    int resp_size;
    int k,s,w,f;
    int highpasss,lowpasss;
    int highpass_widths,lowpass_widths;
    float lowpass,highpass,lowpass_width,highpass_width;
    float *freq_resp;
    float pi4 = M_PI/4.0;
    float mult,add,c;
    filterData filter_data;

    resp_size = (filter->size + 2*filter->taper_size)/2 + 1;

    filter_data                = new_filter_data();
    filter_data->freq_resp     = MALLOC(resp_size,float);
    filter_data->eog_freq_resp = MALLOC(resp_size,float);
    filter_data->np            = resp_size;

    for (k = 0; k < resp_size; k++) {
        filter_data->freq_resp[k]     = 1.0;
        filter_data->eog_freq_resp[k] = 1.0;
    }
    *highpass_effective = FALSE;

    for (f = 0; f < 2; f++) {
        highpass       = f == 0 ? filter->highpass  : filter->eog_highpass;
        highpass_width = f == 0 ? filter->highpass_width : filter->eog_highpass_width;
        lowpass        = f == 0 ? filter->lowpass   : filter->eog_lowpass;
        lowpass_width  = f == 0 ? filter->lowpass_width  : filter->eog_lowpass_width;
        freq_resp      = f == 0 ? filter_data->freq_resp : filter_data->eog_freq_resp;
        /*
     * Start simple first
     */
        highpasss = ((resp_size-1)*highpass)/(0.5*sfreq);
        lowpasss = ((resp_size-1)*lowpass)/(0.5*sfreq);

        lowpass_widths = ((resp_size-1)*lowpass_width)/(0.5*sfreq);
        lowpass_widths = (lowpass_widths+1)/2;    /* What user specified */

        if (filter->highpass_width > 0.0) {
            highpass_widths = ((resp_size-1)*highpass_width)/(0.5*sfreq);
            highpass_widths  = (highpass_widths+1)/2;    /* What user specified */
        }
        else
            highpass_widths = 3;	   	             /* Minimal */

        if (filter->filter_on) {
            fprintf(stderr,"filter : %7.3f ... %6.1f Hz   bins : %d ... %d of %d hpw : %d lpw : %d\n",
                    highpass,
                    lowpass,
                    highpasss,
                    lowpasss,
                    resp_size,
                    highpass_widths,
                    lowpass_widths);
        }
        if (highpasss > highpass_widths + 1) {
            w    = highpass_widths;
            mult = 1.0/w;
            add  = 3.0;
            for (k = 0; k < highpasss-w+1; k++)
                freq_resp[k] = 0.0;
            for (k = -w+1, s = highpasss-w+1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4*(k*mult+add));
                    freq_resp[s] = freq_resp[s]*c*c;
                }
            }
            *highpass_effective = TRUE;
        }
        else
            *highpass_effective = *highpass_effective || (filter->highpass == 0.0);

        if (lowpass_widths > 0) {
            w    = lowpass_widths;
            mult = 1.0/w;
            add  = 1.0;
            for (k = -w+1, s = lowpasss-w+1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4*(k*mult+add));
                    freq_resp[s] = freq_resp[s]*c*c;
                }
            }
            for (k = s; k < resp_size; k++)
                freq_resp[k] = 0.0;
        }
        else {
            for (k = lowpasss; k < resp_size; k++)
                freq_resp[k] = 0.0;
        }
        if (filter->filter_on) {
            if (*highpass_effective)
                fprintf(stderr,"Highpass filter will work as specified.\n");
            else
                fprintf(stderr,"NOTE: Highpass filter omitted due to a too low corner frequency.\n");
        }
        else
            fprintf(stderr,"NOTE: Filter is presently switched off.\n");
    }
    *filter_datap      = filter_data;
    *filter_data_freep = filter_data_free;
    return;
}

int mne_apply_filter(mneFilterDef filter, void *datap, float *data, int ns, int zero_pad, float dc_offset, int kind)
/*
 * Do the magick trick
 */
{
    int   k,p,n;
    filterData d = (filterData)datap;
    float *freq_resp;

    if (ns != filter->size + 2*filter->taper_size) {
        printf("Incorrect data length in apply_filter");
        return FAIL;
    }
    /*
   * Zero padding
   */
    if (zero_pad) {
        for (k = 0; k < filter->taper_size; k++)
            data[k] = 0.0;
        for (k = filter->taper_size + filter->size; k < ns; k++)
            data[k] = 0.0;
    }
    if (!filter->filter_on)	/* Nothing else to do */
        return OK;
    /*
   * Make things nice by compensating for the dc offset
   */
    if (dc_offset != 0.0) {
        for (k = filter->taper_size; k < filter->taper_size + filter->size; k++)
            data[k] = data[k] - dc_offset;
    }
    if (!d)
        return OK;
    if (!d->freq_resp)
        return OK;
    /*
   * Next comes the FFT
   */
    mne_fft_ana(data,ns,&d->precalc);
    /*
   * Multiply with the frequency response
   * See FFTpack doc for details of the arrangement
   */
    n = ns % 2 == 0 ? ns/2 : (ns+1)/2;
    p = 0;
    /*
   * No imaginary part for the DC component
   */
    if (kind == FIFFV_EOG_CH)
        freq_resp = d->eog_freq_resp;
    else
        freq_resp = d->freq_resp;
    data[p] = data[p]*freq_resp[0]; p++;
    /*
   * The other components
   */
    for (k = 1 ; k < n ; k++) {
        data[p] = data[p]*freq_resp[k]; p++;
        data[p] = data[p]*freq_resp[k]; p++;
    }
    /*
   * Then the last value
   */
    if (ns % 2 == 0)
        data[p] = data[p]*freq_resp[k];

    mne_fft_syn(data,ns,&d->precalc);

    return OK;
}


//============================= mne_raw_routines.c =============================

static FiffDirNode::SPtr find_raw (const FiffDirNode::SPtr& node)
/*
      * Find the raw data
      */
{
    FiffDirNode::SPtr raw;
    QList<FiffDirNode::SPtr> temp;
    temp = node->dir_tree_find(FIFFB_RAW_DATA);
    if (temp.size() == 0) {
        temp = node->dir_tree_find(FIFFB_CONTINUOUS_DATA);
        if (temp.size() > 0)
            raw = temp[0];
    }
    else
        raw = temp[0];
    return raw;
}


static FiffDirNode::SPtr find_maxshield (const FiffDirNode::SPtr& node)

{
    FiffDirNode::SPtr raw;
    QList<FiffDirNode::SPtr> temp;
    temp = node->dir_tree_find(FIFFB_SMSH_RAW_DATA);
    if (temp.size() > 0)
        raw = temp[0];
    return (raw);
}


static int get_meas_info (//fiffFile file,	 /* The file we are reading */
                          FiffStream::SPtr& stream,
                          FiffDirNode::SPtr& node,	 /* The directory node containing our data */
                          fiffId *id,	         /* The block id from the nearest FIFFB_MEAS
                                                                              parent */
                          int *nchan,	         /* Number of channels */
                          float *sfreq,	         /* Sampling frequency */
                          float *highpass,       /* Highpass filter freq. */
                          float *lowpass,        /* Lowpass filter setting */
                          fiffChInfo *chp,	 /* Channel descriptions */
                          fiffCoordTrans *trans, /* Coordinate transformation
                                                                              (head <-> device) */
                          fiffTime *start_time)  /* Measurement date (starting time) */
/*
      * Find channel information from
      * nearest FIFFB_MEAS_INFO parent of
      * node.
      */
{
    FiffTag::SPtr t_pTag;
//    fiffTagRec tag;
//    fiffDirEntry this_ent;
    fiffChInfo ch;
    fiffChInfo this_ch;
    fiffCoordTrans t;
    int j,k;
    int to_find = 4;
    QList<FiffDirNode::SPtr> hpi;
    FiffDirNode::SPtr meas;
    fiff_int_t kind, pos;

//    tag.data    = NULL;
    *chp        = NULL;
    ch          = NULL;
    *trans      = NULL;
    *id         = NULL;
    *start_time = NULL;
    /*
    * Find desired parents
    */
//    meas = node->dir_tree_find(FIFFB_MEAS);
    if (!(meas = find_meas(node))) {
//    if (meas.size() == 0) {
        printf ("Meas. block not found!");
        goto bad;
    }

//    meas_info = node->dir_tree_find(FIFFB_MEAS_INFO);
    if (!(node = find_meas_info(node))) {
//    if (meas_info.count() == 0) {
        printf ("Meas. info not found!");
        goto bad;
    }
    /*
   * Is there a block id is in the FIFFB_MEAS node?
   */
//    if (meas->id != NULL) {
    if (!meas->id.isEmpty()) {
        *id = MALLOC(1,fiffIdRec);
//        memcpy (*id,meas[0]->id,sizeof(fiffIdRec));
        (*id)->version = meas->id.version;
        (*id)->machid[0] = meas->id.machid[0];
        (*id)->machid[1] = meas->id.machid[1];
        (*id)->time = meas->id.time;
    }
    /*
   * Others from FIFFB_MEAS_INFO
   */
    *lowpass  = -1;
    *highpass = -1;
    for (k = 0; k < node->nent; k++) {
        kind = node->dir[k]->kind;
        pos  = node->dir[k]->pos;
        switch (kind) {

        case FIFF_NCHAN :
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
//            *nchan = *(int *)(tag.data);
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *nchan = *t_pTag->toInt();
            ch = MALLOC(*nchan,fiffChInfoRec);
            for (j = 0; j < *nchan; j++)
                ch[j].scanNo = -1;
            to_find = to_find + *nchan - 1;
            break;

        case FIFF_SFREQ :
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
//            *sfreq = *(float *)(tag.data);
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *sfreq = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_LOWPASS :
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
//            *lowpass = *(float *)(tag.data);
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *lowpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_HIGHPASS :
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
//            *highpass = *(float *)(tag.data);
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *highpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_CH_INFO :		/* Information about one channel */
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
//            this_ch = (fiffChInfo)(tag.data);
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            this_ch = (fiffChInfo)t_pTag->data();
            if (this_ch->scanNo <= 0 || this_ch->scanNo > *nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                memcpy(ch+this_ch->scanNo-1,this_ch,
                       sizeof(fiffChInfoRec));
            to_find--;
            break;

        case FIFF_MEAS_DATE :
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            if (*start_time)
                FREE(*start_time);
//            *start_time = (fiffTime)tag.data;
            *start_time = (fiffTime)t_pTag->data();
//            tag.data = NULL;
            break;

        case FIFF_COORD_TRANS :
//            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                goto bad;
//            t = (fiffCoordTrans)tag.data;
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
            *t = *(fiffCoordTrans)t_pTag->data();
            /*
            * Require this particular transform!
            */
            if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                *trans = t;
//                tag.data = NULL;
                break;
            }
        }
    }
    /*
    * Search for the coordinate transformation from
    * HPI_RESULT block if it was not previously found
    */
//    hpi = fiff_dir_tree_find(node,FIFFB_HPI_RESULT);
//    node = hpi[0];

    hpi = node->dir_tree_find(FIFFB_HPI_RESULT);
    node = hpi[0];

//    FREE(hpi);
    if (hpi.size() > 0 && *trans == NULL)
        for (k = 0; k < hpi[0]->nent; k++)
            if (hpi[0]->dir[k]->kind ==  FIFF_COORD_TRANS) {
//                if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
//                    goto bad;
//                t = (fiffCoordTrans)tag.data;
                if (!FiffTag::read_tag(stream,t_pTag,hpi[0]->dir[k]->pos))
                    goto bad;
                t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
                *t = *(fiffCoordTrans)t_pTag->data();

                /*
                * Require this particular transform!
                */
                if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                    *trans = t;
//                    tag.data = NULL;
                    break;
                }
            }
    if (to_find < 3) {
        if (*lowpass < 0) {
            *lowpass = *sfreq/2.0;
            to_find--;
        }
        if (*highpass < 0) {
            *highpass = 0.0;
            to_find--;
        }
    }
    if (to_find != 0) {
        printf ("Not all essential tags were found!");
        goto bad;
    }
//    FREE (tag.data);
    *chp = ch;
    return (0);

bad : {
        FREE (ch);
//        FREE (tag.data);
        return (-1);
    }
}


int mne_load_raw_info(char *name,int allow_maxshield,mneRawInfo *infop)
/*
      * Load raw data information from a fiff file
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

//    fiffFile       in       = NULL;

    int            res      = FIFF_FAIL;
    fiffChInfo     chs      = NULL;	/* Channel info */
    fiffCoordTrans trans    = NULL;	/* The coordinate transformation */
    fiffId         id       = NULL;	/* Measurement id */
    QList<FiffDirEntry::SPtr>   rawDir;	/* Directory of raw data tags */
    mneRawInfo     info     = NULL;
    int            nchan    = 0;		/* Number of channels */
    float          sfreq    = 0.0;	/* Sampling frequency */
    float          highpass;		/* Highpass filter frequency */
    float          lowpass;		/* Lowpass filter frequency */
    FiffDirNode::SPtr    raw;
//    FiffDirEntry   one;
    fiffTime       start_time = NULL;
    int            k;
    int            maxshield_data = FALSE;
    /*
   * Open file
   */
//    if ((in = fiff_open(name)) == NULL)
//        goto out;
    if(!stream->open())
        goto out;
    raw = find_raw(stream->tree());
    if (raw->isEmpty()) {
        if (allow_maxshield) {
            raw = find_maxshield(stream->tree());
            if (raw->isEmpty()) {
                printf("No raw data in this file.");
                goto out;
            }
            maxshield_data = TRUE;
        }
        else {
            printf("No raw data in this file.");
            goto out;
        }
    }
    /*
   * Get the essential measurement information
   */
    if (get_meas_info (stream,raw,&id,&nchan,&sfreq,&highpass,&lowpass,&chs,&trans,&start_time) < 0)
        goto out;
    /*
    * Get the raw directory
    */
//    rawDir = MALLOC(raw->nent,fiffDirEntryRec);
//    memcpy(rawDir,raw->dir,raw->nent*sizeof(fiffDirEntryRec));
    rawDir = raw->dir;
    /*
   * Ready to put everything together
   */
    info = MALLOC(1,mneRawInfoRec);
    info->filename       = mne_strdup(name);
    info->nchan          = nchan;
    info->chInfo         = chs;
    info->coord_frame    = FIFFV_COORD_DEVICE;
    info->trans          = trans;
    info->sfreq          = sfreq;
    info->lowpass        = lowpass;
    info->highpass       = highpass;
//    info->rawDir         = NULL;
    info->maxshield_data = maxshield_data;
    if (id) {
        info->id           = MALLOC(1,fiffIdRec);
        *info->id          = *id;
    }
    else
        info->id           = NULL;
    /*
   * Getting starting time from measurement ID is not too accurate...
   */
    if (start_time)
        info->start_time = *start_time;
    else {
        if (id)
            info->start_time = id->time;
        else {
            info->start_time.secs = 0;
            info->start_time.usecs = 0;
        }
    }
    info->buf_size   = 0;
//    for (k = 0, one = raw->dir; k < raw->nent; k++, one++) {
    for (k = 0; k < raw->nent; k++) {
//        raw->dir[k]->kind
//                raw->dir[k]->type
//                raw->dir[k].size
        if (raw->dir[k]->kind == FIFF_DATA_BUFFER) {
            if (raw->dir[k]->type == FIFFT_DAU_PACK16 || raw->dir[k]->type == FIFFT_SHORT)
                info->buf_size = raw->dir[k]->size/(nchan*sizeof(fiff_short_t));
            else if (raw->dir[k]->type == FIFFT_FLOAT)
                info->buf_size = raw->dir[k]->size/(nchan*sizeof(fiff_float_t));
            else if (raw->dir[k]->type == FIFFT_INT)
                info->buf_size = raw->dir[k]->size/(nchan*sizeof(fiff_int_t));
            else {
                printf("We are not prepared to handle raw data type: %d",raw->dir[k]->type);
                goto out;
            }
            break;
        }
    }
    if (info->buf_size <= 0) {
        printf("No raw data buffers available.");
        goto out;
    }
    info->rawDir     = rawDir;
    info->ndir       = raw->nent;
    *infop = info;
    res = FIFF_OK;

out : {
        if (res != FIFF_OK) {
            FREE(chs);
            FREE(trans);
//            FREE(rawDir);
            FREE(info);
        }
        FREE(id);
//        fiff_close(in);
        stream->close();
        return (res);
    }
}


//============================= mne_events.c =============================


void mne_free_event(mneEvent e)
{
    if (!e)
        return;
    FREE(e->comment);
    FREE(e);
    return;
}


void mne_free_event_list(mneEventList list)

{
    int k;
    if (!list)
        return;
    for (k = 0; k < list->nevent; k++)
        mne_free_event(list->events[k]);
    FREE(list->events);
    FREE(list);
    return;
}




//============================= mne_raw_data.c =============================

#define APPROX_RING_BUF_SIZE (600*1024*1024)

static int approx_ring_buf_size = APPROX_RING_BUF_SIZE;



static mneRawData new_raw_data()

{
    mneRawData new_data        = MALLOC(1,mneRawDataRec);
    new_data->filename         = NULL;
//    new_data->file             = NULL;
    new_data->info             = NULL;
    new_data->bufs             = NULL;
    new_data->nbuf             = 0;
    new_data->proj             = NULL;
    new_data->ch_names         = NULL;
    new_data->bad              = NULL;
    new_data->badlist          = NULL;
    new_data->nbad             = 0;
    new_data->first_samp       = 0;
    new_data->omit_samp        = 0;
    new_data->omit_samp_old    = 0;
    new_data->event_list       = NULL;
    new_data->max_event        = 0;
    new_data->dig_trigger      = NULL;
    new_data->dig_trigger_mask = 0;
    new_data->ring             = NULL;
    new_data->filt_ring        = NULL;
    new_data->filt_bufs        = NULL;
    new_data->nfilt_buf        = 0;
    new_data->first_sample_val = NULL;
    new_data->filter           = NULL;
    new_data->filter_data      = NULL;
    new_data->filter_data_free = NULL;
    new_data->offsets          = NULL;
    new_data->deriv            = NULL;
    new_data->deriv_matched    = NULL;
    new_data->deriv_offsets    = NULL;
    new_data->user             = NULL;
    new_data->user_free        = NULL;
    new_data->comp             = NULL;
    new_data->comp_file        = MNE_CTFV_NOGRAD;
    new_data->comp_now         = MNE_CTFV_NOGRAD;
    new_data->sss              = NULL;
    return new_data;
}


static void free_bufs(mneRawBufDef bufs, int nbuf)

{
    int k;
    for (k = 0; k < nbuf; k++) {
        FREE(bufs[k].ch_filtered);
        /*
     * Clear the pointers only, not the data which are in the ringbuffer
     */
        FREE(bufs[k].vals);
    }
    FREE(bufs);
}


void mne_raw_free_data(mneRawData d)

{
    if (!d)
        return;
//    fiff_close(d->file);
    d->stream->close();
    FREE(d->filename);
    mne_free_name_list(d->ch_names,d->info->nchan);

    free_bufs(d->bufs,d->nbuf);
    mne_free_ring_buffer(d->ring);

    free_bufs(d->filt_bufs,d->nfilt_buf);
    mne_free_ring_buffer(d->filt_ring);

    mne_free_proj_op(d->proj);
    mne_free_name_list(d->badlist,d->nbad);
    FREE(d->first_sample_val);
    FREE(d->bad);
    FREE(d->offsets);
    mne_free_ctf_comp_data_set(d->comp);
    if(d->sss)
        delete d->sss;

    if (d->filter_data_free)
        d->filter_data_free(d->filter_data);
    if (d->user_free)
        d->user_free(d->user);
    FREE(d->dig_trigger);
    mne_free_event_list(d->event_list);

    mne_free_raw_info(d->info);

    delete d->deriv;
    delete d->deriv_matched;
    FREE(d->deriv_offsets);

    FREE(d);
    return;
}



void mne_raw_add_filter_response(mneRawData data, int *highpass_effective)
/*
      * Add the standard filter frequency response function
      */
{
    if (!data)
        return;
    /*
   * Free the previous filter definition
   */
    if (data->filter_data_free)
        data->filter_data_free(data->filter_data);
    data->filter_data      = NULL;
    data->filter_data_free = NULL;
    /*
   * Nothing more to do if there is no filter
   */
    if (!data->filter)
        return;
    /*
   * Create a new one
   */
    mne_create_filter_response(data->filter,
                               data->info->sfreq,
                               &data->filter_data,
                               &data->filter_data_free,
                               highpass_effective);
}



static void setup_filter_bufs(mneRawData data)
/*
 * These will hold the filtered data
 */
{
    mneFilterDef filter;
    int       nfilt_buf;
    mneRawBufDef bufs;
    int       j,k;
    int       firstsamp;
    int       nring_buf;
    int       highpass_effective;

    free_bufs(data->filt_bufs,data->nfilt_buf);
    data->filt_bufs = NULL;
    data->nfilt_buf = 0;
    mne_free_ring_buffer(data->filt_ring);
    data->filt_ring = NULL;

    if (!data || !data->filter)
        return;
    filter = data->filter;

    for (nfilt_buf = 0, firstsamp = data->first_samp-filter->taper_size;
         firstsamp < data->nsamp + data->first_samp;
         firstsamp = firstsamp + filter->size)
        nfilt_buf++;
#ifdef DEBUG
    fprintf(stderr,"%d filter buffers needed\n",nfilt_buf);
#endif
    bufs = MALLOC(nfilt_buf,mneRawBufDefRec);
    for (k = 0, firstsamp = data->first_samp-filter->taper_size; k < nfilt_buf; k++,
         firstsamp = firstsamp + filter->size) {
        bufs[k].ns          = filter->size + 2*filter->taper_size;
        bufs[k].firsts      = firstsamp;
        bufs[k].lasts       = firstsamp + bufs[k].ns - 1;
//        bufs[k].ent         = NULL;
        bufs[k].nchan       = data->info->nchan;
        bufs[k].is_skip     = FALSE;
        bufs[k].vals        = NULL;
        bufs[k].valid       = FALSE;
        bufs[k].ch_filtered = MALLOC(data->info->nchan,int);
        bufs[k].comp_status = MNE_CTFV_NOGRAD;

        for (j = 0; j < data->info->nchan; j++)
            bufs[k].ch_filtered[j] = FALSE;
    }
    data->filt_bufs = bufs;
    data->nfilt_buf = nfilt_buf;
    nring_buf       = approx_ring_buf_size/((2*filter->taper_size+filter->size)*
                                            data->info->nchan*sizeof(float));
    data->filt_ring = mne_initialize_ring(nring_buf);
    mne_raw_add_filter_response(data,&highpass_effective);

    return;
}


static int load_one_buffer(mneRawData data, mneRawBufDef buf)
/*
 * load just one
 */
{
    if (buf->ent->kind == FIFF_DATA_SKIP) {
        printf("Cannot load a skip");
        return FAIL;
    }
    if (!buf->vals) {		/* The data space may have been reused */
        buf->valid = FALSE;
        mne_allocate_from_ring(data->ring, buf->nchan,buf->ns,&buf->vals);
    }
    if (buf->valid)
        return OK;

#ifdef DEBUG
    fprintf(stderr,"Read buffer %d .. %d\n",buf->firsts,buf->lasts);
#endif

    if (mne_read_raw_buffer_t(data->stream,
                              buf->ent,
                              buf->vals,
                              buf->nchan,
                              buf->ns,
                              data->info->chInfo,
                              NULL,0) != OK) {
        buf->valid = FALSE;
        return FAIL;
    }
    buf->valid       = TRUE;
    buf->comp_status = data->comp_file;
    return OK;
}

static int compensate_buffer(mneRawData data, mneRawBufDef buf)
/*
 * Apply compensation channels
 */
{
    mneCTFcompData temp;

    if (!data->comp)
        return OK;
    if (!data->comp->undo && !data->comp->current)
        return OK;
    if (buf->comp_status == data->comp_now)
        return OK;
    if (!buf->vals)
        return OK;
    /*
   * Have to do the hard job
   */
    if (data->comp->undo) {
        temp = data->comp->current;
        data->comp->current = data->comp->undo;
        data->comp->undo    = temp;
        /*
     * Undo the previous compensation
     */
        if (mne_apply_ctf_comp_t(data->comp,FALSE,buf->vals,data->info->nchan,buf->ns) != OK) {
            temp                = data->comp->undo;
            data->comp->undo    = data->comp->current;
            data->comp->current = temp;
            goto bad;
        }
        temp                = data->comp->undo;
        data->comp->undo    = data->comp->current;
        data->comp->current = temp;
    }
    if (data->comp->current) {
        /*
     * Apply new compensation
     */
        if (mne_apply_ctf_comp_t(data->comp,TRUE,buf->vals,data->info->nchan,buf->ns) != OK)
            goto bad;
    }
    buf->comp_status = data->comp_now;
    return OK;

bad :
    return FAIL;
}




int mne_raw_pick_data(mneRawData     data,
                      mneChSelection sel,
                      int            firsts,
                      int            ns,
                      float          **picked)
/*
 * Data from a selection of channels
 */
{
    int          k,s,p,start,c,fills;
    int          ns2,s2;
    mneRawBufDef this_buf;
    float        *values;
    int          need_some;

    float        **deriv_vals = NULL;
    int          deriv_ns     = 0;
    int          nderiv       = 0;

    if (firsts < data->first_samp) {
        for (s = 0, p = firsts; p < data->first_samp; s++, p++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0.0;
            else
                for (c = 0; c < data->info->nchan; c++)
                    picked[c][s] = 0.0;
        }
        ns     = ns - s;
        firsts = data->first_samp;
    }
    else
        s = 0;
    /*
   * There is possibly nothing to do
   */
    if (sel) {
        for (c = 0, need_some = FALSE; c < sel->nchan; c++) {
            if (sel->pick[c] >= 0 || sel->pick_deriv[c] >= 0) {
                need_some = TRUE;
                break;
            }
        }
        if (!need_some)
            return OK;
    }
    /*
   * Have to to the hard work
   */
    for (k = 0, this_buf = data->bufs, s = 0; k < data->nbuf; k++, this_buf++) {
        if (this_buf->lasts >= firsts) {
            start = firsts - this_buf->firsts;
            if (start < 0)
                start = 0;
            if (this_buf->is_skip) {
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    if (sel) {
                        for (c = 0; c < sel->nchan; c++)
                            if (sel->pick[c] >= 0)
                                picked[c][s] = 0.0;
                    }
                    else {
                        for (c = 0; c < data->info->nchan; c++)
                            picked[c][s] = 0.0;
                    }
                }
            }
            else {
                /*
         * Load the buffer
         */
                if (load_one_buffer(data,this_buf) != OK)
                    return FAIL;
                /*
         * Apply compensation
         */
                if (compensate_buffer(data,this_buf) != OK)
                    return FAIL;
                ns2 = s2 = 0;
                if (sel) {
                    /*
           * Do we need the derived channels?
           */
                    if (sel->nderiv > 0 && data->deriv_matched) {
                        if (deriv_ns < this_buf->ns || nderiv != data->deriv_matched->deriv_data->nrow) {
                            FREE_CMATRIX(deriv_vals);
                            deriv_vals  = ALLOC_CMATRIX(data->deriv_matched->deriv_data->nrow,this_buf->ns);
                            nderiv      = data->deriv_matched->deriv_data->nrow;
                            deriv_ns    = this_buf->ns;
                        }
                        if (mne_sparse_mat_mult2(data->deriv_matched->deriv_data->data,this_buf->vals,this_buf->ns,deriv_vals) == FAIL) {
                            FREE_CMATRIX(deriv_vals);
                            return FAIL;
                        }
                    }
                    for (c = 0; c < sel->nchan; c++) {
                        /*
             * First pick the ordinary channels...
             */
                        if (sel->pick[c] >= 0) {
                            values = this_buf->vals[sel->pick[c]];
                            for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                                picked[c][s2] = values[p];
                        }
                        /*
             * ...then the derived ones
             */
                        else if (sel->pick_deriv[c] >= 0 && data->deriv_matched) {
                            values = deriv_vals[sel->pick_deriv[c]];
                            for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                                picked[c][s2] = values[p];
                        }
                    }
                }
                else {
                    for (c = 0; c < data->info->nchan; c++)
                        for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                            picked[c][s2] = this_buf->vals[c][p];
                }
                s  = s2;
                ns = ns2;
            }
            if (ns == 0)
                break;
        }
    }
    /*
   * Extend with the last available sample or zero if the request is beyond the data
   */
    if (s > 0) {
        fills = s-1;
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = picked[c][fills];
            else
                for (c = 0; c < data->info->nchan; c++)
                    picked[c][s] = picked[c][fills];
        }
    }
    else {
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0;
            else
                for (c = 0; c < data->info->nchan; c++)
                    picked[c][s] = 0;
        }
    }
    FREE_CMATRIX(deriv_vals);
    return OK;
}


int mne_raw_pick_data_proj(mneRawData     data,
                           mneChSelection sel,
                           int            firsts,
                           int            ns,
                           float          **picked)
/*
 * Data from a set of channels, apply projection
 */
{
    int          k,s,p,start,c,fills;
    mneRawBufDef this_buf;
    float        **values;
    float        *pvalues;
    float        *deriv_pvalues = NULL;

    if (!data->proj || (sel && !mne_proj_op_affect(data->proj,sel->chspick,sel->nchan) && !mne_proj_op_affect(data->proj,sel->chspick_nospace,sel->nchan)))
        return mne_raw_pick_data(data,sel,firsts,ns,picked);

    if (firsts < data->first_samp) {
        for (s = 0, p = firsts; p < data->first_samp; s++, p++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0.0;
            else
                for (c = 0; c < data->info->nchan; c++)
                    picked[c][s] = 0.0;
        }
        ns         = ns - s;
        firsts     = data->first_samp;
    }
    else
        s = 0;
    pvalues = MALLOC(data->info->nchan,float);
    for (k = 0, this_buf = data->bufs; k < data->nbuf; k++, this_buf++) {
        if (this_buf->lasts >= firsts) {
            start = firsts - this_buf->firsts;
            if (start < 0)
                start = 0;
            if (this_buf->is_skip) {
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    if (sel) {
                        for (c = 0; c < sel->nchan; c++)
                            if (sel->pick[c] >= 0)
                                picked[c][s] = 0.0;
                    }
                    else {
                        for (c = 0; c < data->info->nchan; c++)
                            picked[c][s] = 0.0;
                    }
                }
            }
            else {
                /*
         * Load the buffer
         */
                if (load_one_buffer(data,this_buf) != OK)
                    return FAIL;
                /*
         * Apply compensation
         */
                if (compensate_buffer(data,this_buf) != OK)
                    return FAIL;
                /*
         * Apply projection
         */
                values = this_buf->vals;
                if (sel && sel->nderiv > 0 && data->deriv_matched)
                    deriv_pvalues = MALLOC(data->deriv_matched->deriv_data->nrow,float);
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    for (c = 0; c < data->info->nchan; c++)
                        pvalues[c] = values[c][p];
                    if (mne_proj_op_proj_vector(data->proj,pvalues,data->info->nchan,TRUE) != OK)
                        qWarning()<<"Error";
                    if (sel) {
                        if (sel->nderiv > 0 && data->deriv_matched) {
                            if (mne_sparse_vec_mult2(data->deriv_matched->deriv_data->data,pvalues,deriv_pvalues) == FAIL)
                                return FAIL;
                        }
                        for (c = 0; c < sel->nchan; c++) {
                            /*
               * First try the ordinary channels...
               */
                            if (sel->pick[c] >= 0)
                                picked[c][s] = pvalues[sel->pick[c]];
                            /*
               * ...then the derived ones
               */
                            else if (sel->pick_deriv[c] >= 0 && data->deriv_matched)
                                picked[c][s] = deriv_pvalues[sel->pick_deriv[c]];
                        }
                    }
                    else {
                        for (c = 0; c < data->info->nchan; c++) {
                            picked[c][s] = pvalues[c];
                        }
                    }
                }
            }
            if (ns == 0)
                break;
        }
    }
    FREE(deriv_pvalues);
    FREE(pvalues);
    /*
   * Extend with the last available sample or zero if the request is beyond the data
   */
    if (s > 0) {
        fills = s-1;
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = picked[c][fills];
            else
                for (c = 0; c < data->info->nchan; c++)
                    picked[c][s] = picked[c][fills];
        }
    }
    else {
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0;
            else
                for (c = 0; c < data->info->nchan; c++)
                    picked[c][s] = 0;
        }
    }
    return OK;
}



static int load_one_filt_buf(mneRawData data, mneRawBufDef buf)
/*
 * Load and filter one buffer
 */
{
    int k;
    int res;

    float **vals;

    if (!buf->vals) {
        buf->valid = FALSE;
        mne_allocate_from_ring(data->filt_ring, buf->nchan, buf->ns,&buf->vals);
    }
    if (buf->valid)
        return OK;

    vals = MALLOC(buf->nchan,float *);
    for (k = 0; k < buf->nchan; k++) {
        buf->ch_filtered[k] = FALSE;
        vals[k] = buf->vals[k] + data->filter->taper_size;
    }

    res = mne_raw_pick_data_proj(data,NULL,buf->firsts + data->filter->taper_size,buf->ns - 2*data->filter->taper_size,vals);

    FREE(vals);

#ifdef DEBUG
    if (res == OK)
        fprintf(stderr,"Loaded filtered buffer %d...%d %d %d last = %d\n",
                buf->firsts,buf->lasts,buf->lasts-buf->firsts+1,buf->ns,data->first_samp + data->nsamp);
#endif
    buf->valid = res == OK;
    return res;
}



int mne_raw_pick_data_filt(mneRawData     data,
                           mneChSelection sel,
                           int            firsts,
                           int            ns,
                           float          **picked)
/*
 * Data for a selection (filtered and picked)
 */
{
    int          k,s,bs,c;
    int          bs1,bs2,s1,s2,lasts;
    mneRawBufDef this_buf;
    float        *values;
    float        **deriv_vals = NULL;
    float        *dc          = NULL;
    float        dc_offset;
    int          deriv_ns     = 0;
    int          nderiv       = 0;
    int          filter_was;

    if (!data->filter || !data->filter->filter_on)
        return mne_raw_pick_data_proj(data,sel,firsts,ns,picked);

    if (sel) {
        for (s = 0; s < ns; s++)
            for (c = 0; c < sel->nchan; c++)
                picked[c][s] = 0.0;
    }
    else {
        for (s = 0; s < ns; s++)
            for (c = 0; c < data->info->nchan; c++)
                picked[c][s] = 0.0;
    }
    lasts = firsts + ns - 1;
    /*
   * Take into account the initial dc offset (compensate and project)
   */
    if (data->first_sample_val) {
        dc = MALLOC(data->info->nchan,float);

        for (k = 0; k < data->info->nchan; k++)
            dc[k] = data->first_sample_val[k];
        /*
     * Is this correct??
     */
        if (data->comp && data->comp->current)
            if (mne_apply_ctf_comp(data->comp,TRUE,dc,data->info->nchan,NULL,0) != OK)
                goto bad;
        if (data->proj)
            if (mne_proj_op_proj_vector(data->proj,dc,data->info->nchan,TRUE) != OK)
                goto bad;
    }
    filter_was = data->filter->filter_on;
    /*
   * Find the first buffer to consider
   */
    for (k = 0, this_buf = data->filt_bufs; k < data->nfilt_buf; k++, this_buf++) {
        if (this_buf->lasts >= firsts)
            break;
    }
    for (; k < data->nfilt_buf && this_buf->firsts <= lasts; k++, this_buf++) {
#ifdef DEBUG
        fprintf(stderr,"this_buf (%d): %d..%d\n",k,this_buf->firsts,this_buf->lasts);
#endif
        /*
     * Load the buffer first and apply projection
     */
        if (load_one_filt_buf(data,this_buf) != OK)
            goto bad;
        /*
     * Then filter all relevant channels (not stimuli)
     */
        if (sel) {
            for (c = 0; c < sel->nchan; c++) {
                if (sel->pick[c] >= 0) {
                    if (!this_buf->ch_filtered[sel->pick[c]]) {
                        /*
             * Do not filter stimulus channels
             */
                        dc_offset = 0.0;
                        if (data->info->chInfo[sel->pick[c]].kind == FIFFV_STIM_CH)
                            data->filter->filter_on = FALSE;
                        else if (dc)
                            dc_offset = dc[sel->pick[c]];
                        if (mne_apply_filter(data->filter,data->filter_data,this_buf->vals[sel->pick[c]],this_buf->ns,TRUE,
                                             dc_offset,data->info->chInfo[sel->pick[c]].kind) != OK) {
                            data->filter->filter_on = filter_was;
                            goto bad;
                        }
                        this_buf->ch_filtered[sel->pick[c]] = TRUE;
                        data->filter->filter_on = filter_was;
                    }
                }
            }
            /*
       * Also check channels included in derivations if they are used
       */
            if (sel->nderiv > 0 && data->deriv_matched) {
                MneDeriv* der = data->deriv_matched;
                for (c = 0; c < der->deriv_data->ncol; c++) {
                    if (der->in_use[c] > 0 &&
                            !this_buf->ch_filtered[c]) {
                        /*
             * Do not filter stimulus channels
             */
                        dc_offset = 0.0;
                        if (data->info->chInfo[c].kind == FIFFV_STIM_CH)
                            data->filter->filter_on = FALSE;
                        else if (dc)
                            dc_offset = dc[c];
                        if (mne_apply_filter(data->filter,data->filter_data,this_buf->vals[c],this_buf->ns,TRUE,
                                             dc_offset,data->info->chInfo[c].kind) != OK) {
                            data->filter->filter_on = filter_was;
                            goto bad;
                        }
                        this_buf->ch_filtered[c] = TRUE;
                        data->filter->filter_on = filter_was;
                    }
                }
            }
        }
        else {
            /*
       * Simply filter all channels if there is no selection
       */
            for (c = 0; c < data->info->nchan; c++) {
                if (!this_buf->ch_filtered[c]) {
                    /*
           * Do not filter stimulus channels
           */
                    dc_offset = 0.0;
                    if (data->info->chInfo[c].kind == FIFFV_STIM_CH)
                        data->filter->filter_on = FALSE;
                    else if (dc)
                        dc_offset = dc[c];
                    if (mne_apply_filter(data->filter,data->filter_data,this_buf->vals[c],this_buf->ns,TRUE,
                                         dc_offset,data->info->chInfo[c].kind) != OK) {
                        data->filter->filter_on = filter_was;
                        goto bad;
                    }
                    this_buf->ch_filtered[c] = TRUE;
                    data->filter->filter_on = filter_was;
                }
            }
        }
        /*
     * Decide the picking limits
     */
        if (firsts >= this_buf->firsts) {
            bs1 = firsts - this_buf->firsts;
            s1  = 0;
        }
        else {
            bs1 = 0;
            s1  = this_buf->firsts - firsts;
        }
        if (lasts >= this_buf->lasts) {
            bs2 = this_buf->ns;
            s2  = this_buf->lasts - lasts + ns;
        }
        else {
            bs2 = lasts - this_buf->lasts + this_buf->ns;
            s2  = ns;
        }
#ifdef DEBUG
        fprintf(stderr,"buf  : %d..%d %d\n",bs1,bs2,bs2-bs1);
        fprintf(stderr,"dest : %d..%d %d\n",s1,s2,s2-s1);
#endif
        /*
     * Then pick data from all relevant channels
     */
        if (sel) {
            if (sel->nderiv > 0 && data->deriv_matched) {
                /*
         * Compute derived data if we need it
         */
                if (deriv_ns < this_buf->ns || nderiv != data->deriv_matched->deriv_data->nrow) {
                    FREE_CMATRIX(deriv_vals);
                    deriv_vals  = ALLOC_CMATRIX(data->deriv_matched->deriv_data->nrow,this_buf->ns);
                    nderiv      = data->deriv_matched->deriv_data->nrow;
                    deriv_ns    = this_buf->ns;
                }
                if (mne_sparse_mat_mult2(data->deriv_matched->deriv_data->data,this_buf->vals,this_buf->ns,deriv_vals) == FAIL)
                    goto bad;
            }
            for (c = 0; c < sel->nchan; c++) {
                /*
         * First the ordinary channels
         */
                if (sel->pick[c] >= 0) {
                    values = this_buf->vals[sel->pick[c]];
                    for (s = s1, bs = bs1; s < s2; s++, bs++)
                        picked[c][s] += values[bs];
                }
                else if (sel->pick_deriv[c] >= 0 && data->deriv_matched) {
                    values = deriv_vals[sel->pick_deriv[c]];
                    for (s = s1, bs = bs1; s < s2; s++, bs++)
                        picked[c][s] += values[bs];
                }
            }
        }
        else {
            for (c = 0; c < data->info->nchan; c++) {
                values = this_buf->vals[c];
                for (s = s1, bs = bs1; s < s2; s++, bs++)
                    picked[c][s] += values[bs];
            }
        }
    }
    FREE_CMATRIX(deriv_vals);
    FREE(dc);
    return OK;

bad : {
        FREE_CMATRIX(deriv_vals);
        FREE(dc);
        return FAIL;
    }
}

mneRawData mne_raw_open_file_comp(char *name, int omit_skip, int allow_maxshield, mneFilterDef filter, int comp_set)
/*
 * Open a raw data file
 */
{
    mneRawInfo         info  = NULL;
    mneRawData         data  = NULL;

    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));
//    fiffFile           in    = NULL;

    FiffDirEntry::SPtr dir;
    QList<FiffDirEntry::SPtr> dir0;
//    fiffTagRec   tag;
    FiffTag::SPtr t_pTag;
    fiffChInfo   ch;
    mneRawBufDef bufs;
    int k, b, nbuf, ndir, nnames;
    int current_dir0 = 0;

//    tag.data = NULL;

    if (mne_load_raw_info(name,allow_maxshield,&info) == FAIL)
        goto bad;

    for (k = 0; k < info->nchan; k++) {
        ch = info->chInfo+k;
        if (strcmp(ch->ch_name,MNE_DEFAULT_TRIGGER_CH) == 0) {
            if (fabs(1.0-ch->range) > 1e-5) {
                ch->range = 1.0;
                fprintf(stderr,"%s range set to %f\n",MNE_DEFAULT_TRIGGER_CH,ch->range);
            }
        }
        /*
     * Take care of the nonzero unit multiplier
     */
        if (ch->unit_mul != 0) {
            ch->cal = pow(10.0,(double)(ch->unit_mul))*ch->cal;
            fprintf(stderr,"Ch %s unit multiplier %d -> 0\n",ch->ch_name,ch->unit_mul);
            ch->unit_mul = 0;
        }
    }
//    if ((in = fiff_open(name)) == NULL)
//        goto bad;
    if(!stream->open())
        goto bad;

    data           = new_raw_data();
    data->filename = mne_strdup(name);
    data->stream   = stream;
    data->info     = info;
    /*
   * Add the channel name list
   */
    mne_channel_names_to_name_list(info->chInfo,info->nchan,&data->ch_names,&nnames);
    if (nnames != info->nchan) {
        printf("Channel names were not translated correctly into a name list");
        goto bad;
    }
    /*
   * Compensation data
   */
    data->comp = mne_read_ctf_comp_data(data->filename);
    if (data->comp) {
        if (data->comp->ncomp > 0)
            fprintf(stderr,"Read %d compensation data sets from %s\n",data->comp->ncomp,data->filename);
        else
            fprintf(stderr,"No compensation data in %s\n",data->filename);
    }
    else
        qWarning() << "err_print_error()";
    if ((data->comp_file = mne_get_ctf_comp(data->info->chInfo,data->info->nchan)) == FAIL)
        goto bad;
    fprintf(stderr,"Compensation in file : %s\n",mne_explain_ctf_comp(mne_map_ctf_comp_kind(data->comp_file)));
    if (comp_set < 0)
        data->comp_now = data->comp_file;
    else
        data->comp_now = comp_set;

    if (mne_ctf_set_compensation(data->comp,data->comp_now,data->info->chInfo,data->info->nchan,NULL,0) == FAIL)
        goto bad;
    /*
   * SSS data
   */
    data->sss = MneSssData::read_sss_data(data->filename);
    if (data->sss && data->sss->job != FIFFV_SSS_JOB_NOTHING && data->sss->ncomp > 0) {
        fprintf(stderr,"SSS data read from %s :\n",data->filename);
        data->sss->print(stderr);
    }
    else {
        fprintf(stderr,"No SSS data in %s\n",data->filename);
        if(data->sss)
            delete data->sss;
        data->sss = NULL;
    }
    /*
   * Buffers
   */
    dir0 = data->info->rawDir;
    ndir = data->info->ndir;
    /*
   * Take into account the first sample
   */
    if (dir0[current_dir0]->kind == FIFF_FIRST_SAMPLE) {
//        if (fiff_read_this_tag(in->fd,dir0->pos,&tag) == FIFF_FAIL)
//            goto bad;
        if (!FiffTag::read_tag(stream,t_pTag,dir0[current_dir0]->pos))
            goto bad;
        data->first_samp = *t_pTag->toInt();
        current_dir0++;
        ndir--;
    }
    if (dir0[current_dir0]->kind == FIFF_DATA_SKIP) {
        int nsamp_skip;
//        if (fiff_read_this_tag(in->fd,dir0->pos,&tag) == FIFF_FAIL)
//            goto bad;
        if (!FiffTag::read_tag(stream,t_pTag,dir0[current_dir0]->pos))
            goto bad;
        nsamp_skip = data->info->buf_size*(*t_pTag->toInt());
        fprintf(stderr,"Data skip of %d samples in the beginning\n",nsamp_skip);
        current_dir0++;
        ndir--;
        if (dir0[current_dir0]->kind == FIFF_FIRST_SAMPLE) {
//            if (fiff_read_this_tag(in->fd,dir0->pos,&tag) == FIFF_FAIL)
//                goto bad;
            if (!FiffTag::read_tag(stream,t_pTag,dir0[current_dir0]->pos))
                goto bad;
            data->first_samp += *t_pTag->toInt();
            current_dir0++;
            ndir--;
        }
        if (omit_skip) {
            data->omit_samp     = data->first_samp + nsamp_skip;
            data->omit_samp_old = nsamp_skip;
            data->first_samp    = 0;
        }
        else {
            data->first_samp     = data->first_samp + nsamp_skip;
        }
    }
    else if (omit_skip) {
        data->omit_samp  = data->first_samp;
        data->first_samp = 0;
    }
#ifdef DEBUG
    fprintf(stderr,"data->first_samp = %d\n",data->first_samp);
#endif
    /*
   * Figure out the buffers
   */
//    for (k = 0, dir = dir0, nbuf = 0; k < ndir; k++, dir++)
    for (k = 0, nbuf = 0; k < ndir; k++)
        if (dir0[k]->kind == FIFF_DATA_BUFFER ||
                dir0[k]->kind == FIFF_DATA_SKIP)
            nbuf++;
    bufs = MALLOC(nbuf,mneRawBufDefRec);

//    for (k = 0, nbuf = 0, dir = dir0; k < ndir; k++, dir++)
    for (k = 0, nbuf = 0; k < ndir; k++)
        if (dir0[k]->kind == FIFF_DATA_BUFFER ||
                dir0[k]->kind == FIFF_DATA_SKIP) {
            bufs[nbuf].ns          = 0;
            bufs[nbuf].ent         = dir0[k];
            bufs[nbuf].nchan       = data->info->nchan;
            bufs[nbuf].is_skip     = dir0[k]->kind == FIFF_DATA_SKIP;
            bufs[nbuf].vals        = NULL;
            bufs[nbuf].valid       = FALSE;
            bufs[nbuf].ch_filtered = NULL;
            bufs[nbuf].comp_status = data->comp_file;
            nbuf++;
        }
    data->bufs  = bufs;
    data->nbuf  = nbuf;
    data->nsamp = 0;
    for (k = 0; k < nbuf; k++) {
        dir = bufs[k].ent;
        if (dir->kind == FIFF_DATA_BUFFER) {
            if (dir->type == FIFFT_DAU_PACK16 || dir->type == FIFFT_SHORT)
                bufs[k].ns = dir->size/(data->info->nchan*sizeof(fiff_dau_pack16_t));
            else if (dir->type == FIFFT_FLOAT)
                bufs[k].ns = dir->size/(data->info->nchan*sizeof(fiff_float_t));
            else if (dir->type == FIFFT_INT)
                bufs[k].ns = dir->size/(data->info->nchan*sizeof(fiff_int_t));
            else {
                printf("We are not prepared to handle raw data type: %d",dir->type);
                goto bad;
            }
        }
        else if (dir->kind == FIFF_DATA_SKIP) {
//            if (fiff_read_this_tag(in->fd,dir->pos,&tag) == FIFF_FAIL)
//                goto bad;
            if (!FiffTag::read_tag(stream,t_pTag,dir->pos))
                goto bad;
            bufs[k].ns = data->info->buf_size*(*t_pTag->toInt());
        }
        bufs[k].firsts = k == 0 ? data->first_samp : bufs[k-1].lasts + 1;
        bufs[k].lasts  = bufs[k].firsts + bufs[k].ns - 1;
        data->nsamp += bufs[k].ns;
    }
//    FREE(tag.data);
    /*
   * Set up the first sample values
   */
    data->bad = MALLOC(data->info->nchan,int);
    data->offsets = MALLOC(data->info->nchan,float);
    for (k = 0; k < data->info->nchan; k++) {
        data->bad[k] = FALSE;
        data->offsets[k] = 0.0;
    }
    /*
    * Th bad channel stuff
    */
    {
        if (mne_read_bad_channel_list(name,&data->badlist,&data->nbad) == OK) {
            for (b = 0; b < data->nbad; b++) {
                for (k = 0; k < data->info->nchan; k++) {
                    if (strcasecmp(data->info->chInfo[k].ch_name,data->badlist[b]) == 0) {
                        data->bad[k] = TRUE;
                        break;
                    }
                }
            }
            fprintf(stderr,"%d bad channels read from %s%s",data->nbad,name,data->nbad > 0 ? ":\n" : "\n");
            if (data->nbad > 0) {
                fprintf(stderr,"\t");
                for (k = 0; k < data->nbad; k++)
                    fprintf(stderr,"%s%c",data->badlist[k],k < data->nbad-1 ? ' ' : '\n');
            }
        }
    }
    /*
   * Initialize the raw data buffers
   */
    nbuf = approx_ring_buf_size/(data->info->buf_size*data->info->nchan*sizeof(float));
    data->ring = mne_initialize_ring(nbuf);
    /*
   * Initialize the filter buffers
   */
    data->filter  = MALLOC(1,mneFilterDefRec);
    *data->filter = *filter;
    setup_filter_bufs(data);

    {
        float **vals = ALLOC_CMATRIX(data->info->nchan,1);

        if (mne_raw_pick_data(data,NULL,data->first_samp,1,vals) == FAIL)
            goto bad;
        data->first_sample_val = MALLOC(data->info->nchan,float);
        for (k = 0; k < data->info->nchan; k++)
            data->first_sample_val[k] = vals[k][0];
        FREE_CMATRIX(vals);
        fprintf(stderr,"Initial dc offsets determined\n");
    }
    fprintf(stderr,"Raw data file %s:\n",name);
    fprintf(stderr,"\tnchan  = %d\n",data->info->nchan);
    fprintf(stderr,"\tnsamp  = %d\n",data->nsamp);
    fprintf(stderr,"\tsfreq  = %-8.3f Hz\n",data->info->sfreq);
    fprintf(stderr,"\tlength = %-8.3f sec\n",data->nsamp/data->info->sfreq);

    return data;

bad : {
        if (data)
            mne_raw_free_data(data);
        else
            mne_free_raw_info(info);

        return NULL;
    }
}

mneRawData mne_raw_open_file(char *name, int omit_skip, int allow_maxshield, mneFilterDef filter)
/*
 * Wrapper for mne_raw_open_file to work as before
 */
{
    return mne_raw_open_file_comp(name,omit_skip,allow_maxshield,filter,-1);
}



//============================= mne_read_data.c =============================

MneMeasData* mne_read_meas_data_add(const QString&       name,       /* Name of the measurement file */
                                   int                  set,        /* Which data set */
                                   mneInverseOperator   op,         /* For consistency checks */
                                   MneNamedMatrix*       fwd,        /* Another option for consistency checks */
                                   char                 **namesp,   /* Yet another option: explicit name list */
                                   int                  nnamesp,
                                   MneMeasData*          add_to)     /* Add to this */
/*
      * Read an evoked-response data file
      */
{
    /*
   * Data read from the file
   */
    fiffChInfo     chs = NULL;
    int            nchan_file,nsamp;
    float          dtmin,dtmax,sfreq;
    char           *comment = NULL;
    float          **data   = NULL;
    float          lowpass,highpass;
    int            nave;
    int            aspect_kind;
    fiffId         id = NULL;
    fiffCoordTrans t = NULL;
    fiffTime       meas_date = NULL;
    const char    *stim14_name;
    /*
   * Desired channels
   */
    char        **names = NULL;
    int         nchan   = 0;
    /*
   * Selected channels
   */
    int         *sel   = NULL;
    int         stim14 = -1;
    /*
   * Other stuff
   */
    float       *source,tmin,tmax;
    int         k,p,c,np,n1,n2;
    MneMeasData*    res = NULL;
    MneMeasData*    new_data = add_to;
    MneMeasDataSet* dataset = NULL;

    stim14_name = getenv(MNE_ENV_TRIGGER_CH);
    if (!stim14_name || strlen(stim14_name) == 0)
        stim14_name = MNE_DEFAULT_TRIGGER_CH;

    if (add_to)
        mne_channel_names_to_name_list(add_to->chs,add_to->nchan,&names,&nchan);
    else {
        if (op) {
            names = op->eigen_fields->collist;
            nchan = op->nchan;
        }
        else if (fwd) {
            names = fwd->collist;
            nchan = fwd->ncol;
        }
        else {
            names = namesp;
            nchan = nnamesp;
        }
        if (!names)
            nchan = 0;
    }
    /*
   * Read the evoked data file
   */
    if (mne_read_evoked(name,set-1,
                        &nchan_file,&nsamp,&dtmin,&sfreq,&chs,&data,
                        &comment,&highpass,&lowpass,&nave,&aspect_kind,&t,&id,&meas_date) == FAIL)
        goto out;

    if (id)
        printf("\tMeasurement file id: %s\n",mne_format_file_id(id));

#ifdef FOO
    if (add_to) {			/* Should add consistency check here */
        fprintf(stderr,"\tWarning: data set consistency check is still in the works.\n");
    }
#endif
    /*
   * Pick out the necessary channels
   */
    if (nchan > 0) {
        sel     = MALLOC(nchan,int);
        for (k = 0; k < nchan; k++)
            sel[k] = -1;
        for (c = 0; c < nchan_file; c++) {
            for (k = 0; k < nchan; k++) {
                if (sel[k] == -1 && strcmp(chs[c].ch_name,names[k]) == 0) {
                    sel[k] = c;
                    break;
                }
            }
            if (strcmp(stim14_name,chs[c].ch_name) == 0) {
                stim14 = c;
            }
        }
        for (k = 0; k < nchan; k++)
            if (sel[k] == -1) {
                printf("All channels needed were not in the MEG/EEG data file "
                       "(first missing: %s).",names[k]);
                goto out;
            }
    }
    else {			/* Load all channels */
        sel = MALLOC(nchan_file,int);
        for (c = 0, nchan = 0; c < nchan_file; c++) {
            if (chs[c].kind == FIFFV_MEG_CH || chs[c].kind == FIFFV_EEG_CH) {
                sel[nchan] = c;
                nchan++;
            }
            if (strcmp(stim14_name,chs[c].ch_name) == 0) {
                stim14 = c;
            }
        }
    }
    /*
   * Cut the data to the analysis time range
   */
    n1    = 0;
    n2    = nsamp;
    np    = n2 - n1;
    dtmax = dtmin + (np-1)/sfreq;
    /*
   * Then the analysis time range
   */
    tmin = dtmin;
    tmax = dtmax;
    fprintf(stderr,"\tData time range: %8.1f ... %8.1f ms\n",1000*tmin,1000*tmax);
    /*
   * Just put it together
   */
    if (!new_data) {			/* We need a new meas data structure */
        new_data     = new MneMeasData;
        new_data->filename  = mne_strdup(name.toLatin1().data());
        new_data->meas_id   = id; id = NULL;
        /*
     * Getting starting time from measurement ID is not too accurate...
     */
        if (meas_date)
            new_data->meas_date = *meas_date;
        else {
            if (new_data->meas_id)
                new_data->meas_date = new_data->meas_id->time;
            else {
                new_data->meas_date.secs = 0;
                new_data->meas_date.usecs = 0;
            }
        }
        new_data->lowpass   = lowpass;
        new_data->highpass  = highpass;
        new_data->chs       = MALLOC(nchan,fiffChInfoRec);
        new_data->nchan     = nchan;
        new_data->sfreq     = sfreq;

        if (t) {
            new_data->meg_head_t    = t;
            t = NULL;
            fprintf(stderr,"\tUsing MEG <-> head transform from the present data set\n");
        }
        if (op != NULL && op->mri_head_t != NULL) { /* Copy if available */
            if (!new_data->mri_head_t)
                new_data->mri_head_t    = MALLOC(1,fiffCoordTransRec);
            *(new_data->mri_head_t) = *(op->mri_head_t);
            fprintf(stderr,"\tPicked MRI <-> head transform from the inverse operator\n");
        }
        /*
     * Channel list
     */
        for (k = 0; k < nchan; k++)
            new_data->chs[k] = chs[sel[k]];

        new_data->op  = op;		/* Attach inverse operator */
        new_data->fwd = fwd;		/* ...or a fwd operator */
        if (op) 			/* Attach the projection operator and CTF compensation info to the data, too */
            new_data->proj = mne_dup_proj_op(op->proj);
        else {
            new_data->proj = mne_read_proj_op(name);
            if (new_data->proj && new_data->proj->nitems > 0) {
                fprintf(stderr,"\tLoaded projection from %s:\n",name.toLatin1().data());
                mne_proj_op_report(stderr,"\t\t",new_data->proj);
            }
            new_data->comp = mne_read_ctf_comp_data(name);
            if (new_data->comp == NULL)
                goto out;
            if (new_data->comp->ncomp > 0)
                fprintf(stderr,"\tRead %d compensation data sets from %s\n",new_data->comp->ncomp,name.toLatin1().data());
        }
        /*
     * Th bad channel stuff
     */
        {
            int b;

            new_data->bad = MALLOC(new_data->nchan,int);
            for (k = 0; k < new_data->nchan; k++)
                new_data->bad[k] = FALSE;

            if (mne_read_bad_channel_list(name,&new_data->badlist,&new_data->nbad) == OK) {
                for (b = 0; b < new_data->nbad; b++) {
                    for (k = 0; k < new_data->nchan; k++) {
                        if (strcasecmp(new_data->chs[k].ch_name,new_data->badlist[b]) == 0) {
                            new_data->bad[k] = TRUE;
                            break;
                        }
                    }
                }
                fprintf(stderr,"\t%d bad channels read from %s%s",new_data->nbad,name.toLatin1().data(),new_data->nbad > 0 ? ":\n" : "\n");
                if (new_data->nbad > 0) {
                    fprintf(stderr,"\t\t");
                    for (k = 0; k < new_data->nbad; k++)
                        fprintf(stderr,"%s%c",new_data->badlist[k],k < new_data->nbad-1 ? ' ' : '\n');
                }
            }
        }
    }
    /*
   * New data set is created anyway
   */
    dataset = new MneMeasDataSet;
    dataset->tmin      = tmin;
    dataset->tstep     = 1.0/sfreq;
    dataset->first     = n1;
    dataset->np        = np;
    dataset->nave      = nave;
    dataset->kind      = aspect_kind;
    dataset->data      = ALLOC_CMATRIX(np,nchan);
    dataset->comment   = comment;    comment = NULL;
    dataset->baselines = MALLOC(nchan,float);
    /*
   * Pick data from all channels
   */
    for (k = 0; k < nchan; k++) {
        source = data[sel[k]];
        /*
     * Shift the response
     */
        for (p = 0; p < np; p++)
            dataset->data[p][k] = source[p+n1];
        dataset->baselines[k] = 0.0;
    }
    /*
   * Pick the digital trigger channel, too
   */
    if (stim14 >= 0) {
        dataset->stim14 = MALLOC(np,float);
        source = data[stim14];
        for (p = 0; p < np; p++) 	/* Copy the data and correct for the possible non-unit calibration */
            dataset->stim14[p] = source[p+n1]/chs[stim14].cal;
    }
    new_data->sets.append(dataset); dataset = NULL;
    new_data->nset++;
    if (!add_to)
        new_data->current = new_data->sets[0];
    res = new_data;
    fprintf(stderr,"\t%s dataset %s from %s\n",
            add_to ? "Added" : "Loaded",
            new_data->sets[new_data->nset-1]->comment ? new_data->sets[new_data->nset-1]->comment : "unknown",name.toLatin1().data());

out : {
        FREE(sel);
        FREE(comment);
        FREE_CMATRIX(data);
        FREE(chs);
        FREE(t);
        FREE(id);
        if (res == NULL && !add_to)
            delete new_data;
        if (add_to)
            mne_free_name_list(names,nchan);
        return res;
    }
}


MneMeasData* mne_read_meas_data(const QString&       name,       /* Name of the measurement file */
                               int                  set,        /* Which data set */
                               mneInverseOperator   op,         /* For consistency checks */
                               MneNamedMatrix*       fwd,        /* Another option for consistency checks */
                               char                 **namesp,   /* Yet another option: explicit name list */
                               int                  nnamesp)

{
    return mne_read_meas_data_add(name,set,op,fwd,namesp,nnamesp,NULL);
}



#endif // DIPOLEFITHELPERS_H
