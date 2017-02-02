
#include "fwd_types.h"


#include "dipole_fit_data.h"
#include "guess_data.h"
#include "mne_meas_data.h"
#include "mne_meas_data_set.h"
#include "ecd.h"

#include <Eigen/Dense>


#include <QFile>



using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;



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



#define X_3 0
#define Y_3 1
#define Z_3 2


#define MALLOC_3(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_3(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE_3(x) if ((char *)(x) != NULL) free((char *)(x))

/*
 * Float, double, and int arrays
 */
#define ALLOC_FLOAT_3(x) MALLOC_3(x,float)




#define ALLOC_CMATRIX_3(x,y) mne_cmatrix_3((x),(y))
#define FREE_CMATRIX_3(m) mne_free_cmatrix_3((m))


/*
 * Dot product and length
 */
#define VEC_DOT_3(x,y) ((x)[X_3]*(y)[X_3] + (x)[Y_3]*(y)[Y_3] + (x)[Z_3]*(y)[Z_3])
#define VEC_LEN_3(x) sqrt(VEC_DOT_3(x,x))


/*
 * Others...
 */

#define VEC_DIFF_3(from,to,diff) {\
    (diff)[X_3] = (to)[X_3] - (from)[X_3];\
    (diff)[Y_3] = (to)[Y_3] - (from)[Y_3];\
    (diff)[Z_3] = (to)[Z_3] - (from)[Z_3];\
    }

#define VEC_COPY_3(to,from) {\
    (to)[X_3] = (from)[X_3];\
    (to)[Y_3] = (from)[Y_3];\
    (to)[Z_3] = (from)[Z_3];\
    }

#define CROSS_PRODUCT_3(x,y,xy) {\
    (xy)[X_3] =   (x)[Y_3]*(y)[Z_3]-(y)[Y_3]*(x)[Z_3];\
    (xy)[Y_3] = -((x)[X_3]*(y)[Z_3]-(y)[X_3]*(x)[Z_3]);\
    (xy)[Z_3] =   (x)[X_3]*(y)[Y_3]-(y)[X_3]*(x)[Y_3];\
    }







//============================= mne_matop.c =============================




#define MIN_3(a,b) ((a) < (b) ? (a) : (b))




void mne_transpose_square_3(float **mat, int n)
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



float mne_dot_vectors_3(float *v1,
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



void mne_add_scaled_vector_to_3(float *v1,float scale, float *v2,int nn)

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










typedef struct {
    float          limit;
    int            report_dim;
    float          *B;
    double         B2;
    DipoleForward*  fwd;
} *fitDipUser,fitDipUserRec;





int mne_is_diag_cov_3(mneCovMatrix c)

{
    return c->cov_diag != NULL;
}






void mne_scale_vector_3 (double scale,float *v,int   nn)

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









//float
Eigen::MatrixXf toFloatEigenMatrix_3(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}


void fromFloatEigenMatrix_3(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_3(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_3(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}





void fromFloatEigenVector_3(const Eigen::VectorXf& from_vec, float *to_vec, const int n)
{
    for ( int i = 0; i < n; ++i)
        to_vec[i] = from_vec[i];
}





float **mne_lu_invert_3(float **mat,int dim)
/*
      * Invert a matrix using the LU decomposition from
      * LAPACK
      */
{
    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_3(mat, dim, dim);
    Eigen::MatrixXf eigen_mat_inv = eigen_mat.inverse();
    fromFloatEigenMatrix_3(eigen_mat_inv, mat);
    return mat;
}












static void matrix_error_3(int kind, int nr, int nc)

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



float **mne_cmatrix_3 (int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_3(nr,float *);
    if (!m) matrix_error_3(1,nr,nc);
    whole = MALLOC_3(nr*nc,float);
    if (!whole) matrix_error_3(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}



void mne_free_cmatrix_3 (float **m)
{
    if (m) {
        FREE_3(*m);
        FREE_3(m);
    }
}






int mne_svd_3(float **mat,	/* The matrix */
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
      * mat = ALLOC_CMATRIX_3(m,n);
      * vv  = ALLOC_CMATRIX_3(MIN(m,n),n);
      * uu  = ALLOC_CMATRIX_3(MIN(m,n),m);
      * sing = MALLOC_3(MIN(m,n),float);
      *
      * mat is modified by this operation
      *
      * This simply allocates the workspace and calls the
      * LAPACK Fortran routine
      */

{
    int    udim = MIN_3(m,n);

    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_3(mat, m, n);

    //ToDo Optimize computation depending of whether uu or vv are defined
    Eigen::JacobiSVD< Eigen::MatrixXf > svd(eigen_mat ,Eigen::ComputeFullU | Eigen::ComputeFullV);

    fromFloatEigenVector_3(svd.singularValues(), sing, svd.singularValues().size());

    if (uu != NULL)
        fromFloatEigenMatrix_3(svd.matrixU().transpose(), uu, udim, m);

    if (vv != NULL)
        fromFloatEigenMatrix_3(svd.matrixV().transpose(), vv, m, n);

    return 0;
    //  return info;
}







void mne_free_name_list_3(char **list, int nlist)
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
        FREE_3(list[k]);
    }
    FREE_3(list);
    return;
}







void mne_free_proj_op_proj_3(mneProjOp op)

{
    if (op == NULL)
        return;

    mne_free_name_list_3(op->names,op->nch);
    FREE_CMATRIX_3(op->proj_data);

    op->names  = NULL;
    op->nch  = 0;
    op->nvec = 0;
    op->proj_data = NULL;

    return;
}


void mne_free_proj_op_item_3(mneProjItem it)

{
    if (it == NULL)
        return;

    if(it->vecs)
        delete it->vecs;

    FREE_3(it->desc);
    FREE_3(it);
    return;
}


void mne_free_proj_op_3(mneProjOp op)

{
    int k;

    if (op == NULL)
        return;

    for (k = 0; k < op->nitems; k++)
        mne_free_proj_op_item_3(op->items[k]);
    FREE_3(op->items);

    mne_free_proj_op_proj_3(op);

    FREE_3(op);
    return;
}





void mne_free_cov_3(mneCovMatrix c)
/*
* Free a covariance matrix and all its data
*/
{
    if (c == NULL)
        return;
    FREE_3(c->cov);
    FREE_3(c->cov_diag);
    if(c->cov_sparse)
        delete c->cov_sparse;
    mne_free_name_list_3(c->names,c->ncov);
    FREE_CMATRIX_3(c->eigen);
    FREE_3(c->lambda);
    FREE_3(c->inv_lambda);
    FREE_3(c->chol);
    FREE_3(c->ch_class);
    mne_free_proj_op_3(c->proj);
    if (c->sss)
        delete c->sss;
    mne_free_name_list_3(c->bads,c->nbad);
    FREE_3(c);
    return;
}










#define EPS_3 0.05

int mne_get_values_from_data_3 (float time,         /* Interesting time point */
                              float integ,	  /* Time integration */
                              float **data,	  /* The data values (time by time) */
                              int   nsamp,	  /* How many time points? */
                              int   nch,          /* How many channels */
                              float tmin,	  /* Time of first sample */
                              float sfreq,	  /* Sampling frequency */
                              int   use_abs,      /* Use absolute values */
                              float *value)	  /* The picked values */
/*
      * Pick a signal value using linear interpolation
      */
{
    int   n1,n2,k;
    float s1,s2;
    float f1,f2;
    float sum;
    float width;
    int   ch;

    for (ch = 0; ch < nch; ch++) {
        /*
     * Find out the correct samples
     */
        if (fabs(sfreq*integ) < EPS_3) { /* This is the single-sample case */
            s1 = sfreq*(time - tmin);
            n1 = floor(s1);
            f1 = 1.0 + n1 - s1;
            if (n1 < 0 || n1 > nsamp-1) {
                printf("Sample value out of range %d (0..%d)",n1,nsamp-1);
                return(-1);
            }
            /*
       * Avoid rounding error
       */
            if (n1 == nsamp-1) {
                if (fabs(f1-1.0) < 1e-3)
                    f1 = 1.0;
            }
            if (f1 < 1.0 && n1 > nsamp-2) {
                printf("Sample value out of range %d (0..%d) %.4f",n1,nsamp-1,f1);
                return(-1);
            }
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1*fabs(data[n1][ch]) + (1.0-f1)*fabs(data[n1+1][ch]);
                else
                    sum = f1*data[n1][ch] + (1.0-f1)*data[n1+1][ch];
            }
            else {
                if (use_abs)
                    sum = fabs(data[n1][ch]);
                else
                    sum = data[n1][ch];
            }
        }
        else {			/* Multiple samples */
            s1 = sfreq*(time - 0.5*integ - tmin);
            s2 = sfreq*(time + 0.5*integ - tmin);
            n1 = ceil(s1); n2 = floor(s2);
            if (n2 < n1) {		/* We are within one sample interval */
                n1 = floor(s1);
                if (n1 < 0 || n1 > nsamp-2)
                    return (-1);
                f1 = s1 - n1;
                f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5*((f1+f2)*fabs(data[n1+1][ch]) + (2.0-f1-f2)*fabs(data[n1][ch]));
                else
                    sum = 0.5*((f1+f2)*data[n1+1][ch] + (2.0-f1-f2)*data[n1][ch]);
            }
            else {
                f1 = n1 - s1;
                f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp-1) {
                    printf("Sample value out of range %d (0..%d)",n1,nsamp-1);
                    return(-1);
                }
                if (n2 < 0 || n2 > nsamp-1) {
                    printf("Sample value out of range %d (0..%d)",n2,nsamp-1);
                    return(-1);
                }
                if (f1 != 0.0 && n1 < 1)
                    return(-1);
                if (f2 != 0.0 && n2 > nsamp-2)
                    return(-1);
                sum = 0.0;
                width = 0.0;
                if (n2 > n1) {		/* Do the whole intervals */
                    if (use_abs) {
                        sum = 0.5*fabs(data[n1][ch]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + fabs(data[k][ch]);
                        sum = sum + 0.5*fabs(data[n2][ch]);
                    }
                    else {
                        sum = 0.5*data[n1][ch];
                        for (k = n1+1; k < n2; k++)
                            sum = sum + data[k][ch];
                        sum = sum + 0.5*data[n2][ch];
                    }
                    width = n2 - n1;
                }
                /*
         * Add tails
         */
                if (use_abs) {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*fabs(data[n1-1][ch]) + (2.0-f1)*fabs(data[n1][ch]));
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*fabs(data[n2+1][ch]) + (2.0-f2)*fabs(data[n2][ch]));
                }
                else {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*data[n1-1][ch] + (2.0-f1)*data[n1][ch]);
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*data[n2+1][ch] + (2.0-f2)*data[n2][ch]);
                }
                width = width + f1 + f2;
                sum = sum/width;
            }
        }
        value[ch] = sum;
    }
    return (0);
}















