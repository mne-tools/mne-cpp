

#include "dipole_fit_data.h"
#include "guess_data.h"
#include "ecd.h"

#include <Eigen/Dense>



using namespace Eigen;
using namespace INVERSELIB;



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



#define VEC_COPY_3(to,from) {\
    (to)[X_3] = (from)[X_3];\
    (to)[Y_3] = (from)[Y_3];\
    (to)[Z_3] = (from)[Z_3];\
    }



#define MIN_3(a,b) ((a) < (b) ? (a) : (b))







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





float mne_dot_vectors_3 (float *v1,
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









//float
Eigen::MatrixXf toFloatEigenMatrix_3(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromFloatEigenMatrix_3(const Eigen::MatrixXf& from_mat, float **to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}


void fromFloatEigenVector_3(const Eigen::VectorXf& from_vec, float *to_vec, const int n)
{
    for ( int i = 0; i < n; ++i)
        to_vec[i] = from_vec[i];
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
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitData::DipoleFitData()
{

}


//*************************************************************************************************************

DipoleFitData::~DipoleFitData()
{

}








//============================= dipole_forward.c =============================


void print_fields(float       *rd,
                  float       *Q,
                  float       time,
                  float       integ,
                  DipoleFitData* fit,
                  mneMeasData data)

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








//============================= fit_dipoles.c =============================







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






























bool DipoleFitData::fit_one(DipoleFitData* fit,	            /* Precomputed fitting data */
                    GuessData*     guess,	            /* The initial guesses */
                    float         time,              /* Which time is it? */
                    float         *B,	            /* The field to fit */
                    int           verbose,
                    ECD&          res               /* The fitted dipole */
                    )
/*
 * Fit a single dipole to the given data
 */
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
