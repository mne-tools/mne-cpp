//=============================================================================================================
/**
 * @file     mne_surface.cpp
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
 * @brief    Definition of the MNESurface Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface.h"
#include "mne_source_space.h"
#include "mne_triangle.h"
#include "mne_proj_data.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>

#include <QFile>
#include <QDebug>

#define _USE_MATH_DEFINES
#include <math.h>

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================

constexpr int FAIL = -1;
constexpr int OK   =  0;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESurface::MNESurface()
{
}

//=============================================================================================================

MNESurface::~MNESurface()
{
}

//=============================================================================================================
// Const geometry methods
//=============================================================================================================

double MNESurface::sum_solids(const Eigen::Vector3f& from) const
{
    int k;
    double tot_angle, angle;
    for (k = 0, tot_angle = 0.0; k < ntri; k++) {
        angle = solid_angle(from, tris[k]);
        tot_angle += angle;
    }
    return tot_angle;
}

//=============================================================================================================

void MNESurface::triangle_coords(const Eigen::Vector3f& r, int tri, float &x, float &y, float &z) const
{
    double a, b, c, v1, v2, det;
    const MNETriangle* this_tri;

    this_tri = &tris[tri];

    Eigen::Vector3d rr = (r - this_tri->r1).cast<double>();
    z = rr.dot(this_tri->nn.cast<double>());

    a = this_tri->r12.cast<double>().squaredNorm();
    b = this_tri->r13.cast<double>().squaredNorm();
    c = this_tri->r12.cast<double>().dot(this_tri->r13.cast<double>());

    v1 = rr.dot(this_tri->r12.cast<double>());
    v2 = rr.dot(this_tri->r13.cast<double>());

    det = a * b - c * c;

    x = (b * v1 - c * v2) / det;
    y = (a * v2 - c * v1) / det;
}

//=============================================================================================================

int MNESurface::nearest_triangle_point(const Eigen::Vector3f& r, const MNEProjData *user, int tri, float &x, float &y, float &z) const
{
    double p, q, p0, q0, t0;
    double a, b, c, v1, v2, det;
    double best, dist, dist0;
    const MNEProjData* pd = user;
    const MNETriangle* this_tri;

    this_tri = &tris[tri];
    Eigen::Vector3d rr = (r - this_tri->r1).cast<double>();
    dist = rr.dot(this_tri->nn.cast<double>());

    if (pd) {
        if (!pd->act[tri])
            return false;
        a = pd->a[tri];
        b = pd->b[tri];
        c = pd->c[tri];
    }
    else {
        a = this_tri->r12.cast<double>().squaredNorm();
        b = this_tri->r13.cast<double>().squaredNorm();
        c = this_tri->r12.cast<double>().dot(this_tri->r13.cast<double>());
    }

    v1 = rr.dot(this_tri->r12.cast<double>());
    v2 = rr.dot(this_tri->r13.cast<double>());

    det = a * b - c * c;

    p = (b * v1 - c * v2) / det;
    q = (a * v2 - c * v1) / det;

    if (p >= 0.0 && p <= 1.0 &&
            q >= 0.0 && q <= 1.0 &&
            q <= 1.0 - p) {
        x = p;
        y = q;
        z = dist;
        return true;
    }
    /*
     * Side 1 -> 2
     */
    p0 = p + 0.5 * (q * c) / a;
    if (p0 < 0.0) p0 = 0.0;
    else if (p0 > 1.0) p0 = 1.0;
    q0 = 0.0;
    dist0 = sqrt((p - p0) * (p - p0) * a +
                 (q - q0) * (q - q0) * b +
                 (p - p0) * (q - q0) * c +
                 dist * dist);
    best = dist0;
    x = p0;
    y = q0;
    z = dist0;
    /*
     * Side 2 -> 3
     */
    t0 = 0.5 * ((2.0 * a - c) * (1.0 - p) + (2.0 * b - c) * q) / (a + b - c);
    if (t0 < 0.0) t0 = 0.0;
    else if (t0 > 1.0) t0 = 1.0;
    p0 = 1.0 - t0;
    q0 = t0;
    dist0 = sqrt((p - p0) * (p - p0) * a +
                 (q - q0) * (q - q0) * b +
                 (p - p0) * (q - q0) * c +
                 dist * dist);
    if (dist0 < best) {
        best = dist0;
        x = p0;
        y = q0;
        z = dist0;
    }
    /*
     * Side 1 -> 3
     */
    p0 = 0.0;
    q0 = q + 0.5 * (p * c) / b;
    if (q0 < 0.0) q0 = 0.0;
    else if (q0 > 1.0) q0 = 1.0;
    dist0 = sqrt((p - p0) * (p - p0) * a +
                 (q - q0) * (q - q0) * b +
                 (p - p0) * (q - q0) * c +
                 dist * dist);
    if (dist0 < best) {
        best = dist0;
        x = p0;
        y = q0;
        z = dist0;
    }
    return true;
}

//=============================================================================================================

int MNESurface::nearest_triangle_point(const Eigen::Vector3f& r, int tri, float &x, float &y, float &z) const
{
    return nearest_triangle_point(r, nullptr, tri, x, y, z);
}