int mne_proj_op_proj_vector_3(mneProjOp op, float *vec, int nvec, int do_complement)
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
        res = REALLOC_3(res,op->nch,float);
        res_size = op->nch;
    }

    for (k = 0; k < op->nch; k++)
        res[k] = 0.0;

    for (p = 0; p < op->nvec; p++) {
        pvec = op->proj_data[p];
        w = mne_dot_vectors_3(pvec,vec,op->nch);
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








//============================= mne_whiten.c =============================

int mne_whiten_data(float **data, float **whitened_data, int np, int nchan, mneCovMatrix C)
/*
      * Apply the whitening operation
      */
{
    int    j,k;
    float  *one = NULL,*orig,*white;
    double *inv;

    if (data == NULL || np <= 0)
        return OK;

    if (C->ncov != nchan) {
        printf("Incompatible covariance matrix. Cannot whiten the data.");
        return FAIL;
    }
    inv = C->inv_lambda;
    if (mne_is_diag_cov_3(C)) {
        //    printf("<DEBUG> Performing Diag\n");
        for (j = 0; j < np; j++) {
            orig = data[j];
            white = whitened_data[j];
            for (k = 0; k < nchan; k++)
                white[k] = orig[k]*inv[k];
        }
    }
    else {
        /*
     * This is arranged so that whitened_data can be the same matrix as the original
     */
        one = MALLOC_3(nchan,float);
        for (j = 0; j < np; j++) {
            orig = data[j];
            white = whitened_data[j];
            for (k = C->nzero; k < nchan; k++)
                one[k] = mne_dot_vectors_3(C->eigen[k],orig,nchan);
            for (k = 0; k < C->nzero; k++)
                white[k] = 0.0;
            for (k = C->nzero; k < nchan; k++)
                white[k] = one[k]*inv[k];
        }
        FREE_3(one);
    }
    return OK;
}


int mne_whiten_one_data(float *data, float *whitened_data, int nchan, mneCovMatrix C)

{
    float *datap[1];
    float *whitened_datap[1];

    datap[0] = data;
    whitened_datap[0] = whitened_data;

    return mne_whiten_data(datap,whitened_datap,1,nchan,C);
}












//*************************************************************************************************************
//============================= dipole_fit_setup.c =============================
static void free_dipole_fit_funcs(dipoleFitFuncs f)

{
    if (!f)
        return;

    if (f->meg_client_free && f->meg_client)
        f->meg_client_free(f->meg_client);
    if (f->eeg_client_free && f->eeg_client)
        f->eeg_client_free(f->eeg_client);

    FREE_3(f);
    return;
}





//============================= mne_cov_matrix.c =============================




















static int mne_lt_packed_index_3(int j, int k)

{
    if (j >= k)
        return k + j*(j+1)/2;
    else
        return j + k*(k+1)/2;
}





//static void regularize_cov(mneCovMatrix c,       /* The matrix to regularize */
//                           float        *regs,   /* Regularization values to apply (fractions of the
//                                                     * average diagonal values for each class */
//                           int          *active) /* Which of the channels are 'active' */
///*
//      * Regularize different parts of the noise covariance matrix differently
//      */
//{
//    int    j;
//    float  sums[3],nn[3];
//    int    nkind = 3;

//    if (!c->cov || !c->ch_class)
//        return;

//    for (j = 0; j < nkind; j++) {
//        sums[j] = 0.0;
//        nn[j]   = 0;
//    }
//    /*
//   * Compute the averages over the diagonal elements for each class
//   */
//    for (j = 0; j < c->ncov; j++) {
//        if (c->ch_class[j] >= 0) {
//            if (!active || active[j]) {
//                sums[c->ch_class[j]] += c->cov[mne_lt_packed_index(j,j)];
//                nn[c->ch_class[j]]++;
//            }
//        }
//    }
//    printf("Average noise-covariance matrix diagonals:\n");
//    for (j = 0; j < nkind; j++) {
//        if (nn[j] > 0) {
//            sums[j] = sums[j]/nn[j];
//            if (j == MNE_COV_CH_MEG_MAG)
//                printf("\tMagnetometers       : %-7.2f fT    reg = %-6.2f\n",1e15*sqrt(sums[j]),regs[j]);
//            else if (j == MNE_COV_CH_MEG_GRAD)
//                printf("\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f\n",1e13*sqrt(sums[j]),regs[j]);
//            else
//                printf("\tEEG                 : %-7.2f uV    reg = %-6.2f\n",1e6*sqrt(sums[j]),regs[j]);
//            sums[j] = regs[j]*sums[j];
//        }
//    }
//    /*
//   * Add thee proper amount to the diagonal
//   */
//    for (j = 0; j < c->ncov; j++)
//        if (c->ch_class[j] >= 0)
//            c->cov[mne_lt_packed_index(j,j)] += sums[c->ch_class[j]];

//    printf("Noise-covariance regularized as requested.\n");
//    return;
//}


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
            sums[c->ch_class[j]] += c->cov[mne_lt_packed_index_3(j,j)];
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
            c->cov[mne_lt_packed_index_3(j,j)] += sums[c->ch_class[j]];

    fprintf(stderr,"Noise-covariance regularized as requested.\n");
    return;
}




