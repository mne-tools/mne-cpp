//=============================================================================================================
/**
 * @file     fwd_bem_model.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FwdBemModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_bem_model.h"
#include "fwd_bem_solution.h"
#include "fwd_eeg_sphere_model.h"
#include <mne/mne_surface.h>
#include <mne/mne_triangle.h>
#include <mne/mne_source_space.h>

#include "fwd_comp_data.h"

#include <memory>

#include "fwd_thread_arg.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_named_matrix.h>

#include <QFile>
#include <QList>
#include <QThread>
#include <QtConcurrent>

#define _USE_MATH_DEFINES
#include <math.h>

#include <Eigen/Dense>

static const Eigen::Vector3f Qx(1.0f, 0.0f, 0.0f);
static const Eigen::Vector3f Qy(0.0f, 1.0f, 0.0f);
static const Eigen::Vector3f Qz(0.0f, 0.0f, 1.0f);

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

//=============================================================================================================
// Coordinate index constants
//=============================================================================================================

namespace {
constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;
}

//=============================================================================================================
// Inline vector utility functions replacing legacy C macros
//=============================================================================================================

template<typename T1, typename T2, typename T3>
inline void vec_diff(const T1& from, const T2& to, T3* diff)
{
    diff[X] = to[X] - from[X];
    diff[Y] = to[Y] - from[Y];
    diff[Z] = to[Z] - from[Z];
}

template<typename T1, typename T2>
inline void vec_copy(T1* to, const T2& from)
{
    to[X] = from[X];
    to[Y] = from[Y];
    to[Z] = from[Z];
}

template<typename T1, typename T2>
inline auto vec_dot(const T1& x, const T2& y) -> decltype(x[0]*y[0])
{
    return x[X]*y[X] + x[Y]*y[Y] + x[Z]*y[Z];
}

template<typename T>
inline double vec_len(const T& x)
{
    return sqrt(static_cast<double>(x[X]*x[X] + x[Y]*x[Y] + x[Z]*x[Z]));
}

template<typename T1, typename T2, typename T3>
inline void cross_product(const T1& x, const T2& y, T3* xy)
{
    xy[X] =   x[Y]*y[Z] - y[Y]*x[Z];
    xy[Y] = -(x[X]*y[Z] - y[X]*x[Z]);
    xy[Z] =   x[X]*y[Y] - y[X]*x[Y];
}

namespace FWDLIB
{

static struct {
    int  kind;
    const QString name;
} surf_expl[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };

static struct {
    int  method;
    const QString name;
} method_expl[] = { { FWD_BEM_CONSTANT_COLL , "constant collocation" },
{ FWD_BEM_LINEAR_COLL   , "linear collocation" },
{ -1                    , "unknown" } };

}

#define BEM_SUFFIX     "-bem.fif"
#define BEM_SOL_SUFFIX "-bem-sol.fif"


static QString strip_from(const QString& s, const QString& suffix)
{
    QString res;

    if (s.endsWith(suffix)) {
        res = s;
        res.chop(suffix.size());
    }
    else
        res = s;

    return res;
}


namespace FWDLIB
{

/**
 * @brief Lookup record mapping a FIFF coordinate frame integer ID to its human-readable name.
 */
typedef struct {
    int frame;
    const QString name;
} frameNameRec_40;

}

const QString mne_coord_frame_name_40(int frame)

{
    static FWDLIB::frameNameRec_40 frames[] = {
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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdBemModel::FwdBemModel()
: nsurf      (0)
, bem_method (FWD_BEM_UNKNOWN)
, nsol       (0)
, head_mri_t ()
, ip_approach_limit(FWD_BEM_IP_APPROACH_LIMIT)
, use_ip_approach(false)
{
}

//=============================================================================================================

FwdBemModel::~FwdBemModel()
{
    // surfs cleaned up automatically via shared_ptr
    this->fwd_bem_free_solution();
}

//=============================================================================================================

void FwdBemModel::fwd_bem_free_solution()
{
    this->solution.resize(0, 0);
    this->sol_name.clear();
    this->v0.resize(0);
    this->bem_method = FWD_BEM_UNKNOWN;
    this->nsol       = 0;
}

//=============================================================================================================

QString FwdBemModel::fwd_bem_make_bem_sol_name(const QString& name)
/*
     * Make a standard BEM solution file name
     */
{
    QString s1, s2;

    s1 = strip_from(name,".fif");
    s2 = strip_from(s1,"-sol");
    s1 = strip_from(s2,"-bem");
    s2 = QString("%1%2").arg(s1).arg(BEM_SOL_SUFFIX);
    return s2;
}

//=============================================================================================================

const QString& FwdBemModel::fwd_bem_explain_surface(int kind)
{
    int k;

    for (k = 0; surf_expl[k].kind >= 0; k++)
        if (surf_expl[k].kind == kind)
            return surf_expl[k].name;

    return surf_expl[k].name;
}

//=============================================================================================================

const QString& FwdBemModel::fwd_bem_explain_method(int method)

{
    int k;

    for (k = 0; method_expl[k].method >= 0; k++)
        if (method_expl[k].method == method)
            return method_expl[k].name;

    return method_expl[k].name;
}

//=============================================================================================================

int FwdBemModel::get_int(FiffStream::SPtr &stream, const FiffDirNode::SPtr &node, int what, int *res)
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

//=============================================================================================================

MNESurface *FwdBemModel::fwd_bem_find_surface(int kind)
{
    //    if (!model) {
    //        printf("No model specified for fwd_bem_find_surface");
    //        return NULL;
    //    }
    for (int k = 0; k < this->nsurf; k++)
        if (this->surfs[k]->id == kind)
            return this->surfs[k].get();
    printf("Desired surface (%d = %s) not found.", kind,fwd_bem_explain_surface(kind).toUtf8().constData());
    return NULL;
}

//=============================================================================================================

std::unique_ptr<FwdBemModel> FwdBemModel::fwd_bem_load_surfaces(const QString &name, const std::vector<int>& kinds)
/*
 * Load a set of surfaces
 */
{
    std::vector<std::shared_ptr<MNESurface>> surfs;
    const int nkind = static_cast<int>(kinds.size());
    Eigen::VectorXf sigma_tmp(nkind);
    int         j,k;

    if (nkind <= 0) {
        qCritical("No surfaces specified to fwd_bem_load_surfaces");
        return nullptr;
    }

    for (k = 0; k < nkind; k++) {
        float cond = -1.0f;
        MNESurface* s = MNESurface::read_bem_surface(name,kinds[k],TRUE,&cond);
        if (s == NULL)
            return nullptr;
        if (cond < 0.0) {
            qCritical("No conductivity available for surface %s",fwd_bem_explain_surface(kinds[k]).toUtf8().constData());
            delete s;
            return nullptr;
        }
        if (s->coord_frame != FIFFV_COORD_MRI) {
            qCritical("FsSurface %s not specified in MRI coordinates.",fwd_bem_explain_surface(kinds[k]).toUtf8().constData());
            delete s;
            return nullptr;
        }
        sigma_tmp[k] = cond;
        surfs.push_back(std::shared_ptr<MNESurface>(s));
    }
    auto m = std::make_unique<FwdBemModel>();

    m->surf_name = name;
    m->nsurf     = nkind;
    m->surfs     = std::move(surfs);
    m->sigma     = sigma_tmp;
    m->ntri.resize(nkind);
    m->np.resize(nkind);
    m->gamma.resize(nkind, nkind);
    m->source_mult.resize(nkind);
    m->field_mult.resize(nkind);
    /*
     * Build a shifted conductivity array with sigma[-1] = 0 (outside)
     */
    Eigen::VectorXf sigma1(nkind + 1);
    sigma1[0] = 0.0f;
    sigma1.tail(nkind) = m->sigma;
    // sigma[j] below refers to sigma1[j+1], sigma[j-1] to sigma1[j]
    /*
     * Gamma factors and multipliers
     */
    for (j = 0; j < m->nsurf; j++) {
        m->ntri[j] = m->surfs[j]->ntri;
        m->np[j]   = m->surfs[j]->np;
        m->source_mult[j] = 2.0f / (sigma1[j+1] + sigma1[j]);
        m->field_mult[j]  = sigma1[j+1] - sigma1[j];
        for (k = 0; k < m->nsurf; k++)
            m->gamma(j, k) = (sigma1[k+1] - sigma1[k]) / (sigma1[j+1] + sigma1[j]);
    }

    return m;
}

//=============================================================================================================

std::unique_ptr<FwdBemModel> FwdBemModel::fwd_bem_load_homog_surface(const QString &name)
/*
 * Load surfaces for the homogeneous model
 */
{
    return fwd_bem_load_surfaces(name, {FIFFV_BEM_SURF_ID_BRAIN});
}

//=============================================================================================================

std::unique_ptr<FwdBemModel> FwdBemModel::fwd_bem_load_three_layer_surfaces(const QString &name)
/*
 * Load surfaces for three-layer model
 */
{
    return fwd_bem_load_surfaces(name, {FIFFV_BEM_SURF_ID_HEAD, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_BRAIN});
}

//=============================================================================================================

int FwdBemModel::fwd_bem_load_solution(const QString &name, int bem_method)
/*
     * Load the potential solution matrix and attach it to the model
     */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

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
        QList<FiffDirNode::SPtr> nodes = stream->dirtree()->dir_tree_find(FIFFB_BEM);

        if (nodes.size() == 0) {
            printf ("No BEM data in %s",name.toUtf8().constData());
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
        for (k = 0, dim = 0; k < nsurf; k++)
            dim = dim + ((method == FWD_BEM_LINEAR_COLL) ? surfs[k]->np : surfs[k]->ntri);
        if (dims[0] != dim || dims[1] != dim) {
            printf("Expected a %d x %d solution matrix instead of a %d x %d  one",dim,dim,dims[0],dims[1]);
            goto not_found;
        }

        MatrixXf tmp_sol = t_pTag->toFloatMatrix().transpose();
        nsol = dims[1];
    }
    fwd_bem_free_solution();
    sol_name   = name;
    solution   = t_pTag->toFloatMatrix().transpose();
    this->nsol = nsol;
    this->bem_method = method;
    stream->close();

    return TRUE;

bad : {
        stream->close();
        return FAIL;
    }

not_found : {
        stream->close();
        return FALSE;
    }
}

//=============================================================================================================

int FwdBemModel::fwd_bem_set_head_mri_t(const FiffCoordTrans &t)
/*
     * Set the coordinate transformation
     */
{
    if (t.from == FIFFV_COORD_HEAD && t.to == FIFFV_COORD_MRI) {
        head_mri_t = t;
        return OK;
    }
    else if (t.from == FIFFV_COORD_MRI && t.to == FIFFV_COORD_HEAD) {
        head_mri_t = t.inverted();
        return OK;
    }
    else {
        printf ("Improper coordinate transform delivered to fwd_bem_set_head_mri_t");
        return FAIL;
    }
}

//=============================================================================================================

std::unique_ptr<MNESurface> FwdBemModel::make_guesses(MNESurface* guess_surf, float guessrad, const Eigen::Vector3f& guess_r0, float grid, float exclude, float mindist)
{
    QString bemname;
    MNESurface* sphere = NULL;
    std::unique_ptr<MNESurface> res;
    int        k;
    float      dist;

    if (!guess_surf) {
        printf("Making a spherical guess space with radius %7.1f mm...\n",1000*guessrad);

        QFile bemFile(QString(QCoreApplication::applicationDirPath() + "/../resources/general/surf2bem/icos.fif"));
        if ( !QCoreApplication::startingUp() )
            bemFile.setFileName(QCoreApplication::applicationDirPath() + QString("/../resources/general/surf2bem/icos.fif"));
        else if (!bemFile.exists())
            bemFile.setFileName("../resources/general/surf2bem/icos.fif");

        if( !bemFile.exists () ){
            qDebug() << bemFile.fileName() << "does not exists.";
            goto out;
        }

        bemname = bemFile.fileName();

        if ((sphere = MNESurface::read_bem_surface(bemname,9003,FALSE,NULL)) == NULL)
            goto out;

        for (k = 0; k < sphere->np; k++) {
            dist = vec_len(&sphere->rr(k,0));
            sphere->rr(k,X) = guessrad*sphere->rr(k,X)/dist + guess_r0[X];
            sphere->rr(k,Y) = guessrad*sphere->rr(k,Y)/dist + guess_r0[Y];
            sphere->rr(k,Z) = guessrad*sphere->rr(k,Z)/dist + guess_r0[Z];
        }
        if (sphere->add_geometry_info(TRUE) == FAIL)
            goto out;
        guess_surf = sphere;
    }
    else {
        printf("Guess surface (%d = %s) is in %s coordinates\n",
               guess_surf->id,FwdBemModel::fwd_bem_explain_surface(guess_surf->id).toUtf8().constData(),
               mne_coord_frame_name_40(guess_surf->coord_frame).toUtf8().constData());
    }
    printf("Filtering (grid = %6.f mm)...\n",1000*grid);
    res.reset((MNESurface*)MNESourceSpace::make_volume_source_space(*guess_surf,grid,exclude,mindist));

out : {
        if(sphere)
            delete sphere;
        return res;
    }
}

//=============================================================================================================

double FwdBemModel::calc_beta(const Eigen::Vector3d& rk, const Eigen::Vector3d& rk1)

{
    Eigen::Vector3d rkk1 = rk1 - rk;
    double size = rkk1.norm();

    return log((rk.norm() * size + rk.dot(rkk1)) /
              (rk1.norm() * size + rk1.dot(rkk1))) / size;
}

//=============================================================================================================

void FwdBemModel::lin_pot_coeff(const Eigen::Vector3f& from, MNETriangle& to, Eigen::Vector3d& omega)	/* The final result */
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
    double triple;		/* vec_dot(y1 x y2,y3) */
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
    vec_diff(from,to.r1,y1);
    vec_diff(from,to.r2,y2);
    vec_diff(from,to.r3,y3);

    cross_product(y1,y2,cross);
    triple = vec_dot(cross,y3);

    l1 = vec_len(y1);
    l2 = vec_len(y2);
    l3 = vec_len(y3);
    ss = (l1*l2*l3+vec_dot(y1,y2)*l3+vec_dot(y1,y3)*l2+vec_dot(y2,y3)*l1);
    solid  = 2.0*atan2(triple,ss);
    if (std::fabs(solid) < solid_eps) {
        for (k = 0; k < 3; k++)
            omega[k] = 0.0;
    }
    else {
        /*
         * Calculate the magic vector vec_omega
         */
        for (j = 0; j < 3; j++)
            beta[j] = calc_beta(Eigen::Map<const Eigen::Vector3d>(yy[j]),Eigen::Map<const Eigen::Vector3d>(yy[j+1]));
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
        area2 = 2.0*to.area;
        n2 = 1.0/(area2*area2);
        for (k = 0; k < 3; k++) {
            cross_product(yy[k+1],yy[k-1],z);
            vec_diff(yy[k+1],yy[k-1],diff);
            omega[k] = n2*(-area2*vec_dot(z,to.nn)*solid +
                           triple*vec_dot(diff,vec_omega));
        }
    }
#ifdef CHECK
    /*
       * Check it out!
       *
       * omega1 + omega2 + omega3 = solid
       */
    rel1 = (solid + omega[0]+omega[1]+omega[2])/solid;
    /*
       * The other way of evaluating...
       */
    for (j = 0; j < 3; j++)
        check[j] = 0;
    for (k = 0; k < 3; k++) {
        cross_product(to.nn,yy[k],z);
        for (j = 0; j < 3; j++)
            check[j] = check[j] + omega[k]*z[j];
    }
    for (j = 0; j < 3; j++)
        check[j] = -area2*check[j]/triple;
    fprintf (stderr,"(%g,%g,%g) =? (%g,%g,%g)\n",
             check[0],check[1],check[2],
             vec_omega[0],vec_omega[1],vec_omega[2]);
    for (j = 0; j < 3; j++)
        check[j] = check[j] - vec_omega[j];
    rel2 = sqrt(vec_dot(check,check)/vec_dot(vec_omega,vec_omega));
    fprintf (stderr,"err1 = %g, err2 = %g\n",100*rel1,100*rel2);
#endif
    return;
}