//=============================================================================================================

Eigen::Vector3f MNESurface::project_to_triangle(int tri, float p, float q) const
{
    const MNETriangle* this_tri = &tris[tri];

    return Eigen::Vector3f(
        this_tri->r1 + p * this_tri->r12 + q * this_tri->r13
    );
}

//=============================================================================================================

Eigen::Vector3f MNESurface::project_to_triangle(int best, const Eigen::Vector3f& r) const
{
    float p, q, dist;
    nearest_triangle_point(r, best, p, q, dist);
    return project_to_triangle(best, p, q);
}

//=============================================================================================================

int MNESurface::project_to_surface(const MNEProjData *proj_data, const Eigen::Vector3f& r, float &distp) const
{
    float dist;
    float p, q;
    float p0, q0, dist0;
    int best;
    int k;

    p0 = q0 = 0.0;
    dist0 = 0.0;
    for (best = -1, k = 0; k < ntri; k++) {
        if (nearest_triangle_point(r, proj_data, k, p, q, dist)) {
            if (best < 0 || std::fabs(dist) < std::fabs(dist0)) {
                dist0 = dist;
                best = k;
                p0 = p;
                q0 = q;
            }
        }
    }
    distp = dist0;
    return best;
}

//=============================================================================================================

void MNESurface::find_closest_on_surface_approx(const PointsT& r, int np_points,
                                                 Eigen::VectorXi& nearest_tri,
                                                 Eigen::VectorXf& dist, int nstep) const
{
    auto p = std::make_unique<MNEProjData>(this);
    int k, was;

    qInfo("%s for %d points %d steps...", nearest_tri[0] < 0 ? "Closest" : "Approx closest", np_points, nstep);

    for (k = 0; k < np_points; k++) {
        was = nearest_tri[k];
        Eigen::Vector3f pt = Eigen::Map<const Eigen::Vector3f>(r.row(k).data());
        decide_search_restriction(*p, nearest_tri[k], nstep, pt);
        nearest_tri[k] = project_to_surface(p.get(), pt, dist[k]);
        if (nearest_tri[k] < 0) {
            decide_search_restriction(*p, -1, nstep, pt);
            nearest_tri[k] = project_to_surface(p.get(), pt, dist[k]);
        }
    }
    (void)was;

    qInfo("[done]\n");
}

//=============================================================================================================

void MNESurface::decide_search_restriction(MNEProjData& p,
                                           int approx_best,
                                           int nstep,
                                           const Eigen::Vector3f& r) const
{
    int k;
    Eigen::Vector3f diff_vec;
    float dist_val, mindist;
    int minvert;

    for (k = 0; k < ntri; k++)
        p.act[k] = 0;

    if (approx_best < 0) {
        mindist = 1000.0;
        minvert = 0;
        for (k = 0; k < np; k++) {
            diff_vec = rr.row(k).transpose() - r;
            dist_val = diff_vec.norm();
            if (dist_val < mindist && nneighbor_tri[k] > 0) {
                mindist = dist_val;
                minvert = k;
            }
        }
    }
    else {
        const MNETriangle* this_tri = &tris[approx_best];
        diff_vec = this_tri->r1 - r;
        mindist = diff_vec.norm();
        minvert = this_tri->vert[0];

        diff_vec = this_tri->r2 - r;
        dist_val = diff_vec.norm();
        if (dist_val < mindist) {
            mindist = dist_val;
            minvert = this_tri->vert[1];
        }
        diff_vec = this_tri->r3 - r;
        dist_val = diff_vec.norm();
        if (dist_val < mindist) {
            mindist = dist_val;
            minvert = this_tri->vert[2];
        }
    }

    activate_neighbors(minvert, p.act, nstep);

    for (k = 0, p.nactive = 0; k < ntri; k++)
        if (p.act[k])
            p.nactive++;
}

//=============================================================================================================

void MNESurface::activate_neighbors(int start, Eigen::VectorXi &act, int nstep) const
{
    int k;

    if (nstep == 0)
        return;

    for (k = 0; k < nneighbor_tri[start]; k++)
        act[neighbor_tri[start][k]] = 1;
    for (k = 0; k < nneighbor_vert[start]; k++)
        activate_neighbors(neighbor_vert[start][k], act, nstep - 1);
}

//=============================================================================================================
// Static factory methods
//=============================================================================================================

std::unique_ptr<MNESurface> MNESurface::read_bem_surface(const QString& name, int which, bool add_geometry)
{
    float sigma;
    return read_bem_surface(name, which, add_geometry, sigma, true);
}

//=============================================================================================================

std::unique_ptr<MNESurface> MNESurface::read_bem_surface(const QString& name, int which, bool add_geometry, float& sigma)
{
    return read_bem_surface(name, which, add_geometry, sigma, true);
}

//=============================================================================================================

std::unique_ptr<MNESurface> MNESurface::read_bem_surface2(const QString& name, int which, bool add_geometry)
{
    float sigma;
    return read_bem_surface(name, which, add_geometry, sigma, false);
}