//============================= dipole_fit_setup.c =============================


static dipoleFitFuncs new_dipole_fit_funcs()

{
    dipoleFitFuncs f = MALLOC_3(1,dipoleFitFuncsRec);

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






//============================= misc_util.c =============================

char *mne_strdup_3(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}































//============================= fwd_bem_model.c =============================

static struct {
    int  kind;
    const char *name;
} surf_expl[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };


const char *fwd_bem_explain_surface(int kind)

{
    int k;

    for (k = 0; surf_expl[k].kind >= 0; k++)
        if (surf_expl[k].kind == kind)
            return surf_expl[k].name;

    return surf_expl[k].name;
}



static struct {
    int  method;
    const char *name;
} method_expl[] = { { FWD_BEM_CONSTANT_COLL , "constant collocation" },
{ FWD_BEM_LINEAR_COLL   , "linear collocation" },
{ -1                    , "unknown" } };


const char *fwd_bem_explain_method(int method)

{
    int k;

    for (k = 0; method_expl[k].method >= 0; k++)
        if (method_expl[k].method == method)
            return method_expl[k].name;

    return method_expl[k].name;
}










fwdBemModel fwd_bem_new_model()

{
    fwdBemModel m = MALLOC_3(1,fwdBemModelRec);

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




void fwd_bem_free_solution_3(fwdBemModel m)

{
    if (!m)
        return;
    FREE_CMATRIX_3(m->solution); m->solution = NULL;
    FREE_3(m->sol_name); m->sol_name = NULL;
    FREE_3(m->v0); m->v0 = NULL;
    m->bem_method = FWD_BEM_UNKNOWN;
    m->nsol       = 0;

    return;
}






void fwd_bem_free_model_3(fwdBemModel m)

{
    int k;

    if (!m)
        return;

    FREE_3(m->surf_name);
    for (k = 0; k < m->nsurf; k++)
        delete m->surfs[k];
    FREE_3(m->surfs);
    FREE_3(m->ntri);
    FREE_3(m->np);
    FREE_3(m->sigma);
    FREE_3(m->source_mult);
    FREE_3(m->field_mult);
    FREE_CMATRIX_3(m->gamma);
    if(m->head_mri_t)
        delete m->head_mri_t;
    fwd_bem_free_solution_3(m);

    FREE_3(m);
    return;
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

    surfs = MALLOC_3(nkind,MneSurfaceOrVolume::MneCSurface*);
    sigma = MALLOC_3(nkind,float);
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

    m->surf_name = mne_strdup_3(name);
    m->nsurf     = nkind;
    m->surfs     = surfs;
    m->sigma     = sigma;
    m->ntri      = MALLOC_3(nkind,int);
    m->np        = MALLOC_3(nkind,int);
    m->gamma = ALLOC_CMATRIX_3(nkind,nkind);
    m->source_mult = MALLOC_3(nkind,float);
    m->field_mult  = MALLOC_3(nkind,float);
    /*
   * Dirty trick for the zero conductivity outside
   */
    sigma1 = MALLOC_3(nkind+1,float);
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
    FREE_3(sigma1);

    return m;

bad : {
        FREE_3(sigma);
        for (k = 0; k < nkind; k++)
            delete surfs[k];
        FREE_3(surfs);
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
        sol = ALLOC_CMATRIX_3(tmp_sol.rows(),tmp_sol.cols());
        fromFloatEigenMatrix_3(tmp_sol, sol);
        nsol = dims[1];
    }
    fwd_bem_free_solution_3(m);
    m->sol_name = mne_strdup_3(name);
    m->solution = sol;
    m->nsol     = nsol;
    m->bem_method = method;
    stream->close();

    return TRUE;

bad : {
        stream->close();
        FREE_CMATRIX_3(sol);
        return FAIL;
    }

not_found : {
        stream->close();
        FREE_CMATRIX_3(sol);
        return FALSE;
    }
}




int fwd_bem_set_head_mri_t(fwdBemModel m, FiffCoordTransOld* t)
/*
* Set the coordinate transformation
*/
{
    if (t->from == FIFFV_COORD_HEAD && t->to == FIFFV_COORD_MRI) {
        if(m->head_mri_t)
            delete m->head_mri_t;
        m->head_mri_t = new FiffCoordTransOld( *t );
        return OK;
    }
    else if (t->from == FIFFV_COORD_MRI && t->to == FIFFV_COORD_HEAD) {
        if(m->head_mri_t)
            delete m->head_mri_t;
        m->head_mri_t = t->fiff_invert_transform();
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
        res = mne_strdup_3(s);
        *p = c;
    }
    else
        res = mne_strdup_3(s);
    return res;
}

#define BEM_SUFFIX     "-bem.fif"
#define BEM_SOL_SUFFIX "-bem-sol.fif"

//char *fwd_bem_make_bem_name(char *name)
///*
// * Make a standard BEM file name
// */
//{
//    char *s1,*s2;

//    s1 = strip_from(name,(char*)(".fif"));
//    s2 = strip_from(s1,(char*)("-sol"));
//    FREE_3(s1);
//    s1 = strip_from(s2,(char*)("-bem"));
//    FREE_3(s2);
//    s2 = MALLOC_3(strlen(s1)+strlen(BEM_SUFFIX)+1,char);
//    sprintf(s2,"%s%s",s1,BEM_SUFFIX);
//    FREE_3(s1);
//    return s2;
//}

char *fwd_bem_make_bem_sol_name(char *name)
/*
 * Make a standard BEM solution file name
 */
{
    char *s1,*s2;

    s1 = strip_from(name,(char*)(".fif"));
    s2 = strip_from(s1,(char*)("-sol"));
    FREE_3(s1);
    s1 = strip_from(s2,(char*)("-bem"));
    FREE_3(s2);
    s2 = MALLOC_3(strlen(s1)+strlen(BEM_SOL_SUFFIX)+1,char);
    sprintf(s2,"%s%s",s1,BEM_SOL_SUFFIX);
    FREE_3(s1);
    return s2;
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

    return (mne_lu_invert_3(solids,ntot));
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

    row = MALLOC_3(nlast,float);
    sub = MALLOC_3(ntot,float *);
    mult = (1.0 + ip_mult)/ip_mult;

    fprintf(stderr,"\t\tCombining...");
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square_3(ip_solution,nlast);
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
                row[k] = mne_dot_vectors_3(sub[j],ip_solution[k],nlast);
            mne_add_scaled_vector_to_3(row,-2.0,sub[j],nlast);
        }
#endif
        joff = joff+ntri[s];
    }
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square_3(ip_solution,nlast);
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
    mne_scale_vector_3(ip_mult,solution[0],ntot*ntot);
    fprintf(stderr,"done.\n");
    FREE_3(row); FREE_3(sub);
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

    VEC_DIFF_3(rk,rk1,rkk1);
    size = VEC_LEN_3(rkk1);

    res = log((VEC_LEN_3(rk)*size + VEC_DOT_3(rk,rkk1))/
              (VEC_LEN_3(rk1)*size + VEC_DOT_3(rk1,rkk1)))/size;
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
    double triple;		/* VEC_DOT_3(y1 x y2,y3) */
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
    VEC_DIFF_3(from,to->r1,y1);
    VEC_DIFF_3(from,to->r2,y2);
    VEC_DIFF_3(from,to->r3,y3);

    CROSS_PRODUCT_3(y1,y2,cross);
    triple = VEC_DOT_3(cross,y3);

    l1 = VEC_LEN_3(y1);
    l2 = VEC_LEN_3(y2);
    l3 = VEC_LEN_3(y3);
    ss = (l1*l2*l3+VEC_DOT_3(y1,y2)*l3+VEC_DOT_3(y1,y3)*l2+VEC_DOT_3(y2,y3)*l1);
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
            CROSS_PRODUCT_3(yy[k+1],yy[k-1],z);
            VEC_DIFF_3(yy[k+1],yy[k-1],diff);
            omega[k] = n2*(-area2*VEC_DOT_3(z,to->nn)*solid +
                           triple*VEC_DOT_3(diff,vec_omega));
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
    rel2 = sqrt(VEC_DOT_3(check,check)/VEC_DOT_3(vec_omega,vec_omega));
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

    mat = ALLOC_CMATRIX_3(np_tot,np_tot);
    for (j = 0; j < np_tot; j++)
        for (k = 0; k < np_tot; k++)
            mat[j][k] = 0.0;
    row        = MALLOC_3(np_max,double);
    sub_mat = MALLOC_3(np_max,float *);
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
    FREE_3(row);
    FREE_3(sub_mat);
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

    fwd_bem_free_solution_3(m);

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
        FREE_CMATRIX_3(ip_solution);

    }
    m->bem_method = FWD_BEM_LINEAR_COLL;
    fprintf(stderr,"Solution ready.\n");
    return OK;