//=============================================================================================================

void FwdBemModel::correct_auto_elements(MNESurface& surf, Eigen::MatrixXf& mat)
/*
          * Improve auto-element approximation...
          */
{
    float sum,miss;
    int   nnode = surf.np;
    int   ntri  = surf.ntri;
    int   nmemb;
    int   j,k;
    float pi2 = 2.0*M_PI;
    MNETriangle*   tri;

#ifdef SIMPLE
    for (j = 0; j < nnode; j++) {
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + mat(j,k);
        fprintf (stderr,"row %d sum = %g missing = %g\n",j+1,sum/pi2,
                 1.0-sum/pi2);
        mat(j,j) = pi2 - sum;
    }
#else
    for (j = 0; j < nnode; j++) {
        /*
         * How much is missing?
         */
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + mat(j,k);
        miss  = pi2-sum;
        nmemb = surf.nneighbor_tri[j];
        /*
         * The node itself receives one half
         */
        mat(j,j) = miss/2.0;
        /*
         * The rest is divided evenly among the member nodes...
         */
        miss = miss/(4.0*nmemb);
        for (k = 0,tri = surf.tris.data(); k < ntri; k++,tri++) {
            if (tri->vert[0] == j) {
                mat(j,tri->vert[1]) = mat(j,tri->vert[1]) + miss;
                mat(j,tri->vert[2]) = mat(j,tri->vert[2]) + miss;
            }
            else if (tri->vert[1] == j) {
                mat(j,tri->vert[0]) = mat(j,tri->vert[0]) + miss;
                mat(j,tri->vert[2]) = mat(j,tri->vert[2]) + miss;
            }
            else if (tri->vert[2] == j) {
                mat(j,tri->vert[0]) = mat(j,tri->vert[0]) + miss;
                mat(j,tri->vert[1]) = mat(j,tri->vert[1]) + miss;
            }
        }
        /*
         * Just check it it out...
         *
        for (k = 0, sum = 0; k < nnode; k++)
          sum = sum + mat(j,k);
        fprintf (stderr,"row %d sum = %g\n",j+1,sum/pi2);
        */
    }
#endif
    return;
}

//=============================================================================================================

Eigen::MatrixXf FwdBemModel::fwd_bem_lin_pot_coeff(const std::vector<MNESurface*>& surfs)
/*
 * Calculate the coefficients for linear collocation approach
 */
{
    int   np1,np2,ntri,np_tot,np_max;
    MNETriangle*   tri;
    Eigen::Vector3d omega;
    int    j,k,p,q,c;
    int    joff,koff;
    MNESurface* surf1;
    MNESurface* surf2;

    for (p = 0, np_tot = np_max = 0; p < surfs.size(); p++) {
        np_tot += surfs[p]->np;
        if (surfs[p]->np > np_max)
            np_max = surfs[p]->np;
    }

    Eigen::MatrixXf mat = Eigen::MatrixXf::Zero(np_tot, np_tot);
    Eigen::VectorXd row(np_max);
    for (p = 0, joff = 0; p < surfs.size(); p++, joff = joff + np1) {
        surf1 = surfs[p];
        np1   = surf1->np;
        for (q = 0, koff = 0; q < surfs.size(); q++, koff = koff + np2) {
            surf2 = surfs[q];
            np2   = surf2->np;
            ntri  = surf2->ntri;

            printf("\t\t%s (%d) -> %s (%d) ... ",
                    fwd_bem_explain_surface(surf1->id).toUtf8().constData(),np1,
                    fwd_bem_explain_surface(surf2->id).toUtf8().constData(),np2);

            for (j = 0; j < np1; j++) {
                for (k = 0; k < np2; k++)
                    row[k] = 0.0;
                for (k = 0, tri = surf2->tris.data(); k < ntri; k++,tri++) {
                    /*
               * No contribution from a triangle that
               * this vertex belongs to
               */
                    if (p == q && (tri->vert[0] == j || tri->vert[1] == j || tri->vert[2] == j))
                        continue;
                    /*
               * Otherwise do the hard job
               */
                    lin_pot_coeff(Eigen::Map<const Eigen::Vector3f>(&surf1->rr(j,0)),*tri,omega);
                    for (c = 0; c < 3; c++)
                        row[tri->vert[c]] = row[tri->vert[c]] - omega[c];
                }
                for (k = 0; k < np2; k++)
                    mat(j+joff,k+koff) = row[k];
            }
            if (p == q) {
                Eigen::MatrixXf sub_mat = mat.block(joff, koff, np1, np1);
                correct_auto_elements(*surf1, sub_mat);
                mat.block(joff, koff, np1, np1) = sub_mat;
            }
            printf("[done]\n");
        }
    }
    return mat;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_linear_collocation_solution()
/*
     * Compute the linear collocation potential solution
     */
{
    Eigen::MatrixXf coeff;
    float ip_mult;
    int k;

    fwd_bem_free_solution();

    // Extract raw surface pointers for coefficient functions
    std::vector<MNESurface*> rawSurfs;
    rawSurfs.reserve(nsurf);
    for (auto& s : surfs) rawSurfs.push_back(s.get());

    printf("\nComputing the linear collocation solution...\n");
    fprintf (stderr,"\tMatrix coefficients...\n");
    coeff = fwd_bem_lin_pot_coeff(rawSurfs);
    if (coeff.size() == 0)
        goto bad;

    for (k = 0, nsol = 0; k < nsurf; k++)
        nsol += surfs[k]->np;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    solution = fwd_bem_multi_solution(coeff, &gamma, nsurf, np);
    if (solution.size() == 0)
        goto bad;

    /*
       * IP approach?
       */
    if ((nsurf == 3) &&
            (ip_mult = sigma[nsurf-2]/sigma[nsurf-1]) <= ip_approach_limit) {
        Eigen::MatrixXf ip_solution;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tMatrix coefficients (homog)...\n");
        std::vector<MNESurface*> last_surfs = { surfs.back().get() };
        coeff = fwd_bem_lin_pot_coeff(last_surfs);
        if (coeff.size() == 0)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        ip_solution = fwd_bem_homog_solution(coeff, surfs[nsurf-1]->np);
        if (ip_solution.size() == 0)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");

        fwd_bem_ip_modify_solution(solution, ip_solution, ip_mult, nsurf, this->np);
    }
    bem_method = FWD_BEM_LINEAR_COLL;
    printf("Solution ready.\n");
    return OK;

bad : {
        fwd_bem_free_solution();
        return FAIL;
    }
}

//=============================================================================================================

Eigen::MatrixXf FwdBemModel::fwd_bem_multi_solution(Eigen::MatrixXf& solids, const Eigen::MatrixXf *gamma, int nsurf, const Eigen::VectorXi& ntri)
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
            mult = (gamma == nullptr) ? pi2 : pi2 * (*gamma)(p, q);
            for (j = joff; j < jup; j++)
                for (k = koff; k < kup; k++)
                    solids(j,k) = defl - solids(j,k)*mult;
            koff = kup;
        }
        joff = jup;
    }
    for (k = 0; k < ntot; k++)
        solids(k,k) = solids(k,k) + 1.0;

    Eigen::MatrixXf result = solids.inverse();
    return result;
}

//=============================================================================================================

Eigen::MatrixXf FwdBemModel::fwd_bem_homog_solution(Eigen::MatrixXf& solids, int ntri)
/*
          * Invert I - solids/(2*M_PI)
          * Take deflation into account
          * The matrix is destroyed after inversion
          * This is the homogeneous model case
          */
{
    return fwd_bem_multi_solution (solids,nullptr,1,Eigen::VectorXi::Constant(1,ntri));
}

//=============================================================================================================

void FwdBemModel::fwd_bem_ip_modify_solution(Eigen::MatrixXf &solution, Eigen::MatrixXf& ip_solution, float ip_mult, int nsurf, const Eigen::VectorXi &ntri)
/*
          * Modify the solution according to the IP approach
          */
{
    int s;
    int j,k,joff,koff,ntot,nlast;
    float mult;

    for (s = 0, koff = 0; s < nsurf-1; s++)
        koff = koff + ntri[s];
    nlast = ntri[nsurf-1];
    ntot  = koff + nlast;

    Eigen::VectorXf row(nlast);
    mult = (1.0 + ip_mult)/ip_mult;

    printf("\t\tCombining...");
    printf("t ");
    ip_solution.transposeInPlace();

    for (s = 0, joff = 0; s < nsurf; s++) {
        printf("%d3 ",s+1);
        /*
         * For each row in this surface block, compute dot products
         * with the transposed ip_solution and subtract 2x the result
         */
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++) {
                row[k] = solution.row(j + joff).segment(koff, nlast).dot(ip_solution.row(k).head(nlast));
            }
            solution.row(j + joff).segment(koff, nlast) -= 2.0f * row.transpose();
        }
        joff = joff + ntri[s];
    }

    printf("t ");
    ip_solution.transposeInPlace();

    printf("33 ");
    /*
     * The lower right corner is a special case
     */
    for (j = 0; j < nlast; j++)
        for (k = 0; k < nlast; k++)
            solution(j + koff, k + koff) += mult * ip_solution(j,k);
    /*
     * Final scaling
     */
    printf("done.\n\t\tScaling...");
    solution *= ip_mult;
    printf("done.\n");
    return;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_check_solids(const Eigen::MatrixXf& angles, int ntri1, int ntri2, float desired)
/*
 * Check the angle computations
 */
{
    float sum;
    int j,k;
    int res = 0;

    Eigen::VectorXf sums(ntri1);
    for (j = 0; j < ntri1; j++) {
        sum = 0;
        for (k = 0; k < ntri2; k++)
            sum = sum + angles(j,k);
        sums[j] = sum/(2*M_PI);
    }
    for (j = 0; j < ntri1; j++)
        /*
        * Three cases:
        * same surface: sum = 2*pi
        * to outer:     sum = 4*pi
        * to inner:     sum = 0*pi;
        */
        if (std::fabs(sums[j]-desired) > 1e-4) {
            printf("solid angle matrix: rowsum[%d] = 2PI*%g",
                   j+1,sums[j]);
            res = -1;
            break;
        }
    return res;
}

//=============================================================================================================

