//=============================================================================================================
/**
 * @file     fwd_eeg_sphere_model.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FwdEegSphereModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils/simplex_algorithm.h"

#include "fwd_eeg_sphere_model.h"
#include "fwd_eeg_sphere_model_set.h"

#include <QtAlgorithms>

#include <qmath.h>

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;
using namespace FWDLIB;

#define MIN_1(a,b) ((a) < (b) ? (a) : (b))

//ToDo Remove after refactoring
#define X_1 0
#define Y_1 1
#define Z_1 2
/*
 * Dot product and length
 */
#define VEC_DOT_1(x,y) ((x)[X_1]*(y)[X_1] + (x)[Y_1]*(y)[Y_1] + (x)[Z_1]*(y)[Z_1])
#define VEC_LEN_1(x) sqrt(VEC_DOT_1(x,x))
/*
 * Others...
 */
#define VEC_DIFF_1(from,to,diff) {\
    (diff)[X_1] = (to)[X_1] - (from)[X_1];\
    (diff)[Y_1] = (to)[Y_1] - (from)[Y_1];\
    (diff)[Z_1] = (to)[Z_1] - (from)[Z_1];\
    }

#define VEC_COPY_1(to,from) {\
    (to)[X_1] = (from)[X_1];\
    (to)[Y_1] = (from)[Y_1];\
    (to)[Z_1] = (from)[Z_1];\
    }

#define CROSS_PRODUCT_1(x,y,xy) {\
    (xy)[X_1] =   (x)[Y_1]*(y)[Z_1]-(y)[Y_1]*(x)[Z_1];\
    (xy)[Y_1] = -((x)[X_1]*(y)[Z_1]-(y)[X_1]*(x)[Z_1]);\
    (xy)[Z_1] =   (x)[X_1]*(y)[Y_1]-(y)[X_1]*(x)[Y_1];\
    }

#define MALLOC_1(x,t) (t *)malloc((x)*sizeof(t))

#define REALLOC_1(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE(x) if ((char *)(x) != NULL) free((char *)(x))

#define ALLOC_DCMATRIX_1(x,y) mne_dmatrix_1((x),(y))
#define FREE_DCMATRIX_1(m) mne_free_dcmatrix_1((m))

#define ALLOC_CMATRIX_1(x,y) mne_cmatrix_1((x),(y))

/*
 * float matrices
 */
#define FREE_CMATRIX_1(m) mne_free_cmatrix_1((m))

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

static void matrix_error_1(int kind, int nr, int nc)

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