bad : {
        fwd_bem_free_solution_3(m);
        FREE_CMATRIX_3(coeff);
        return FAIL;
    }
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

    VEC_DIFF_3(from,tri->r1,v1);
    VEC_DIFF_3(from,tri->r2,v2);
    VEC_DIFF_3(from,tri->r3,v3);

    CROSS_PRODUCT_3(v1,v2,cross);
    triple = VEC_DOT_3(cross,v3);

    l1 = VEC_LEN_3(v1);
    l2 = VEC_LEN_3(v2);
    l3 = VEC_LEN_3(v3);
    s = (l1*l2*l3+VEC_DOT_3(v1,v2)*l3+VEC_DOT_3(v1,v3)*l2+VEC_DOT_3(v2,v3)*l1);

    return (2.0*atan2(triple,s));
}




//============================= fwd_bem_constant_collocation.c =============================


static int fwd_bem_check_solids (float **angles,int ntri1,int ntri2, float desired)
/*
 * Check the angle computations
 */
{
    float *sums = MALLOC_3(ntri1,float);
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
    FREE_3(sums);
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

    sub_solids = MALLOC_3(ntri_tot,float *);
    solids = ALLOC_CMATRIX_3(ntri_tot,ntri_tot);
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
                FREE_CMATRIX_3(solids);
                FREE_3(sub_solids);
                return NULL;
            }
        }
    }
    FREE_3(sub_solids);
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

    fwd_bem_free_solution_3(m);

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
        FREE_CMATRIX_3(ip_solution);
    }
    m->bem_method = FWD_BEM_CONSTANT_COLL;
    fprintf (stderr,"Solution ready.\n");

    return OK;

bad : {
        fwd_bem_free_solution_3(m);
        FREE_CMATRIX_3(solids);
        return FAIL;
    }
}