Eigen::MatrixXf FwdBemModel::fwd_bem_solid_angles(const std::vector<MNESurface*>& surfs)
/*
          * Compute the solid angle matrix
          */
{
    MNESurface* surf1;
    MNESurface* surf2;
    MNETriangle* tri;
    int ntri1,ntri2,ntri_tot;
    int j,k,p,q;
    int joff,koff;
    float result;
    float desired;

    for (p = 0,ntri_tot = 0; p < surfs.size(); p++)
        ntri_tot += surfs[p]->ntri;

    Eigen::MatrixXf solids = Eigen::MatrixXf::Zero(ntri_tot, ntri_tot);
    for (p = 0, joff = 0; p < surfs.size(); p++, joff = joff + ntri1) {
        surf1 = surfs[p];
        ntri1 = surf1->ntri;
        for (q = 0, koff = 0; q < surfs.size(); q++, koff = koff + ntri2) {
            surf2 = surfs[q];
            ntri2 = surf2->ntri;
            printf("\t\t%s (%d) -> %s (%d) ... ",fwd_bem_explain_surface(surf1->id).toUtf8().constData(),ntri1,fwd_bem_explain_surface(surf2->id).toUtf8().constData(),ntri2);
            for (j = 0; j < ntri1; j++)
                for (k = 0, tri = surf2->tris.data(); k < ntri2; k++, tri++) {
                    if (p == q && j == k)
                        result = 0.0;
                    else
                        result = MNESurfaceOrVolume::solid_angle (surf1->tris[j].cent,*tri);
                    solids(j+joff,k+koff) = result;
                }
            printf("[done]\n");
            if (p == q)
                desired = 1;
            else if (p < q)
                desired = 0;
            else
                desired = 2;
            Eigen::MatrixXf sub_block = solids.block(joff, koff, ntri1, ntri2);
            if (fwd_bem_check_solids(sub_block,ntri1,ntri2,desired) == FAIL) {
                return Eigen::MatrixXf();
            }
        }
    }
    return solids;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_constant_collocation_solution()
/*
     * Compute the solution for the constant collocation approach
     */
{
    Eigen::MatrixXf solids;
    int    k;
    float  ip_mult;

    fwd_bem_free_solution();

    // Extract raw surface pointers for coefficient functions
    std::vector<MNESurface*> rawSurfs;
    rawSurfs.reserve(nsurf);
    for (auto& s : surfs) rawSurfs.push_back(s.get());

    printf("\nComputing the constant collocation solution...\n");
    printf("\tSolid angles...\n");
    solids = fwd_bem_solid_angles(rawSurfs);
    if (solids.size() == 0)
        goto bad;

    for (k = 0, nsol = 0; k < nsurf; k++)
        nsol += surfs[k]->ntri;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    solution = fwd_bem_multi_solution(solids, &gamma, nsurf, ntri);
    if (solution.size() == 0)
        goto bad;
    /*
       * IP approach?
       */
    if ((nsurf == 3) &&
            (ip_mult = sigma[nsurf-2]/sigma[nsurf-1]) <= ip_approach_limit) {
        Eigen::MatrixXf ip_solution;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tSolid angles (homog)...\n");
        std::vector<MNESurface*> last_surfs = { surfs.back().get() };
        solids = fwd_bem_solid_angles(last_surfs);
        if (solids.size() == 0)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        ip_solution = fwd_bem_homog_solution(solids, surfs[nsurf-1]->ntri);
        if (ip_solution.size() == 0)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");
        fwd_bem_ip_modify_solution(solution, ip_solution, ip_mult, nsurf, this->ntri);
    }
    bem_method = FWD_BEM_CONSTANT_COLL;
    fprintf (stderr,"Solution ready.\n");

    return OK;

bad : {
        fwd_bem_free_solution();
        return FAIL;
    }
}

//=============================================================================================================

int FwdBemModel::fwd_bem_compute_solution(int bem_method)
/*
 * Compute the solution
 */
{
    /*
        * Compute the solution
        */
    if (bem_method == FWD_BEM_LINEAR_COLL)
        return fwd_bem_linear_collocation_solution();
    else if (bem_method == FWD_BEM_CONSTANT_COLL)
        return fwd_bem_constant_collocation_solution();

    fwd_bem_free_solution();
    printf ("Unknown BEM method: %d\n",bem_method);
    return FAIL;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_load_recompute_solution(const QString& name, int bem_method, int force_recompute)
/*
 * Load or recompute the potential solution matrix
 */
{
    int solres;

    if (!force_recompute) {
        fwd_bem_free_solution();
        solres = fwd_bem_load_solution(name,bem_method);
        if (solres == TRUE) {
            printf("\nLoaded %s BEM solution from %s\n",fwd_bem_explain_method(this->bem_method).toUtf8().constData(),name.toUtf8().constData());
            return OK;
        }
        else if (solres == FAIL)
            return FAIL;
#ifdef DEBUG
        else
            printf("Desired BEM  solution not available in %s (%s)\n",name,err_get_error());
#endif
    }
    if (bem_method == FWD_BEM_UNKNOWN)
        bem_method = FWD_BEM_LINEAR_COLL;
    return fwd_bem_compute_solution(bem_method);
}

//=============================================================================================================

float FwdBemModel::fwd_bem_inf_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp, const Eigen::Vector3f& dir)
/*
     * Infinite-medium magnetic field
     * (without \mu_0/4\pi)
     */
{
    Eigen::Vector3f diff = rp - rd;
    float diff2 = diff.squaredNorm();
    Eigen::Vector3f cr = Q.cross(diff);

    return cr.dot(dir) / (diff2 * std::sqrt(diff2));
}

//=============================================================================================================

float FwdBemModel::fwd_bem_inf_pot(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp)
/*
     * The infinite medium potential
     */
{
    Eigen::Vector3f diff = rp - rd;
    float diff2 = diff.squaredNorm();
    return Q.dot(diff) / (4.0 * M_PI * diff2 * std::sqrt(diff2));
}

//=============================================================================================================

int FwdBemModel::fwd_bem_specify_els(FwdCoilSet *els)
/*
     * Set up for computing the solution at a set of electrodes
     */
{
    FwdCoil*     el;
    MNESurface*  scalp;
    int         k,p,q,v;
    float       r[3],w[3],dist;
    int         best;
    MNETriangle* tri;
    float       x,y,z;
    FwdBemSolution* sol = nullptr;

    if (solution.size() == 0) {
        printf("Solution not computed in fwd_bem_specify_els");
        goto bad;
    }
    if (!els || els->ncoil() == 0)
        return OK;
    els->fwd_free_coil_set_user_data();
    /*
       * Hard work follows
       */
    els->user_data = std::make_unique<FwdBemSolution>();
    sol = els->user_data.get();

    sol->ncoil = els->ncoil();
    sol->np    = nsol;
    sol->solution  = Eigen::MatrixXf::Zero(sol->ncoil,sol->np);
    /*
       * Go through all coils
       */
    for (k = 0; k < els->ncoil(); k++) {
        el = els->coils[k].get();
        scalp = surfs[0].get();
        /*
         * Go through all 'integration points'
         */
        for (p = 0; p < el->np; p++) {
            vec_copy(r,&el->rmag(p, 0));
            if (!head_mri_t.isEmpty())
                FiffCoordTrans::apply_trans(r,head_mri_t,FIFFV_MOVE);
            best = scalp->project_to_surface(nullptr,Eigen::Map<const Eigen::Vector3f>(r),dist);
            if (best < 0) {
                printf("One of the electrodes could not be projected onto the scalp surface. How come?");
                goto bad;
            }
            if (bem_method == FWD_BEM_CONSTANT_COLL) {
                /*
                 * Simply pick the value at the triangle
                 */
                for (q = 0; q < nsol; q++)
                    sol->solution(k, q) += el->w[p] * solution(best, q);
            }
            else if (bem_method == FWD_BEM_LINEAR_COLL) {
                /*
                 * Calculate a linear interpolation between the vertex values
                 */
                tri = &scalp->tris[best];
                scalp->triangle_coords(Eigen::Map<const Eigen::Vector3f>(r),best,x,y,z);

                w[0] = el->w[p]*(1.0 - x - y);
                w[1] = el->w[p]*x;
                w[2] = el->w[p]*y;
                for (v = 0; v < 3; v++) {
                    for (q = 0; q < nsol; q++)
                        sol->solution(k, q) += w[v] * solution(tri->vert[v], q);
                }
            }
            else {
                printf("Unknown BEM approximation method : %d\n",bem_method);
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

//=============================================================================================================

void FwdBemModel::fwd_bem_pot_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet* els, int all_surfs, Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad)
/*
 * Compute the potentials due to a current dipole
 */
{
    MNETriangle* tri;
    int         ntri;
    int         s,k,p,nsol,pp;
    float       mult;
    Eigen::Vector3f ee;
    Eigen::Vector3f mri_rd = rd;
    Eigen::Vector3f mri_Q = Q;

    Eigen::VectorXf* grads[] = {&xgrad, &ygrad, &zgrad};

    if (v0.size() == 0)
        v0.resize(this->nsol);
    float* v0p = v0.data();

    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(mri_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(mri_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    for (pp = X; pp <= Z; pp++) {
        Eigen::VectorXf& grad = *grads[pp];

        ee = Eigen::Vector3f::Unit(pp);
        if (!head_mri_t.isEmpty())
            FiffCoordTrans::apply_trans(ee.data(),head_mri_t,FIFFV_NO_MOVE);

        for (s = 0, p = 0; s < nsurf; s++) {
            ntri = surfs[s]->ntri;
            tri  = surfs[s]->tris.data();
            mult = source_mult[s];
            for (k = 0; k < ntri; k++, tri++)
                v0p[p++] = mult*fwd_bem_inf_pot_der(mri_rd,mri_Q,tri->cent,ee);
        }
        if (els) {
            FwdBemSolution* sol = els->user_data.get();
            nsol = sol->ncoil;
            for (k = 0; k < nsol; k++)
                grad[k] = sol->solution.row(k).dot(v0);
        }
        else {
            nsol = all_surfs ? this->nsol : surfs[0]->ntri;
            for (k = 0; k < nsol; k++)
                grad[k] = solution.row(k).dot(v0);
        }
    }
    return;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_lin_pot_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *els, int all_surfs, Eigen::VectorXf& pot)
/*
 * Compute the potentials due to a current dipole
 * using the linear potential approximation
 */
{
    float *rr_row;
    int   np;
    int   s,k,p,nsol;
    float mult;
    Eigen::Vector3f mri_rd = rd;
    Eigen::Vector3f mri_Q = Q;

    if (v0.size() == 0)
        v0.resize(this->nsol);
    float* v0p = v0.data();

    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(mri_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(mri_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < nsurf; s++) {
        np     = surfs[s]->np;
        mult   = source_mult[s];
        for (k = 0; k < np; k++)
            v0p[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,Eigen::Map<const Eigen::Vector3f>(&surfs[s]->rr(k,0)));
    }
    if (els) {
        FwdBemSolution* sol = els->user_data.get();
        nsol = sol->ncoil;
        for (k = 0; k < nsol; k++)
            pot[k] = sol->solution.row(k).dot(v0);
    }
    else {
        nsol = all_surfs ? this->nsol : surfs[0]->np;
        for (k = 0; k < nsol; k++)
            pot[k] = solution.row(k).dot(v0);
    }
    return;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_lin_pot_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *els, int all_surfs, Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad)
/*
 * Compute the derivaties of potentials due to a current dipole with respect to the dipole position
 * using the linear potential approximation
 */
{
    int   np;
    int   s,k,p,nsol,pp;
    float mult;
    Eigen::Vector3f mri_rd = rd;
    Eigen::Vector3f mri_Q = Q;
    Eigen::Vector3f ee;

    Eigen::VectorXf* grads[] = {&xgrad, &ygrad, &zgrad};

    if (v0.size() == 0)
        v0.resize(this->nsol);
    float* v0p = v0.data();

    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(mri_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(mri_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    for (pp = X; pp <= Z; pp++) {
        Eigen::VectorXf& grad = *grads[pp];

        ee = Eigen::Vector3f::Unit(pp);
        if (!head_mri_t.isEmpty())
            FiffCoordTrans::apply_trans(ee.data(),head_mri_t,FIFFV_NO_MOVE);

        for (s = 0, p = 0; s < nsurf; s++) {
            np     = surfs[s]->np;
            mult   = source_mult[s];
            for (k = 0; k < np; k++)
                v0p[p++] = mult*fwd_bem_inf_pot_der(mri_rd,mri_Q,Eigen::Map<const Eigen::Vector3f>(&surfs[s]->rr(k,X)),ee);
        }
        if (els) {
            FwdBemSolution* sol = els->user_data.get();
            nsol = sol->ncoil;
            for (k = 0; k < nsol; k++)
                grad[k] = sol->solution.row(k).dot(v0);
        }
        else {
            nsol = all_surfs ? this->nsol : surfs[0]->np;
            for (k = 0; k < nsol; k++)
                grad[k] = solution.row(k).dot(v0);
        }
    }
    return;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_pot_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *els, int all_surfs, Eigen::VectorXf& pot)
/*
          * Compute the potentials due to a current dipole
          */
{
    MNETriangle* tri;
    int         ntri;
    int         s,k,p,nsol;
    float       mult;
    Eigen::Vector3f mri_rd = rd;
    Eigen::Vector3f mri_Q = Q;

    if (v0.size() == 0)
        v0.resize(this->nsol);
    float* v0p = v0.data();

    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(mri_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(mri_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < nsurf; s++) {
        ntri = surfs[s]->ntri;
        tri  = surfs[s]->tris.data();
        mult = source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0p[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,tri->cent);
    }
    if (els) {
        FwdBemSolution* sol = els->user_data.get();
        nsol = sol->ncoil;
        for (k = 0; k < nsol; k++)
            pot[k] = sol->solution.row(k).dot(v0);
    }
    else {
        nsol = all_surfs ? this->nsol : surfs[0]->ntri;
        for (k = 0; k < nsol; k++)
            pot[k] = solution.row(k).dot(v0);
    }
    return;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_pot_els(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *els, float *pot, void *client) /* The model */
/*
     * This version calculates the potential on all surfaces
     */
{
    FwdBemModel*    m = (FwdBemModel*)client;
    FwdBemSolution* sol = els->user_data.get();

    if (!m) {
        printf("No BEM model specified to fwd_bem_pot_els");
        return FAIL;
    }
    if (m->solution.size() == 0) {
        printf("No solution available for fwd_bem_pot_els");
        return FAIL;
    }
    if (!sol || sol->ncoil != els->ncoil()) {
        printf("No appropriate electrode-specific data available in fwd_bem_pot_coils");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
        Eigen::VectorXf potVec(els->ncoil());
        m->fwd_bem_pot_calc(rd,Q,els,FALSE,potVec);
        Eigen::Map<Eigen::VectorXf>(pot, els->ncoil()) = potVec;
    }
    else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
        Eigen::VectorXf potVec(els->ncoil());
        m->fwd_bem_lin_pot_calc(rd,Q,els,FALSE,potVec);
        Eigen::Map<Eigen::VectorXf>(pot, els->ncoil()) = potVec;
    }
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_pot_grad_els(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *els, float *pot, float *xgrad, float *ygrad, float *zgrad, void *client) /* The model */
/*
     * This version calculates the potential on all surfaces
     */
{
    FwdBemModel*    m = (FwdBemModel*)client;
    FwdBemSolution* sol = els->user_data.get();

    if (!m) {
        qCritical("No BEM model specified to fwd_bem_pot_els");
        return FAIL;
    }
    if (m->solution.size() == 0) {
        qCritical("No solution available for fwd_bem_pot_els");
        return FAIL;
    }
    if (!sol || sol->ncoil != els->ncoil()) {
        qCritical("No appropriate electrode-specific data available in fwd_bem_pot_coils");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
        int n = els->ncoil();
        if (pot) {
            Eigen::VectorXf potVec(n);
            m->fwd_bem_pot_calc(rd,Q,els,FALSE,potVec);
            Eigen::Map<Eigen::VectorXf>(pot, n) = potVec;
        }
        Eigen::VectorXf xg(n), yg(n), zg(n);
        m->fwd_bem_pot_grad_calc(rd,Q,els,FALSE,xg,yg,zg);
        Eigen::Map<Eigen::VectorXf>(xgrad, n) = xg;
        Eigen::Map<Eigen::VectorXf>(ygrad, n) = yg;
        Eigen::Map<Eigen::VectorXf>(zgrad, n) = zg;
    }
    else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
        int n = els->ncoil();
        if (pot) {
            Eigen::VectorXf potVec(n);
            m->fwd_bem_lin_pot_calc(rd,Q,els,FALSE,potVec);
            Eigen::Map<Eigen::VectorXf>(pot, n) = potVec;
        }
        Eigen::VectorXf xg(n), yg(n), zg(n);
        m->fwd_bem_lin_pot_grad_calc(rd,Q,els,FALSE,xg,yg,zg);
        Eigen::Map<Eigen::VectorXf>(xgrad, n) = xg;
        Eigen::Map<Eigen::VectorXf>(ygrad, n) = yg;
        Eigen::Map<Eigen::VectorXf>(zgrad, n) = zg;
    }
    else {
        qCritical("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}

//=============================================================================================================

#define ARSINH(x) log((x) + sqrt(1.0+(x)*(x)))

void FwdBemModel::calc_f(const Eigen::Vector3d& xx, const Eigen::Vector3d& yy, Eigen::Vector3d& f0, Eigen::Vector3d& fx, Eigen::Vector3d& fy)
{
    double det = -xx[1]*yy[0] + xx[2]*yy[0] +
            xx[0]*yy[1] - xx[2]*yy[1] - xx[0]*yy[2] + xx[1]*yy[2];

    f0[0] = -xx[2]*yy[1] + xx[1]*yy[2];
    f0[1] = xx[2]*yy[0] - xx[0]*yy[2];
    f0[2] = -xx[1]*yy[0] + xx[0]*yy[1];

    fx[0] =  yy[1] - yy[2];
    fx[1] = -yy[0] + yy[2];
    fx[2] = yy[0] - yy[1];

    fy[0] = -xx[1] + xx[2];
    fy[1] = xx[0] - xx[2];
    fy[2] = -xx[0] + xx[1];

    f0 /= det;
    fx /= det;
    fy /= det;
}

//=============================================================================================================

void FwdBemModel::calc_magic(double u, double z, double A, double B, Eigen::Vector3d& beta, double& D)
{
    double B2 = 1.0 + B*B;
    double ABu = A + B*u;
    D = sqrt(u*u + z*z + ABu*ABu);
    beta[0] = ABu/sqrt(u*u + z*z);
    beta[1] = (A*B + B2*u)/sqrt(A*A + B2*z*z);
    beta[2] = (B*z*z - A*u)/(z*D);
}

//=============================================================================================================

void FwdBemModel::field_integrals(const Eigen::Vector3f& from, MNETriangle& to, double& I1p, Eigen::Vector2d& T, Eigen::Vector2d& S1, Eigen::Vector2d& S2, Eigen::Vector3d& f0, Eigen::Vector3d& fx, Eigen::Vector3d& fy)
{
    double y1[3],y2[3],y3[3];
    double xx[4],yy[4];
    double A,B,z,dx;
    Eigen::Vector3d beta;
    double I1,Tx,Ty,Txx,Tyy,Sxx,mult;
    double S1x,S1y,S2x,S2y;
    double D1,B2;
    int k;
    /*
       * Preliminaries...
       *
       * 1. Move origin to viewpoint...
       *
       */
    vec_diff(from,to.r1,y1);
    vec_diff(from,to.r2,y2);
    vec_diff(from,to.r3,y3);
    /*
       * 2. Calculate local xy coordinates...
       */
    xx[0] = vec_dot(y1,to.ex);
    xx[1] = vec_dot(y2,to.ex);
    xx[2] = vec_dot(y3,to.ex);
    xx[3] = xx[0];

    yy[0] = vec_dot(y1,to.ey);
    yy[1] = vec_dot(y2,to.ey);
    yy[2] = vec_dot(y3,to.ey);
    yy[3] = yy[0];

    calc_f(Eigen::Map<const Eigen::Vector3d>(xx), Eigen::Map<const Eigen::Vector3d>(yy), f0, fx, fy);
    /*
       * 3. Distance of the plane from origin...
       */
    z = vec_dot(y1,to.nn);
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
        calc_magic(xx[k+1],z,A,B,beta,D1);
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
        calc_magic(xx[k],z,A,B,beta,D1);
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
    I1p = I1;
    T[0] = Tx;
    T[1] = Ty;
    S1[0] = S1x;
    S1[1] = S1y;
    S2[0] = S2x;
    S2[1] = -S1x;
}

//=============================================================================================================

double FwdBemModel::one_field_coeff(const Eigen::Vector3f& dest, const Eigen::Vector3f& normal, MNETriangle& tri)
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
    vec_diff(dest,tri.r1,y1);
    vec_diff(dest,tri.r2,y2);
    vec_diff(dest,tri.r3,y3);
    for (j = 0; j < 3; j++)
        beta[j] = calc_beta(Eigen::Map<const Eigen::Vector3d>(yy[j]),Eigen::Map<const Eigen::Vector3d>(yy[j+1]));
    bbeta[0] = beta[2] - beta[0];
    bbeta[1] = beta[0] - beta[1];
    bbeta[2] = beta[1] - beta[2];

    for (j = 0; j < 3; j++)
        coeff[j] = 0.0;
    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            coeff[k] = coeff[k] + yy[j][k]*bbeta[j];
    return (vec_dot(coeff,normal));
}

//=============================================================================================================

Eigen::MatrixXf FwdBemModel::fwd_bem_field_coeff(FwdCoilSet *coils)	/* Gradiometer coil positions */
/*
     * Compute the weighting factors to obtain the magnetic field
     */
{
    MNESurface*     surf;
    MNETriangle*    tri;
    FwdCoil*        coil;
    FwdCoilSet*     tcoils = NULL;
    int            ntri;
    int            j,k,p,s,off;
    double         res;
    double         mult;

    if (solution.size() == 0) {
        printf("Solution matrix missing in fwd_bem_field_coeff");
        return Eigen::MatrixXf();
    }
    if (bem_method != FWD_BEM_CONSTANT_COLL) {
        printf("BEM method should be constant collocation for fwd_bem_field_coeff");
        return Eigen::MatrixXf();
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (head_mri_t.isEmpty()) {
                printf("head -> mri coordinate transform missing in fwd_bem_field_coeff");
                return Eigen::MatrixXf();
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return Eigen::MatrixXf();
                }
                /*
                    * Make a transformed duplicate
                    */
                if ((tcoils = coils->dup_coil_set(head_mri_t)) == NULL)
                    return Eigen::MatrixXf();
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return Eigen::MatrixXf();
        }
    }
    ntri  = nsol;
    Eigen::MatrixXf coeff = Eigen::MatrixXf::Zero(coils->ncoil(), ntri);

    for (s = 0, off = 0; s < nsurf; s++) {
        surf = surfs[s].get();
        ntri = surf->ntri;
        tri  = surf->tris.data();
        mult = field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil(); j++) {
                coil = coils->coils[j].get();
                res = 0.0;
                for (p = 0; p < coil->np; p++)
                    res = res + coil->w[p]*one_field_coeff(Eigen::Map<const Eigen::Vector3f>(&coil->rmag(p, 0)),Eigen::Map<const Eigen::Vector3f>(&coil->cosmag(p, 0)),*tri);
                coeff(j,k+off) = mult*res;
            }
        }
        off = off + ntri;
    }
    delete tcoils;
    return coeff;
}

//=============================================================================================================

double FwdBemModel::calc_gamma(const Eigen::Vector3d& rk, const Eigen::Vector3d& rk1)
{
    Eigen::Vector3d rkk1 = rk1 - rk;
    double size = rkk1.norm();

    return log((rk1.norm() * size + rk1.dot(rkk1)) /
              (rk.norm() * size + rk.dot(rkk1))) / size;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_one_lin_field_coeff_ferg(const Eigen::Vector3f& dest, const Eigen::Vector3f& dir, MNETriangle& tri, Eigen::Vector3d& res)
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

    vec_diff(tri.r2,tri.r3,rjk[0]);
    vec_diff(tri.r3,tri.r1,rjk[1]);
    vec_diff(tri.r1,tri.r2,rjk[2]);

    for (k = 0; k < 3; k++) {
        y1[k] = tri.r1[k] - dest[k];
        y2[k] = tri.r2[k] - dest[k];
        y3[k] = tri.r3[k] - dest[k];
    }
    clen  = vec_dot(y1,tri.nn);
    for (k = 0; k < 3; k++) {
        c[k]  = clen*tri.nn[k];
        A[k]  = dest[k] + c[k];
        c1[k] = tri.r1[k] - A[k];
        c2[k] = tri.r2[k] - A[k];
        c3[k] = tri.r3[k] - A[k];
    }
    /*
       * beta and gamma...
       */
    for (sum = 0.0, k = 0; k < 3; k++) {
        cross_product(cc[k],cc[k+1],cross);
        beta  = vec_dot(cross,tri.nn);
        gamma = calc_gamma (Eigen::Map<const Eigen::Vector3d>(yy[k]),Eigen::Map<const Eigen::Vector3d>(yy[k+1]));
        sum = sum + beta*gamma;
    }
    /*
       * Solid angle...
       */
    cross_product(y1,y2,cross);
    triple = vec_dot(cross,y3);

    l1 = vec_len(y1);
    l2 = vec_len(y2);
    l3 = vec_len(y3);
    solid = 2.0*atan2(triple,
                      (l1*l2*l3+
                       vec_dot(y1,y2)*l3+
                       vec_dot(y1,y3)*l2+
                       vec_dot(y2,y3)*l1));
    /*
       * Now we are ready to assemble it all together
       */
    common = (sum-clen*solid)/(2.0*tri.area);
    for (k = 0; k < 3; k++)
        res[k] = -vec_dot(rjk[k],dir)*common;
    return;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_one_lin_field_coeff_uran(const Eigen::Vector3f& dest, const Eigen::Vector3f& dir_in, MNETriangle& tri, Eigen::Vector3d& res)
{
    double      I1;
    Eigen::Vector2d T, S1, S2;
    Eigen::Vector3d f0, fx, fy;
    double      res_x,res_y;
    double      x_fac,y_fac;
    int         k;
    /*
       * Compute the component integrals
       */
    field_integrals(dest,tri,I1,T,S1,S2,f0,fx,fy);
    /*
       * Compute the coefficient for each node...
       */
    Eigen::Vector3f dir = dir_in.normalized();

    x_fac = -vec_dot(dir,tri.ex);
    y_fac = -vec_dot(dir,tri.ey);
    for (k = 0; k < 3; k++) {
        res_x = f0[k]*T[0] + fx[k]*S1[0] + fy[k]*S2[0] + fy[k]*I1;
        res_y = f0[k]*T[1] + fx[k]*S1[1] + fy[k]*S2[1] - fx[k]*I1;
        res[k] = x_fac*res_x + y_fac*res_y;
    }
}

//=============================================================================================================

void FwdBemModel::fwd_bem_one_lin_field_coeff_simple(const Eigen::Vector3f& dest, const Eigen::Vector3f& normal, MNETriangle& source, Eigen::Vector3d& res)
{
    float diff[3];
    float vec_result[3];
    float dl;
    int   k;
    const Eigen::Vector3f* rr[3];

    rr[0] = &source.r1;
    rr[1] = &source.r2;
    rr[2] = &source.r3;

    for (k = 0; k < 3; k++) {
        vec_diff(*rr[k],dest,diff);
        dl = vec_dot(diff,diff);
        cross_product(diff,source.nn,vec_result);
        res[k] = source.area*vec_dot(vec_result,normal)/(3.0*dl*sqrt(dl));
    }
    return;
}

//=============================================================================================================

Eigen::MatrixXf FwdBemModel::fwd_bem_lin_field_coeff(FwdCoilSet *coils, int method)    /* Which integration formula to use */
/*
          * Compute the weighting factors to obtain the magnetic field
          * in the linear potential approximation
          */
{
    MNESurface*  surf;
    MNETriangle* tri;
    FwdCoil*     coil;
    FwdCoilSet*  tcoils = NULL;
    int         ntri;
    int         j,k,p,pp,off,s;
    Eigen::Vector3d res, one;
    float       mult;
    linFieldIntFunc func;

    if (solution.size() == 0) {
        printf("Solution matrix missing in fwd_bem_lin_field_coeff");
        return Eigen::MatrixXf();
    }
    if (bem_method != FWD_BEM_LINEAR_COLL) {
        printf("BEM method should be linear collocation for fwd_bem_lin_field_coeff");
        return Eigen::MatrixXf();
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (head_mri_t.isEmpty()) {
                printf("head -> mri coordinate transform missing in fwd_bem_lin_field_coeff");
                return Eigen::MatrixXf();
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return Eigen::MatrixXf();
                }
                /*
                    * Make a transformed duplicate
                    */
                if ((tcoils = coils->dup_coil_set(head_mri_t)) == NULL)
                    return Eigen::MatrixXf();
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return Eigen::MatrixXf();
        }
    }
    if (method == FWD_BEM_LIN_FIELD_FERGUSON)
        func = fwd_bem_one_lin_field_coeff_ferg;
    else if (method == FWD_BEM_LIN_FIELD_URANKAR)
        func = fwd_bem_one_lin_field_coeff_uran;
    else
        func = fwd_bem_one_lin_field_coeff_simple;

    Eigen::MatrixXf coeff = Eigen::MatrixXf::Zero(coils->ncoil(), nsol);
    /*
       * Process each of the surfaces
       */
    for (s = 0, off = 0; s < nsurf; s++) {
        surf = surfs[s].get();
        ntri = surf->ntri;
        tri  = surf->tris.data();
        mult = field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil(); j++) {
                coil = coils->coils[j].get();
                res.setZero();
                /*
             * Accumulate the coefficients for each triangle node...
             */
                for (p = 0; p < coil->np; p++) {
                    func(Eigen::Map<const Eigen::Vector3f>(&coil->rmag(p, 0)),Eigen::Map<const Eigen::Vector3f>(&coil->cosmag(p, 0)),*tri,one);
                    res += coil->w[p] * one;
                }
                /*
             * Add these to the corresponding coefficient matrix
             * elements...
             */
                for (pp = 0; pp < 3; pp++)
                    coeff(j,tri->vert[pp]+off) = coeff(j,tri->vert[pp]+off) + mult*res[pp];
            }
        }
        off = off + surf->np;
    }
    /*
       * Discard the duplicate
       */
    delete tcoils;
    return coeff;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_specify_coils(FwdCoilSet *coils)
/*
     * Set up for computing the solution at a set of coils
      */
{
    Eigen::MatrixXf sol;
    FwdBemSolution* csol = nullptr;

    if (solution.size() == 0) {
        printf("Solution not computed in fwd_bem_specify_coils");
        goto bad;
    }
    if(coils)
        coils->fwd_free_coil_set_user_data();
    if (!coils || coils->ncoil() == 0)
        return OK;
    if (bem_method == FWD_BEM_CONSTANT_COLL)
        sol = fwd_bem_field_coeff(coils);
    else if (bem_method == FWD_BEM_LINEAR_COLL)
        sol = fwd_bem_lin_field_coeff(coils,FWD_BEM_LIN_FIELD_SIMPLE);
    else {
        printf("Unknown BEM method in fwd_bem_specify_coils : %d",bem_method);
        goto bad;
    }
    if (sol.size() == 0)
        goto bad;
    coils->user_data = std::make_unique<FwdBemSolution>();
    csol = coils->user_data.get();

    csol->ncoil     = coils->ncoil();
    csol->np        = nsol;
    csol->solution  = sol * solution;

    return OK;

bad : {
        return FAIL;
    }
}

//=============================================================================================================

void FwdBemModel::fwd_bem_lin_field_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils, Eigen::VectorXf& B)
/*
     * Calculate the magnetic field in a set of coils
     */
{
    int   s,k,p,np;
    FwdCoil* coil;
    float  mult;
    Eigen::Vector3f my_rd = rd;
    Eigen::Vector3f my_Q = Q;
    FwdBemSolution* sol = coils.user_data.get();
    /*
       * Infinite-medium potentials
       */
    if (v0.size() == 0)
        v0.resize(nsol);
    float* v0p = v0.data();
    /*
       * The dipole location and orientation must be transformed
       */
    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(my_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(my_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    /*
       * Compute the inifinite-medium potentials at the vertices
       */
    for (s = 0, p = 0; s < nsurf; s++) {
        np     = surfs[s]->np;
        mult   = source_mult[s];
        for (k = 0; k < np; k++)
            v0p[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,Eigen::Map<const Eigen::Vector3f>(&surfs[s]->rr(k,0)));
    }
    /*
       * Primary current contribution
       * (can be calculated in the coil/dipole coordinates)
       */
    for (k = 0; k < coils.ncoil(); k++) {
        coil = coils.coils[k].get();
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(
                rd,
                Q,
                Eigen::Map<const Eigen::Vector3f>(&coil->rmag(p, 0)),
                Eigen::Map<const Eigen::Vector3f>(&coil->cosmag(p, 0)));
    }
    /*
       * Volume current contribution
       */
    for (k = 0; k < coils.ncoil(); k++)
        B[k] = B[k] + sol->solution.row(k).dot(v0);
    /*
       * Scale correctly
       */
    for (k = 0; k < coils.ncoil(); k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_field_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils, Eigen::VectorXf& B)
/*
     * Calculate the magnetic field in a set of coils
     */
{
    int   s,k,p,ntri;
    FwdCoil* coil;
    MNETriangle* tri;
    float   mult;
    Eigen::Vector3f my_rd = rd;
    Eigen::Vector3f my_Q = Q;
    FwdBemSolution* sol = coils.user_data.get();
    /*
       * Infinite-medium potentials
       */
    if (v0.size() == 0)
        v0.resize(nsol);
    float* v0p = v0.data();
    /*
       * The dipole location and orientation must be transformed
       */
    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(my_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(my_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    /*
       * Compute the inifinite-medium potentials at the centers of the triangles
       */
    for (s = 0, p = 0; s < nsurf; s++) {
        ntri = surfs[s]->ntri;
        tri  = surfs[s]->tris.data();
        mult = source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0p[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,tri->cent);
    }
    /*
       * Primary current contribution
       * (can be calculated in the coil/dipole coordinates)
       */
    for (k = 0; k < coils.ncoil(); k++) {
        coil = coils.coils[k].get();
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(
                rd,
                Q,
                Eigen::Map<const Eigen::Vector3f>(&coil->rmag(p, 0)),
                Eigen::Map<const Eigen::Vector3f>(&coil->cosmag(p, 0)));
    }
    /*
       * Volume current contribution
       */
    for (k = 0; k < coils.ncoil(); k++)
        B[k] = B[k] + sol->solution.row(k).dot(v0);
    /*
       * Scale correctly
       */
    for (k = 0; k < coils.ncoil(); k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}

//=============================================================================================================

void FwdBemModel::fwd_bem_field_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils, Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad)
/*
 * Calculate the magnetic field in a set of coils
 */
{
    FwdBemSolution* sol = coils.user_data.get();
    int            s,k,p,ntri,pp;
    FwdCoil*       coil;
    MNETriangle*   tri;
    float          mult;
    Eigen::VectorXf* grads[] = {&xgrad, &ygrad, &zgrad};
    Eigen::Vector3f ee, mri_ee;
    Eigen::Vector3f mri_rd = rd;
    Eigen::Vector3f mri_Q = Q;
    /*
       * Infinite-medium potentials
       */
    if (v0.size() == 0)
        v0.resize(nsol);
    float* v0p = v0.data();
    /*
       * The dipole location and orientation must be transformed
       */
    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(mri_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(mri_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    for (pp = X; pp <= Z; pp++) {
        Eigen::VectorXf& grad = *grads[pp];
        /*
         * Select the correct gradient component
         */
        ee = Eigen::Vector3f::Unit(pp);
        mri_ee = ee;
        if (!head_mri_t.isEmpty())
            FiffCoordTrans::apply_trans(mri_ee.data(),head_mri_t,FIFFV_NO_MOVE);
        /*
         * Compute the inifinite-medium potential derivatives at the centers of the triangles
         */
        for (s = 0, p = 0; s < nsurf; s++) {
            ntri = surfs[s]->ntri;
            tri  = surfs[s]->tris.data();
            mult = source_mult[s];
            for (k = 0; k < ntri; k++, tri++) {
                v0p[p++] = mult*fwd_bem_inf_pot_der(mri_rd,mri_Q,tri->cent,mri_ee);
            }
        }
        /*
         * Primary current contribution
         * (can be calculated in the coil/dipole coordinates)
         */
        for (k = 0; k < coils.ncoil(); k++) {
            coil = coils.coils[k].get();
            grad[k] = 0.0;
            for (p = 0; p < coil->np; p++)
                grad[k] = grad[k] + coil->w[p]*fwd_bem_inf_field_der(
                    rd,
                    Q,
                    Eigen::Map<const Eigen::Vector3f>(&coil->rmag(p, 0)),
                    Eigen::Map<const Eigen::Vector3f>(&coil->cosmag(p, 0)),
                    ee);
        }
        /*
         * Volume current contribution
         */
        for (k = 0; k < coils.ncoil(); k++)
            grad[k] = grad[k] + sol->solution.row(k).dot(v0);
        /*
         * Scale correctly
         */
        for (k = 0; k < coils.ncoil(); k++)
            grad[k] = MAG_FACTOR*grad[k];
    }
    return;
}

//=============================================================================================================

float FwdBemModel::fwd_bem_inf_field_der(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp, const Eigen::Vector3f& dir, const Eigen::Vector3f& comp)
/*
 * Derivative of the infinite-medium magnetic field with respect to
 * one of the dipole position coordinates (without \mu_0/4\pi)
 */
{
    Eigen::Vector3f diff = rp - rd;
    float diff2 = diff.squaredNorm();
    float diff3 = std::sqrt(diff2) * diff2;
    float diff5 = diff3 * diff2;
    Eigen::Vector3f cr = Q.cross(diff);
    Eigen::Vector3f crn = dir.cross(Q);

    return 3 * cr.dot(dir) * comp.dot(diff) / diff5 - comp.dot(crn) / diff3;
}

//=============================================================================================================

float FwdBemModel::fwd_bem_inf_pot_der(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, const Eigen::Vector3f& rp, const Eigen::Vector3f& comp)
/*
 * Derivative of the infinite-medium potential with respect to one of
 * the dipole position coordinates
 */
{
    Eigen::Vector3f diff = rp - rd;
    float diff2 = diff.squaredNorm();
    float diff3 = std::sqrt(diff2) * diff2;
    float diff5 = diff3 * diff2;

    float res = 3 * Q.dot(diff) * comp.dot(diff) / diff5 - comp.dot(Q) / diff3;
    return res / (4.0 * M_PI);
}

//=============================================================================================================

void FwdBemModel::fwd_bem_lin_field_grad_calc(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils, Eigen::VectorXf& xgrad, Eigen::VectorXf& ygrad, Eigen::VectorXf& zgrad)
/*
     * Calculate the gradient with respect to dipole position coordinates in a set of coils
     */
{
    FwdBemSolution* sol = coils.user_data.get();

    int     s,k,p,np,pp;
    FwdCoil *coil;
    float   mult;
    Eigen::Vector3f ee, mri_ee;
    Eigen::Vector3f mri_rd = rd;
    Eigen::Vector3f mri_Q = Q;
    Eigen::VectorXf* grads[] = {&xgrad, &ygrad, &zgrad};
    /*
       * Space for infinite-medium potentials
       */
    if (v0.size() == 0)
        v0.resize(nsol);
    float* v0p = v0.data();
    /*
       * The dipole location and orientation must be transformed
       */
    if (!head_mri_t.isEmpty()) {
        FiffCoordTrans::apply_trans(mri_rd.data(),head_mri_t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(mri_Q.data(),head_mri_t,FIFFV_NO_MOVE);
    }
    for (pp = X; pp <= Z; pp++) {
        Eigen::VectorXf& grad = *grads[pp];
        /*
         * Select the correct gradient component
         */
        ee = Eigen::Vector3f::Unit(pp);
        mri_ee = ee;
        if (!head_mri_t.isEmpty())
            FiffCoordTrans::apply_trans(mri_ee.data(),head_mri_t,FIFFV_NO_MOVE);
        /*
         * Compute the inifinite-medium potentials at the vertices
         */
        for (s = 0, p = 0; s < nsurf; s++) {
            np     = surfs[s]->np;
            mult   = source_mult[s];

            for (k = 0; k < np; k++)
                v0p[p++] = mult*fwd_bem_inf_pot_der(mri_rd,mri_Q,Eigen::Map<const Eigen::Vector3f>(&surfs[s]->rr(k,X)),mri_ee);
        }
        /*
         * Primary current contribution
         * (can be calculated in the coil/dipole coordinates)
         */
        for (k = 0; k < coils.ncoil(); k++) {
            coil = coils.coils[k].get();
            grad[k] = 0.0;
            for (p = 0; p < coil->np; p++)
                grad[k] = grad[k] + coil->w[p]*fwd_bem_inf_field_der(
                    rd,
                    Q,
                    Eigen::Map<const Eigen::Vector3f>(&coil->rmag(p, 0)),
                    Eigen::Map<const Eigen::Vector3f>(&coil->cosmag(p, 0)),
                    ee);
        }
        /*
         * Volume current contribution
         */
        for (k = 0; k < coils.ncoil(); k++)
            grad[k] = grad[k] + sol->solution.row(k).dot(v0);
        /*
         * Scale correctly
         */
        for (k = 0; k < coils.ncoil(); k++)
            grad[k] = MAG_FACTOR*grad[k];
    }
    return;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *coils, float *B, void *client)  /* The model */
/*
     * This version calculates the magnetic field in a set of coils
     * Call fwd_bem_specify_coils first to establish the coil-specific
     * solution matrix
     */
{
    FwdBemModel* m = (FwdBemModel*)client;
    FwdBemSolution* sol = coils->user_data.get();

    if (!m) {
        printf("No BEM model specified to fwd_bem_field");
        return FAIL;
    }
    if (!sol || sol->solution.size() == 0 || sol->ncoil != coils->ncoil()) {
        printf("No appropriate coil-specific data available in fwd_bem_field");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
        Eigen::VectorXf Bvec(coils->ncoil());
        m->fwd_bem_field_calc(rd,Q,*coils,Bvec);
        Eigen::Map<Eigen::VectorXf>(B, coils->ncoil()) = Bvec;
    }
    else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
        Eigen::VectorXf Bvec(coils->ncoil());
        m->fwd_bem_lin_field_calc(rd,Q,*coils,Bvec);
        Eigen::Map<Eigen::VectorXf>(B, coils->ncoil()) = Bvec;
    }
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}

//=============================================================================================================

int FwdBemModel::fwd_bem_field_grad(const Eigen::Vector3f& rd,
                                    const Eigen::Vector3f& Q,
                                    FwdCoilSet *coils,
                                    float Bval[],
                                    float xgrad[],
                                    float ygrad[],
                                    float zgrad[],
                                    void *client)  /* Client data to be passed to some foward modelling routines */
{
    FwdBemModel* m = (FwdBemModel*)client;
    FwdBemSolution* sol = coils->user_data.get();

    if (!m) {
        qCritical("No BEM model specified to fwd_bem_field");
        return FAIL;
    }

    if (!sol || sol->solution.size() == 0 || sol->ncoil != coils->ncoil()) {
        qCritical("No appropriate coil-specific data available in fwd_bem_field");
        return FAIL;
    }

    if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
        int n = coils->ncoil();
        if (Bval) {
            Eigen::VectorXf Bvec(n);
            m->fwd_bem_field_calc(rd,Q,*coils,Bvec);
            Eigen::Map<Eigen::VectorXf>(Bval, n) = Bvec;
        }

        Eigen::VectorXf xg(n), yg(n), zg(n);
        m->fwd_bem_field_grad_calc(rd,Q,*coils,xg,yg,zg);
        Eigen::Map<Eigen::VectorXf>(xgrad, n) = xg;
        Eigen::Map<Eigen::VectorXf>(ygrad, n) = yg;
        Eigen::Map<Eigen::VectorXf>(zgrad, n) = zg;
    } else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
        int n = coils->ncoil();
        if (Bval) {
            Eigen::VectorXf Bvec(n);
            m->fwd_bem_lin_field_calc(rd,Q,*coils,Bvec);
            Eigen::Map<Eigen::VectorXf>(Bval, n) = Bvec;
        }

        Eigen::VectorXf xg(n), yg(n), zg(n);
        m->fwd_bem_lin_field_grad_calc(rd,Q,*coils,xg,yg,zg);
        Eigen::Map<Eigen::VectorXf>(xgrad, n) = xg;
        Eigen::Map<Eigen::VectorXf>(ygrad, n) = yg;
        Eigen::Map<Eigen::VectorXf>(zgrad, n) = zg;
    } else {
        qCritical("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }

    return OK;
}

//=============================================================================================================

void FwdBemModel::meg_eeg_fwd_one_source_space(FwdThreadArg* a)
/*
 * Compute the MEG or EEG forward solution for one source space
 * and possibly for only one source component
 */
{
    MNESourceSpace* s = a->s;
    int            j,p,q;
    float          *xyz[3];

    p = a->off;
    q = 3*a->off;
    if (a->fixed_ori) {					  /* The normal source component only */
        if (a->field_pot_grad && a->res_grad) {                   /* Gradient requested? */
            for (j = 0; j < s->np; j++) {
                if (s->inuse[j]) {
                    if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                          Eigen::Map<const Eigen::Vector3f>(&s->nn(j,0)),
                                          a->coils_els,
                                          a->res[p],
                                          a->res_grad[q],
                                          a->res_grad[q+1],
                                          a->res_grad[q+2],
                                          a->client) != OK) {
                        goto bad;
                    }
                    q = q + 3;
                    p++;
                }
            }
        } else {
            for (j = 0; j < s->np; j++)
                if (s->inuse[j])
                    if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                     Eigen::Map<const Eigen::Vector3f>(&s->nn(j,0)),
                                     a->coils_els,
                                     a->res[p++],
                                     a->client) != OK)
                        goto bad;
        }
    }
    else {						  /* All source components */
        if (a->field_pot_grad && a->res_grad) {               /* Gradient requested? */
            for (j = 0; j < s->np; j++) {
                if (s->inuse[j]) {
                    if (a->comp < 0) {				  /* Compute all components */
                        if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                              Qx,
                                              a->coils_els,
                                              a->res[p],
                                              a->res_grad[q],
                                              a->res_grad[q+1],
                                              a->res_grad[q+2],
                                              a->client) != OK)
                            goto bad;
                        q = q + 3; p++;
                        if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                              Qy,
                                              a->coils_els,
                                              a->res[p],
                                              a->res_grad[q],
                                              a->res_grad[q+1],
                                              a->res_grad[q+2],
                                              a->client) != OK)
                            goto bad;
                        q = q + 3; p++;
                        if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                              Qz,
                                              a->coils_els,
                                              a->res[p],
                                              a->res_grad[q],
                                              a->res_grad[q+1],
                                              a->res_grad[q+2],
                                              a->client) != OK)
                            goto bad;
                        q = q + 3; p++;
                    }
                    else if (a->comp == 0) {			  /* Compute x component */
                        if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                              Qx,
                                              a->coils_els,
                                              a->res[p],
                                              a->res_grad[q],
                                              a->res_grad[q+1],
                                              a->res_grad[q+2],
                                              a->client) != OK)
                            goto bad;
                        q = q + 3; p++;
                        q = q + 3; p++;
                        q = q + 3; p++;
                    }
                    else if (a->comp == 1) {			  /* Compute y component */
                        q = q + 3; p++;
                        if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                              Qy,
                                              a->coils_els,
                                              a->res[p],
                                              a->res_grad[q],
                                              a->res_grad[q+1],
                                              a->res_grad[q+2],
                                              a->client) != OK)
                            goto bad;
                        q = q + 3; p++;
                        q = q + 3; p++;
                    }
                    else if (a->comp == 2) {			  /* Compute z component */
                        q = q + 3; p++;
                        q = q + 3; p++;
                        if (a->field_pot_grad(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),
                                              Qz,
                                              a->coils_els,
                                              a->res[p],
                                              a->res_grad[q],
                                              a->res_grad[q+1],
                                              a->res_grad[q+2],
                                              a->client) != OK)
                            goto bad;
                        q = q + 3; p++;
                    }
                }
            }
        }
        else {
            for (j = 0; j < s->np; j++) {
                if (s->inuse[j]) {
                    if (a->vec_field_pot) {
                        xyz[0] = a->res[p++];
                        xyz[1] = a->res[p++];
                        xyz[2] = a->res[p++];
                        if (a->vec_field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),a->coils_els,xyz,a->client) != OK)
                            goto bad;
                    }
                    else {
                        if (a->comp < 0) {				  /* Compute all components here */
                            if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),Qx,a->coils_els,a->res[p++],a->client) != OK)
                                goto bad;
                            if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),Qy,a->coils_els,a->res[p++],a->client) != OK)
                                goto bad;
                            if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),Qz,a->coils_els,a->res[p++],a->client) != OK)
                                goto bad;
                        }
                        else if (a->comp == 0) {			  /* Compute x component */
                            if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),Qx,a->coils_els,a->res[p++],a->client) != OK)
                                goto bad;
                            p++; p++;
                        }
                        else if (a->comp == 1) {			  /* Compute y component */
                            p++;
                            if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),Qy,a->coils_els,a->res[p++],a->client) != OK)
                                goto bad;
                            p++;
                        }
                        else if (a->comp == 2) {			  /* Compute z component */
                            p++; p++;
                            if (a->field_pot(Eigen::Map<const Eigen::Vector3f>(&s->rr(j,0)),Qz,a->coils_els,a->res[p++],a->client) != OK)
                                goto bad;
                        }
                    }
                }
            }
        }
    }
    a->stat = OK;
    return;

