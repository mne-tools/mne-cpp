//=============================================================================================================
/**
* @file     fwd_eeg_sphere_model.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FwdEegSphereModel Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_model.h"

#include <qmath.h>



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;

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

#define MALLOC_1(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_1(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define ALLOC_DCMATRIX_1(x,y) mne_dmatrix_1((x),(y))

#define ALLOC_CMATRIX_1(x,y) mne_cmatrix_1((x),(y))


/*
* float matrices
*/
#define FREE_CMATRIX_1(m) mne_free_cmatrix_1((m))



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





//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModel::FwdEegSphereModel()
{

}


//*************************************************************************************************************

//FwdEegSphereModel::FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel)
//: name(p_FwdEegSphereModel.name)
//, layers(p_FwdEegSphereModel.layers)
//, r0(p_FwdEegSphereModel.r0)
//, fn(p_FwdEegSphereModel.fn)
//, nterms(p_FwdEegSphereModel.nterms)
//, mu(p_FwdEegSphereModel.mu)
//, lambda(p_FwdEegSphereModel.lambda)
//, nfit(p_FwdEegSphereModel.nfit)
//, scale_pos(p_FwdEegSphereModel.scale_pos)
//{

//}


//*************************************************************************************************************

FwdEegSphereModel::~FwdEegSphereModel()
{

}


//*************************************************************************************************************
// fwd_multi_spherepot.c
double FwdEegSphereModel::fwd_eeg_get_multi_sphere_model_coeff(int n)
{
    double **M,**Mn,**help,**Mm;
    static double **mat1 = NULL;
    static double **mat2 = NULL;
    static double **mat3 = NULL;
    static double *c1 = NULL;
    static double *c2 = NULL;
    static double *cr = NULL;
    static double *cr_mult = NULL;
    double div,div_mult;
    double n1;
#ifdef TEST
    double rel1,rel2;
    double b,c;
#endif
    int    k;

    if (this->nlayer == 0 || this->nlayer == 1)
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
        c1 = REALLOC_1(c1,this->nlayer-1,double);
        c2 = REALLOC_1(c2,this->nlayer-1,double);
        cr = REALLOC_1(cr,this->nlayer-1,double);
        cr_mult = REALLOC_1(cr_mult,this->nlayer-1,double);
        for (k = 0; k < this->nlayer-1; k++) {
            c1[k] = this->layers[k].sigma/this->layers[k+1].sigma;
            c2[k] = c1[k] - 1.0;
            cr_mult[k] = this->layers[k].rel_rad;
            cr[k] = cr_mult[k];
            cr_mult[k] = cr_mult[k]*cr_mult[k];
        }
        if (mat1 == NULL)
            mat1 = ALLOC_DCMATRIX_1(2,2);
        if (mat2 == NULL)
            mat2 = ALLOC_DCMATRIX_1(2,2);
        if (mat3 == NULL)
            mat3 = ALLOC_DCMATRIX_1(2,2);
    }
    /*
   * Increment the radius coefficients
   */
    for (k = 0; k < this->nlayer-1; k++)
        cr[k] = cr[k]*cr_mult[k];
    /*
   * Multiply the matrices
   */
    M  = mat1;
    Mn = mat2;
    Mm = mat3;
    M[0][0] = M[1][1] = 1.0;
    M[0][1] = M[1][0] = 0.0;
    div      = 1.0;
    div_mult = 2.0*n + 1.0;
    n1       = n + 1.0;

    for (k = this->nlayer-2; k >= 0; k--) {

        Mm[0][0] = (n + n1*c1[k]);
        Mm[0][1] = n1*c2[k]/cr[k];
        Mm[1][0] = n*c2[k]*cr[k];
        Mm[1][1] = n1 + n*c1[k];

        Mn[0][0] = Mm[0][0]*M[0][0] + Mm[0][1]*M[1][0];
        Mn[0][1] = Mm[0][0]*M[0][1] + Mm[0][1]*M[1][1];
        Mn[1][0] = Mm[1][0]*M[0][0] + Mm[1][1]*M[1][0];
        Mn[1][1] = Mm[1][0]*M[0][1] + Mm[1][1]*M[1][1];
        help = M;
        M = Mn;
        Mn = help;
        div = div*div_mult;

    }
    return n*div/(n*M[1][1] + n1*M[1][0]);
}


//*************************************************************************************************************
// fwd_multi_spherepot.c
bool FwdEegSphereModel::fwd_eeg_spherepot_vec( float   *rd, float   **el, int neeg, float **Vval_vec, void *client)
{
    FwdEegSphereModel* m = (FwdEegSphereModel*)client;
    float fact = 0.25/M_PI;
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
                pos_len = m->layers[m->nlayer-1].rad/VEC_LEN_1(pos);
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


//*************************************************************************************************************
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


//*************************************************************************************************************
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot(   float   *rd,       /* Dipole position */
                                            float   *Q,	 /* Dipole moment */
                                            float   **el,	 /* Electrode positions */
                                            int     neeg,	 /* Number of electrodes */
                                            float   *Vval,	 /* The potential values */
                                            void    *client)
/*
      * This routine calculates the potentials for a specific dipole direction
      *
      * This routine uses the acceleration with help of equivalent sources
      * in the homogeneous sphere.
      */
{
    FwdEegSphereModel* m = (FwdEegSphereModel*)client;
    float fact = 0.25/M_PI;
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
                pos_len = m->layers[m->nlayer-1].rad/VEC_LEN_1(pos);
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


//*************************************************************************************************************
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot_coil(  float *rd, float *Q, FwdCoilSet* els, float *Vval, void *client)
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
            if (fwd_eeg_spherepot(rd,Q,el->rmag,el->np,vval_one,client) != OK) {
                free(vval_one);
                return FAIL;
            }
            for (c = 0, val = 0.0; c < el->np; c++)
                val += el->w[c]*vval_one[c];
            *Vval = val;
        }
        Vval++;
    }
    free(vval_one);
    return OK;
}