float **mne_cmatrix_1(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_1(nr,float *);
    if (!m) matrix_error_1(1,nr,nc);
    whole = MALLOC_1(nr*nc,float);
    if (!whole) matrix_error_1(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

double **mne_dmatrix_1(int nr, int nc)

{
    int i;
    double **m;
    double *whole;

    m = MALLOC_1(nr,double *);
    if (!m) matrix_error_1(1,nr,nc);
    whole = MALLOC_1(nr*nc,double);
    if (!whole) matrix_error_1(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

void mne_free_cmatrix_1 (float **m)
{
    if (m) {
        free(*m);
        free(m);
    }
}

void mne_free_dcmatrix_1 (double **m)

{
    if (m) {
        FREE(*m);
        FREE(m);
    }
}

#define MAXTERMS 1000
#define EPS      1e-10
#define SIN_EPS  1e-3

static int         terms = 0;       /* These statistics may be useful */
static int         eval = 0;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModel::FwdEegSphereModel()
: fn(NULL)
, nterms  (0)
, lambda  (NULL)
, mu      (NULL)
, nfit    (0)
, scale_pos (0)
{
    r0[0] = 0.0;
    r0[1] = 0.0;
    r0[2] = 0.0;
}

//=============================================================================================================

FwdEegSphereModel::FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel)
{
    int k;

    if (!p_FwdEegSphereModel.name.isEmpty())
        this->name = p_FwdEegSphereModel.name;
    if (p_FwdEegSphereModel.nlayer() > 0) {
        for (k = 0; k < p_FwdEegSphereModel.nlayer(); k++)
            this->layers.append(p_FwdEegSphereModel.layers[k]);
    }
    VEC_COPY_1(this->r0,p_FwdEegSphereModel.r0);
    if (p_FwdEegSphereModel.nterms > 0) {
        this->fn = VectorXd(p_FwdEegSphereModel.nterms);
        this->nterms = p_FwdEegSphereModel.nterms;
        for (k = 0; k < p_FwdEegSphereModel.nterms; k++)
            this->fn[k] = p_FwdEegSphereModel.fn[k];
    }
    if (p_FwdEegSphereModel.nfit > 0) {
        this->mu     = VectorXf(p_FwdEegSphereModel.nfit);
        this->lambda = VectorXf(p_FwdEegSphereModel.nfit);
        this->nfit   = p_FwdEegSphereModel.nfit;
        for (k = 0; k < p_FwdEegSphereModel.nfit; k++) {
            this->mu[k] = p_FwdEegSphereModel.mu[k];
            this->lambda[k] = p_FwdEegSphereModel.lambda[k];
        }
    }
    this->scale_pos = p_FwdEegSphereModel.scale_pos;
}

//=============================================================================================================

FwdEegSphereModel* FwdEegSphereModel::fwd_create_eeg_sphere_model(const QString& name,
                                                     int nlayer,
                                                     const VectorXf& rads,
                                                     const VectorXf& sigmas)
/*
      * Produce a new sphere model structure
      */
{
    FwdEegSphereModel* new_model = new FwdEegSphereModel();

    new_model->name   = name;

    for (int k = 0; k < nlayer; k++) {
        FwdEegSphereLayer layer;
        layer.rad = layer.rel_rad = rads[k];
        layer.sigma = sigmas[k];
        new_model->layers.append(layer);
    }
    /*
   * Sort...
   */
    qSort (new_model->layers.begin(), new_model->layers.end(), FwdEegSphereLayer::comp_layers);

    /*
   * Scale the radiuses
   */
    float R  = new_model->layers[nlayer-1].rad;
    float rR = new_model->layers[nlayer-1].rel_rad;
    for (int k = 0; k < nlayer; k++) {
        new_model->layers[k].rad     = new_model->layers[k].rad/R;
        new_model->layers[k].rel_rad = new_model->layers[k].rel_rad/rR;
    }
    return new_model;
}

//=============================================================================================================

FwdEegSphereModel::~FwdEegSphereModel()
{
}

//=============================================================================================================

FwdEegSphereModel* FwdEegSphereModel::setup_eeg_sphere_model(const QString& eeg_model_file, QString eeg_model_name, float eeg_sphere_rad)
{
    FwdEegSphereModelSet* eeg_models = NULL;
    FwdEegSphereModel*    eeg_model  = NULL;

    if (eeg_model_name.isEmpty())
        eeg_model_name = QString("Default");

    eeg_models = FwdEegSphereModelSet::fwd_load_eeg_sphere_models(eeg_model_file,NULL);
    eeg_models->fwd_list_eeg_sphere_models(stderr);

    if ((eeg_model = eeg_models->fwd_select_eeg_sphere_model(eeg_model_name)) == NULL)
        goto bad;

    if (!eeg_model->fwd_setup_eeg_sphere_model(eeg_sphere_rad,true,3))
        goto bad;
    printf("Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
           eeg_model->name.toUtf8().constData(),1000*eeg_sphere_rad);
    printf("\n");
    delete eeg_models;
    return eeg_model;

bad : {
        delete eeg_models;
        delete eeg_model;
        return NULL;
    }
}

//=============================================================================================================

fitUser FwdEegSphereModel::new_fit_user(int nfit, int nterms)

{
    fitUser u = MALLOC_1(1,fitUserRec);
    u->y      = MALLOC_1(nterms-1,double);
    u->resi   = MALLOC_1(nterms-1,double);
    u->M      = ALLOC_DCMATRIX_1(nterms-1,nfit-1);
    u->uu     = ALLOC_DCMATRIX_1(nfit-1,nterms-1);
    u->vv     = ALLOC_DCMATRIX_1(nfit-1,nfit-1);
    u->sing   = MALLOC_1(nfit,double);
    u->fn     = MALLOC_1(nterms,double);
    u->w      = MALLOC_1(nterms,double);
    u->nfit   = nfit;
    u->nterms = nterms;
    return u;
}

//=============================================================================================================
// fwd_multi_spherepot.c
double FwdEegSphereModel::fwd_eeg_get_multi_sphere_model_coeff(int n)
{
    MatrixXd M,Mn,help,Mm;
    static MatrixXd mat1;
    static MatrixXd mat2;
    static MatrixXd mat3;
    static VectorXd c1;
    static VectorXd c2;
    static VectorXd cr;
    static VectorXd cr_mult;
    double div,div_mult;
    double n1;
#ifdef TEST
    double rel1,rel2;
    double b,c;
#endif
    int    k;

    if (this->nlayer() == 0 || this->nlayer() == 1)
        return 1.0;
    /*
   * Now follows the tricky case
   */
#ifdef TEST
    if (this->nlayer() == 2) {
        rel1 = layers[0].sigma/layers[1].sigma;
        n1 = n + 1.0;
        div_mult = 2.0*n + 1;
        b = pow(this->layers[0].rel_rad,div_mult);
        return div_mult/((n1 + n*rel1) + b*n1*(rel1-1.0));
    }
    else if (this->nlayer() == 3) {
        rel1 = this->layers[0].sigma/this->layers[1].sigma;
        rel2 = this->layers[1].sigma/this->layers[2].sigma;
        n1 = n + 1.0;
        div_mult = 2.0*n + 1.0;
        b = pow(this->layers[0].rel_rad,div_mult);
        c = pow(this->layers[1].rel_rad,div_mult);
        div_mult = div_mult*div_mult;
        div = (b*n*n1*(rel1-1.0)*(rel2-1.0) + c*(rel1*n + n1)*(rel2*n + n1))/c +
                n1*(b*(rel1-1.0)*(rel2*n1 + n) + c*(rel1*n + n1)*(rel2-1.0));
        return div_mult/div;
    }
#endif
    if (n == 1) {
        /*
     * Initialize the arrays
     */
        c1.resize(this->nlayer()-1);
        c2.resize(this->nlayer()-1);
        cr.resize(this->nlayer()-1);
        cr_mult.resize(this->nlayer()-1);
        for (k = 0; k < this->nlayer()-1; k++) {
            c1[k] = this->layers[k].sigma/this->layers[k+1].sigma;
            c2[k] = c1[k] - 1.0;
            cr_mult[k] = this->layers[k].rel_rad;
            cr[k] = cr_mult[k];
            cr_mult[k] = cr_mult[k]*cr_mult[k];
        }
        if (mat1.cols() == 0)
            mat1 = MatrixXd(2,2);
        if (mat2.cols() == 0)
            mat2 = MatrixXd(2,2);
        if (mat3.cols() == 0)
            mat3 = MatrixXd(2,2);
    }
    /*
   * Increment the radius coefficients
   */
    for (k = 0; k < this->nlayer()-1; k++)
        cr[k] = cr[k]*cr_mult[k];
    /*
   * Multiply the matrices
   */
    M  = mat1;
    Mn = mat2;
    Mm = mat3;
    M(0,0) = M(1,1) = 1.0;
    M(0,1) = M(1,0) = 0.0;
    div      = 1.0;
    div_mult = 2.0*n + 1.0;
    n1       = n + 1.0;

    for (k = this->nlayer()-2; k >= 0; k--) {

        Mm(0,0) = (n + n1*c1[k]);
        Mm(0,1) = n1*c2[k]/cr[k];
        Mm(1,0) = n*c2[k]*cr[k];
        Mm(1,1) = n1 + n*c1[k];

        Mn(0,0) = Mm(0,0)*M(0,0) + Mm(0,1)*M(1,0);
        Mn(0,1) = Mm(0,0)*M(0,1) + Mm(0,1)*M(1,1);
        Mn(1,0) = Mm(1,0)*M(0,0) + Mm(1,1)*M(1,0);
        Mn(1,1) = Mm(1,0)*M(0,1) + Mm(1,1)*M(1,1);
        help = M;
        M = Mn;
        Mn = help;
        div = div*div_mult;

    }
    return n*div/(n*M(1,1) + n1*M(1,0));
}

//=============================================================================================================
// fwd_multi_spherepot.c
void FwdEegSphereModel::next_legen(int n, double x, double *p0, double *p01, double *p1, double *p11)        /* Input: P1(n-2) Output: P1(n-1) */
/*
          * Compute the next Legendre polynomials of the
          * first and second kind using the recursion formulas.
          *
          * The routine initializes automatically with the known values
          * when n = 1
          */
{
    double  help0,help1;

    if (n > 1) {
        help0 = *p0;
        help1 = *p1;
        *p0 = ((2*n-1)*x*help0 - (n-1)*(*p01))/n;
        *p1 = ((2*n-1)*x*help1 - n*(*p11))/(n-1);
        *p01 = help0;
        *p11 = help1;
    }
    else if (n == 0) {
        *p0   = 1.0;
        *p1   = 0.0;
    }
    else if (n == 1) {
        *p01  = 1.0;
        *p0   = x;
        *p11  = 0.0;
        *p1   = sqrt(1.0-x*x);
    }
    return;
}

//=============================================================================================================

void FwdEegSphereModel::calc_pot_components(double beta, double cgamma, double *Vrp, double *Vtp, const Eigen::VectorXd& fn, int nterms)
{
    double Vt = 0.0;
    double Vr = 0.0;
    double p0,p01,p1,p11;
    double betan,multn;
    int    n;

    betan = 1.0;
    p0 = p01 = p1 = p11 = 0.0;
    for (n = 1; n <= nterms; n++) {
        if (betan < EPS) {
            terms = terms + n;
            eval  = eval + 1;
            break;
        }
        next_legen (n,cgamma,&p0,&p01,&p1,&p11);
        multn = betan*fn[n-1];	/* The 2*n + 1 factor is included in fn */
        Vr = Vr + multn*p0;
        Vt = Vt + multn*p1/n;
        betan = beta*betan;
    }
     *Vrp = Vr;
     *Vtp = Vt;
    return;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_multi_spherepot(float *rd, float *Q, float **el, int neeg, float *Vval, void *client)	  /* The model definition */
/*
 * Compute the electric potentials in a set of electrodes in spherically
 * Symmetric head model.
 *
 * The code is based on the formulas presented in
 *
 * Z. Zhang, A fast method to compute surface potentials
 * generated by dipoles within multilayer anisotropic spheres,
 * Phys. Med. Biol., 40, 335 - 349, 1995.
 *
 * and
 *
 * J.C. Moscher, R.M. Leahy, and P.S. Lewis, Matrix Kernels for
 * Modeling of EEG and MEG Data, Los Alamos Technical Report,
 * LA-UR-96-1993, 1996.
 *
 * This version does not use the acceleration with help of equivalent sources
 * in the homogeneous model
 *
 */
{
    FwdEegSphereModel* m = (FwdEegSphereModel*)client;
    float  my_rd[3],pos[3];
    int    k,p;
    float  pos2,rd_len,pos_len;
    double beta,cos_gamma,Vr,Vt;
    float  vec1[3],vec2[3],v1,v2;
    float  cos_beta,Qr,Qt,Q2,c;
    float  pi4_inv = 0.25/M_PI;
    float  sigmaM_inv;
    /*
       * Precompute the coefficients
       */
    if (m->fn.size() == 0 || m->nterms != MAXTERMS) {
        m->fn.resize(MAXTERMS);
        m->nterms = MAXTERMS;
        for (k = 0; k < MAXTERMS; k++)
            m->fn[k] = (2*k+3)*m->fwd_eeg_get_multi_sphere_model_coeff(k+1);
    }
    /*
       * Move to the sphere coordinates
       */
    for (p = 0; p < 3; p++)
        my_rd[p] = rd[p] - m->r0[p];
    rd = my_rd;
    rd_len = VEC_LEN_1(rd);
    Q2     = VEC_DOT_1(Q,Q);
    /*
       * Ignore dipoles outside the innermost sphere
       */
    if (rd_len >= m->layers[0].rad) {
        for (k = 0; k < neeg; k++)
            Vval[k] = 0.0;
        return OK;
    }
    /*
       * Special case: rd and Q are parallel
       */
    c = VEC_DOT_1(rd,Q)/(rd_len*sqrt(Q2));
    if ((1.0-c*c) < SIN_EPS)	{	/* Almost parallel:
                         * Q is purely radial */
        Qr = sqrt(Q2);
        Qt = 0.0;
        cos_beta = 0.0;
        v1 = 0.0;
        vec1[0] = vec1[1] = vec1[2] = 0.0;
    }
    else {
        CROSS_PRODUCT_1(rd,Q,vec1);
        v1 = VEC_LEN_1(vec1);
        cos_beta = 0.0;
        Qr = Qt = 0.0;
    }
    for (k = 0; k < neeg; k++) {
        for (p = 0; p < 3; p++)
            pos[p] = el[k][p] - m->r0[p];
        /*
         * Should the position be scaled or not?
         */
        if (m->scale_pos) {
            pos_len = m->layers[m->nlayer()-1].rad/VEC_LEN_1(pos);
#ifdef DEBUG
            printf("%10.4f %10.4f %10.4f %10.4f\n",pos_len,1000*pos[0],1000*pos[1],1000*pos[2]);
#endif
            for (p = 0; p < 3; p++)
                pos[p] = pos_len*pos[p];
        }
        pos2 = VEC_DOT_1(pos,pos);
        pos_len = sqrt(pos2);
        /*
         * Calculate the two ingredients for the final result
         */
        cos_gamma = VEC_DOT_1(pos,rd)/(rd_len*pos_len);
        beta = rd_len/pos_len;
        calc_pot_components(beta,cos_gamma,&Vr,&Vt,m->fn,m->nterms);
        /*
         * Then compute the combined result
         */
        if (v1 > 0.0) {
            CROSS_PRODUCT_1(rd,pos,vec2);
            v2 = VEC_LEN_1(vec2);

            if (v2  > 0.0)
                cos_beta = VEC_DOT_1(vec1,vec2)/(v1*v2);
            else
                cos_beta = 0.0;

            Qr = VEC_DOT_1(Q,rd)/rd_len;
            Qt = sqrt(Q2 - Qr*Qr);
        }
        Vval[k] = pi4_inv*(Qr*Vr + Qt*cos_beta*Vt)/pos2;
    }
    /*
       * Scale by the conductivity if we have the layers
       * defined
       */
    if (m->nlayer() > 0) {
        sigmaM_inv = 1.0/m->layers[m->nlayer()-1].sigma;
        for (k = 0; k < neeg; k++)
            Vval[k] = Vval[k]*sigmaM_inv;
    }
    return OK;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_multi_spherepot_coil1(float *rd, float *Q, FwdCoilSet *els, float *Vval, void *client)           /* Client data will be the sphere model definition */
/*
 * Calculate the EEG in the sphere model using the fwdCoilSet structure
 *
 * This version does not use the acceleration with help of equivalent sources
 * in the homogeneous model
 *
 */
{
    float *vval_one = NULL,val;
    int   nvval = 0;
    int   k,c;
    FwdCoil* el;

    for (k = 0; k < els->ncoil; k++, el++) {
        el = els->coils[k];
        if (el->coil_class == FWD_COILC_EEG) {
            if (el->np > nvval) {
                vval_one = REALLOC_1(vval_one,el->np,float);
                nvval = el->np;
            }
            if (fwd_eeg_multi_spherepot(rd,Q,el->rmag,el->np,vval_one,client) != OK) {
                FREE(vval_one);
                return FAIL;
            }
            for (c = 0, val = 0.0; c < el->np; c++)
                val += el->w[c]*vval_one[c];
            *Vval = val;
        }
        Vval++;
    }
    FREE(vval_one);
    return OK;
}

//=============================================================================================================
// fwd_multi_spherepot.c
bool FwdEegSphereModel::fwd_eeg_spherepot_vec( float   *rd, float   **el, int neeg, float **Vval_vec, void *client)
{
    FwdEegSphereModel* m = (FwdEegSphereModel*)client;
    float fact = 0.25f/(float)M_PI;
    float a_vec[3];
    float a,a2,a3;
    float rrd,rd2,rd2_inv,r,r2,ra,rda;
    float F;
    float c1,c2,m1,m2;
    int   k,p,eq;
    float *this_pos;
    float orig_rd[3],scaled_rd[3];
    float pos[3],pos_len;
    /*
   * Shift to the sphere model coordinates
   */
    for (p = 0; p < 3; p++)
        orig_rd[p] = rd[p] - m->r0[p];
    rd = scaled_rd;
    /*
   * Initialize the arrays
   */
    for (k = 0 ; k < neeg ; k++) {
        Vval_vec[X_1][k] = 0.0;
        Vval_vec[Y_1][k] = 0.0;
        Vval_vec[Z_1][k] = 0.0;
    }
    /*
   * Ignore dipoles outside the innermost sphere
   */
    if (VEC_LEN_1(orig_rd) >= m->layers[0].rad)
        return true;
    /*
   * Default to homogeneous model if no model was previously set
   */
#ifdef FOO
    if (nequiv == 0) /* what to do */
        eeg_set_homog_sphere_model();
#endif
    /*
   * Make a weighted sum over the equivalence parameters
   */
    for (eq = 0; eq < m->nfit; eq++) {
        /*
     * Scale the dipole position
     */
        for (p = 0; p < 3; p++)
            rd[p] = m->mu[eq]*orig_rd[p];

        rd2     = VEC_DOT_1(rd,rd);
        rd2_inv = 1.0/rd2;

        /*
     * Go over all electrodes
     */
        for (k = 0; k < neeg ; k++) {
            this_pos = el[k];

            for (p = 0; p < 3; p++)
                pos[p] = this_pos[p] - m->r0[p];
            /*
       * Scale location onto the surface of the sphere
       */
            if (m->scale_pos) {
                pos_len = m->layers[m->nlayer()-1].rad/VEC_LEN_1(pos);
                for (p = 0; p < 3; p++)
                    pos[p] = pos_len*pos[p];
            }
            this_pos = pos;

            /* Vector from dipole to the field point */

            VEC_DIFF_1 (rd,this_pos,a_vec);

            /* Compute the dot products needed */

            a2  = VEC_DOT_1(a_vec,a_vec);       a = sqrt(a2);
            a3  = 2.0/(a2*a);
            r2  = VEC_DOT_1(this_pos,this_pos); r = sqrt(r2);
            rrd = VEC_DOT_1(this_pos,rd);
            ra  = r2 - rrd;
            rda = rrd - rd2;

            /* The main ingredients */

            F  = a*(r*a + ra);
            c1 = a3*rda + 1.0/a - 1.0/r;
            c2 = a3 + (a+r)/(r*F);

            /* Mix them together and scale by lambda/(rd*rd) */

            m1 = (c1 - c2*rrd);
            m2 = c2*rd2;

            Vval_vec[X_1][k] = Vval_vec[X_1][k] + m->lambda[eq]*rd2_inv*(m1*rd[X_1] + m2*this_pos[X_1]);
            Vval_vec[Y_1][k] = Vval_vec[Y_1][k] + m->lambda[eq]*rd2_inv*(m1*rd[Y_1] + m2*this_pos[Y_1]);
            Vval_vec[Z_1][k] = Vval_vec[Z_1][k] + m->lambda[eq]*rd2_inv*(m1*rd[Z_1] + m2*this_pos[Z_1]);
        }             /* All electrodes done */
    }               /* All equivalent dipoles done */
    /*
   * Finish by scaling by 1/(4*M_PI);
   */
    for (k = 0; k  < neeg; k++) {
        Vval_vec[X_1][k] = fact*Vval_vec[X_1][k];
        Vval_vec[Y_1][k] = fact*Vval_vec[Y_1][k];
        Vval_vec[Z_1][k] = fact*Vval_vec[Z_1][k];
    }
    return true;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot_coil_vec(float *rd, FwdCoilSet* els, float **Vval_vec, void *client)
{
    float **vval_one = NULL;
    float val;
    int   nvval = 0;
    int   k,c,p;
    FwdCoil* el;

    for (k = 0; k < els->ncoil; k++, el++) {
        el = els->coils[k];
        if (el->coil_class == FWD_COILC_EEG) {
            if (el->np > nvval) {
                FREE_CMATRIX_1(vval_one);
                vval_one = ALLOC_CMATRIX_1(3,el->np);
                nvval = el->np;
            }
            if (!fwd_eeg_spherepot_vec(rd,el->rmag,el->np,vval_one,client)) {
                FREE_CMATRIX_1(vval_one);
                return FAIL;
            }
            for (p = 0; p < 3; p++) {
                for (c = 0, val = 0.0; c < el->np; c++)
                    val += el->w[c]*vval_one[p][c];
                Vval_vec[p][k] = val;
            }
        }
    }
    FREE_CMATRIX_1(vval_one);
    return OK;
}

//=============================================================================================================

int FwdEegSphereModel::fwd_eeg_spherepot_grad_coil(float *rd, float Q[], FwdCoilSet *coils, float Vval[], float xgrad[], float ygrad[], float zgrad[], void *client)  /* Client data to be passed to some foward modelling routines */
/*
          * Quick and dirty solution: use differences
          *
          * This routine uses the acceleration with help of equivalent sources
          * in the homogeneous sphere.
          *
          */
{
    float my_rd[3];
    float step  = 0.0005;
    float step2 = 2*step;
    float *grads[3];
    int   p,q;

    grads[0] = xgrad;
    grads[1] = ygrad;
    grads[2] = zgrad;

    for (p = 0; p < 3; p++) {
        VEC_COPY_1(my_rd,rd);
        my_rd[p] = my_rd[p] + step;
        if (fwd_eeg_spherepot_coil(my_rd,Q,coils,grads[p],client) == FAIL)
            return FAIL;
        VEC_COPY_1(my_rd,rd);
        my_rd[p] = my_rd[p] - step;
        if (fwd_eeg_spherepot_coil(my_rd,Q,coils,Vval,client) == FAIL)
            return FAIL;
        for (q = 0; q < coils->ncoil; q++)
            grads[p][q] = (grads[p][q]-Vval[q])/step2;
    }
    if (Vval) {
        if (fwd_eeg_spherepot_coil(rd,Q,coils,Vval,client) == FAIL)
            return FAIL;
    }
    return OK;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot(   float   *rd,       /* Dipole position */
                                            float   *Q,	 /* Dipole moment */
                                            float   **el,	 /* Electrode positions */
                                            int     neeg,	 /* Number of electrodes */
                                            VectorXf& Vval,	 /* The potential values */
                                            void    *client)
/*
      * This routine calculates the potentials for a specific dipole direction
      *
      * This routine uses the acceleration with help of equivalent sources
      * in the homogeneous sphere.
      */
{
    FwdEegSphereModel* m = (FwdEegSphereModel*)client;
    float fact = 0.25f/M_PI;
    float a_vec[3];
    float a,a2,a3;
    float rrd,rd2,rd2_inv,r,r2,ra,rda;
    float F;
    float c1,c2,m1,m2,f1,f2;
    int   k,p,eq;
    float *this_pos;
    float orig_rd[3],scaled_rd[3];
    float pos[3],pos_len;
    /*
   * Shift to the sphere model coordinates
   */
    for (p = 0; p < 3; p++)
        orig_rd[p] = rd[p] - m->r0[p];
    rd = scaled_rd;
    /*
   * Initialize the arrays
   */
    for (k = 0 ; k < neeg ; k++)
        Vval[k] = 0.0;
    /*
   * Ignore dipoles outside the innermost sphere
   */
    if (VEC_LEN_1(orig_rd) >= m->layers[0].rad)
        return true;
    /*
   * Default to homogeneous model if no model was previously set
   */
#ifdef FOO
    if (nequiv == 0) /* what to do */
        eeg_set_homog_sphere_model();
#endif
    /*
   * Make a weighted sum over the equivalence parameters
   */
    for (eq = 0; eq < m->nfit; eq++) {
        /*
     * Scale the dipole position
     */
        for (p = 0; p < 3; p++)
            rd[p] = m->mu[eq]*orig_rd[p];

        rd2     = VEC_DOT_1(rd,rd);
        rd2_inv = 1.0/rd2;

        f1 = VEC_DOT_1(rd,Q);
        /*
     * Go over all electrodes
     */
        for (k = 0; k < neeg ; k++) {
            this_pos = el[k];

            for (p = 0; p < 3; p++)
                pos[p] = this_pos[p] - m->r0[p];
            /*
       * Scale location onto the surface of the sphere
       */
            if (m->scale_pos) {
                pos_len = m->layers[m->nlayer()-1].rad/VEC_LEN_1(pos);
                for (p = 0; p < 3; p++)
                    pos[p] = pos_len*pos[p];
            }
            this_pos = pos;

            /* Vector from dipole to the field point */

            VEC_DIFF_1 (rd,this_pos,a_vec);

            /* Compute the dot products needed */

            a2  = VEC_DOT_1(a_vec,a_vec);       a = sqrt(a2);
            a3  = 2.0/(a2*a);
            r2  = VEC_DOT_1(this_pos,this_pos); r = sqrt(r2);
            rrd = VEC_DOT_1(this_pos,rd);
            ra  = r2 - rrd;
            rda = rrd - rd2;

            /* The main ingredients */

            F  = a*(r*a + ra);
            c1 = a3*rda + 1.0/a - 1.0/r;
            c2 = a3 + (a+r)/(r*F);

            /* Mix them together and scale by lambda/(rd*rd) */

            m1 = (c1 - c2*rrd);
            m2 = c2*rd2;

            f2 = VEC_DOT_1(this_pos,Q);
            Vval[k] = Vval[k] + m->lambda[eq]*rd2_inv*(m1*f1 + m2*f2);
        }             /* All electrodes done */
    }               /* All equivalent dipoles done */
    /*
   * Finish by scaling by 1/(4*M_PI);
   */
    for (k = 0; k  < neeg; k++)
        Vval[k] = fact*Vval[k];
    return OK;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot_coil(  float *rd, float *Q, FwdCoilSet* els, float *Vval, void *client)
{
    VectorXf vval_one;
    float val;
    int   nvval = 0;
    int   k,c;
    FwdCoil* el;

    for (k = 0; k < els->ncoil; k++, el++) {
        el = els->coils[k];
        if (el->coil_class == FWD_COILC_EEG) {
            if (el->np > nvval) {
                vval_one.resize(el->np);
                nvval = el->np;
            }
            if (fwd_eeg_spherepot(rd,Q,el->rmag,el->np,vval_one,client) != OK) {
                return FAIL;
            }
            for (c = 0, val = 0.0; c < el->np; c++)
                val += el->w[c]*vval_one[c];
            *Vval = val;
        }
        Vval++;
    }
    return OK;
}

//=============================================================================================================
// fwd_eeg_sphere_models.c
bool FwdEegSphereModel::fwd_setup_eeg_sphere_model(float rad, bool fit_berg_scherg, int nfit)
{
    int nterms = 200;
    float  rv;

    /*
     * Scale the relative radiuses
     */
    for (int k = 0; k < this->nlayer(); k++)
        this->layers[k].rad = rad*this->layers[k].rel_rad;

    if (fit_berg_scherg) {
        if (this->fwd_eeg_fit_berg_scherg(nterms,nfit,rv)) {
            printf("Equiv. model fitting -> ");
            printf("RV = %g %%\n",100*rv);
            for (int k = 0; k < nfit; k++)
                printf("mu%d = %g\tlambda%d = %g\n", k+1,this->mu[k],k+1,this->layers[this->nlayer()-1].sigma*this->lambda[k]);
        }
        else
            return false;
    }

    printf("Defined EEG sphere model with rad = %7.2f mm\n", 1000.0*rad);
    return true;
}

Eigen::MatrixXd toDoubleEigenMatrix(double **mat, const int m, const int n)
{
    Eigen::MatrixXd eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromDoubleEigenMatrix(const Eigen::MatrixXd& from_mat, double **to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromDoubleEigenMatrix(const Eigen::MatrixXd& from_mat, double **to_mat)
{
    fromDoubleEigenMatrix(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

void fromDoubleEigenVector(const Eigen::VectorXd& from_vec, double *to_vec, const int n)
{
    for ( int i = 0; i < n; ++i)
        to_vec[i] = from_vec[i];
}

void fromDoubleEigenVector(const Eigen::VectorXd& from_vec, double *to_vec)
{
    fromDoubleEigenVector(from_vec, to_vec, from_vec.size());
}

//============================= fwd_fit_berg_scherg.c
static double dot_dvectors (double *v1,
                            double *v2,
                            int   nn)
{
    double result = 0.0;
    int   k;

    for (k = 0; k < nn; k++)
        result = result + v1[k]*v2[k];
    return (result);
}
//============================= fwd_fit_berg_scherg.c
static int c_dsvd(double **mat,		/* The matrix */
                  int   m,int n,	/* m rows n columns */
                  double *sing,	        /* Singular values (must have size
                                                           * MIN(m,n)+1 */
                  double **uu,		/* Left eigenvectors */
                  double **vv)		/* Right eigenvectors */
/*
      * Compute the SVD of mat.
      * The singular vector calculations depend on whether
      * or not u and v are given.
      * The allocations should be done as follows
      *
      * mat = ALLOC_DCMATRIX(m,n);
      * vv  = ALLOC_DCMATRIX(MIN(m,n),n);
      * uu  = ALLOC_DCMATRIX(MIN(m,n),m);
      * sing = MALLOC(MIN(m,n),double);
      *
      * mat is modified by this operation
      *
      * This simply allocates the workspace and calls the
      * LAPACK Fortran routine
      */
{
    int    udim = MIN_1(m,n);

    Eigen::MatrixXd eigen_mat = toDoubleEigenMatrix(mat, m, n);

    //ToDo Optimize computation depending of whether uu or vv are defined
    Eigen::JacobiSVD< Eigen::MatrixXd > svd(eigen_mat,Eigen::ComputeFullU | Eigen::ComputeFullV);

    fromDoubleEigenVector(svd.singularValues(), sing, svd.singularValues().size());

    if ( uu != NULL )
        fromDoubleEigenMatrix(svd.matrixU().transpose(), uu, udim, m);

    if ( vv != NULL )
        fromDoubleEigenMatrix(svd.matrixV().transpose(), vv, n, n);

    return 0;
    //  return info;
}

/*
 * Include the simplex and SVD code here.
 * It is not too much of a problem
 */

//============================= fwd_fit_berg_scherg.c

//============================= fwd_fit_berg_scherg.c
namespace FWDLIB
{

typedef struct {
    double lambda;		/* Magnitude for the apparent dipole */
    double mu;			/* Distance multiplier for the apparent dipole */
} *bergSchergPar,bergSchergParRec;

} // Namepsace

//============================= fwd_fit_berg_scherg.c
static int comp_pars(const void *p1,const void *p2)
/*
      * Comparison function for sorting layers
      */
{
    bergSchergPar v1 = (bergSchergPar)p1;
    bergSchergPar v2 = (bergSchergPar)p2;

    if (v1->mu > v2->mu)
        return -1;
    else if (v1->mu < v2->mu)
        return 1;
    else
        return 0;
}

//============================= fwd_fit_berg_scherg.c
static void sort_parameters(VectorXd& mu,VectorXd& lambda,int nfit)
/*
      * Sort the parameters so that largest mu comes first
      */
{
    bergSchergPar pars = MALLOC_1(nfit,bergSchergParRec);

    for (int k = 0; k < nfit; k++) {
        pars[k].mu = mu[k];
        pars[k].lambda = lambda[k];
    }
    
    qsort (pars, nfit, sizeof(bergSchergParRec), comp_pars);

    for (int k = 0; k < nfit; k++) {
        mu[k]     = pars[k].mu;
        lambda[k] = pars[k].lambda;
    }
    
    FREE(pars);
}

//============================= fwd_fit_berg_scherg.c
static bool report_fit(int    loop,
                      const VectorXd &fitpar,
                      double Smin)

/*
      * Report our progress
      */
{
#ifdef LOG_FIT
    for (int k = 0; k < fitpar.size(); k++)
        printf("%g ",mu[k]);
    printf("%g\n",Smin);
#endif
    return true;
}

//============================= fwd_fit_berg_scherg.c
static MatrixXd get_initial_simplex(const VectorXd &pars,
                                    double simplex_size)

{
    int npar = pars.size();

    MatrixXd simplex = MatrixXd::Zero(npar+1,npar);

    simplex.rowwise() += pars.transpose();

    for (int k = 1; k < npar+1; k++)
        simplex(k,k-1) += simplex_size;

    return simplex;
}

void FwdEegSphereModel::compose_linear_fitting_data(const VectorXd& mu,fitUser u)
{
    double mu1n,k1;
    int k,p;
    /*
   * y is the data to be fitted (nterms-1 x 1)
   * M is the model matrix      (nterms-1 x nfit-1)
   */
    for (k = 0; k < u->nterms-1; k++) {
        k1 = k + 1;
        mu1n = pow(mu[0],k1);
        u->y[k] = u->w[k]*(u->fn[k+1] - mu1n*u->fn[0]);
        for (p = 0; p < u->nfit-1; p++)
            u->M[k][p] = u->w[k]*(pow(mu[p+1],k1)-mu1n);
    }
}

// fwd_fit_berg_scherg.c
double FwdEegSphereModel::compute_linear_parameters(const VectorXd& mu,
                                        VectorXd& lambda,
                                        fitUser u)
/*
      * Compute the best-fitting linear parameters
      * Return the corresponding RV
      */
{
    int k,p,q;
    VectorXd vec(u->nfit-1);
    double sum;

    compose_linear_fitting_data(mu,u);

    c_dsvd(u->M,u->nterms-1,u->nfit-1,u->sing,u->uu,u->vv);
    /*
   * Compute the residuals
   */
    for (k = 0; k < u->nterms-1; k++)
        u->resi[k] = u->y[k];

    for (p = 0; p < u->nfit-1; p++) {
        vec[p] = dot_dvectors(u->uu[p],u->y,u->nterms-1);
        for (k = 0; k < u->nterms-1; k++)
            u->resi[k] = u->resi[k] - u->uu[p][k]*vec[p];
        vec[p] = vec[p]/u->sing[p];
    }

    for (p = 0; p < u->nfit-1; p++) {
        for (q = 0, sum = 0.0; q < u->nfit-1; q++)
            sum += u->vv[q][p]*vec[q];
        lambda[p+1] = sum;
    }
    for (p = 1, sum = 0.0; p < u->nfit; p++)
        sum += lambda[p];
    lambda[0] = u->fn[0] - sum;
    return dot_dvectors(u->resi,u->resi,u->nterms-1)/dot_dvectors(u->y,u->y,u->nterms-1);
}

// fwd_fit_berg_scherg.c
double FwdEegSphereModel::one_step (const VectorXd& mu, const void *user_data)
/*
      * Evaluate the residual sum of squares fit for one set of
      * mu values
      */
{
    int k,p;
    double  dot;
    fitUser u = (fitUser)user_data;

    for (k = 0; k < u->nfit; k++) {
        if (std::fabs(mu[k]) > 1.0)
            return 1.0;
    }
    /*
   * Compose the data for the linear fitting
   */
    compose_linear_fitting_data(mu,u);
    /*
   * Compute SVD
   */
    c_dsvd(u->M,u->nterms-1,u->nfit-1,u->sing,u->uu,NULL);
    /*
   * Compute the residuals
   */
    for (k = 0; k < u->nterms-1; k++)
        u->resi[k] = u->y[k];
    for (p = 0; p < u->nfit-1; p++) {
        dot = dot_dvectors(u->uu[p],u->y,u->nterms-1);
        for (k = 0; k < u->nterms-1; k++)
            u->resi[k] = u->resi[k] - u->uu[p][k]*dot;
    }
    /*
   * Return their sum of squares
   */
    return dot_dvectors(u->resi,u->resi,u->nterms-1);
}

// fwd_fit_berg_scherg.c
bool FwdEegSphereModel::fwd_eeg_fit_berg_scherg(int   nterms,              /* Number of terms to use in the series expansion
                                                                                    * when fitting the parameters */
                            int   nfit,	               /* Number of equivalent dipoles to fit */
                            float &rv)
/*
      * This routine fits the Berg-Scherg equivalent spherical model
      * dipole parameters by minimizing the difference between the
      * actual and approximative series expansions
      */
{
    bool res = false;
    int   k;
    double rd,R,f;
    double simplex_size = 0.01;
    MatrixXd simplex;
    VectorXd func_val;
    double ftol = 1e-9;
    VectorXd lambda;
    VectorXd mu;
    int   neval;
    int   max_eval = 1000;
    int   report   = 1;
    fitUser u = new_fit_user(nfit,nterms);

    if (nfit < 2) {
        printf("fwd_fit_berg_scherg does not work with less than two equivalent sources.");
        return false;
    }

    /*
   * (1) Calculate the coefficients of the true expansion
   */
    for (k = 0; k < nterms; k++)
        u->fn[k] = this->fwd_eeg_get_multi_sphere_model_coeff(k+1);

    /*
   * (2) Calculate the weighting
   */
    rd = R = this->layers[0].rad;
    for (k = 1; k < this->nlayer(); k++) {
        if (this->layers[k].rad > R)
            R = this->layers[k].rad;
        if (this->layers[k].rad < rd)
            rd = this->layers[k].rad;
    }
    f = rd/R;

#ifdef ZHANG
    /*
   * This is the Zhang weighting
   */
    for (k = 1; k < nterms; k++)
        u->w[k-1] = pow(f,k);
#else
    /*
   * This is the correct weighting
   */
    for (k = 1; k < nterms; k++)
        u->w[k-1] = sqrt((2.0*k+1)*(3.0*k+1.0)/k)*pow(f,(k-1.0));
#endif

    /*
   * (3) Prepare for simplex minimization
   */
    func_val = VectorXd(nfit+1);
    lambda   = VectorXd(nfit);
    mu       = VectorXd(nfit);
    /*
   * (4) Rather arbitrary initial guess
   */
    for (k = 0; k < nfit; k++) {
        /*
    mu[k] = (k+1)*0.1*f;
     */
        mu[k] = (rand() / (RAND_MAX + 1.0))*f;//replacement for: mu[k] = drand48()*f;
    }

    simplex = get_initial_simplex(mu,simplex_size);
    for (k = 0; k < nfit+1; k++)
        func_val[k] = one_step(static_cast<VectorXd>(simplex.row(k)),u);

    /*
   * (5) Do the nonlinear minimization
   */
    if (!(res = SimplexAlgorithm::simplex_minimize<double>( simplex,
                                                            func_val,
                                                            ftol,
                                                            one_step,
                                                            u,
                                                            max_eval,
                                                            neval,
                                                            report,
                                                            report_fit)))
        goto out;

    for (k = 0; k < nfit; k++)
        mu[k] = simplex(0,k);

    /*
   * (6) Do the final step: calculation of the linear parameters
   */
    rv = compute_linear_parameters(mu,lambda,u);

    sort_parameters(mu,lambda,nfit);
#ifdef LOG_FIT
    printf("RV = %g %%\n",100*rv);
#endif
    this->mu.resize(nfit);
    this->lambda.resize(nfit);
    this->nfit   = nfit;
    for (k = 0; k < nfit; k++) {
        this->mu[k] = mu[k];
        /*
     * This division takes into account the actual conductivities
     */
        this->lambda[k] = lambda[k]/this->layers[this->nlayer()-1].sigma;
#ifdef LOG_FIT
        printf("lambda%d = %g\tmu%d = %g\n",k+1,lambda[k],k+1,mu[k]);
#endif
    }

    /*
   * This is the cleanup code
   */
out : {
        if (u) {
            FREE(u->fn);
            FREE_DCMATRIX_1(u->M);
            FREE_DCMATRIX_1(u->uu);
            FREE_DCMATRIX_1(u->vv);
            FREE(u->y);
            FREE(u->w);
            FREE(u->resi);
            FREE(u->sing);
        }
        return res;
    }
}