bad : {
        a->stat = FAIL;
        return;
    }
}

//=============================================================================================================

int FwdBemModel::compute_forward_meg(std::vector<std::unique_ptr<MNESourceSpace>>& spaces,
                                     FwdCoilSet *coils,
                                     FwdCoilSet *comp_coils,
                                     MNECTFCompDataSet *comp_data,
                                     bool fixed_ori,
                                     const Vector3f &r0,
                                     bool use_threads,
                                     FiffNamedMatrix& resp,
                                     FiffNamedMatrix& resp_grad,
                                     bool bDoGrad)
/*
 * Compute the MEG forward solution
 * Use either the sphere model or BEM in the calculations
 */
{
    float               **res = NULL;       /* The forward solution matrix */
    float               **res_grad = NULL;  /* The gradient with respect to the dipole position */
    std::vector<float>  res_data;            /* Contiguous storage for res */
    std::vector<float*> res_ptrs;            /* Row pointers for res */
    std::vector<float>  res_grad_data;       /* Contiguous storage for res_grad */
    std::vector<float*> res_grad_ptrs;       /* Row pointers for res_grad */
    MatrixXd            matRes;
    MatrixXd            matResGrad;
    int                 nrow = 0;
    FwdCompData         *comp = NULL;
    fwdFieldFunc        field;              /* Computes the field for one dipole orientation */
    fwdVecFieldFunc     vec_field;          /* Computes the field for all dipole orientations */
    fwdFieldGradFunc    field_grad;         /* Computes the field and gradient with respect to dipole position
                                             * for one dipole orientation */
    int                 nmeg = coils->ncoil();/* Number of channels */
    int                 nsource;            /* Total number of sources */
    int                 nspace = static_cast<int>(spaces.size());
    int                 k,p,q,off;
    QStringList         names;              /* Channel names */
    void                *client;
    FwdThreadArg*       one_arg = NULL;
    int                 nproc = QThread::idealThreadCount();
    QStringList         emptyList;

    if (true) {
        /*
         * Use the new compensated field computation
         * It works the same way independent of whether or not the compensation is in effect
         */
#ifdef TEST
        printf("Using differences.\n");
        comp = FwdCompData::fwd_make_comp_data(comp_data,
                                               coils,comp_coils,
                                               FwdBemModel::fwd_bem_field,
                                               nullptr,
                                               my_bem_field_grad,
                                               this);
#else
        comp = FwdCompData::fwd_make_comp_data(comp_data,
                                               coils,
                                               comp_coils,
                                               FwdBemModel::fwd_bem_field,
                                               nullptr,
                                               FwdBemModel::fwd_bem_field_grad,
                                               this);
#endif
        if (!comp)
            goto bad;
        /*
        * Field computation matrices...
        */
        qDebug() << "!!!TODO Speed the following with Eigen up!";
        printf("Composing the field computation matrix...");
        if (fwd_bem_specify_coils(coils) == FAIL)
            goto bad;
        printf("[done]\n");

        if (comp->set && comp->set->current) { /* Test just to specify confusing output */
            printf("Composing the field computation matrix (compensation coils)...");
            if (fwd_bem_specify_coils(comp->comp_coils) == FAIL)
                goto bad;
            printf("[done]\n");
        }
        field      = FwdCompData::fwd_comp_field;
        vec_field  = nullptr;
        field_grad = FwdCompData::fwd_comp_field_grad;
        client     = comp;
    }
    else {
        /*
         * Use the new compensated field computation
         * It works the same way independent of whether or not the compensation is in effect
         */
#ifdef TEST
        printf("Using differences.\n");
        comp = FwdCompData::fwd_make_comp_data(comp_data,coils,comp_coils,
                                               fwd_sphere_field,
                                               fwd_sphere_field_vec,
                                               my_sphere_field_grad,
                                               const_cast<Vector3f*>(&r0));
#else
        comp = FwdCompData::fwd_make_comp_data(comp_data,coils,comp_coils,
                                               fwd_sphere_field,
                                               fwd_sphere_field_vec,
                                               fwd_sphere_field_grad,
                                               const_cast<Vector3f*>(&r0));
#endif
        if (!comp)
            goto bad;
        field       = FwdCompData::fwd_comp_field;
        vec_field   = FwdCompData::fwd_comp_field_vec;
        field_grad  = FwdCompData::fwd_comp_field_grad;
        client      = comp;
    }
    /*
       * Count the sources
       */
    for (k = 0, nsource = 0; k < nspace; k++)
        nsource += spaces[k]->nuse;
    /*
       * Allocate space for the solution
       */
    {
        int res_rows = fixed_ori ? nsource : 3*nsource;
        res_data.assign(res_rows * nmeg, 0.0f);
        res_ptrs.resize(res_rows);
        for (int i = 0; i < res_rows; i++)
            res_ptrs[i] = res_data.data() + i * nmeg;
        res = res_ptrs.data();
    }
    if (bDoGrad) {
        int grad_rows = fixed_ori ? 3*nsource : 3*3*nsource;
        res_grad_data.assign(grad_rows * nmeg, 0.0f);
        res_grad_ptrs.resize(grad_rows);
        for (int i = 0; i < grad_rows; i++)
            res_grad_ptrs[i] = res_grad_data.data() + i * nmeg;
        res_grad = res_grad_ptrs.data();
    }
    /*
     * Set up the argument for the field computation
     */
    one_arg = new FwdThreadArg();
    one_arg->res            = res;
    one_arg->res_grad       = res_grad;
    one_arg->off            = 0;
    one_arg->coils_els      = coils;
    one_arg->client         = client;
    one_arg->s              = NULL;
    one_arg->fixed_ori      = fixed_ori;
    one_arg->field_pot      = field;
    one_arg->vec_field_pot  = vec_field;
    one_arg->field_pot_grad = field_grad;

    if (nproc < 2)
        use_threads = false;

    if (use_threads) {
        int            nthread  = (fixed_ori || vec_field || nproc < 6) ? nspace : 3*nspace;
        QList <FwdThreadArg*> args;
        int            stat;
        /*
        * We need copies to allocate separate workspace for each thread
        */
        if (fixed_ori || vec_field || nproc < 6) {
            for (k = 0, off = 0; k < nthread; k++) {
                FwdThreadArg* t_arg = FwdThreadArg::create_meg_multi_thread_duplicate(one_arg,true);
                t_arg->s   = spaces[k].get();
                t_arg->off = off;
                off = fixed_ori ? off + spaces[k]->nuse : off + 3*spaces[k]->nuse;
                args.append(t_arg);
            }
            printf("%d processors. I will use one thread for each of the %d source spaces.\n",
                    nproc,nspace);
        }
        else {
            for (k = 0, off = 0, q = 0; k < nspace; k++) {
                for (p = 0; p < 3; p++,q++) {
                    FwdThreadArg* t_arg = FwdThreadArg::create_meg_multi_thread_duplicate(one_arg,true);
                    t_arg->s    = spaces[k].get();
                    t_arg->off  = off;
                    t_arg->comp = p;
                    args.append(t_arg);
                }
                off = fixed_ori ? off + spaces[k]->nuse : off + 3*spaces[k]->nuse;
            }
            printf("%d processors. I will use %d threads : %d source spaces x 3 source components.\n",
                    nproc,nthread,nspace);
        }
        printf("Computing MEG at %d source locations (%s orientations)...",
                nsource,fixed_ori ? "fixed" : "free");
        /*
        * Ready to start the threads & Wait for them to complete
        */
        QtConcurrent::blockingMap(args, meg_eeg_fwd_one_source_space);
        /*
        * Check the results
        */
        for (k = 0, stat = OK; k < nthread; k++)
            if (args[k]->stat != OK) {
                stat = FAIL;
                break;
            }
        for (k = 0; k < args.size(); k++)//nthread == args.size()
            FwdThreadArg::free_meg_multi_thread_duplicate(args[k],true);
        if (stat != OK)
            goto bad;
    }
    else {
        printf("Computing MEG at %d source locations (%s orientations, no threads)...",
                nsource,fixed_ori ? "fixed" : "free");
        for (k = 0, off = 0; k < nspace; k++) {
            one_arg->s   = spaces[k].get();
            one_arg->off = off;
            meg_eeg_fwd_one_source_space(one_arg);
            if (one_arg->stat != OK)
                goto bad;
            off = fixed_ori ? off + one_arg->s->nuse : off + 3*one_arg->s->nuse;
        }
    }
    printf("done.\n");
    {
        QStringList orig_names;
        for (k = 0; k < nmeg; k++)
            orig_names.append(coils->coils[k]->chname);
        names = orig_names;
    }
    if(one_arg)
        delete one_arg;
    if(comp)
        delete comp;

    // Store solution in fiff named matrix
    nrow = fixed_ori ? nsource : 3*nsource;
    matRes.conservativeResize(nrow,nmeg);
    for(int i = 0; i < nrow; i++) {
        for(int j = 0; j < nmeg; j++) {
            matRes(i,j) = res[i][j];
        }
    }
    resp.nrow = nrow;
    resp.ncol = nmeg;
    resp.row_names = emptyList;
    resp.col_names = names;
    resp.data = matRes;
    resp.transpose_named_matrix();

    if (bDoGrad && res_grad) {
        nrow = fixed_ori ? 3*nsource : 3*3*nsource;
        matResGrad = MatrixXd::Zero(nrow,nmeg);
        for(int i = 0; i < nrow; i++) {
            for(int j = 0; j < nmeg; j++) {
                matResGrad(i,j) = res_grad[i][j];
            }
        }
        resp_grad.nrow = nrow;
        resp_grad.ncol = nmeg;
        resp_grad.row_names = emptyList;
        resp_grad.col_names = names;
        resp_grad.data = matResGrad;
        resp_grad.transpose_named_matrix();
    }
    return OK;

bad : {
        if(one_arg)
            delete one_arg;
        if(comp)
            delete comp;
        return FAIL;
    }
}

