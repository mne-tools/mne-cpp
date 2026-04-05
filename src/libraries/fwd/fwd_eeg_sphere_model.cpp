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

#include "math/simplex_algorithm.h"

#include "fwd_eeg_sphere_model.h"
#include "fwd_eeg_sphere_model_set.h"

#include <qmath.h>

#include <Eigen/Core>

#include <algorithm>
#include <vector>
#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;
using namespace FWDLIB;

//=============================================================================================================
// Local constants and helpers
//=============================================================================================================

namespace {
constexpr int FAIL  = -1;
constexpr int OK    =  0;
constexpr int MAXTERMS = 1000;
constexpr double EPS     = 1e-10;
constexpr double SIN_EPS = 1e-3;
} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModel::FwdEegSphereModel()
: nterms    (0)
, nfit      (0)
, scale_pos (0)
{
    r0.setZero();
}

//=============================================================================================================

FwdEegSphereModel::FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel)
{
    int k;

    if (!p_FwdEegSphereModel.name.isEmpty())
        this->name = p_FwdEegSphereModel.name;
    if (p_FwdEegSphereModel.nlayer() > 0) {
        for (k = 0; k < p_FwdEegSphereModel.nlayer(); k++)
            this->layers.push_back(p_FwdEegSphereModel.layers[k]);
    }
    this->r0 = p_FwdEegSphereModel.r0;
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

FwdEegSphereModel::UPtr FwdEegSphereModel::fwd_create_eeg_sphere_model(const QString& name,
                                                     int nlayer,
                                                     const VectorXf& rads,
                                                     const VectorXf& sigmas)
/*
      * Produce a new sphere model structure
      */
{
    auto new_model = std::make_unique<FwdEegSphereModel>();

    new_model->name   = name;

    for (int k = 0; k < nlayer; k++) {
        FwdEegSphereLayer layer;
        layer.rad = layer.rel_rad = rads[k];
        layer.sigma = sigmas[k];
        new_model->layers.push_back(layer);
    }
    /*
   * Sort...
   */
    std::sort(new_model->layers.begin(), new_model->layers.end(), FwdEegSphereLayer::comp_layers);

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

FwdEegSphereModel::UPtr FwdEegSphereModel::setup_eeg_sphere_model(const QString& eeg_model_file, QString eeg_model_name, float eeg_sphere_rad)
{
    if (eeg_model_name.isEmpty())
        eeg_model_name = QString("Default");

    FwdEegSphereModelSet::UPtr eeg_models(FwdEegSphereModelSet::fwd_load_eeg_sphere_models(eeg_model_file, nullptr));
    eeg_models->fwd_list_eeg_sphere_models();

    FwdEegSphereModel::UPtr eeg_model(eeg_models->fwd_select_eeg_sphere_model(eeg_model_name));
    if (!eeg_model) {
        return nullptr;
    }

    if (!eeg_model->fwd_setup_eeg_sphere_model(eeg_sphere_rad,true,3)) {
        return nullptr;
    }

    qInfo("Using EEG sphere model \"%s\" with scalp radius %7.1f mm",
           eeg_model->name.toUtf8().constData(),1000*eeg_sphere_rad);
    return eeg_model;
}

//=============================================================================================================

fitUser FwdEegSphereModel::new_fit_user(int nfit, int nterms)

{
    fitUser u = new fitUserRec();
    u->y.resize(nterms-1);
    u->resi.resize(nterms-1);
    u->M      = MatrixXd::Zero(nterms-1,nfit-1);
    u->uu     = MatrixXd::Zero(nfit-1,nterms-1);
    u->vv     = MatrixXd::Zero(nfit-1,nfit-1);
    u->sing.resize(nfit);
    u->fn.resize(nterms);
    u->w.resize(nterms);
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
void FwdEegSphereModel::next_legen(int n, double x, double &p0, double &p01, double &p1, double &p11)
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
        help0 = p0;
        help1 = p1;
        p0 = ((2*n-1)*x*help0 - (n-1)*(p01))/n;
        p1 = ((2*n-1)*x*help1 - n*(p11))/(n-1);
        p01 = help0;
        p11 = help1;
    }
    else if (n == 0) {
        p0   = 1.0;
        p1   = 0.0;
    }
    else if (n == 1) {
        p01  = 1.0;
        p0   = x;
        p11  = 0.0;
        p1   = sqrt(1.0-x*x);
    }
    return;
}

//=============================================================================================================

void FwdEegSphereModel::calc_pot_components(double beta, double cgamma, double &Vrp, double &Vtp, const Eigen::VectorXd& fn, int nterms)
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
            break;
        }
        next_legen (n,cgamma,p0,p01,p1,p11);
        multn = betan*fn[n-1];	/* The 2*n + 1 factor is included in fn */
        Vr = Vr + multn*p0;
        Vt = Vt + multn*p1/n;
        betan = beta*betan;
    }
    Vrp = Vr;
    Vtp = Vt;
    return;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_multi_spherepot(const Eigen::Vector3f& rd_in, const Eigen::Vector3f& Q_in, const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& el, int neeg, Eigen::VectorXf& Vval, void *client)
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
    auto* m = static_cast<FwdEegSphereModel*>(client);
    Eigen::Vector3f rd = rd_in - m->r0;
    Eigen::Vector3f Q  = Q_in;
    Eigen::Vector3f pos;
    int    k;
    float  pos2,rd_len,pos_len;
    double beta,cos_gamma,Vr,Vt;
    Eigen::Vector3f vec1, vec2;
    float  v1,v2;
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
    rd_len = rd.norm();
    Q2     = Q.dot(Q);
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
    c = rd.dot(Q)/(rd_len*sqrt(Q2));
    if ((1.0-c*c) < SIN_EPS)	{	/* Almost parallel:
                         * Q is purely radial */
        Qr = sqrt(Q2);
        Qt = 0.0;
        cos_beta = 0.0;
        v1 = 0.0;
        vec1.setZero();
    }
    else {
        vec1 = rd.cross(Q);
        v1 = vec1.norm();
        cos_beta = 0.0;
        Qr = Qt = 0.0;
    }
    for (k = 0; k < neeg; k++) {
        pos = el.row(k).transpose() - m->r0;
        /*
         * Should the position be scaled or not?
         */
        if (m->scale_pos) {
            pos_len = m->layers[m->nlayer()-1].rad/pos.norm();
#ifdef DEBUG
            qInfo("%10.4f %10.4f %10.4f %10.4f",pos_len,1000*pos[0],1000*pos[1],1000*pos[2]);
#endif
            pos *= pos_len;
        }
        pos2 = pos.dot(pos);
        pos_len = sqrt(pos2);
        /*
         * Calculate the two ingredients for the final result
         */
        cos_gamma = pos.dot(rd)/(rd_len*pos_len);
        beta = rd_len/pos_len;
        calc_pot_components(beta,cos_gamma,Vr,Vt,m->fn,m->nterms);
        /*
         * Then compute the combined result
         */
        if (v1 > 0.0) {
            vec2 = rd.cross(pos);
            v2 = vec2.norm();

            if (v2  > 0.0)
                cos_beta = vec1.dot(vec2)/(v1*v2);
            else
                cos_beta = 0.0;

            Qr = Q.dot(rd)/rd_len;
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
int FwdEegSphereModel::fwd_eeg_multi_spherepot_coil1(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet &els, Eigen::Ref<Eigen::VectorXf> Vval, void *client)           /* Client data will be the sphere model definition */
/*
 * Calculate the EEG in the sphere model using the fwdCoilSet structure
 *
 * This version does not use the acceleration with help of equivalent sources
 * in the homogeneous model
 *
 */
{
    VectorXf vval_one;
    float val;
    int   nvval = 0;
    int   k,c;
    FwdCoil* el;

    Vval.resize(els.ncoil());
    for (k = 0; k < els.ncoil(); k++, el++) {
        el = els.coils[k].get();
        if (el->coil_class == FWD_COILC_EEG) {
            if (el->np > nvval) {
                vval_one.resize(el->np);
                nvval = el->np;
            }
            if (fwd_eeg_multi_spherepot(rd,Q,el->rmag,el->np,vval_one,client) != OK) {
                return FAIL;
            }
            for (c = 0, val = 0.0; c < el->np; c++)
                val += el->w[c]*vval_one[c];
            Vval[k] = val;
        }
    }
    return OK;
}

//=============================================================================================================
// fwd_multi_spherepot.c
bool FwdEegSphereModel::fwd_eeg_spherepot_vec(const Eigen::Vector3f& rd_in, const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& el, int neeg, Eigen::MatrixXf& Vval_vec, void *client)
{
    auto* m = static_cast<FwdEegSphereModel*>(client);
    float fact = 0.25f / static_cast<float>(M_PI);
    Eigen::Vector3f a_vec;
    float a,a2,a3;
    float rrd,rd2,rd2_inv,r,r2,ra,rda;
    float F;
    float c1,c2,m1,m2;
    int   k,eq;
    Eigen::Vector3f orig_rd = rd_in - m->r0;
    Eigen::Vector3f rd;
    Eigen::Vector3f pos;
    float pos_len;
    /*
   * Initialize the arrays
   */
    for (k = 0 ; k < neeg ; k++) {
        Vval_vec(0,k) = 0.0;
        Vval_vec(1,k) = 0.0;
        Vval_vec(2,k) = 0.0;
    }
    /*
   * Ignore dipoles outside the innermost sphere
   */
    if (orig_rd.norm() >= m->layers[0].rad)
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
        rd = m->mu[eq] * orig_rd;

        rd2     = rd.dot(rd);
        rd2_inv = 1.0/rd2;

        /*
     * Go over all electrodes
     */
        for (k = 0; k < neeg ; k++) {

            pos = el.row(k).transpose() - m->r0;
            /*
       * Scale location onto the surface of the sphere
       */
            if (m->scale_pos) {
                pos_len = m->layers[m->nlayer()-1].rad/pos.norm();
                pos *= pos_len;
            }

            /* Vector from dipole to the field point */

            a_vec = pos - rd;

            /* Compute the dot products needed */

            a2  = a_vec.dot(a_vec);           a = sqrt(a2);
            a3  = 2.0/(a2*a);
            r2  = pos.dot(pos);               r = sqrt(r2);
            rrd = pos.dot(rd);
            ra  = r2 - rrd;
            rda = rrd - rd2;

            /* The main ingredients */

            F  = a*(r*a + ra);
            c1 = a3*rda + 1.0/a - 1.0/r;
            c2 = a3 + (a+r)/(r*F);

            /* Mix them together and scale by lambda/(rd*rd) */

            m1 = (c1 - c2*rrd);
            m2 = c2*rd2;

            Vval_vec(0,k) = Vval_vec(0,k) + m->lambda[eq]*rd2_inv*(m1*rd[0] + m2*pos[0]);
            Vval_vec(1,k) = Vval_vec(1,k) + m->lambda[eq]*rd2_inv*(m1*rd[1] + m2*pos[1]);
            Vval_vec(2,k) = Vval_vec(2,k) + m->lambda[eq]*rd2_inv*(m1*rd[2] + m2*pos[2]);
        }             /* All electrodes done */
    }               /* All equivalent dipoles done */
    /*
   * Finish by scaling by 1/(4*M_PI);
   */
    for (k = 0; k  < neeg; k++) {
        Vval_vec(0,k) = fact*Vval_vec(0,k);
        Vval_vec(1,k) = fact*Vval_vec(1,k);
        Vval_vec(2,k) = fact*Vval_vec(2,k);
    }
    return true;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot_coil_vec(const Eigen::Vector3f& rd, FwdCoilSet& els, Eigen::Ref<Eigen::MatrixXf> Vval_vec, void *client)
{
    Eigen::MatrixXf vval_one;
    float val;
    int   nvval = 0;
    int   k,c,p;
    FwdCoil* el;

    for (k = 0; k < els.ncoil(); k++, el++) {
        el = els.coils[k].get();
        if (el->coil_class == FWD_COILC_EEG) {
            if (el->np > nvval) {
                vval_one.resize(3, el->np);
                nvval = el->np;
            }
            if (!fwd_eeg_spherepot_vec(rd,el->rmag,el->np,vval_one,client)) {
                return FAIL;
            }
            for (p = 0; p < 3; p++) {
                for (c = 0, val = 0.0; c < el->np; c++)
                    val += el->w[c]*vval_one(p,c);
                Vval_vec(p,k) = val;
            }
        }
    }
    return OK;
}

//=============================================================================================================

int FwdEegSphereModel::fwd_eeg_spherepot_grad_coil(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet &coils, Eigen::Ref<Eigen::VectorXf> Vval, Eigen::Ref<Eigen::VectorXf> xgrad, Eigen::Ref<Eigen::VectorXf> ygrad, Eigen::Ref<Eigen::VectorXf> zgrad, void *client)  /* Client data to be passed to some foward modelling routines */
/*
          * Quick and dirty solution: use differences
          *
          * This routine uses the acceleration with help of equivalent sources
          * in the homogeneous sphere.
          *
          */
{
    Eigen::Vector3f my_rd;
    float step  = 0.0005;
    float step2 = 2*step;
    int   p,q;

    Eigen::Ref<Eigen::VectorXf>* grads[3] = { &xgrad, &ygrad, &zgrad };

    for (p = 0; p < 3; p++) {
        my_rd = rd;
        my_rd[p] += step;
        if (fwd_eeg_spherepot_coil(my_rd,Q,coils,*grads[p],client) == FAIL)
            return FAIL;
        my_rd = rd;
        my_rd[p] -= step;
        if (fwd_eeg_spherepot_coil(my_rd,Q,coils,Vval,client) == FAIL)
            return FAIL;
        for (q = 0; q < coils.ncoil(); q++)
            (*grads[p])[q] = ((*grads[p])[q]-Vval[q])/step2;
    }
    if (fwd_eeg_spherepot_coil(rd,Q,coils,Vval,client) == FAIL)
        return FAIL;
    return OK;
}

//=============================================================================================================
// fwd_multi_spherepot.c
int FwdEegSphereModel::fwd_eeg_spherepot(   const Eigen::Vector3f& rd_in,
                                            const Eigen::Vector3f& Q_in,
                                            const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& el,
                                            int     neeg,
                                            VectorXf& Vval,
                                            void    *client)
/*
      * This routine calculates the potentials for a specific dipole direction
      *
      * This routine uses the acceleration with help of equivalent sources
      * in the homogeneous sphere.
      */
{
    auto* m = static_cast<FwdEegSphereModel*>(client);
    float fact = 0.25f/M_PI;
    Eigen::Vector3f a_vec;
    float a,a2,a3;
    float rrd,rd2,rd2_inv,r,r2,ra,rda;
    float F;
    float c1,c2,m1,m2,f1,f2;
    int   k,eq;
    Eigen::Vector3f orig_rd = rd_in - m->r0;
    Eigen::Vector3f rd;
    Eigen::Vector3f Q = Q_in;
    Eigen::Vector3f pos;
    float pos_len;
    /*
   * Initialize the arrays
   */
    for (k = 0 ; k < neeg ; k++)
        Vval[k] = 0.0;
    /*
   * Ignore dipoles outside the innermost sphere
   */
    if (orig_rd.norm() >= m->layers[0].rad)
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
        rd = m->mu[eq] * orig_rd;

        rd2     = rd.dot(rd);
        rd2_inv = 1.0/rd2;

        f1 = rd.dot(Q);
        /*
     * Go over all electrodes
     */
        for (k = 0; k < neeg ; k++) {

            pos = el.row(k).transpose() - m->r0;
            /*
       * Scale location onto the surface of the sphere
       */
            if (m->scale_pos) {
                pos_len = m->layers[m->nlayer()-1].rad/pos.norm();
                pos *= pos_len;
            }

            /* Vector from dipole to the field point */

            a_vec = pos - rd;

            /* Compute the dot products needed */

            a2  = a_vec.dot(a_vec);           a = sqrt(a2);
            a3  = 2.0/(a2*a);
            r2  = pos.dot(pos);               r = sqrt(r2);
            rrd = pos.dot(rd);
            ra  = r2 - rrd;
            rda = rrd - rd2;

            /* The main ingredients */

            F  = a*(r*a + ra);
            c1 = a3*rda + 1.0/a - 1.0/r;
            c2 = a3 + (a+r)/(r*F);

            /* Mix them together and scale by lambda/(rd*rd) */

            m1 = (c1 - c2*rrd);
            m2 = c2*rd2;

            f2 = pos.dot(Q);
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
int FwdEegSphereModel::fwd_eeg_spherepot_coil(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& els, Eigen::Ref<Eigen::VectorXf> Vval, void *client)
{
    VectorXf vval_one;
    float val;
    int   nvval = 0;
    int   k,c;
    FwdCoil* el;

    for (k = 0; k < els.ncoil(); k++, el++) {
        el = els.coils[k].get();
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
            Vval[k] = val;
        }
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
            qInfo("Equiv. model fitting -> RV = %g %%",100*rv);
            for (int k = 0; k < nfit; k++)
                qInfo("mu%d = %g\tlambda%d = %g", k+1,this->mu[k],k+1,this->layers[this->nlayer()-1].sigma*this->lambda[k]);
        }
        else
            return false;
    }

    qInfo("Defined EEG sphere model with rad = %7.2f mm", 1000.0*rad);
    return true;
}

static void compute_svd(Eigen::MatrixXd& mat,
                        Eigen::VectorXd& sing,
                        Eigen::MatrixXd& uu,
                        Eigen::MatrixXd* vv)
/*
 * Compute the SVD of mat.
 * Results are stored in sing, uu, and optionally vv.
 * mat is not modified.
 */
{
    int udim = std::min(static_cast<int>(mat.rows()), static_cast<int>(mat.cols()));

    Eigen::JacobiSVD<Eigen::MatrixXd> svd(mat, Eigen::ComputeFullU | Eigen::ComputeFullV);

    sing = svd.singularValues();
    uu = svd.matrixU().transpose().topRows(udim);

    if (vv != nullptr)
        *vv = svd.matrixV().transpose();
}

/*
 * Include the simplex and SVD code here.
 * It is not too much of a problem
 */


namespace FWDLIB
{

/**
 * @brief Berg-Scherg parameter pair (magnitude and distance multiplier) for an equivalent dipole in the EEG sphere model approximation.
 */
struct BergSchergPar {
    double lambda;		/**< Magnitude for the apparent dipole. */
    double mu;			/**< Distance multiplier for the apparent dipole. */
};

} // namespace

static void sort_parameters(VectorXd& mu,VectorXd& lambda,int nfit)
{
    std::vector<BergSchergPar> pars(nfit);

    for (int k = 0; k < nfit; k++) {
        pars[k].mu = mu[k];
        pars[k].lambda = lambda[k];
    }

    std::sort(pars.begin(), pars.end(), [](const BergSchergPar& a, const BergSchergPar& b) {
        return a.mu > b.mu;
    });

    for (int k = 0; k < nfit; k++) {
        mu[k]     = pars[k].mu;
        lambda[k] = pars[k].lambda;
    }
}

static bool report_fit(int    loop,
                      const VectorXd &fitpar,
                      double Smin,
                      double /*fval_hi*/,
                      double /*par_diff*/)
{
#ifdef LOG_FIT
    for (int k = 0; k < fitpar.size(); k++)
        qInfo("%g ",mu[k]);
    qInfo("%g",Smin);
#endif
    return true;
}

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
            u->M(k,p) = u->w[k]*(pow(mu[p+1],k1)-mu1n);
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

    compute_svd(u->M, u->sing, u->uu, &u->vv);
    /*
   * Compute the residuals
   */
    for (k = 0; k < u->nterms-1; k++)
        u->resi[k] = u->y[k];

    for (p = 0; p < u->nfit-1; p++) {
        vec[p] = u->uu.row(p).head(u->nterms-1).dot(u->y.head(u->nterms-1));
        for (k = 0; k < u->nterms-1; k++)
            u->resi[k] = u->resi[k] - u->uu(p,k)*vec[p];
        vec[p] = vec[p]/u->sing[p];
    }

    for (p = 0; p < u->nfit-1; p++) {
        for (q = 0, sum = 0.0; q < u->nfit-1; q++)
            sum += u->vv(q,p)*vec[q];
        lambda[p+1] = sum;
    }
    for (p = 1, sum = 0.0; p < u->nfit; p++)
        sum += lambda[p];
    lambda[0] = u->fn[0] - sum;
    return u->resi.head(u->nterms-1).squaredNorm() / u->y.head(u->nterms-1).squaredNorm();
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
    compute_svd(u->M, u->sing, u->uu, nullptr);
    /*
   * Compute the residuals
   */
    for (k = 0; k < u->nterms-1; k++)
        u->resi[k] = u->y[k];
    for (p = 0; p < u->nfit-1; p++) {
        dot = u->uu.row(p).head(u->nterms-1).dot(u->y.head(u->nterms-1));
        for (k = 0; k < u->nterms-1; k++)
            u->resi[k] = u->resi[k] - u->uu(p,k)*dot;
    }
    /*
   * Return their sum of squares
   */
    return u->resi.head(u->nterms-1).squaredNorm();
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
        qWarning("fwd_fit_berg_scherg does not work with less than two equivalent sources.");
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
        func_val[k] = one_step(VectorXd(simplex.row(k).transpose()),u);

    /*
   * (5) Do the nonlinear minimization
   */
    res = SimplexAlgorithm::simplex_minimize<double>( simplex,
                                                      func_val,
                                                      ftol,
                                                      0.0,
                                                      one_step,
                                                      u,
                                                      max_eval,
                                                      neval,
                                                      report,
                                                      report_fit);

    if (res) {
        for (k = 0; k < nfit; k++)
            mu[k] = simplex(0,k);

        /*
       * (6) Do the final step: calculation of the linear parameters
       */
        rv = compute_linear_parameters(mu,lambda,u);

        sort_parameters(mu,lambda,nfit);
#ifdef LOG_FIT
        qInfo("RV = %g %%",100*rv);
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
            qInfo("lambda%d = %g\tmu%d = %g",k+1,lambda[k],k+1,mu[k]);
#endif
        }
    }

    delete u;
    return res;
}