//============================= fwd_bem_model.c =============================


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

    fwd_bem_free_solution_3(m);
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
        fwd_bem_free_solution_3(m);
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

    ptry = ALLOC_FLOAT_3(ndim);
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
    FREE_3(ptry);
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

    psum = ALLOC_FLOAT_3(ndim);
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
    FREE_3(psum);
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
        VEC_DIFF_3(r0,user->rr[k],diff);
        one = VEC_LEN_3(diff);
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
        VEC_DIFF_3(r0,user->rr[k],diff);
        one = VEC_LEN_3(diff);
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
            ave += VEC_LEN_3(diff);
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
    float **simplex = ALLOC_CMATRIX_3(npar+1,npar);
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

    init_vals = MALLOC_3(4,float);

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

    r0[X_3] = init_simplex[0][X_3];
    r0[Y_3] = init_simplex[0][Y_3];
    r0[Z_3] = init_simplex[0][Z_3];
    *R    = opt_rad(r0,&user);

    res = OK;
    goto out;

out : {
        FREE_3(init_vals);
        FREE_CMATRIX_3(init_simplex);
        return res;
    }
}





//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitData::DipoleFitData()
: mri_head_t (NULL)
, meg_head_t (NULL)
, chs (NULL)
, meg_coils (NULL)
, eeg_els (NULL)
, nmeg (0)
, neeg (0)
, ch_names (NULL)
, pick (NULL)
, bemname (NULL)
, bem_model (NULL)
, eeg_model (NULL)
, fixed_noise (FALSE)
, noise (NULL)
, noise_orig (NULL)
, nave (1)
, user (NULL)
, user_free (NULL)
, proj (NULL)
, sphere_funcs (NULL)
, bem_funcs (NULL)
, mag_dipole_funcs (NULL)
, funcs (NULL)
, column_norm (COLUMN_NORM_NONE)
, fit_mag_dipoles (FALSE)
{
    r0[0] = 0.0f;
    r0[1] = 0.0f;
    r0[2] = 0.0f;
}


//*************************************************************************************************************

DipoleFitData::~DipoleFitData()
{
    if(mri_head_t)
        delete mri_head_t;
    if(meg_head_t)
        delete meg_head_t;

    FREE_3(chs);

    if(meg_coils)
        delete meg_coils;
    if(eeg_els)
        delete eeg_els;

    FREE_3(bemname);

    mne_free_cov_3(noise);
    mne_free_cov_3(noise_orig);
    mne_free_name_list_3(ch_names,nmeg+neeg);

    if(pick)
        delete pick;
    fwd_bem_free_model_3(bem_model);

    if(eeg_model)
        delete eeg_model;
    if (user_free)
        user_free(user);

    mne_free_proj_op_3(proj);

    free_dipole_fit_funcs(sphere_funcs);
    free_dipole_fit_funcs(bem_funcs);
    free_dipole_fit_funcs(mag_dipole_funcs);
}


//*************************************************************************************************************

int DipoleFitData::setup_forward_model(DipoleFitData *d, mneCTFcompDataSet comp_data, FwdCoilSet *comp_coils)
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
        FREE_3(d->bemname); d->bemname = bemsolname;

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

            FiffCoordTransOld::fiff_coord_trans(d->r0,d->mri_head_t,TRUE);
            printf("Fitted sphere model origin : %6.1f %6.1f %6.1f mm rad = %6.1f mm.\n",
                   1000*d->r0[X_3],1000*d->r0[Y_3],1000*d->r0[Z_3],1000*R);
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
        VEC_COPY_3(d->eeg_model->r0,d->r0);
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
           1000*d->r0[X_3],1000*d->r0[Y_3],1000*d->r0[Z_3]);
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


//*************************************************************************************************************

mneCovMatrix DipoleFitData::ad_hoc_noise(FwdCoilSet *meg, FwdCoilSet *eeg, float grad_std, float mag_std, float eeg_std)
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

    stds = MALLOC_3(nchan,double);
    ch_names = MALLOC_3(nchan,char *);

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
    FREE_3(ch_names);
    return mne_new_cov(FIFFV_MNE_NOISE_COV,nchan,names,NULL,stds);
}


//*************************************************************************************************************

int DipoleFitData::make_projection(const QList<QString> &projnames, fiffChInfo chs, int nch, mneProjOp *res)
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


//*************************************************************************************************************

int DipoleFitData::scale_noise_cov(DipoleFitData *f, int nave)
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


//*************************************************************************************************************

int DipoleFitData::scale_dipole_fit_noise_cov(DipoleFitData *f, int nave)
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


//*************************************************************************************************************

int DipoleFitData::select_dipole_fit_noise_cov(DipoleFitData *f, mshMegEegData d)
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
        float  *w    = MALLOC_3(f->noise_orig->ncov,float);
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
        FREE_3(w);
    }
    else {
        if (f->noise && f->nave == nave)
            return OK;
        f->noise = mne_dup_cov(f->noise_orig);
    }

    return scale_dipole_fit_noise_cov(f,nave);
}


//*************************************************************************************************************

DipoleFitData *DipoleFitData::setup_dipole_fit_data(const QString &mriname, const QString &measname, char *bemname, Vector3f *r0, FwdEegSphereModel *eeg_model, int accurate_coils, const QString &badname, const QString &noisename, float grad_std, float mag_std, float eeg_std, float mag_reg, float grad_reg, float eeg_reg, int diagnoise, const QList<QString> &projnames, int include_meg, int include_eeg)              /**< Include EEG in the fitting? */
/*
          * Background work for modelling
          */
{
    DipoleFitData*  res = new DipoleFitData;
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
        badlist = REALLOC_3(badlist,nbad+file_nbad,char *);
        for (k = 0; k < file_nbad; k++)
            badlist[nbad++] = file_bads[k];
        FREE_3(file_bads);
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
        //#ifdef USE_SHARE_PATH
        //        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
        //#else
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

        char *coilfile = MALLOC_3(strlen(qPath.toLatin1().data())+1,char);
        strcpy(coilfile,qPath.toLatin1().data());
        //#endif

        if (!coilfile)
            goto bad;
        if ((templates = FwdCoilSet::read_coil_defs(coilfile)) == NULL) {
            FREE_3(coilfile);
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
    res->bemname   = mne_strdup_3(bemname);
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
                FREE_3(comp_chs);
                goto bad;
            }
            printf("%d compensation channels in %s\n",comp_coils->ncoil,measname.toLatin1().data());
        }
        FREE_3(comp_chs);
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
        if(res)
            delete res;
        return NULL;
    }
}


//*************************************************************************************************************

//============================= dipole_forward.c =============================

void print_fields(float       *rd,
                  float       *Q,
                  float       time,
                  float       integ,
                  DipoleFitData* fit,
                  MneMeasData* data)