//=============================================================================================================

std::unique_ptr<MNESurface> MNESurface::read_bem_surface(const QString& name, int which, bool add_geometry, float& sigma, bool check_too_many_neighbors)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FiffDirNode::SPtr> surfs;
    QList<FiffDirNode::SPtr> bems;
    FiffDirNode::SPtr node;
    FiffTag::UPtr t_pTag;

    int     id = -1;
    int     nnode, ntri_count;
    MNESurface* s = nullptr;
    std::unique_ptr<MNESurface> s_ptr;
    int k;
    MatrixXf tmp_node_normals;
    int coord_frame = FIFFV_COORD_MRI;
    float sigmaLocal = -1.0;
    MatrixXf tmp_nodes;
    MatrixXi tmp_triangles;

    if (!stream->open())
        return nullptr;

    bems = stream->dirtree()->dir_tree_find(FIFFB_BEM);
    if (bems.size() > 0) {
        node = bems[0];
        if (node->find_tag(stream, FIFF_BEM_COORD_FRAME, t_pTag)) {
            coord_frame = *t_pTag->toInt();
        }
    }
    surfs = stream->dirtree()->dir_tree_find(FIFFB_BEM_SURF);
    if (surfs.size() == 0) {
        qCritical("No BEM surfaces found in %s", name.toUtf8().constData());
        stream->close(); return nullptr;
    }
    if (which >= 0) {
        for (k = 0; k < surfs.size(); ++k) {
            node = surfs[k];
            if (node->find_tag(stream, FIFF_BEM_SURF_ID, t_pTag)) {
                id = *t_pTag->toInt();
                if (id == which)
                    break;
            }
        }
        if (id != which) {
            qCritical("Desired surface not found in %s", name.toUtf8().constData());
            stream->close(); return nullptr;
        }
    }
    else
        node = surfs[0];

    if (!node->find_tag(stream, FIFF_BEM_SURF_NNODE, t_pTag)) {
        stream->close(); return nullptr;
    }
    nnode = *t_pTag->toInt();

    if (!node->find_tag(stream, FIFF_BEM_SURF_NTRI, t_pTag)) {
        stream->close(); return nullptr;
    }
    ntri_count = *t_pTag->toInt();

    if (!node->find_tag(stream, FIFF_BEM_SURF_NODES, t_pTag)) {
        stream->close(); return nullptr;
    }
    tmp_nodes = t_pTag->toFloatMatrix().transpose();

    if (node->find_tag(stream, FIFF_BEM_SURF_NORMALS, t_pTag)) {
        tmp_node_normals = t_pTag->toFloatMatrix().transpose();
    }

    if (!node->find_tag(stream, FIFF_BEM_SURF_TRIANGLES, t_pTag)) {
        stream->close(); return nullptr;
    }
    tmp_triangles = t_pTag->toIntMatrix().transpose();

    if (node->find_tag(stream, FIFF_MNE_COORD_FRAME, t_pTag)) {
        coord_frame = *t_pTag->toInt();
    }
    else if (node->find_tag(stream, FIFF_BEM_COORD_FRAME, t_pTag)) {
        coord_frame = *t_pTag->toInt();
    }
    if (node->find_tag(stream, FIFF_BEM_SIGMA, t_pTag)) {
        sigmaLocal = *t_pTag->toFloat();
    }

    stream->close();

    s_ptr = std::make_unique<MNESurface>();
    s = s_ptr.get();
    tmp_triangles.array() -= 1;
    s->itris       = tmp_triangles;
    s->id          = which;
    s->sigma       = sigmaLocal;
    s->coord_frame = coord_frame;
    s->rr          = tmp_nodes;
    if (tmp_node_normals.rows() > 0)
        s->nn          = tmp_node_normals;
    s->ntri        = ntri_count;
    s->np          = nnode;
    s->type        = FIFFV_MNE_SPACE_SURFACE;
    s->nuse_tri    = 0;
    s->tot_area    = 0.0;
    s->dist_limit  = -1.0;
    s->vol_dims[0] = s->vol_dims[1] = s->vol_dims[2] = 0;
    s->MRI_vol_dims[0] = s->MRI_vol_dims[1] = s->MRI_vol_dims[2] = 0;
    s->cm[0] = s->cm[1] = s->cm[2] = 0.0;

    if (add_geometry) {
        if (check_too_many_neighbors) {
            if (s->add_geometry_info(s->nn.rows() == 0) != OK) {
                return nullptr;
            }
        }
        else {
            if (s->add_geometry_info2(s->nn.rows() == 0) != OK) {
                return nullptr;
            }
        }
    }
    else if (s->nn.rows() == 0) {
        if (s->add_vertex_normals() != OK) {
            return nullptr;
        }
    }
    else
        s->add_triangle_data();

    s->nuse   = s->np;
    s->inuse  = Eigen::VectorXi::Ones(s->np);
    s->vertno = Eigen::VectorXi::LinSpaced(s->np, 0, s->np - 1);
    sigma = sigmaLocal;

    return s_ptr;
}
