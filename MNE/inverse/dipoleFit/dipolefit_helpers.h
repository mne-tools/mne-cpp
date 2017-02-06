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

#include <fiff/fiff_dir_node.h>


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


#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
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
#define ALLOC_COMPLEX_DCMATRIX(x,y) mne_complex_dmatrix((x),(y))


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



//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info(const QString& name,
                                  fiffChInfo     *megp,	 /* MEG channels */
                                  int            *nmegp,
                                  fiffChInfo     *meg_compp,
                                  int            *nmeg_compp,
                                  fiffChInfo     *eegp,	 /* EEG channels */
                                  int            *neegp,
                                  FiffCoordTransOld* *meg_head_t,
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
    FiffCoordTransOld* t = NULL;
    fiff_int_t kind, pos;
    int j,k,to_find;

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
            t = FiffCoordTransOld::read_helper( t_pTag );
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
        if ((t = FiffCoordTransOld::mne_read_meas_transform(name)) == NULL) {
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






//============================= mne_filename_util.c =============================


//char *mne_compose_mne_name(const char *path, const char *filename)
///*
//      * Compose a filename under the "$MNE_ROOT" directory
//      */
//{
//    char *res;
//    char *mne_root;

//    if (filename == NULL) {
//        qCritical("No file name specified to mne_compose_mne_name");
//        return NULL;
//    }
//    mne_root = getenv(MNE_ENV_ROOT);
//    if (mne_root == NULL || strlen(mne_root) == 0) {
//        qCritical("Environment variable MNE_ROOT not set");
//        return NULL;
//    }
//    if (path == NULL || strlen(path) == 0) {
//        res = MALLOC(strlen(mne_root)+strlen(filename)+2,char);
//        strcpy(res,mne_root);
//        strcat(res,"/");
//        strcat(res,filename);
//    }
//    else {
//        res = MALLOC(strlen(mne_root)+strlen(filename)+strlen(path)+3,char);
//        strcpy(res,mne_root);
//        strcat(res,"/");
//        strcat(res,path);
//        strcat(res,"/");
//        strcat(res,filename);
//    }
//    return res;
//}




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

mneCTFcompDataSet mne_new_ctf_comp_data_set()
{
    mneCTFcompDataSet res = MALLOC(1,mneCTFcompDataSetRec);

//    res->comps   = NULL;
    res->ncomp   = 0;
    res->chs     = NULL;
    res->nch     = 0;
    res->current = NULL;
    res->undo    = NULL;
    return res;
}


void mne_free_ctf_comp_data_set(mneCTFcompDataSet set)

{
    int k;

    if (!set)
        return;

    for (k = 0; k < set->ncomp; k++)
        if(set->comps[k])
            delete set->comps[k];
//    FREE(set->comps);
    set->comps.clear();
    FREE(set->chs);
    if(set->current)
        delete set->current;
    FREE(set);
    return;
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
//        res->comps = MALLOC(set->ncomp,mneCTFcompData);
        res->ncomp = set->ncomp;
        for (k = 0; k < res->ncomp; k++)
            if(set->comps[k])
                res->comps.append(new MneCTFCompData(*set->comps[k]));
    }

    if(set->current)
        res->current = new MneCTFCompData(*set->current);

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
    MneCTFCompData* this_comp;
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



mneCTFcompDataSet mne_read_ctf_comp_data(const QString& name)
/*
* Read all CTF compensation data from a given file
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    mneCTFcompDataSet set = NULL;
    MneCTFCompData*    one;
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
        one = new MneCTFCompData;
        one->data = mat; mat = NULL;
        one->kind                = kind;
        one->mne_kind            = mne_unmap_ctf_comp_kind(one->kind);
        one->calibrated          = calibrated;

        if (MneCTFCompData::mne_calibrate_ctf_comp(one,set->chs,set->nch,TRUE) == FAIL) {
            printf("Warning: Compensation data for '%s' omitted\n", mne_explain_ctf_comp(one->kind));//,err_get_error(),mne_explain_ctf_comp(one->kind));
            if(one)
                delete one;
        }
        else {
//            set->comps               = REALLOC(set->comps,set->ncomp+1,mneCTFcompData);
//            set->comps[set->ncomp++] = one;
            set->comps.append(one);
            set->ncomp++;
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
    MneCTFCompData* this_comp;
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
        if(set->current)
            delete set->current;
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
    set->current           = new MneCTFCompData();
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
    MneCTFCompData* this_comp;
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


//int mne_ctf_compensate_to(mneCTFcompDataSet set,            /* The compensation data */
//                          int               compensate_to,  /* What is the desired compensation to achieve */
//                          fiffChInfo        chs,            /* The channels to compensate */
//                          int               nchan,          /* How many? */
//                          fiffChInfo        comp_chs,       /* Maybe a different set, defaults to the same */
//                          int               ncomp_chan,     /* How many */
//                          float             **data,         /* The data in a np x nchan matrix allocated with ALLOC_CMATRIX(np,nchan) */
//                          float             **comp_data,    /* The compensation data in a np x ncomp_chan matrix, defaults to data */
//                          int               np)             /* How many time points */
///*
//* Make data which has the third-order gradient compensation applied
//*/
//{
//    int k;
//    int have_comp_chs;
//    int comp_was = MNE_CTFV_COMP_UNKNOWN;

//    if (!comp_data)
//        comp_data = data;
//    if (!comp_chs) {
//        comp_chs = chs;
//        ncomp_chan = nchan;
//    }
//    if (set) {
//        mne_free_ctf_comp_data(set->undo); set->undo = NULL;
//        mne_free_ctf_comp_data(set->current); set->current = NULL;
//    }
//    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
//        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
//            have_comp_chs++;
//    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
//        printf("No compensation channels in these data.");
//        return FAIL;
//    }
//    /*
//    * Update the 'current' field in 'set' to reflect the compensation possibly present in the data now
//    */
//    if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
//        goto bad;
//    /*
//    * Are we there already?
//    */
//    if (set->current && set->current->mne_kind == compensate_to) {
//        fprintf(stderr,"The data were already compensated as desired (%s)\n",mne_explain_ctf_comp(set->current->kind));
//        return OK;
//    }
//    /*
//    * Undo any previous compensation
//    */
//    for (k = 0; k < np; k++)
//        if (mne_apply_ctf_comp(set,FALSE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
//            goto bad;
//    if (set->current)
//        fprintf(stderr,"The previous compensation (%s) is now undone\n",mne_explain_ctf_comp(set->current->kind));
//    /*
//    * Set to new gradient compensation
//    */
//    if (compensate_to == MNE_CTFV_NOGRAD) {
//        mne_set_ctf_comp(chs,nchan,compensate_to);
//        fprintf(stderr,"No compensation was requested. Original data have been restored.\n");
//    }
//    else {
//        if (mne_set_ctf_comp(chs,nchan,compensate_to) > 0) {
//            if (set->current)
//                comp_was = set->current->mne_kind;
//            if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
//                goto bad;
//            /*
//            * Do the third-order gradient compensation
//            */
//            for (k = 0; k < np; k++)
//                if (mne_apply_ctf_comp(set,TRUE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
//                    goto bad;
//            if (set->current)
//                fprintf(stderr,"The data are now compensated as requested (%s).\n",mne_explain_ctf_comp(set->current->kind));
//        }
//        else
//            fprintf(stderr,"No MEG channels to compensate.\n");
//    }
//    return OK;

//bad : {
//        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
//            mne_set_ctf_comp(chs,nchan,comp_was);
//        return FAIL;
//    }
//}


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
        if(set->undo)
            delete set->undo;
        set->undo = NULL;
        if(set->current)
            delete set->current;
        set->current = NULL;
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
        if(set->current)
            delete set->current;
        set->current = NULL;
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
                          FiffCoordTransOld* *trans, /* Coordinate transformation
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
    FiffCoordTransOld* t;
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
            t = FiffCoordTransOld::read_helper( t_pTag );
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
                t = FiffCoordTransOld::read_helper( t_pTag );
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
    FiffCoordTransOld* trans    = NULL;	/* The coordinate transformation */
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

    if(d->proj)
        delete d->proj;
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
    MneCTFCompData* temp;

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

    if (!data->proj || (sel && !MneProjOp::mne_proj_op_affect(data->proj,sel->chspick,sel->nchan) && !MneProjOp::mne_proj_op_affect(data->proj,sel->chspick_nospace,sel->nchan)))
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
                    if (MneProjOp::mne_proj_op_proj_vector(data->proj,pvalues,data->info->nchan,TRUE) != OK)
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
            if (MneProjOp::mne_proj_op_proj_vector(data->proj,dc,data->info->nchan,TRUE) != OK)
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




#endif // DIPOLEFITHELPERS_H