{
    float *one = MALLOC_3(data->nchan,float);
    int   k;
    float **fwd = NULL;

    if (mne_get_values_from_data_3(time,integ,data->current->data,data->current->np,data->nchan,data->current->tmin,
                                   1.0/data->current->tstep,FALSE,one) == FAIL) {
        fprintf(stderr,"Cannot pick time: %7.1f ms\n",1000*time);
        return;
    }
    for (k = 0; k < data->nchan; k++)
        if (data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_REF_GRAD ||
                data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD) {
            printf("%g ",1e15*one[k]);
        }
    printf("\n");


    fwd = ALLOC_CMATRIX_3(3,fit->nmeg+fit->neeg);
    if (DipoleFitData::compute_dipole_field(fit,rd,FALSE,fwd) == FAIL)
        goto out;

    for (k = 0; k < data->nchan; k++)
        if (data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_REF_GRAD ||
                data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD) {
            printf("%g ",1e15*(Q[X_3]*fwd[X_3][k]+Q[Y_3]*fwd[Y_3][k]+Q[Z_3]*fwd[Z_3][k]));
        }
    printf("\n");

out : {
        FREE_3(one);
        FREE_CMATRIX_3(fwd);
    }
    return;
}


//*************************************************************************************************************

DipoleForward* dipole_forward(DipoleFitData* d,
                              float         **rd,
                              int           ndip,
                              DipoleForward* old)
/*
 * Compute the forward solution and do other nice stuff
 */
{
    DipoleForward* res;
    float         **this_fwd;
    float         S[3];
    int           k,p;
    /*
   * Allocate data if necessary
   */
    if (old && old->ndip == ndip && old->nch == d->nmeg+d->neeg) {
        res = old;
    }
    else {
        delete old; old = NULL;
        res = new DipoleForward;
        res->fwd  = ALLOC_CMATRIX_3(3*ndip,d->nmeg+d->neeg);
        res->uu   = ALLOC_CMATRIX_3(3*ndip,d->nmeg+d->neeg);
        res->vv   = ALLOC_CMATRIX_3(3*ndip,3);
        res->sing = MALLOC_3(3*ndip,float);
        res->nch  = d->nmeg+d->neeg;
        res->rd   = ALLOC_CMATRIX_3(ndip,3);
        res->scales = MALLOC_3(3*ndip,float);
        res->ndip = ndip;
    }

    for (k = 0; k < ndip; k++) {
        VEC_COPY_3(res->rd[k],rd[k]);
        this_fwd = res->fwd + 3*k;
        /*
     * Calculate the field of three orthogonal dipoles
     */
        if ((DipoleFitData::compute_dipole_field(d,rd[k],TRUE,this_fwd)) == FAIL)
            goto bad;
        /*
     * Choice of column normalization
     * (componentwise normalization is not recommended)
     */
        if (d->column_norm == COLUMN_NORM_LOC || d->column_norm == COLUMN_NORM_COMP) {
            for (p = 0; p < 3; p++)
                S[p] = mne_dot_vectors_3(res->fwd[3*k+p],res->fwd[3*k+p],res->nch);
            if (d->column_norm == COLUMN_NORM_COMP) {
                for (p = 0; p < 3; p++)
                    res->scales[3*k+p] = sqrt(S[p]);
            }
            else {
                /*
     * Divide by three or not?
     */
                res->scales[3*k+0] = res->scales[3*k+1] = res->scales[3*k+2] = sqrt(S[0]+S[1]+S[2])/3.0;
            }
            for (p = 0; p < 3; p++) {
                if (res->scales[3*k+p] > 0.0) {
                    res->scales[3*k+p] = 1.0/res->scales[3*k+p];
                    mne_scale_vector_3(res->scales[3*k+p],res->fwd[3*k+p],res->nch);
                }
                else
                    res->scales[3*k+p] = 1.0;
            }
        }
        else {
            res->scales[3*k]   = 1.0;
            res->scales[3*k+1] = 1.0;
            res->scales[3*k+2] = 1.0;
        }
    }

    /*
   * SVD
   */
    if (mne_svd_3(res->fwd,3*ndip,d->nmeg+d->neeg,res->sing,res->vv,res->uu) != 0)
        goto bad;

    return res;

bad : {
        if (!old)
            delete res;
        return NULL;
    }
}


//*************************************************************************************************************

DipoleForward* DipoleFitData::dipole_forward_one(DipoleFitData* d,
                                                 float         *rd,
                                                 DipoleForward* old)
/*
 * Convenience function to compute the field of one dipole
 */
{
    float *rds[1];
    rds[0] = rd;
    return dipole_forward(d,rds,1,old);
}


//*************************************************************************************************************
// fit_dipoles.c
static float fit_eval(float *rd,int npar,void *user)
/*
 * Calculate the residual sum of squares
 */
{
    DipoleFitData* fit   = (DipoleFitData*)user;
    DipoleForward* fwd;
    fitDipUser       fuser = (fitDipUser)fit->user;
    double        Bm2,one;
    int           ncomp,c;

    fwd   = fuser->fwd = DipoleFitData::dipole_forward_one(fit,rd,fuser->fwd);
    ncomp = fwd->sing[2]/fwd->sing[0] > fuser->limit ? 3 : 2;
    if (fuser->report_dim)
        fprintf(stderr,"ncomp = %d\n",ncomp);

    for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
        one = mne_dot_vectors_3(fwd->uu[c],fuser->B,fwd->nch);
        Bm2 = Bm2 + one*one;
    }
    return fuser->B2-Bm2;
}








static int find_best_guess(float     *B,         /* The whitened data */
                           int       nch,
                           GuessData* guess,	 /* Guesses */
                           float     limit,	 /* Pseudoradial component omission limit */
                           int       *bestp,	 /* Which is the best */
                           float     *goodp)	 /* Best goodness of fit */
/*
 * Thanks to the precomputed SVD everything is really simple
 */
{
    int    k,c;
    double B2,Bm2,this_good,one;
    int    best = -1;
    float  good = 0.0;
    DipoleForward* fwd;
    int    ncomp;

    B2 = mne_dot_vectors_3(B,B,nch);
    for (k = 0; k < guess->nguess; k++) {
        fwd = guess->guess_fwd[k];
        if (fwd->nch == nch) {
            ncomp = fwd->sing[2]/fwd->sing[0] > limit ? 3 : 2;
            for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
                one = mne_dot_vectors_3(fwd->uu[c],B,nch);
                Bm2 = Bm2 + one*one;
            }
            this_good = 1.0 - (B2 - Bm2)/B2;
            if (this_good > good) {
                best = k;
                good = this_good;
            }
        }
    }
    if (best < 0) {
        printf("No reasonable initial guess found.");
        return FAIL;
    }
    *bestp = best;
    *goodp = good;
    return OK;
}












static float **make_initial_dipole_simplex(float  *r0,
                                           float  size)
/*
      * Make the initial tetrahedron
      */
{
    /*
   * For this definition of a regular tetrahedron, see
   *
   * http://mathworld.wolfram.com/Tetrahedron.html
   *
   */
    float x = sqrt(3.0)/3.0;
    float r = sqrt(6.0)/12.0;
    float R = 3*r;
    float d = x/2.0;
    float rr[][3] = { { x , 0.0,  -r },
                      { -d, 0.5,  -r },
                      { -d, -0.5, -r },
                      { 0.0, 0.0, R } };

    float **simplex = ALLOC_CMATRIX_3(4,3);
    int j,k;

    for (j = 0; j < 4; j++) {
        VEC_COPY_3(simplex[j],rr[j]);
        for (k = 0; k < 3; k++)
            simplex[j][k] = size*simplex[j][k] + r0[k];
    }
    return simplex;
}