//=============================================================================================================

int FwdBemModel::compute_forward_eeg(std::vector<std::unique_ptr<MNESourceSpace>>& spaces,
                                     FwdCoilSet *els,
                                     bool fixed_ori,
                                     FwdEegSphereModel *eeg_model,
                                     bool use_threads,
                                     FiffNamedMatrix& resp,
                                     FiffNamedMatrix& resp_grad,
                                     bool bDoGrad)
/*
     * Compute the EEG forward solution
     * Use either the sphere model or BEM in the calculations
     */
{
    float            **res = NULL;          /* The forward solution matrix */
    float            **res_grad = NULL;     /* The gradient with respect to the dipole position */
    std::vector<float>  res_data;            /* Contiguous storage for res */
    std::vector<float*> res_ptrs;            /* Row pointers for res */
    std::vector<float>  res_grad_data;       /* Contiguous storage for res_grad */
    std::vector<float*> res_grad_ptrs;       /* Row pointers for res_grad */
    MatrixXd matRes;
    MatrixXd matResGrad;
    int nrow = 0;
    fwdFieldFunc     pot;                   /* Computes the potentials for one dipole orientation */
    fwdVecFieldFunc  vec_pot;               /* Computes the potentials for all dipole orientations */
    fwdFieldGradFunc pot_grad;              /* Computes the potential and gradient with respect to dipole position
                                             * for one dipole orientation */
    int             nsource;                /* Total number of sources */
    int             nspace = static_cast<int>(spaces.size());
    int             neeg = els->ncoil();      /* Number of channels */
    int             k,p,q,off;
    QStringList     names;                  /* Channel names */
    void            *client;
    FwdThreadArg*   one_arg = NULL;
    int             nproc = QThread::idealThreadCount();
    QStringList     emptyList;
    /*
       * Count the sources
       */
    for (k = 0, nsource = 0; k < nspace; k++)
        nsource += spaces[k]->nuse;

    if (true) {
        if (fwd_bem_specify_els(els) == FAIL)
            goto bad;
        client   = this;
        pot      = fwd_bem_pot_els;
        vec_pot  = nullptr;
#ifdef TEST
        printf("Using differences.\n");
        pot_grad = my_bem_pot_grad;
#else
        pot_grad = fwd_bem_pot_grad_els;
#endif
    }
    else {
        if (eeg_model->nfit == 0) {
            printf("Using the standard series expansion for a multilayer sphere model for EEG\n");
            pot      = FwdEegSphereModel::fwd_eeg_multi_spherepot_coil1;
            vec_pot  = nullptr;
            pot_grad = nullptr;
        }
        else {
            printf("Using the equivalent source approach in the homogeneous sphere for EEG\n");
            pot      = FwdEegSphereModel::fwd_eeg_spherepot_coil;
            vec_pot  = FwdEegSphereModel::fwd_eeg_spherepot_coil_vec;
            pot_grad = FwdEegSphereModel::fwd_eeg_spherepot_grad_coil;
        }
        client   = eeg_model;
    }
    /*
     * Allocate space for the solution
     */
    {
        int res_rows = fixed_ori ? nsource : 3*nsource;
        res_data.assign(res_rows * neeg, 0.0f);
        res_ptrs.resize(res_rows);
        for (int i = 0; i < res_rows; i++)
            res_ptrs[i] = res_data.data() + i * neeg;
        res = res_ptrs.data();
    }
    if (bDoGrad) {
        if (!pot_grad) {
            qCritical("EEG gradient calculation function not available");
            goto bad;
        }
        int grad_rows = fixed_ori ? 3*nsource : 3*3*nsource;
        res_grad_data.assign(grad_rows * neeg, 0.0f);
        res_grad_ptrs.resize(grad_rows);
        for (int i = 0; i < grad_rows; i++)
            res_grad_ptrs[i] = res_grad_data.data() + i * neeg;
        res_grad = res_grad_ptrs.data();
    }
    /*
       * Set up the argument for the field computation
       */
    one_arg = new FwdThreadArg();
    one_arg->res            = res;
    one_arg->res_grad       = res_grad;
    one_arg->off            = 0;
    one_arg->coils_els      = els;
    one_arg->client         = client;
    one_arg->s              = NULL;
    one_arg->fixed_ori      = fixed_ori;
    one_arg->field_pot      = pot;
    one_arg->vec_field_pot  = vec_pot;
    one_arg->field_pot_grad = pot_grad;

    if (nproc < 2)
        use_threads = false;

    if (use_threads) {
        int            nthread  = (fixed_ori || vec_pot || nproc < 6) ? nspace : 3*nspace;
        QList <FwdThreadArg*> args;
        int            stat;
        /*
        * We need copies to allocate separate workspace for each thread
        */
        if (fixed_ori || vec_pot || nproc < 6) {
            for (k = 0, off = 0; k < nthread; k++) {
                FwdThreadArg* t_arg = FwdThreadArg::create_eeg_multi_thread_duplicate(one_arg,true);
                t_arg->s   = spaces[k].get();
                t_arg->off = off;
                off = fixed_ori ? off + spaces[k]->nuse : off + 3*spaces[k]->nuse;
                args.append(t_arg);
            }
            printf("%d processors. I will use one thread for each of the %d source spaces.\n",nproc,nspace);
        }
        else {
            for (k = 0, off = 0, q = 0; k < nspace; k++) {
                for (p = 0; p < 3; p++,q++) {
                    FwdThreadArg* t_arg = FwdThreadArg::create_eeg_multi_thread_duplicate(one_arg,true);
                    t_arg->s    = spaces[k].get();
                    t_arg->off  = off;
                    t_arg->comp = p;
                    args.append(t_arg);
                }
                off = fixed_ori ? off + spaces[k]->nuse : off + 3*spaces[k]->nuse;
            }
            printf("%d processors. I will use %d threads : %d source spaces x 3 source components.\n",nproc,nthread,nspace);
        }
        printf("Computing EEG at %d source locations (%s orientations)...",
                nsource,fixed_ori ? "fixed" : "free");
        /*
        * Ready to start the threads & Wait for them to complete
        */
        QtConcurrent::blockingMap(args, meg_eeg_fwd_one_source_space);
        /*
        * Check the results
        */
        for (k = 0, stat = OK; k < nthread; k++)
            if (args[k]->stat != OK) {
                stat = FAIL;
                break;
            }
        for (k = 0; k < args.size(); k++)//nthread == args.size()
            FwdThreadArg::free_eeg_multi_thread_duplicate(args[k],true);
        if (stat != OK)
            goto bad;
    }
    else {
        printf("Computing EEG at %d source locations (%s orientations, no threads)...",
                nsource,fixed_ori ? "fixed" : "free");
        for (k = 0, off = 0; k < nspace; k++) {
            one_arg->s   = spaces[k].get();
            one_arg->off = off;
            meg_eeg_fwd_one_source_space(one_arg);
            if (one_arg->stat != OK)
                goto bad;
            off = fixed_ori ? off + one_arg->s->nuse : off + 3*one_arg->s->nuse;
        }
    }
    printf("done.\n");
    {
        QStringList orig_names;
        for (k = 0; k < neeg; k++)
            orig_names.append(els->coils[k]->chname);
        names = orig_names;
    }
    if(one_arg)
        delete one_arg;

    // Store solution in fiff named matrix
    nrow = fixed_ori ? nsource : 3*nsource;
    matRes.conservativeResize(nrow,neeg);
    for(int i = 0; i < nrow; i++) {
        for(int j = 0; j < neeg; j++) {
            matRes(i,j) = res[i][j];
        }
    }
    resp.nrow = nrow;
    resp.ncol = neeg;
    resp.row_names = emptyList;
    resp.col_names = names;
    resp.data = matRes;
    resp.transpose_named_matrix();

    if (bDoGrad && res_grad) {
       matResGrad = MatrixXd::Zero(fixed_ori ? 3*nsource : 3*3*nsource,neeg);
       for(int i = 0; i < nrow; i++) {
           for(int j = 0; j < neeg; j++) {
               matResGrad(i,j) = res_grad[i][j];
           }
        }
        resp_grad.nrow = nrow;
        resp_grad.ncol = neeg;
        resp_grad.row_names = emptyList;
        resp_grad.col_names = names;
        resp_grad.data = matResGrad;
        resp_grad.transpose_named_matrix();
    }
    return OK;

bad : {
        if(one_arg)
            delete one_arg;
        return FAIL;
    }
}