static int report_func(int     loop,
                       float   *fitpar,
                       int     npar,
                       double  fval_lo,
                       double  fval_hi,
                       double  par_diff)
/*
      * Report periodically
      */
{
    float *r0 = fitpar;

    fprintf(stdout,"loop %d rd %7.2f %7.2f %7.2f fval %g %g par diff %g\n",
            loop,1000*r0[0],1000*r0[1],1000*r0[2],fval_lo,fval_hi,1000*par_diff);

    return OK;
}












static int fit_Q(DipoleFitData* fit,	     /* The fit data */
                 float *B,		     /* Measurement */
                 float *rd,		     /* Dipole position */
                 float limit,		     /* Radial component omission limit */
                 float *Q,		     /* The result */
                 int   *ncomp,
                 float *res)	             /* Residual sum of squares */
/*
 * fit the dipole moment once the location is known
 */
{
    int c;
    DipoleForward* fwd = DipoleFitData::dipole_forward_one(fit,rd,NULL);
    float Bm2,one;

    if (!fwd)
        return FAIL;

    *ncomp = fwd->sing[2]/fwd->sing[0] > limit ? 3 : 2;

    Q[0] = Q[1] = Q[2] = 0.0;
    for (c = 0, Bm2 = 0.0; c < *ncomp; c++) {
        one = mne_dot_vectors_3(fwd->uu[c],B,fwd->nch);
        mne_add_scaled_vector_to_3(fwd->vv[c],one/fwd->sing[c],Q,3);
        Bm2 = Bm2 + one*one;
    }
    /*
   * Counteract the effect of column normalization
   */
    for (c = 0; c < 3; c++)
        Q[c] = fwd->scales[c]*Q[c];
    *res = mne_dot_vectors_3(B,B,fwd->nch) - Bm2;

    delete fwd;

    return OK;
}


static float rtol(float *vals,int nval)

{
    float minv,maxv;
    int   k;

    minv = maxv = vals[0];
    for (k = 1; k < nval; k++) {
        if (vals[k] < minv)
            minv = vals[k];
        if (vals[k] > maxv)
            maxv = vals[k];
    }
    return 2.0*(maxv-minv)/(maxv+minv);
}












/*
 * This routine comes from Numerical recipes
 */

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0
#define MIN_STOL_LOOP 5

static float tryf (float **p,
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

    ptry = ALLOC_FLOAT_3(ndim);
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
    FREE_3(ptry);
    return ytry;
}









int simplex_minimize(float **p,		                              /* The initial simplex */
                     float *y,		                              /* Function values at the vertices */
                     int   ndim,	                              /* Number of variables */
                     float ftol,	                              /* Relative convergence tolerance */
                     float stol,
                     float (*func)(float *x,int npar,void *user_data),/* The function to be evaluated */
                     void  *user_data,				      /* Data to be passed to the above function in each evaluation */
                     int   max_eval,	                              /* Maximum number of function evaluations */
                     int   *neval,	                              /* Number of function evaluations */
                     int   report,                                    /* How often to report (-1 = no_reporting) */
                     int   (*report_func)(int loop,
                                          float *fitpar, int npar,
                                          double fval_lo,
                                          double fval_hi,
                                          double par_diff))            /* The function to be called when reporting */
/*
      * Minimization with the simplex algorithm
      * Modified from Numerical recipes
      */

{
    int   i,j,ilo,ihi,inhi;
    int   mpts = ndim+1;
    float ytry,ysave,sum,rtol,*psum;
    double dsum,diff;
    int   result = 0;
    int   count = 0;
    int   loop  = 1;

    psum = ALLOC_FLOAT_3(ndim);
    *neval = 0;
    for (j = 0; j < ndim; j++) {
        for (i = 0,sum = 0.0; i<mpts; i++)
            sum +=  p[i][j];
        psum[j] = sum;
    }
    if (report_func != NULL && report > 0)
        (void)report_func (0,p[0],ndim,-1.0,-1.0,0.0);

    dsum = 0.0;
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
            if (report_func (loop,p[ilo],ndim,y[ilo],y[ihi],sqrt(dsum))) {
                printf("Interation interrupted.");
                result = -1;
                break;
            }
            count = 0;
        }
        if (rtol < ftol) break;
        if (*neval >=  max_eval) {
            printf("Maximum number of evaluations exceeded.");
            result  =  -1;
            break;
        }
        if (stol > 0) {		/* Has the simplex collapsed? */
            for (dsum = 0.0, j = 0; j < ndim; j++) {
                diff = p[ilo][j] - p[ihi][j];
                dsum += diff*diff;
            }
            if (loop > MIN_STOL_LOOP && sqrt(dsum) < stol)
                break;
        }
        ytry = tryf(p,y,psum,ndim,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            ytry = tryf(p,y,psum,ndim,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryf(p,y,psum,ndim,func,user_data,ihi,neval,BETA);
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
    FREE_3 (psum);
    return (result);
}


//*************************************************************************************************************
// fit_dipoles.c
bool DipoleFitData::fit_one(DipoleFitData* fit,	            /* Precomputed fitting data */
                    GuessData*     guess,	            /* The initial guesses */
                    float         time,              /* Which time is it? */
                    float         *B,	            /* The field to fit */
                    int           verbose,
                    ECD&          res               /* The fitted dipole */
                    )
{
    float  **simplex       = NULL;	       /* The simplex */
    float  vals[4];			       /* Values at the vertices */
    float  limit           = 0.2;	               /* (pseudo) radial component omission limit */
    float  size            = 1e-2;	       /* Size of the initial simplex */
    float  ftol[]          = { 1e-2, 1e-2 };     /* Tolerances on the the two passes */
    float  atol[]          = { 0.2e-3, 0.2e-3 }; /* If dipole movement between two iterations is less than this,
                                                  we consider to have converged */
    int    ntol            = 2;
    int    max_eval        = 1000;	       /* Limit for fit function evaluations */
    int    report_interval = verbose ? 1 : -1;   /* How often to report the intermediate result */

    int        best;
    float      good,rd_guess[3],rd_final[3],Q[3],final_val;
    fitDipUserRec user;
    int        k,p,neval,neval_tot,nchan,ncomp;
    int        fit_fail;

    nchan = fit->nmeg+fit->neeg;
    user.fwd = NULL;

    if (mne_proj_op_proj_vector_3(fit->proj,B,nchan,TRUE) == FAIL)
        goto bad;

    if (mne_whiten_one_data(B,B,nchan,fit->noise) == FAIL)
        goto bad;
    /*
   * Get the initial guess
   */
    if (find_best_guess(B,nchan,guess,limit,&best,&good) < 0)
        goto bad;


    user.limit = limit;
    user.B     = B;
    user.B2    = mne_dot_vectors_3(B,B,nchan);
    user.fwd   = NULL;
    user.report_dim = FALSE;
    fit->user  = &user;

    VEC_COPY_3(rd_guess,guess->rr[best]);
    VEC_COPY_3(rd_final,guess->rr[best]);

    neval_tot = 0;
    fit_fail = FALSE;
    for (k = 0; k < ntol; k++) {
        /*
     * Do first pass with the sphere model
     */
        if (k == 0)
            fit->funcs = fit->sphere_funcs;
        else
            fit->funcs = fit->bemname ? fit->bem_funcs : fit->sphere_funcs;

        simplex = make_initial_dipole_simplex(rd_guess,size);
        for (p = 0; p < 4; p++)
            vals[p] = fit_eval(simplex[p],3,fit);
        if (simplex_minimize(simplex,           /* The initial simplex */
                             vals,              /* Function values at the vertices */
                             3,	            /* Number of variables */
                             ftol[k],           /* Relative convergence tolerance for the target function */
                             atol[k],           /* Absolute tolerance for the change in the parameters */
                             fit_eval,          /* The function to be evaluated */
                             fit,	            /* Data to be passed to the above function in each evaluation */
                             max_eval,          /* Maximum number of function evaluations */
                             &neval,            /* Number of function evaluations */
                             report_interval,   /* How often to report (-1 = no_reporting) */
                             report_func) != OK) {
            if (k == 0)
                goto bad;
            else {
                printf("\nWarning (t = %8.1f ms) : g = %6.1f %% final val = %7.3f rtol = %f\n",
                       1000*time,100*(1 - vals[0]/user.B2),vals[0],rtol(vals,4));
                fit_fail = TRUE;
            }
        }
        VEC_COPY_3(rd_final,simplex[0]);
        VEC_COPY_3(rd_guess,simplex[0]);
        FREE_CMATRIX_3(simplex); simplex = NULL;

        neval_tot += neval;
        final_val  = vals[0];
    }
    /*
   * Confidence limits should be computed here
   */
    /*
   * Compute the dipole moment at the final point
   */
    if (fit_Q(fit,user.B,rd_final,user.limit,Q,&ncomp,&final_val) == OK) {
        res.time  = time;
        res.valid = true;
        for(int i = 0; i < 3; ++i)
            res.rd[i] = rd_final[i];
        for(int i = 0; i < 3; ++i)
            res.Q[i] = Q[i];
        res.good  = 1.0 - final_val/user.B2;
        if (fit_fail)
            res.good = -res.good;
        res.khi2  = final_val;
        if (fit->proj)
            res.nfree = nchan-3-ncomp-fit->proj->nvec;
        else
            res.nfree = nchan-3-ncomp;
        res.neval = neval_tot;
    }
    else
        goto bad;
    delete user.fwd;
    FREE_CMATRIX_3(simplex);

    return true;

bad : {
        delete user.fwd;
        FREE_CMATRIX_3(simplex);
        return false;
    }
}












//*************************************************************************************************************

int DipoleFitData::compute_dipole_field(DipoleFitData* d, float *rd, int whiten, float **fwd)
/*
 * Compute the field and take whitening and projection into account
 */
{
    float *eeg_fwd[3];
    static float Qx[] = {1.0,0.0,0.0};
    static float Qy[] = {0.0,1.0,0.0};
    static float Qz[] = {0.0,0.0,1.0};
    int k;
    /*
   * Compute the fields
   */
    if (d->nmeg > 0) {
        if (d->funcs->meg_vec_field) {
            if (d->funcs->meg_vec_field(rd,d->meg_coils,fwd,d->funcs->meg_client) != OK)
                goto bad;
        }
        else {
            if (d->funcs->meg_field(rd,Qx,d->meg_coils,fwd[0],d->funcs->meg_client) != OK)
                goto bad;
            if (d->funcs->meg_field(rd,Qy,d->meg_coils,fwd[1],d->funcs->meg_client) != OK)
                goto bad;
            if (d->funcs->meg_field(rd,Qz,d->meg_coils,fwd[2],d->funcs->meg_client) != OK)
                goto bad;
        }
    }

    if (d->neeg > 0) {
        if (d->funcs->eeg_vec_pot) {
            eeg_fwd[0] = fwd[0]+d->nmeg;
            eeg_fwd[1] = fwd[1]+d->nmeg;
            eeg_fwd[2] = fwd[2]+d->nmeg;
            if (d->funcs->eeg_vec_pot(rd,d->eeg_els,eeg_fwd,d->funcs->eeg_client) != OK)
                goto bad;
        }
        else {
            if (d->funcs->eeg_pot(rd,Qx,d->eeg_els,fwd[0]+d->nmeg,d->funcs->eeg_client) != OK)
                goto bad;
            if (d->funcs->eeg_pot(rd,Qy,d->eeg_els,fwd[1]+d->nmeg,d->funcs->eeg_client) != OK)
                goto bad;
            if (d->funcs->eeg_pot(rd,Qz,d->eeg_els,fwd[2]+d->nmeg,d->funcs->eeg_client) != OK)
                goto bad;
        }
    }

    /*
   * Apply projection
   */
#ifdef DEBUG
    fprintf(stdout,"orig : ");
    for (k = 0; k < 3; k++)
        fprintf(stdout,"%g ",sqrt(mne_dot_vectors(fwd[k],fwd[k],d->nmeg+d->neeg)));
    fprintf(stdout,"\n");
#endif

    for (k = 0; k < 3; k++)
        if (mne_proj_op_proj_vector_3(d->proj,fwd[k],d->nmeg+d->neeg,TRUE) == FAIL)
            goto bad;

#ifdef DEBUG
    fprintf(stdout,"proj : ");
    for (k = 0; k < 3; k++)
        fprintf(stdout,"%g ",sqrt(mne_dot_vectors(fwd[k],fwd[k],d->nmeg+d->neeg)));
    fprintf(stdout,"\n");
#endif

    /*
   * Whiten
   */
    if (d->noise && whiten) {
        if (mne_whiten_data(fwd,fwd,3,d->nmeg+d->neeg,d->noise) == FAIL)
            goto bad;
    }

#ifdef DEBUG
    fprintf(stdout,"white : ");
    for (k = 0; k < 3; k++)
        fprintf(stdout,"%g ",sqrt(mne_dot_vectors(fwd[k],fwd[k],d->nmeg+d->neeg)));
    fprintf(stdout,"\n");
#endif

    return OK;

bad :
    return FAIL;
}