//=============================================================================================================

#define EPS   1e-5   /* Points closer to origin than this many
                        meters are considered to be at the
                        origin */
#define CEPS       1e-5

int FwdBemModel::fwd_sphere_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *coils, float Bval[], void *client)	/* Client data will be the sphere model origin */
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
    /*
       * Check for a dipole at the origin
       */
    for (k = 0 ; k < coils->ncoil() ; k++)
        if (FWD_IS_MEG_COIL(coils->coils[k]->coil_class))
            Bval[k] = 0.0;
    r = vec_len(myrd);
    if (r > EPS)	{		/* The hard job */

        cross_product(Q,myrd,v);

        for (k = 0; k < coils->ncoil(); k++) {
            this_coil = coils->coils[k].get();
            if (FWD_IS_MEG_COIL(this_coil->type)) {

                np = this_coil->np;

                for (j = 0, sum = 0.0; j < np; j++) {

                    this_pos = &this_coil->rmag(j, 0);
                    this_dir = &this_coil->cosmag(j, 0);

                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;
                    result = 0.0;

                    /* Vector from dipole to the field point */

                    vec_diff(myrd,this_pos,a_vec);

                    /* Compute the dot products needed */

                    a2  = vec_dot(a_vec,a_vec);       a = sqrt(a2);

                    if (a > 0.0) {
                        r2  = vec_dot(this_pos,this_pos); r = sqrt(r2);
                        if (r > 0.0) {
                            rr0 = vec_dot(this_pos,myrd);
                            ar = (r2-rr0);
                            if (std::fabs(ar/(a*r)+1.0) > CEPS) { /* There is a problem on the negative 'z' axis if the dipole location
                                                    * and the field point are on the same line */
                                ar0  = ar/a;

                                ve = vec_dot(v,this_dir); vr = vec_dot(v,this_pos);
                                re = vec_dot(this_pos,this_dir); r0e = vec_dot(rd,this_dir);

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
    return OK;          /* Happy conclusion: this works always */
}

//=============================================================================================================

int FwdBemModel::fwd_sphere_field_vec(const Eigen::Vector3f& rd, FwdCoilSet *coils, float **Bval, void *client)	/* Client data will be the sphere model origin */
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
    /*
       * Check for a dipole at the origin
       */
    r = vec_len(myrd);
    for (k = 0; k < coils->ncoil(); k++) {
        this_coil = coils->coils[k].get();
        if (FWD_IS_MEG_COIL(this_coil->coil_class)) {
            if (r < EPS) {
                Bval[0][k] = Bval[1][k] = Bval[2][k] = 0.0;
            }
            else { 	/* The hard job */

                np = this_coil->np;
                sum[0] = sum[1] = sum[2] = 0.0;

                for (j = 0; j < np; j++) {

                    this_pos = &this_coil->rmag(j, 0);
                    this_dir = &this_coil->cosmag(j, 0);

                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;

                    /* Vector from dipole to the field point */

                    vec_diff(myrd,this_pos,a_vec);

                    /* Compute the dot products needed */

                    a2  = vec_dot(a_vec,a_vec);       a = sqrt(a2);

                    if (a > 0.0) {
                        r2  = vec_dot(this_pos,this_pos); r = sqrt(r2);
                        if (r > 0.0) {
                            rr0 = vec_dot(this_pos,myrd);
                            ar = (r2-rr0);
                            if (std::fabs(ar/(a*r)+1.0) > CEPS) { /* There is a problem on the negative 'z' axis if the dipole location
                                                    * and the field point are on the same line */

                                /* The main ingredients */

                                ar0  = ar/a;
                                F  = a*(r*a + ar);
                                gr = a2/r + ar0 + 2.0*(a+r);
                                g0 = a + 2*r + ar0;

                                re = vec_dot(this_pos,this_dir); r0e = vec_dot(myrd,this_dir);
                                cross_product(myrd,this_dir,v1);
                                cross_product(myrd,this_pos,v2);

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

//=============================================================================================================

int FwdBemModel::fwd_sphere_field_grad(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet *coils, float Bval[], float xgrad[], float ygrad[], float zgrad[], void *client)  /* Client data to be passed to some foward modelling routines */
/*
 * Compute the derivatives of the sphere model field with respect to
 * dipole coordinates
 */
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

    float v[3],a_vec[3];
    float a,a2,r,r2;
    float ar,rr0;
    float vr,ve,re,r0e;
    float F,g0,gr,result,G,F2;

    int   j,k,p;
    float huu;
    float ggr[3],gg0[3];		/* Gradient of gr & g0 */
    float ga[3];			/* Grapdient of a */
    float gar[3];			/* Gradient of ar */
    float gFF[3];			/* Gradient of F divided by F */
    float gresult[3];
    float eQ[3],rQ[3];		/* e x Q and r x Q */
    int   do_field = 0;
    FwdCoil* this_coil;
    float *this_pos,*this_dir;
    int   np;
    float myrd[3];
    float pos[3];
    float *r0 = (float *)client;      /* The sphere model origin */
    /*
       * Shift to the sphere model coordinates
       */
    for (p = 0; p < 3; p++)
        myrd[p] = rd[p] - r0[p];

    if (Bval)
        do_field = 1;

    /* Check for a dipole at the origin */

    r = vec_len(myrd);
    for (k = 0; k < coils->ncoil() ; k++) {
        if (FWD_IS_MEG_COIL(coils->coils[k]->coil_class)) {
            if (do_field)
                Bval[k] = 0.0;
            xgrad[k] = 0.0;
            ygrad[k] = 0.0;
            zgrad[k] = 0.0;
        }
    }
    if (r > EPS) {		/* The hard job */

        v[0] = Q[1]*myrd[2] - Q[2]*myrd[1];
        v[1] = -Q[0]*myrd[2] + Q[2]*myrd[0];
        v[2] = Q[0]*myrd[1] - Q[1]*myrd[0];

        for (k = 0 ; k < coils->ncoil() ; k++) {

            this_coil = coils->coils[k].get();

            if (FWD_IS_MEG_COIL(this_coil->type)) {

                np = this_coil->np;

                for (j = 0; j < np; j++) {

                    this_pos = &this_coil->rmag(j, 0);
                    /*
           * Shift to the sphere model coordinates
           */
                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;

                    this_dir = &this_coil->cosmag(j, 0);

                    /* Vector from dipole to the field point */

                    a_vec[0] = this_pos[0] - myrd[0];
                    a_vec[1] = this_pos[1] - myrd[1];
                    a_vec[2] = this_pos[2] - myrd[2];

                    /* Compute the dot and cross products needed */

                    a2  = vec_dot(a_vec,a_vec);       a = sqrt(a2);
                    r2  = vec_dot(this_pos,this_pos); r = sqrt(r2);
                    rr0 = vec_dot(this_pos,myrd);
                    ar  = (r2 - rr0)/a;

                    ve = vec_dot(v,this_dir); vr = vec_dot(v,this_pos);
                    re = vec_dot(this_pos,this_dir); r0e = vec_dot(myrd,this_dir);

                    /* eQ = this_dir x Q */

                    eQ[0] = this_dir[1]*Q[2] - this_dir[2]*Q[1];
                    eQ[1] = -this_dir[0]*Q[2] + this_dir[2]*Q[0];
                    eQ[2] = this_dir[0]*Q[1] - this_dir[1]*Q[0];

                    /* rQ = this_pos x Q */

                    rQ[0] = this_pos[1]*Q[2] - this_pos[2]*Q[1];
                    rQ[1] = -this_pos[0]*Q[2] + this_pos[2]*Q[0];
                    rQ[2] = this_pos[0]*Q[1] - this_pos[1]*Q[0];

                    /* The main ingredients */

                    F  = a*(r*a + r2 - rr0);
                    F2 = F*F;
                    gr = a2/r + ar + 2.0*(a+r);
                    g0 = a + 2.0*r + ar;
                    G = g0*r0e - gr*re;

                    /* Mix them together... */

                    result = (ve*F + vr*G)/F2;

                    /* The computation of the gradient... */

                    huu = 2.0 + 2.0*a/r;
                    for (p = 0; p <= 2; p++) {
                        ga[p] = -a_vec[p]/a;
                        gar[p] = -(ga[p]*ar + this_pos[p])/a;
                        gg0[p] = ga[p] + gar[p];
                        ggr[p] = huu*ga[p] + gar[p];
                        gFF[p] = ga[p]/a - (r*a_vec[p] + a*this_pos[p])/F;
                        gresult[p] = -2.0*result*gFF[p] + (eQ[p]+gFF[p]*ve)/F +
                                (rQ[p]*G + vr*(gg0[p]*r0e + g0*this_dir[p] - ggr[p]*re))/F2;
                    }

                    if (do_field)
                        Bval[k] = Bval[k] + this_coil->w[j]*result;
                    xgrad[k] = xgrad[k] + this_coil->w[j]*gresult[0];
                    ygrad[k] = ygrad[k] + this_coil->w[j]*gresult[1];
                    zgrad[k] = zgrad[k] + this_coil->w[j]*gresult[2];
                }
                if (do_field)
                    Bval[k] = MAG_FACTOR*Bval[k];
                xgrad[k] = MAG_FACTOR*xgrad[k];
                ygrad[k] = MAG_FACTOR*ygrad[k];
                zgrad[k] = MAG_FACTOR*zgrad[k];
            }
        }
    }
    return OK;			/* Happy conclusion: this works always */
}

//=============================================================================================================

int FwdBemModel::fwd_mag_dipole_field(const Eigen::Vector3f& rm, const Eigen::Vector3f& M, FwdCoilSet *coils, float Bval[], void *client)	/* Client data will be the sphere model origin */
/*
 * This is for a specific dipole component
 */
{
    int     j,k,np;
    FwdCoil* this_coil;
    float   sum,diff[3],dist,dist2,dist5,*dir;

    for (k = 0; k < coils->ncoil(); k++) {
        this_coil = coils->coils[k].get();
        if (FWD_IS_MEG_COIL(this_coil->type)) {
            np = this_coil->np;
            /*
           * Go through all points
           */
            for (j = 0, sum = 0.0; j < np; j++) {
                dir = &this_coil->cosmag(j, 0);
                vec_diff(rm,&this_coil->rmag(j, 0),diff);
                dist = vec_len(diff);
                if (dist > EPS) {
                    dist2 = dist*dist;
                    dist5 = dist2*dist2*dist;
                    sum = sum + this_coil->w[j]*(3*vec_dot(M,diff)*vec_dot(diff,dir) - dist2*vec_dot(M,dir))/dist5;
                }
            }				/* All points done */
            Bval[k] = MAG_FACTOR*sum;
        }
        else if (this_coil->type == FWD_COILC_EEG)
            Bval[k] = 0.0;
    }
    return OK;
}

//=============================================================================================================

int FwdBemModel::fwd_mag_dipole_field_vec(const Eigen::Vector3f& rm, FwdCoilSet *coils, float **Bval, void *client)     /* Client data will be the sphere model origin */
/*
 * This is for all dipole components
 * For EEG this produces a zero result
 */
{
    int     j,k,p,np;
    FwdCoil* this_coil;
    float   sum[3],diff[3],dist,dist2,dist5,*dir;

    for (k = 0; k < coils->ncoil(); k++) {
        this_coil = coils->coils[k].get();
        if (FWD_IS_MEG_COIL(this_coil->type)) {
            np = this_coil->np;
            sum[0] = sum[1] = sum[2] = 0.0;
            /*
           * Go through all points
           */
            for (j = 0; j < np; j++) {
                dir = &this_coil->cosmag(j, 0);
                vec_diff(rm,&this_coil->rmag(j, 0),diff);
                dist = vec_len(diff);
                if (dist > EPS) {
                    dist2 = dist*dist;
                    dist5 = dist2*dist2*dist;
                    for (p = 0; p < 3; p++)
                        sum[p] = sum[p] + this_coil->w[j]*(3*diff[p]*vec_dot(diff,dir) - dist2*dir[p])/dist5;
                }
            }           /* All points done */
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
