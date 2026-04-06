//=============================================================================================================
/**
 * @file     mne_source_space.cpp
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
 * @brief    Definition of the MNESourceSpace Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_source_space.h"
#include "mne_nearest.h"
#include "mne_patch_info.h"
#include "mne_mgh_tag_group.h"
#include "mne_surface.h"
#include "filter_thread_arg.h"

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_sparse_matrix.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <fiff/fiff_byte_swap.h>

#include <QFile>
#include <QTextStream>
#include <QtConcurrent>

#define _USE_MATH_DEFINES
#include <math.h>

using FIFFLIB::FiffCoordTrans;

#define X_51 0
#define Y_51 1
#define Z_51 2

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

#define X_17 0
#define Y_17 1
#define Z_17 2

#define VEC_DOT_17(x,y) ((x)[X_17]*(y)[X_17] + (x)[Y_17]*(y)[Y_17] + (x)[Z_17]*(y)[Z_17])

#define VEC_LEN_17(x) sqrt(VEC_DOT_17(x,x))

#define VEC_DIFF_17(from,to,diff) {\
    (diff)[X_17] = (to)[X_17] - (from)[X_17];\
    (diff)[Y_17] = (to)[Y_17] - (from)[Y_17];\
    (diff)[Z_17] = (to)[Z_17] - (from)[Z_17];\
}

#define VEC_COPY_17(to,from) {\
    (to)[X_17] = (from)[X_17];\
    (to)[Y_17] = (from)[Y_17];\
    (to)[Z_17] = (from)[Z_17];\
}

#define NNEIGHBORS 26

#define CURVATURE_FILE_MAGIC_NUMBER  (16777215)

#define TAG_OLD_MGH_XFORM           30
#define TAG_OLD_COLORTABLE          1
#define TAG_OLD_USEREALRAS          2
#define TAG_USEREALRAS              4

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// FreeSurfer I/O helpers (file-scope, used only from mne_source_space.cpp)
//=============================================================================================================

namespace {

using PointsT    = MNESurfaceOrVolume::PointsT;
using TrianglesT = MNESurfaceOrVolume::TrianglesT;

//=========================================================================
// Leaf I/O functions
//=========================================================================

int read_int3(QFile &in, int &ival)
/*
 * Read the strange 3-byte integer
 */
{
    unsigned int s = 0;

    if (in.read(reinterpret_cast<char*>(&s), 3) != 3) {
        qCritical("read_int3 could not read data");
        return FAIL;
    }
    s = (unsigned int)FIFFLIB::swap_int(s);
    ival = ((s >> 8) & 0xffffff);
    return OK;
}

int read_int(QFile &in, qint32 &ival)
/*
 * Read a 32-bit integer
 */
{
    qint32 s ;
    if (in.read(reinterpret_cast<char*>(&s), sizeof(qint32)) != static_cast<qint64>(sizeof(qint32))) {
        qCritical("read_int could not read data");
        return FAIL;
    }
    ival = FIFFLIB::swap_int(s);
    return OK;
}

int read_int2(QFile &in, int &ival)
/*
      * Read int from short
      */
{
    short s ;
    if (in.read(reinterpret_cast<char*>(&s), sizeof(short)) != static_cast<qint64>(sizeof(short))) {
        qCritical("read_int2 could not read data");
        return FAIL;
    }
    ival = FIFFLIB::swap_short(s);
    return OK;
}

int read_float(QFile &in, float &fval)
/*
      * Read float
      */
{
    float f ;
    if (in.read(reinterpret_cast<char*>(&f), sizeof(float)) != static_cast<qint64>(sizeof(float))) {
        qCritical("read_float could not read data");
        return FAIL;
    }
    fval = FIFFLIB::swap_float(f);
    return OK;
}

int read_long(QFile &in, long long &lval)
/*
 * Read a 64-bit integer
 */
{
    long long s ;
    if (in.read(reinterpret_cast<char*>(&s), sizeof(long long)) != static_cast<qint64>(sizeof(long long))) {
        qCritical("read_long could not read data");
        return FAIL;
    }
    lval = FIFFLIB::swap_long(s);
    return OK;
}

//=========================================================================
// Leaf validation / copy
//=========================================================================

int check_vertex(int no, int maxno)
{
    if (no < 0 || no > maxno-1) {
        printf("Illegal vertex number %d (max %d).",no,maxno);
        return FAIL;
    }
    return OK;
}

MNEVolGeom dup_vol_geom(const MNEVolGeom& g)
{
    MNEVolGeom dup;
    dup = g;
    dup.filename = g.filename;
    return dup;
}

//=========================================================================
// read_vol_geom
//=========================================================================

MNEVolGeom* read_vol_geom(QFile &fp)
/*
 * This the volume geometry reading code from FreeSurfer
 */
{
    char param[64];
    char eq[2];
    char buf[256];
    int vgRead = 0;
    int counter = 0;
    qint64 pos = 0;

    MNEVolGeom* vg = new MNEVolGeom();

    while (!fp.atEnd() && counter < 8)
    {
        QByteArray lineData = fp.readLine(256);
        if (lineData.isEmpty())
            break;
        const char *line = lineData.constData();
        if (strlen(line) == 0)
            break ;
        sscanf(line, "%s %s %*s", param, eq);
        if (!strcmp(param, "valid")) {
            sscanf(line, "%s %s %d", param, eq, &vg->valid);
            vgRead = 1;
            counter++;
        }
        else if (!strcmp(param, "filename")) {
            if (sscanf(line, "%s %s %s", param, eq, buf) >= 3)
                vg->filename = strdup(buf);
            counter++;
        }
        else if (!strcmp(param, "volume")) {
            sscanf(line, "%s %s %d %d %d",
                   param, eq, &vg->width, &vg->height, &vg->depth);
            counter++;
        }
        else if (!strcmp(param, "voxelsize")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, &vg->xsize, &vg->ysize, &vg->zsize);
            /*
       * We like these to be in meters
       */
            vg->xsize = vg->xsize/1000.0;
            vg->ysize = vg->ysize/1000.0;
            vg->zsize = vg->zsize/1000.0;
            counter++;
        }
        else if (!strcmp(param, "xras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->x_ras, vg->x_ras+1, vg->x_ras+2);
            counter++;
        }
        else if (!strcmp(param, "yras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->y_ras, vg->y_ras+1, vg->y_ras+2);
            counter++;
        }
        else if (!strcmp(param, "zras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->z_ras, vg->z_ras+1, vg->z_ras+2);
            counter++;
        }
        else if (!strcmp(param, "cras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->c_ras, vg->c_ras+1, vg->c_ras+2);
            vg->c_ras[0] = vg->c_ras[0]/1000.0;
            vg->c_ras[1] = vg->c_ras[1]/1000.0;
            vg->c_ras[2] = vg->c_ras[2]/1000.0;
            counter++;
        }
        /* remember the current position */
        pos = fp.pos();
    };
    if (!fp.atEnd()) { /* we read one more line */
        if (pos > 0 ) /* if success in getting pos, then */
            fp.seek(pos); /* restore the position */
        /* note that this won't allow compression using pipe */
    }
    if (!vgRead) {
        delete vg;
        vg = new MNEVolGeom();
    }
    return vg;
}

//=========================================================================
// read_tag_data
//=========================================================================

int read_tag_data(QFile &fp, int tag, long long nbytes, unsigned char *&val, long long &nbytesp)
/*
 * Read the data of one tag
 */
{
    unsigned char *dum = NULL;
    size_t snbytes = nbytes;

     val = NULL;
    if (nbytes > 0) {
        dum = new unsigned char[nbytes+1]();
        if (fp.read(reinterpret_cast<char*>(dum), nbytes) != static_cast<qint64>(snbytes)) {
            printf("Failed to read %d bytes of tag data",static_cast<int>(nbytes));
            delete[] dum;
            return FAIL;
        }
        dum[nbytes] = '\0'; /* Ensure null termination */
        val     = dum;
        nbytesp = nbytes;
    }
    else {			/* Need to handle special cases */
        if (tag == TAG_OLD_SURF_GEOM) {
            MNEVolGeom* g = read_vol_geom(fp);
            if (!g)
                return FAIL;
            val     = (unsigned char *)g;
            nbytesp = sizeof(MNEVolGeom);
        }
        else if (tag == TAG_OLD_USEREALRAS || tag == TAG_USEREALRAS) {
            int *vi = new int[1]();
            if (read_int(fp,*vi) == FAIL)
                vi = 0;
            val = (unsigned char *)vi;
            nbytesp = sizeof(int);
        }
        else {
            printf("Encountered an unknown tag with no length specification : %d\n",tag);
            val     = NULL;
            nbytesp = 0;
        }
    }
    return OK;
}

//=========================================================================
// add_mgh_tag_to_group
//=========================================================================

void add_mgh_tag_to_group(std::optional<MNEMghTagGroup>& g, int tag, long long len, unsigned char *data)
{
    if (!g)
        g = MNEMghTagGroup();
    auto new_tag = std::make_unique<MNEMghTag>();
    new_tag->tag  = tag;
    new_tag->len  = len;
    new_tag->data = QByteArray(reinterpret_cast<const char*>(data), static_cast<int>(len));
    delete[] data;
    g->tags.push_back(std::move(new_tag));
}

//=========================================================================
// read_next_tag
//=========================================================================

int read_next_tag(QFile &fp, int &tagp, long long &lenp, unsigned char *&datap)
/*
 * Read the next tag in the file
 */
{
    int       ilen,tag;
    long long len;

    if (read_int(fp,tag) == FAIL) {
        tagp = 0;
        return OK;
    }
    if (fp.atEnd()) {
        tagp = 0;
        return OK;
    }
    switch (tag) {
    case TAG_OLD_MGH_XFORM: /* This is obviously a burden of the past */
        if (read_int(fp,ilen) == FAIL)
            return FAIL;
        len = ilen - 1;
        break ;
    case TAG_OLD_SURF_GEOM:
    case TAG_OLD_USEREALRAS:
    case TAG_OLD_COLORTABLE:
        len = 0 ;
        break ;
    default:
        if (read_long(fp,len) == FAIL)
            return FAIL;
        break;
    }
     lenp = len;
     tagp = tag;
    if (read_tag_data(fp,tag,len,datap,lenp) == FAIL)
        return FAIL;
    return OK;
}

//=========================================================================
// read_mgh_tags
//=========================================================================

int read_mgh_tags(QFile &fp, std::optional<MNEMghTagGroup>& tagsp)
/*
 * Read all the tags from the file
 */
{
    long long     len;
    int           tag;
    unsigned char *tag_data;

    while (1) {
        if (read_next_tag(fp,tag,len,tag_data) == FAIL)
            return FAIL;
        if (tag == 0)
            break;
        add_mgh_tag_to_group(tagsp,tag,len,tag_data);
    }
    return OK;
}

//=========================================================================
// read_curvature_file
//=========================================================================

int read_curvature_file(const QString& fname,
                        Eigen::VectorXf& curv)

{
    QFile fp(fname);
    int  magic;

    float curvmin,curvmax;
    int   ncurv  = 0;
    int   nface,val_pervert;
    int   val,k;
    float fval;

    if (!fp.open(QIODevice::ReadOnly)) {
        qCritical() << fname;
        goto bad;
    }
    if (read_int3(fp,magic) != 0) {
        qCritical() << "Bad magic in" << fname;
        goto bad;
    }
    if (magic == CURVATURE_FILE_MAGIC_NUMBER) {	    /* A new-style curvature file */
        /*
 * How many and faces
 */
        if (read_int(fp,ncurv) != 0)
            goto bad;
        if (read_int(fp,nface) != 0)
            goto bad;
#ifdef DEBUG
        printf("nvert = %d nface = %d\n",ncurv,nface);
#endif
        if (read_int(fp,val_pervert) != 0)
            goto bad;
        if (val_pervert != 1) {
            qCritical("Values per vertex not equal to one.");
            goto bad;
        }
        /*
 * Read the curvature values
 */
        curv.resize(ncurv);
        curvmin = curvmax = 0.0;
        for (k = 0; k < ncurv; k++) {
            if (read_float(fp,fval) != 0)
                goto bad;
            curv[k] = fval;
            if (curv[k] > curvmax)
                curvmax = curv[k];
            if (curv[k] < curvmin)
                curvmin = curv[k];
        }
    }
    else {			                    /* An old-style curvature file */
        ncurv = magic;
        /*
 * How many vertices
 */
        if (read_int3(fp,nface) != 0)
            goto bad;
#ifdef DEBUG
        printf("nvert = %d nface = %d\n",ncurv,nface);
#endif
        /*
 * Read the curvature values
 */
        curv.resize(ncurv);
        curvmin = curvmax = 0.0;
        for (k = 0; k < ncurv; k++) {
            if (read_int2(fp,val) != 0)
                goto bad;
            curv[k] = (float)val/100.0;
            if (curv[k] > curvmax)
                curvmax = curv[k];
            if (curv[k] < curvmin)
                curvmin = curv[k];

        }
    }
#ifdef DEBUG
    printf("Curvature range: %f...%f\n",curvmin,curvmax);
#endif
    return OK;

bad : {
        curv.resize(0);
        return FAIL;
    }
}

//=========================================================================
// read_triangle_file
//=========================================================================

int read_triangle_file(const QString& fname,
                       PointsT& vertices,
                       TrianglesT& triangles,
                       std::optional<MNEMghTagGroup>* tagsp)
/*
      * Read the FS triangulated surface
      */
{
    QFile fp(fname);
    int  magic;
    char c;

    qint32  nvert,ntri,nquad;
    PointsT    vert;
    TrianglesT tri;
    int   k,p;
    int   quad[4];
    int   val;
    int   which;

    if (!fp.open(QIODevice::ReadOnly)) {
        qCritical() << fname;
        goto bad;
    }
    if (read_int3(fp,magic) != 0) {
        qCritical() << "Bad magic in" << fname;
        goto bad;
    }
    if (magic != TRIANGLE_FILE_MAGIC_NUMBER &&
            magic != QUAD_FILE_MAGIC_NUMBER &&
            magic != NEW_QUAD_FILE_MAGIC_NUMBER) {
        qCritical() << "Bad magic in" << fname;
        goto bad;
    }
    if (magic == TRIANGLE_FILE_MAGIC_NUMBER) {
        /*
     * Get the comment
     */
        printf("Triangle file : ");
        for (fp.getChar(&c); c != '\n'; fp.getChar(&c)) {
            if (fp.atEnd()) {
                qCritical()<<"Bad triangle file.";
                goto bad;
            }
            putc(c,stderr);
        }
        fp.getChar(&c);
        /*
     * How many vertices and triangles?
     */
        if (read_int(fp,nvert) != 0)
            goto bad;
        if (read_int(fp,ntri) != 0)
            goto bad;
        printf(" nvert = %d ntri = %d\n",nvert,ntri);
        vert.resize(nvert, 3);
        tri.resize(ntri, 3);
        /*
     * Read the vertices
     */
        for (k = 0; k < nvert; k++) {
            if (read_float(fp,vert(k,X_17)) != 0)
                goto bad;
            if (read_float(fp,vert(k,Y_17)) != 0)
                goto bad;
            if (read_float(fp,vert(k,Z_17)) != 0)
                goto bad;
        }
        /*
     * Read the triangles
     */
        for (k = 0; k < ntri; k++) {
            if (read_int(fp,tri(k,X_17)) != 0)
                goto bad;
            if (check_vertex(tri(k,X_17),nvert) != OK)
                goto bad;
            if (read_int(fp,tri(k,Y_17)) != 0)
                goto bad;
            if (check_vertex(tri(k,Y_17),nvert) != OK)
                goto bad;
            if (read_int(fp,tri(k,Z_17)) != 0)
                goto bad;
            if (check_vertex(tri(k,Z_17),nvert) != OK)
                goto bad;
        }
    }
    else if (magic == QUAD_FILE_MAGIC_NUMBER ||
             magic == NEW_QUAD_FILE_MAGIC_NUMBER) {
        if (read_int3(fp,nvert) != 0)
            goto bad;
        if (read_int3(fp,nquad) != 0)
            goto bad;
        printf("%s file : nvert = %d nquad = %d\n",
                magic == QUAD_FILE_MAGIC_NUMBER ? "Quad" : "New quad",
                nvert,nquad);
        vert.resize(nvert, 3);
        if (magic == QUAD_FILE_MAGIC_NUMBER) {
            for (k = 0; k < nvert; k++) {
                if (read_int2(fp,val) != 0)
                    goto bad;
                vert(k,X_17) = val/100.0;
                if (read_int2(fp,val) != 0)
                    goto bad;
                vert(k,Y_17) = val/100.0;
                if (read_int2(fp,val) != 0)
                    goto bad;
                vert(k,Z_17) = val/100.0;
            }
        }
        else {			/* NEW_QUAD_FILE_MAGIC_NUMBER */
            for (k = 0; k < nvert; k++) {
                if (read_float(fp,vert(k,X_17)) != 0)
                    goto bad;
                if (read_float(fp,vert(k,Y_17)) != 0)
                    goto bad;
                if (read_float(fp,vert(k,Z_17)) != 0)
                    goto bad;
            }
        }
        ntri = 2*nquad;
        tri.resize(ntri, 3);
        for (k = 0, ntri = 0; k < nquad; k++) {
            for (p = 0; p < 4; p++) {
                if (read_int3(fp,quad[p]) != 0)
                    goto bad;
            }

            /*
     * The randomization is borrowed from FreeSurfer code
     * Strange...
     */
#define EVEN(n)      ((((n) / 2) * 2) == n)
#ifdef FOO
#define WHICH_FACE_SPLIT(vno0, vno1) \
    (1*nearbyint(sqrt(1.9*vno0) + sqrt(3.5*vno1)))

            which = WHICH_FACE_SPLIT(quad[0], quad[1]) ;
#endif
            which = quad[0];
            /*
    printf("%f ",sqrt(1.9*quad[0]) + sqrt(3.5*quad[1]));
     */

            if (EVEN(which)) {
                tri(ntri,X_17) = quad[0];
                tri(ntri,Y_17) = quad[1];
                tri(ntri,Z_17) = quad[3];
                ntri++;

                tri(ntri,X_17) = quad[2];
                tri(ntri,Y_17) = quad[3];
                tri(ntri,Z_17) = quad[1];
                ntri++;
            }
            else {
                tri(ntri,X_17) = quad[0];
                tri(ntri,Y_17) = quad[1];
                tri(ntri,Z_17) = quad[2];
                ntri++;

                tri(ntri,X_17) = quad[0];
                tri(ntri,Y_17) = quad[2];
                tri(ntri,Z_17) = quad[3];
                ntri++;
            }
        }
    }
    /*
     * Optionally read the tags
     */
    if (tagsp) {
        std::optional<MNEMghTagGroup> tags;
        if (read_mgh_tags(fp, tags) == FAIL) {
            goto bad;
        }
        *tagsp = std::move(tags);
    }
    /*
     * Convert mm to m and store as Eigen matrices
     */
    vert /= 1000.0f;
    vertices = std::move(vert);
    triangles = std::move(tri);
    return OK;

bad : {
        return FAIL;
    }
}

//=========================================================================
// get_volume_geom_from_tag
//=========================================================================

std::optional<MNEVolGeom> get_volume_geom_from_tag(const MNEMghTagGroup *tagsp)
{
    MNEMghTag*      tag  = NULL;

    if (tagsp) {
        for (const auto &t : tagsp->tags)
            if (t->tag == TAG_OLD_SURF_GEOM) {
                tag = t.get();
                break;
            }
        if (tag)
            return dup_vol_geom(*reinterpret_cast<MNEVolGeom*>(tag->data.data()));
    }
    return std::nullopt;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESourceSpace::MNESourceSpace(int np)
{
    this->np      = np;
    if (np > 0) {
        rr      = PointsT::Zero(np, 3);
        nn      = NormalsT::Zero(np, 3);
        inuse   = VectorXi::Zero(np);
        vertno  = VectorXi::Zero(np);
    }
    nuse     = 0;
    ntri     = 0;
    tot_area = 0.0;

    nuse_tri  = 0;

    // tris, use_tris are std::vector<MNETriangle> — default-constructed empty

    // neighbor_tri, nneighbor_tri, curv, val,
    // neighbor_vert, nneighbor_vert, vert_dist
    // are Eigen/std::vector types — default-constructed empty

    coord_frame = FIFFV_COORD_MRI;
    id          = FIFFV_MNE_SURF_UNKNOWN;
    subject     = "";
    type        = FIFFV_MNE_SPACE_SURFACE;

    // nearest is std::vector<MNENearest> — default-constructed empty
    // patches is std::vector<optional<MNEPatchInfo>> — default-constructed empty

    dist       = FIFFLIB::FiffSparseMatrix();
    dist_limit = -1.0;

    voxel_surf_RAS_t.reset();
    vol_dims[0] = vol_dims[1] = vol_dims[2] = 0;

    MRI_volume           = "";
    MRI_surf_RAS_RAS_t.reset();
    MRI_voxel_surf_RAS_t.reset();
    MRI_vol_dims[0] = MRI_vol_dims[1] = MRI_vol_dims[2] = 0;
    interpolator.reset();

    vol_geom.reset();
    mgh_tags.reset();

    cm[X_51] = cm[Y_51] = cm[Z_51] = 0.0;
}

//=============================================================================================================

MNESourceSpace::~MNESourceSpace()
{
}

//=============================================================================================================

MNESourceSpace::SPtr MNESourceSpace::clone() const
{
    // Base class clone — creates a MNESourceSpace with the same fields.
    // Derived classes (e.g., MNEHemisphere) override this to preserve their type.
    auto copy = std::make_shared<MNESourceSpace>(this->np);
    copy->type        = this->type;
    copy->id          = this->id;
    copy->np          = this->np;
    copy->ntri        = this->ntri;
    copy->coord_frame = this->coord_frame;
    copy->rr          = this->rr;
    copy->nn          = this->nn;
    copy->nuse        = this->nuse;
    copy->inuse       = this->inuse;
    copy->vertno      = this->vertno;
    copy->itris       = this->itris;
    copy->use_itris   = this->use_itris;
    copy->nuse_tri    = this->nuse_tri;
    copy->dist_limit  = this->dist_limit;
    copy->dist        = this->dist;
    copy->nearest     = this->nearest;
    copy->neighbor_tri  = this->neighbor_tri;
    copy->neighbor_vert = this->neighbor_vert;
    return copy;
}

//=============================================================================================================

void MNESourceSpace::enable_all_sources()
{
    int k;
    for (k = 0; k < np; k++)
        inuse[k] = TRUE;
    nuse = np;
    return;
}

//=============================================================================================================

int MNESourceSpace::is_left_hemi() const
/*
 * Left or right hemisphere?
 */
{
    int k;
    float xave;

    for (k = 0, xave = 0.0; k < np; k++)
        xave += rr(k,0);
    if (xave < 0.0)
        return TRUE;
    else
        return FALSE;
}

//=============================================================================================================

qint32 MNESourceSpace::find_source_space_hemi() const
{
    double xave = rr.col(0).sum();
    if (xave < 0)
        return FIFFV_MNE_SURF_LEFT_HEMI;
    else
        return FIFFV_MNE_SURF_RIGHT_HEMI;
}

//=============================================================================================================

void MNESourceSpace::update_inuse(Eigen::VectorXi new_inuse)
/*
 * Update the active vertices
 */
{
    int k,p,nuse_count;

    inuse = std::move(new_inuse);

    for (k = 0, nuse_count = 0; k < np; k++)
        if (inuse[k])
            nuse_count++;

    nuse = nuse_count;
    if (nuse > 0) {
        vertno.conservativeResize(nuse);
        for (k = 0, p = 0; k < np; k++)
            if (inuse[k])
                vertno[p++] = k;
    }
    else {
        vertno.resize(0);
    }
    return;
}

//=============================================================================================================

int MNESourceSpace::transform_source_space(const FiffCoordTrans& t)
/*
     * Transform source space data into another coordinate frame
     */
{
    int k;
    if (coord_frame == t.to)
        return OK;
    if (coord_frame != t.from) {
        printf("Coordinate transformation does not match with the source space coordinate system.");
        return FAIL;
    }
    for (k = 0; k < np; k++) {
        FiffCoordTrans::apply_trans(&rr(k,0),t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(&nn(k,0),t,FIFFV_NO_MOVE);
    }
    if (!tris.empty()) {
        for (k = 0; k < ntri; k++)
            FiffCoordTrans::apply_trans(tris[k].nn.data(),t,FIFFV_NO_MOVE);
    }
    coord_frame = t.to;
    return OK;
}

//=============================================================================================================

int MNESourceSpace::add_patch_stats()
{
    MNENearest* nearest_data = nearest.data();
    MNENearest* this_patch;
    std::vector<std::optional<MNEPatchInfo>> pinfo(nuse);
    int        nave,p,q,k;

    printf("Computing patch statistics...\n");
    if (neighbor_tri.empty())
        if (add_geometry_info(FALSE) != OK)
            return FAIL;

    if (nearest.empty()) {
        qCritical("The patch information is not available.");
        return FAIL;
    }
    if (nuse == 0) {
        patches.clear();
        return OK;
    }
    /*
       * Calculate the average normals and the patch areas
       */
    printf("\tareas, average normals, and mean deviations...");
    std::sort(nearest.begin(), nearest.end(),
              [](const MNENearest& a, const MNENearest& b) { return a.nearest < b.nearest; });
    nearest_data = nearest.data();  // refresh after sort
    nave = 1;
    for (p = 1, q = 0; p < np; p++) {
        if (nearest_data[p].nearest != nearest_data[p-1].nearest) {
            if (nave == 0) {
                qCritical("No vertices belong to the patch of vertex %d",nearest_data[p-1].nearest);
                return FAIL;
            }
            if (vertno[q] == nearest_data[p-1].nearest) { /* Some source space points may have been omitted since
                               * the patch information was computed */
                pinfo[q] = MNEPatchInfo();
                pinfo[q]->vert = nearest_data[p-1].nearest;
                this_patch = nearest_data+p-nave;
                pinfo[q]->memb_vert.resize(nave);
                for (k = 0; k < nave; k++) {
                    pinfo[q]->memb_vert[k] = this_patch[k].vert;
                    this_patch[k].patch    = &(*pinfo[q]);
                }
                pinfo[q]->calculate_area(this);
                pinfo[q]->calculate_normal_stats(this);
                q++;
            }
            nave = 0;
        }
        nave++;
    }
    if (nave == 0) {
        qCritical("No vertices belong to the patch of vertex %d",nearest_data[p-1].nearest);
        return FAIL;
    }
    if (vertno[q] == nearest_data[p-1].nearest) {
        pinfo[q]       = MNEPatchInfo();
        pinfo[q]->vert = nearest_data[p-1].nearest;
        this_patch = nearest_data+p-nave;
        pinfo[q]->memb_vert.resize(nave);
        for (k = 0; k < nave; k++) {
            pinfo[q]->memb_vert[k] = this_patch[k].vert;
            this_patch[k].patch = &(*pinfo[q]);
        }
        pinfo[q]->calculate_area(this);
        pinfo[q]->calculate_normal_stats(this);
        q++;
    }
    printf(" %d/%d [done]\n",q,nuse);

    patches = std::move(pinfo);

    return OK;
}

//=============================================================================================================

void MNESourceSpace::rearrange_source_space()
{
    int k,p;

    for (k = 0, nuse = 0; k < np; k++)
        if (inuse[k])
            nuse++;

    if (nuse == 0) {
        vertno.resize(0);
    }
    else {
        vertno.conservativeResize(nuse);
        for (k = 0, p = 0; k < np; k++)
            if (inuse[k])
                vertno[p++] = k;
    }
    if (!nearest.empty())
        add_patch_stats();
    return;
}

//=============================================================================================================

std::unique_ptr<MNESourceSpace> MNESourceSpace::create_source_space(int np)
/*
          * Create a new source space and all associated data
          */
{
    auto res = std::make_unique<MNESourceSpace>();
    res->np      = np;
    if (np > 0) {
        res->rr      = PointsT::Zero(np, 3);
        res->nn      = NormalsT::Zero(np, 3);
        res->inuse   = VectorXi::Zero(np);
        res->vertno  = VectorXi::Zero(np);
    }
    res->nuse     = 0;
    res->ntri     = 0;
    res->tot_area = 0.0;

    res->nuse_tri  = 0;

    res->sigma       = -1.0;
    res->coord_frame = FIFFV_COORD_MRI;
    res->id          = FIFFV_MNE_SURF_UNKNOWN;
    res->subject.clear();
    res->type        = FIFFV_MNE_SPACE_SURFACE;

    res->dist       = FIFFLIB::FiffSparseMatrix();
    res->dist_limit = -1.0;

    res->voxel_surf_RAS_t.reset();
    res->vol_dims[0] = res->vol_dims[1] = res->vol_dims[2] = 0;

    res->MRI_volume.clear();
    res->MRI_surf_RAS_RAS_t.reset();
    res->MRI_voxel_surf_RAS_t.reset();
    res->MRI_vol_dims[0] = res->MRI_vol_dims[1] = res->MRI_vol_dims[2] = 0;
    res->interpolator.reset();

    res->vol_geom.reset();
    res->mgh_tags.reset();

    res->cm[0] = res->cm[1] = res->cm[2] = 0.0;

    return res;
}

//=============================================================================================================

std::unique_ptr<MNESourceSpace> MNESourceSpace::load_surface(const QString& surf_file,
                                                             const QString& curv_file)
{
    return load_surface_geom(surf_file,curv_file,TRUE,TRUE);
}

//=============================================================================================================

std::unique_ptr<MNESourceSpace> MNESourceSpace::load_surface_geom(const QString& surf_file,
                                                                   const QString& curv_file,
                                                                   int  add_geometry,
                                                                   int  check_too_many_neighbors)
    /*
     * Load the surface and add the geometry information
     */
{
    int   k;
    std::unique_ptr<MNESourceSpace> s;
    std::optional<MNEMghTagGroup> tags;
    Eigen::VectorXf curvs;
    PointsT verts;
    TrianglesT tris;

    if (read_triangle_file(surf_file,
                               verts,
                               tris,
                               &tags) == -1)
        goto bad;

    if (!curv_file.isEmpty()) {
        if (read_curvature_file(curv_file, curvs) == -1)
            goto bad;
        if (curvs.size() != verts.rows()) {
            qCritical()<<"Incorrect number of vertices in the curvature file.";
            goto bad;
        }
    }

    s = std::make_unique<MNESourceSpace>(0);
    s->rr   = std::move(verts);
    s->itris = std::move(tris);
    s->ntri = s->itris.rows();
    s->np   = s->rr.rows();
    if (curvs.size() > 0) {
        s->curv = std::move(curvs);
    }
    s->val = Eigen::VectorXf::Zero(s->np);
    if (add_geometry) {
        if (check_too_many_neighbors) {
            if (s->add_geometry_info(TRUE) != OK)
                goto bad;
        }
        else {
            if (s->add_geometry_info2(TRUE) != OK)
                goto bad;
        }
    }
    else if (s->nn.rows() == 0) {			/* Normals only */
        if (s->add_vertex_normals() != OK)
            goto bad;
    }
    else
        s->add_triangle_data();
    s->nuse   = s->np;
    s->inuse  = Eigen::VectorXi::Ones(s->np);
    s->vertno = Eigen::VectorXi::LinSpaced(s->np, 0, s->np - 1);
    s->mgh_tags = std::move(tags);
    s->vol_geom = get_volume_geom_from_tag(s->mgh_tags ? &(*s->mgh_tags) : nullptr);

    return s;

bad : {
        return nullptr;
    }
}

//=============================================================================================================

static std::optional<FiffCoordTrans> make_voxel_ras_trans(float *r0,
                                                  float *x_ras,
                                                  float *y_ras,
                                                  float *z_ras,
                                                  float *voxel_size)

{
    float rot[3][3],move[3];
    int   j,k;

    VEC_COPY_17(move,r0);

    for (j = 0; j < 3; j++) {
        rot[j][0] = x_ras[j];
        rot[j][1] = y_ras[j];
        rot[j][2] = z_ras[j];
    }

    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            rot[j][k]    = voxel_size[k]*rot[j][k];

    return FiffCoordTrans(FIFFV_MNE_COORD_MRI_VOXEL,FIFFV_COORD_MRI,
                                            Eigen::Map<Eigen::Matrix3f>(&rot[0][0]),
                                            Eigen::Map<Eigen::Vector3f>(move));
}

MNESourceSpace* MNESourceSpace::make_volume_source_space(const MNESurface& surf, float grid, float exclude, float mindist)
/*
     * Make a source space which covers the volume bounded by surf
     */
{
    float min[3],max[3],cm[3];
    int   minn[3],maxn[3];
    const float *node;
    float maxdist,dist,diff[3];
    int   k,c;
    std::unique_ptr<MNESourceSpace> sp;
    int np,nplane,nrow;
    int nneigh;
    int x,y,z;
    /*
        * Figure out the grid size
        */
    cm[X_17] = cm[Y_17] = cm[Z_17] = 0.0;
    node = &surf.rr(0,0);
    for (c = 0; c < 3; c++)
        min[c] = max[c] = node[c];

    for (k = 0; k < surf.np; k++) {
        node = &surf.rr(k,0);
        for (c = 0; c < 3; c++) {
            cm[c] += node[c];
            if (node[c] < min[c])
                min[c] = node[c];
            if (node[c] > max[c])
                max[c] = node[c];
        }
    }
    for (c = 0; c < 3; c++)
        cm[c] = cm[c]/surf.np;
    /*
       * Define the sphere which fits the surface
       */
    maxdist = 0.0;
    for (k = 0; k < surf.np; k++) {
        VEC_DIFF_17(cm,&surf.rr(k,0),diff);
        dist = VEC_LEN_17(diff);
        if (dist > maxdist)
            maxdist = dist;
    }
    printf("FsSurface CM = (%6.1f %6.1f %6.1f) mm\n",
           1000*cm[X_17], 1000*cm[Y_17], 1000*cm[Z_17]);
    printf("FsSurface fits inside a sphere with radius %6.1f mm\n",1000*maxdist);
    printf("FsSurface extent:\n"
           "\tx = %6.1f ... %6.1f mm\n"
           "\ty = %6.1f ... %6.1f mm\n"
           "\tz = %6.1f ... %6.1f mm\n",
           1000*min[X_17],1000*max[X_17],
           1000*min[Y_17],1000*max[Y_17],
           1000*min[Z_17],1000*max[Z_17]);
    for (c = 0; c < 3; c++) {
        if (max[c] > 0)
            maxn[c] = floor(std::fabs(max[c])/grid)+1;
        else
            maxn[c] = -floor(std::fabs(max[c])/grid)-1;
        if (min[c] > 0)
            minn[c] = floor(std::fabs(min[c])/grid)+1;
        else
            minn[c] = -floor(std::fabs(min[c])/grid)-1;
    }
    printf("Grid extent:\n"
           "\tx = %6.1f ... %6.1f mm\n"
           "\ty = %6.1f ... %6.1f mm\n"
           "\tz = %6.1f ... %6.1f mm\n",
           1000*(minn[X_17]*grid),1000*(maxn[X_17]*grid),
           1000*(minn[Y_17]*grid),1000*(maxn[Y_17]*grid),
           1000*(minn[Z_17]*grid),1000*(maxn[Z_17]*grid));
    /*
       * Now make the initial grid
       */
    np = 1;
    for (c = 0; c < 3; c++)
        np = np*(maxn[c]-minn[c]+1);
    nplane = (maxn[X_17]-minn[X_17]+1)*(maxn[Y_17]-minn[Y_17]+1);
    nrow   = (maxn[X_17]-minn[X_17]+1);
    sp = MNESourceSpace::create_source_space(np);
    sp->type = MNE_SOURCE_SPACE_VOLUME;
    sp->nneighbor_vert = Eigen::VectorXi::Constant(sp->np, NNEIGHBORS);
    sp->neighbor_vert.resize(sp->np);
    for (k = 0; k < sp->np; k++) {
        sp->inuse[k]  = TRUE;
        sp->vertno[k] = k;
        sp->nn(k,X_17) = sp->nn(k,Y_17) = 0.0; /* Source orientation is immaterial */
        sp->nn(k,Z_17) = 1.0;
        sp->neighbor_vert[k] = Eigen::VectorXi::Constant(NNEIGHBORS, -1);
        sp->nuse++;
    }
    for (k = 0, z = minn[Z_17]; z <= maxn[Z_17]; z++) {
        for (y = minn[Y_17]; y <= maxn[Y_17]; y++) {
            for (x = minn[X_17]; x <= maxn[X_17]; x++, k++) {
                sp->rr(k,X_17) = x*grid;
                sp->rr(k,Y_17) = y*grid;
                sp->rr(k,Z_17) = z*grid;
                /*
             * Figure out the neighborhood:
             * 6-neighborhood first
             */
                Eigen::VectorXi& neigh = sp->neighbor_vert[k];
                if (z > minn[Z_17])
                    neigh[0]  = k - nplane;
                if (x < maxn[X_17])
                    neigh[1] = k + 1;
                if (y < maxn[Y_17])
                    neigh[2] = k + nrow;
                if (x > minn[X_17])
                    neigh[3] = k - 1;
                if (y > minn[Y_17])
                    neigh[4] = k - nrow;
                if (z < maxn[Z_17])
                    neigh[5] = k + nplane;
                /*
             * Then the rest to complete the 26-neighborhood
             * First the plane below
             */
                if (z > minn[Z_17]) {
                    if (x < maxn[X_17]) {
                        neigh[6] = k + 1 - nplane;
                        if (y < maxn[Y_17])
                            neigh[7] = k + 1 + nrow - nplane;
                    }
                    if (y < maxn[Y_17])
                        neigh[8] = k + nrow - nplane;
                    if (x > minn[X_17]) {
                        if (y < maxn[Y_17])
                            neigh[9] = k - 1 + nrow - nplane;
                        neigh[10] = k - 1 - nplane;
                        if (y > minn[Y_17])
                            neigh[11] = k - 1 - nrow - nplane;
                    }
                    if (y > minn[Y_17]) {
                        neigh[12] = k - nrow - nplane;
                        if (x < maxn[X_17])
                            neigh[13] = k + 1 - nrow - nplane;
                    }
                }
                /*
             * Then the same plane
             */
                if (x < maxn[X_17] && y < maxn[Y_17])
                    neigh[14] = k + 1 + nrow;
                if (x > minn[X_17]) {
                    if (y < maxn[Y_17])
                        neigh[15] = k - 1 + nrow;
                    if (y > minn[Y_17])
                        neigh[16] = k - 1 - nrow;
                }
                if (y > minn[Y_17] && x < maxn[X_17])
                    neigh[17] = k + 1 - nrow - nplane;
                /*
             * Finally one plane above
             */
                if (z < maxn[Z_17]) {
                    if (x < maxn[X_17]) {
                        neigh[18] = k + 1 + nplane;
                        if (y < maxn[Y_17])
                            neigh[19] = k + 1 + nrow + nplane;
                    }
                    if (y < maxn[Y_17])
                        neigh[20] = k + nrow + nplane;
                    if (x > minn[X_17]) {
                        if (y < maxn[Y_17])
                            neigh[21] = k - 1 + nrow + nplane;
                        neigh[22] = k - 1 + nplane;
                        if (y > minn[Y_17])
                            neigh[23] = k - 1 - nrow + nplane;
                    }
                    if (y > minn[Y_17]) {
                        neigh[24] = k - nrow + nplane;
                        if (x < maxn[X_17])
                            neigh[25] = k + 1 - nrow + nplane;
                    }
                }
            }
        }
    }
    printf("%d sources before omitting any.\n",sp->nuse);
    /*
       * Exclude infeasible points
       */
    for (k = 0; k < sp->np; k++) {
        VEC_DIFF_17(cm,&sp->rr(k,0),diff);
        dist = VEC_LEN_17(diff);
        if (dist < exclude || dist > maxdist) {
            sp->inuse[k] = FALSE;
            sp->nuse--;
        }
    }
    printf("%d sources after omitting infeasible sources.\n",sp->nuse);
    {
        std::vector<std::unique_ptr<MNESourceSpace>> sp_vec;
        sp_vec.push_back(std::move(sp));
        if (filter_source_spaces(surf,mindist,FiffCoordTrans(),sp_vec,NULL) != OK) {
            sp = std::move(sp_vec[0]);
            goto bad;
        }
        sp = std::move(sp_vec[0]);
    }
    printf("%d sources remaining after excluding the sources outside the surface and less than %6.1f mm inside.\n",sp->nuse,1000*mindist);
    /*
       * Omit unused vertices from the neighborhoods
       */
    printf("Adjusting the neighborhood info...");
    for (k = 0; k < sp->np; k++) {
        Eigen::VectorXi& neigh = sp->neighbor_vert[k];
        nneigh = sp->nneighbor_vert[k];
        if (sp->inuse[k]) {
            for (c = 0; c < nneigh; c++)
                if (!sp->inuse[neigh[c]])
                    neigh[c] = -1;
        }
        else {
            for (c = 0; c < nneigh; c++)
                neigh[c] = -1;
        }
    }
    printf("[done]\n");
    /*
     * Set up the volume data (needed for creating the interpolation matrix)
     */
    {
        float r0[3],voxel_size[3],x_ras[3],y_ras[3],z_ras[3];
        int   width,height,depth;

        r0[X_17] = minn[X_17]*grid;
        r0[Y_17] = minn[Y_17]*grid;
        r0[Z_17] = minn[Z_17]*grid;

        voxel_size[0] = grid;
        voxel_size[1] = grid;
        voxel_size[2] = grid;

        width  = (maxn[X_17]-minn[X_17]+1);
        height = (maxn[Y_17]-minn[Y_17]+1);
        depth  = (maxn[Z_17]-minn[Z_17]+1);

        for (k = 0; k < 3; k++)
            x_ras[k] = y_ras[k] = z_ras[k] = 0.0;

        x_ras[0] = 1.0;
        y_ras[1] = 1.0;
        z_ras[2] = 1.0;

        sp->voxel_surf_RAS_t = make_voxel_ras_trans(r0,x_ras,y_ras,z_ras,voxel_size);
        if (!sp->voxel_surf_RAS_t || sp->voxel_surf_RAS_t->isEmpty())
            goto bad;

        sp->vol_dims[0] = width;
        sp->vol_dims[1] = height;
        sp->vol_dims[2] = depth;
        VEC_COPY_17(sp->voxel_size,voxel_size);
    }

    return sp.release();

bad : {
        return NULL;
    }
}

//=============================================================================================================

int MNESourceSpace::filter_source_spaces(const MNESurface& surf, float limit, const FiffCoordTrans& mri_head_t, std::vector<std::unique_ptr<MNESourceSpace>>& spaces, QTextStream *filtered)   /* Provide a list of filtered points here */
/*
     * Remove all source space points closer to the surface than a given limit
     */
{
    MNESourceSpace* s;
    int k,p1,p2;
    float r1[3];
    float mindist,dist,diff[3];
    int   minnode;
    int   omit,omit_outside;
    double tot_angle;
    int nspace = static_cast<int>(spaces.size());

    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD && mri_head_t.isEmpty()) {
        printf("Source spaces are in head coordinates and no coordinate transform was provided!");
        return FAIL;
    }
    /*
        * How close are the source points to the surface?
        */
    printf("Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        printf("head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        printf("MRI coordinates.\n");
    else
        printf("unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    printf("Checking that the sources are inside the bounding surface ");
    if (limit > 0.0)
        printf("and at least %6.1f mm away",1000*limit);
    printf(" (will take a few...)\n");
    omit         = 0;
    omit_outside = 0;
    for (k = 0; k < nspace; k++) {
        s = spaces[k].get();
        for (p1 = 0; p1 < s->np; p1++)
            if (s->inuse[p1]) {
                VEC_COPY_17(r1,&s->rr(p1,0));	/* Transform the point to MRI coordinates */
                if (s->coord_frame == FIFFV_COORD_HEAD)
                    FiffCoordTrans::apply_inverse_trans(r1,mri_head_t,FIFFV_MOVE);
                /*
                * Check that the source is inside the inner skull surface
                */
                tot_angle = surf.sum_solids(Eigen::Map<const Eigen::Vector3f>(r1))/(4*M_PI);
                if (std::fabs(tot_angle-1.0) > 1e-5) {
                    omit_outside++;
                    s->inuse[p1] = FALSE;
                    s->nuse--;
                    if (filtered)
                        *filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                  << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
                }
                else if (limit > 0.0) {
                    /*
                        * Check the distance limit
                        */
                    mindist = 1.0;
                    minnode = 0;
                    for (p2 = 0; p2 < surf.np; p2++) {
                        VEC_DIFF_17(r1,&surf.rr(p2,0),diff);
                        dist = VEC_LEN_17(diff);
                        if (dist < mindist) {
                            mindist = dist;
                            minnode = p2;
                        }
                    }
                    if (mindist < limit) {
                        omit++;
                        s->inuse[p1] = FALSE;
                        s->nuse--;
                        if (filtered)
                            *filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                      << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
                    }
                }
            }
    }
    (void)minnode; // squash compiler warning, this is unused
    if (omit_outside > 0)
        printf("%d source space points omitted because they are outside the inner skull surface.\n",
               omit_outside);
    if (omit > 0)
        printf("%d source space points omitted because of the %6.1f-mm distance limit.\n",
               omit,1000*limit);
    printf("Thank you for waiting.\n");
    return OK;
}

//=============================================================================================================

void MNESourceSpace::filter_source_space(FilterThreadArg *arg)
{
    FilterThreadArg* a = arg;
    int    p1,p2;
    double tot_angle;
    int    omit,omit_outside;
    float  r1[3];
    float  mindist,dist,diff[3];
    int    minnode;

    QSharedPointer<MNESurface> surf = a->surf.toStrongRef();
    if (!surf) {
        a->stat = FAIL;
        return;
    }

    omit         = 0;
    omit_outside = 0;

    for (p1 = 0; p1 < a->s->np; p1++) {
        if (a->s->inuse[p1]) {
            VEC_COPY_17(r1,&a->s->rr(p1,0));	/* Transform the point to MRI coordinates */
            if (a->s->coord_frame == FIFFV_COORD_HEAD) {
                Q_ASSERT(a->mri_head_t);
                FiffCoordTrans::apply_inverse_trans(r1,*a->mri_head_t,FIFFV_MOVE);
            }
            /*
           * Check that the source is inside the inner skull surface
           */
            tot_angle = surf->sum_solids(Eigen::Map<const Eigen::Vector3f>(r1))/(4*M_PI);
            if (std::fabs(tot_angle-1.0) > 1e-5) {
                omit_outside++;
                a->s->inuse[p1] = FALSE;
                a->s->nuse--;
                if (a->filtered)
                    *a->filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                 << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
            }
            else if (a->limit > 0.0) {
                /*
         * Check the distance limit
         */
                mindist = 1.0;
                minnode = 0;
                for (p2 = 0; p2 < surf->np; p2++) {
                    VEC_DIFF_17(r1,&surf->rr(p2,0),diff);
                    dist = VEC_LEN_17(diff);
                    if (dist < mindist) {
                        mindist = dist;
                        minnode = p2;
                    }
                }
                if (mindist < a->limit) {
                    omit++;
                    a->s->inuse[p1] = FALSE;
                    a->s->nuse--;
                    if (a->filtered)
                        *a->filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                     << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
                }
            }
        }
    }
    (void)minnode; // squash compiler warning, set but unused
    if (omit_outside > 0)
        printf("%d source space points omitted because they are outside the inner skull surface.\n",
                omit_outside);
    if (omit > 0)
        printf("%d source space points omitted because of the %6.1f-mm distance limit.\n",
                omit,1000*a->limit);
    a->stat = OK;
    return;
}

//=============================================================================================================

int MNESourceSpace::filter_source_spaces(float limit, const QString& bemfile, const FiffCoordTrans& mri_head_t, std::vector<std::unique_ptr<MNESourceSpace>>& spaces, QTextStream *filtered, bool use_threads)
/*
          * Remove all source space points closer to the surface than a given limit
          */
{
    QSharedPointer<MNESurface> surf;
    int             k;
    int             nproc = QThread::idealThreadCount();
    FilterThreadArg* a;
    int nspace = static_cast<int>(spaces.size());

    if (bemfile.isEmpty())
        return OK;

    {
        MNESurface* rawSurf = MNESurface::read_bem_surface(bemfile,FIFFV_BEM_SURF_ID_BRAIN,FALSE,NULL);
        if (!rawSurf) {
            qCritical("BEM model does not have the inner skull triangulation!");
            return FAIL;
        }
        surf.reset(rawSurf);
    }
    /*
     * How close are the source points to the surface?
     */
    printf("Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        printf("head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        printf("MRI coordinates.\n");
    else
        printf("unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    printf("Checking that the sources are inside the inner skull ");
    if (limit > 0.0)
        printf("and at least %6.1f mm away",1000*limit);
    printf(" (will take a few...)\n");
    if (nproc < 2 || nspace == 1 || !use_threads) {
        /*
        * This is the conventional calculation
        */
        for (k = 0; k < nspace; k++) {
            a = new FilterThreadArg();
            a->s = spaces[k].get();
            a->mri_head_t = std::make_unique<FiffCoordTrans>(mri_head_t);
            a->surf = surf;
            a->limit = limit;
            a->filtered = filtered;
            filter_source_space(a);
            if(a)
                delete a;
            spaces[k]->rearrange_source_space();
        }
    }
    else {
        /*
        * Calculate all (both) source spaces simultaneously
        */
        QList<FilterThreadArg*> args;//filterThreadArg *args = MALLOC_17(nspace,filterThreadArg);

        for (k = 0; k < nspace; k++) {
            a = new FilterThreadArg();
            a->s = spaces[k].get();
            a->mri_head_t = std::make_unique<FiffCoordTrans>(mri_head_t);
            a->surf = surf;
            a->limit = limit;
            a->filtered = filtered;
            args.append(a);
        }
        /*
        * Ready to start the threads & Wait for them to complete
        */
        QtConcurrent::blockingMap(args, filter_source_space);

        for (k = 0; k < nspace; k++) {
            spaces[k]->rearrange_source_space();
            if(args[k])
                delete args[k];
        }
    }
    printf("Thank you for waiting.\n\n");

    return OK;
}

//=============================================================================================================

int MNESourceSpace::read_source_spaces(const QString &name, std::vector<std::unique_ptr<MNESourceSpace>>& spaces)
/*
 * Read source spaces from a FIFF file
 */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    std::vector<std::unique_ptr<MNESourceSpace>> local_spaces;
    std::unique_ptr<MNESourceSpace> new_space;
    QList<FiffDirNode::SPtr> sources;
    FiffDirNode::SPtr     node;
    FiffTag::UPtr t_pTag;
    int             j,k,p,q;
    int             ntri;
    int             *nearest = NULL;
    float           *nearest_dist = NULL;
    int             *nneighbors = NULL;
    int             *neighbors  = NULL;
    int             *vol_dims = NULL;

    if(!stream->open())
        goto bad;

    sources = stream->dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
    if (sources.size() == 0) {
        printf("No source spaces available here");
        goto bad;
    }
    for (j = 0; j < sources.size(); j++) {
        new_space = MNESourceSpace::create_source_space(0);
        node = sources[j];
        /*
            * Get the mandatory data first
            */
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag)) {
            goto bad;
        }
        new_space->np = *t_pTag->toInt();
        if (new_space->np == 0) {
            printf("No points in this source space");
            goto bad;
        }
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_POINTS, t_pTag)) {
            goto bad;
        }
        MatrixXf tmp_rr = t_pTag->toFloatMatrix().transpose();
        new_space->rr = tmp_rr;
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag)) {
            goto bad;
        }
        MatrixXf tmp_nn = t_pTag->toFloatMatrix().transpose();
        new_space->nn = tmp_nn;
        if (!node->find_tag(stream, FIFF_MNE_COORD_FRAME, t_pTag)) {
            new_space->coord_frame = FIFFV_COORD_MRI;
        }
        else {
            new_space->coord_frame = *t_pTag->toInt();
        }
        if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_ID, t_pTag)) {
            new_space->id = *t_pTag->toInt();
        }
        if (node->find_tag(stream, FIFF_SUBJ_HIS_ID, t_pTag)) {
            new_space->subject = (char *)t_pTag->data();
        }
        if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_TYPE, t_pTag)) {
            new_space->type = *t_pTag->toInt();
        }
        ntri = 0;
        if (node->find_tag(stream, FIFF_BEM_SURF_NTRI, t_pTag)) {
            ntri = *t_pTag->toInt();
        }
        else if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NTRI, t_pTag)) {
            ntri = *t_pTag->toInt();
        }
        if (ntri > 0) {

            if (!node->find_tag(stream, FIFF_BEM_SURF_TRIANGLES, t_pTag)) {
                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, t_pTag)) {
                    goto bad;
                }
            }

            MatrixXi tmp_itris = t_pTag->toIntMatrix().transpose();
            tmp_itris.array() -= 1;
            new_space->itris = tmp_itris;
            new_space->ntri = ntri;
        }
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE, t_pTag)) {
            if (new_space->type == FIFFV_MNE_SPACE_VOLUME) {
                /*
                    * Use all
                    */
                new_space->nuse   = new_space->np;
                new_space->inuse  = Eigen::VectorXi::Ones(new_space->nuse);
                new_space->vertno = Eigen::VectorXi::LinSpaced(new_space->nuse, 0, new_space->nuse - 1);
            }
            else {
                /*
                    * None in use
                    * NOTE: The consequences of this change have to be evaluated carefully
                    */
                new_space->nuse   = 0;
                new_space->inuse  = Eigen::VectorXi::Zero(new_space->np);
                new_space->vertno.resize(0);
            }
        }
        else {
            new_space->nuse = *t_pTag->toInt();
            if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_SELECTION, t_pTag)) {
                goto bad;
            }

            qDebug() << "ToDo: Check whether new_space->inuse contains the right stuff!!! - use VectorXi instead";
            new_space->inuse = Eigen::VectorXi::Zero(new_space->np);
            if (new_space->nuse > 0) {
                new_space->vertno = Eigen::VectorXi::Zero(new_space->nuse);
                for (k = 0, p = 0; k < new_space->np; k++) {
                    new_space->inuse[k] = t_pTag->toInt()[k]; //DEBUG
                    if (new_space->inuse[k])
                        new_space->vertno[p++] = k;
                }
            }
            else {
                new_space->vertno.resize(0);
            }
            /*
                * Selection triangulation
                */
            ntri = 0;
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE_TRI, t_pTag)) {
                ntri = *t_pTag->toInt();
            }
            if (ntri > 0) {

                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, t_pTag)) {
                    goto bad;
                }

                MatrixXi tmp_itris = t_pTag->toIntMatrix().transpose();
                tmp_itris.array() -= 1;
                new_space->use_itris = tmp_itris;
                new_space->nuse_tri = ntri;
            }
            /*
                * The patch information becomes relevant here
                */
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST, t_pTag)) {
                nearest  = t_pTag->toInt();
                new_space->nearest.resize(new_space->np);
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].vert = k;
                    new_space->nearest[k].nearest = nearest[k];
                    new_space->nearest[k].patch = nullptr;
                }

                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, t_pTag)) {
                    goto bad;
                }
                qDebug() << "ToDo: Check whether nearest_dist contains the right stuff!!! - use VectorXf instead";
                nearest_dist = t_pTag->toFloat();
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].dist = nearest_dist[k];
                }
                //                FREE_17(nearest); nearest = NULL;
                //                FREE_17(nearest_dist); nearest_dist = NULL;
            }
            /*
            * We may have the distance matrix
            */
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, t_pTag)) {
                new_space->dist_limit = *t_pTag->toFloat();
                if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST, t_pTag)) {
                    //                    SparseMatrix<double> tmpSparse = t_pTag->toSparseFloatMatrix();
                    auto dist_lower = FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag);
                    if (!dist_lower) {
                        goto bad;
                    }
                    auto dist_full = dist_lower->mne_add_upper_triangle_rcs();
                    if (!dist_full) {
                        goto bad;
                    }
                    new_space->dist = std::move(*dist_full);
                }
                else
                    new_space->dist_limit = 0.0;
            }
        }
        /*
            * For volume source spaces we might have the neighborhood information
            */
        if (new_space->type == FIFFV_MNE_SPACE_VOLUME) {
            int ntot,nvert,ntot_count,nneigh;
            int *neigh;

            nneighbors = neighbors = NULL;
            ntot = nvert = 0;
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEIGHBORS, t_pTag)) {
                qDebug() << "ToDo: Check whether neighbors contains the right stuff!!! - use VectorXi instead";
                neighbors = t_pTag->toInt();
                ntot      = t_pTag->size()/sizeof(fiff_int_t);
            }
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NNEIGHBORS, t_pTag)) {
                qDebug() << "ToDo: Check whether nneighbors contains the right stuff!!! - use VectorXi instead";
                nneighbors = t_pTag->toInt();
                nvert      = t_pTag->size()/sizeof(fiff_int_t);
            }
            if (neighbors && nneighbors) {
                if (nvert != new_space->np) {
                    printf("Inconsistent neighborhood data in file.");
                    goto bad;
                }
                for (k = 0, ntot_count = 0; k < nvert; k++)
                    ntot_count += nneighbors[k];
                if (ntot_count != ntot) {
                    printf("Inconsistent neighborhood data in file.");
                    goto bad;
                }
                new_space->nneighbor_vert = Eigen::VectorXi::Zero(nvert);
                new_space->neighbor_vert.resize(nvert);
                for (k = 0, q = 0; k < nvert; k++) {
                    new_space->nneighbor_vert[k] = nneigh = nneighbors[k];
                    new_space->neighbor_vert[k] = Eigen::VectorXi(nneigh);
                    for (p = 0; p < nneigh; p++,q++)
                        new_space->neighbor_vert[k][p] = neighbors[q];
                }
            }
            delete[] neighbors;
            delete[] nneighbors;
            nneighbors = neighbors = NULL;
            /*
                * There might be a coordinate transformation and dimensions
                */
            new_space->voxel_surf_RAS_t   = FiffCoordTrans(FiffCoordTrans::readTransformFromNode(stream, node, FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS));
            if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS, t_pTag)) {
                qDebug() << "ToDo: Check whether vol_dims contains the right stuff!!! - use VectorXi instead";
                vol_dims = t_pTag->toInt();
            }
            if (vol_dims)
                VEC_COPY_17(new_space->vol_dims,vol_dims);
            {
                QList<FiffDirNode::SPtr>  mris = node->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);

                if (mris.size() == 0) { /* The old way */
                    new_space->MRI_surf_RAS_RAS_t = FiffCoordTrans(FiffCoordTrans::readTransformFromNode(stream, node, FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS));
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_MRI_FILE, t_pTag)) {
                        qDebug() << "ToDo: Check whether new_space->MRI_volume  contains the right stuff!!! - use QString instead";
                        new_space->MRI_volume = (char *)t_pTag->data();
                    }
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator = std::move(*FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag));
                    }
                }
                else {
                    if (node->find_tag(stream, FIFF_MNE_FILE_NAME, t_pTag)) {
                        new_space->MRI_volume = (char *)t_pTag->data();
                    }
                    new_space->MRI_surf_RAS_RAS_t = FiffCoordTrans(FiffCoordTrans::readTransformFromNode(stream, mris[0], FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS));
                    new_space->MRI_voxel_surf_RAS_t   = FiffCoordTrans(FiffCoordTrans::readTransformFromNode(stream, mris[0], FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS));

                    if (mris[0]->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator = std::move(*FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag));
                    }
                    if (mris[0]->find_tag(stream, FIFF_MRI_WIDTH, t_pTag)) {
                        new_space->MRI_vol_dims[0] = *t_pTag->toInt();
                    }
                    if (mris[0]->find_tag(stream, FIFF_MRI_HEIGHT, t_pTag)) {
                        new_space->MRI_vol_dims[1] = *t_pTag->toInt();
                    }
                    if (mris[0]->find_tag(stream, FIFF_MRI_DEPTH, t_pTag)) {
                        new_space->MRI_vol_dims[2] = *t_pTag->toInt();
                    }
                }
            }
        }
        new_space->add_triangle_data();
        local_spaces.push_back(std::move(new_space));
    }
    stream->close();

     spaces = std::move(local_spaces);

    return FIFF_OK;

bad : {
        stream->close();
        // new_space and local_spaces auto-cleanup via unique_ptr
        delete[] nearest;
        delete[] nearest_dist;
        delete[] neighbors;
        delete[] nneighbors;
        delete[] vol_dims;

        return FIFF_FAIL;
    }
}

//=============================================================================================================

int MNESourceSpace::transform_source_spaces_to(int coord_frame, const FiffCoordTrans& t, std::vector<std::unique_ptr<MNESourceSpace>>& spaces)
/*
 * Facilitate the transformation of the source spaces
 */
{
    MNESourceSpace* s;
    int k;
    int nspace = static_cast<int>(spaces.size());

    for (k = 0; k < nspace; k++) {
        s = spaces[k].get();
        if (s->coord_frame != coord_frame) {
            if (!t.isEmpty()) {
                if (s->coord_frame == t.from && t.to == coord_frame) {
                    if (s->transform_source_space(t) != OK)
                        return FAIL;
                }
                else if (s->coord_frame == t.to && t.from == coord_frame) {
                    FiffCoordTrans my_t = t.inverted();
                    if (s->transform_source_space(my_t) != OK) {
                        return FAIL;
                    }
                }
                else {
                    printf("Could not transform a source space because of transformation incompatibility.");
                    return FAIL;
                }
            }
            else {
                printf("Could not transform a source space because of missing coordinate transformation.");
                return FAIL;
            }
        }
    }
    return OK;
}

//=============================================================================================================

#define LH_LABEL_TAG "-lh.label"
#define RH_LABEL_TAG "-rh.label"

int MNESourceSpace::restrict_sources_to_labels(std::vector<std::unique_ptr<MNESourceSpace>>& spaces, const QStringList& labels, int nlabel)
/*
 * Pick only sources within a label
 */
{
    MNESourceSpace* lh = NULL;
    MNESourceSpace* rh = NULL;
    MNESourceSpace* sp;
    Eigen::VectorXi lh_inuse;
    Eigen::VectorXi rh_inuse;
    Eigen::VectorXi sel;
    Eigen::VectorXi *inuse = nullptr;
    int            k,p;
    int nspace = static_cast<int>(spaces.size());

    if (nlabel == 0)
        return OK;

    for (k = 0; k < nspace; k++) {
        if (spaces[k]->is_left_hemi()) {
            lh = spaces[k].get();
            lh_inuse = Eigen::VectorXi::Zero(lh->np);
        }
        else {
            rh = spaces[k].get();
            rh_inuse = Eigen::VectorXi::Zero(rh->np);
        }
    }
    /*
       * Go through each label file
       */
    for (k = 0; k < nlabel; k++) {
        /*
         * Which hemi?
         */
        if (labels[k].contains(LH_LABEL_TAG)){ //strstr(labels[k],LH_LABEL_TAG) != NULL) {
            sp = lh;
            inuse = &lh_inuse;
        }
        else if (labels[k].contains(RH_LABEL_TAG)){ //strstr(labels[k],RH_LABEL_TAG) != NULL) {
            sp = rh;
            inuse = &rh_inuse;
        }
        else {
            printf("\tWarning: cannot assign label file %s to a hemisphere.\n",labels[k].toUtf8().constData());
            continue;
        }
        if (sp) {
            if (read_label(labels[k],sel) == FAIL)
                goto bad;
            for (p = 0; p < sel.size(); p++) {
                if (sel[p] >= 0 && sel[p] < sp->np)
                    (*inuse)[sel[p]] = sp->inuse[sel[p]];
                else
                    printf("vertex number out of range in %s (%d vs %d)\n",
                           labels[k].toUtf8().constData(),sel[p],sp->np);
            }
            printf("Processed label file %s\n",labels[k].toUtf8().constData());
        }
    }
    if (lh) lh->update_inuse(std::move(lh_inuse));
    if (rh) rh->update_inuse(std::move(rh_inuse));
    return OK;

bad : {
        return FAIL;
    }
}

//=============================================================================================================

int MNESourceSpace::read_label(const QString& label, Eigen::VectorXi& sel)
/*
          * Find the source points within a label
          */
{
    int  res = FAIL;

    int k,p,nlabel;
    char c;
    float fdum;
    /*
       * Read the label file
       */
    QFile inFile(label);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << label;//err_set_sys_error(label);
        goto out;
    }
    inFile.getChar(&c);
    if (c !='#') {
        qCritical("FsLabel file does not start correctly.");
        goto out;
    }
    /*
       * Skip the comment line
       */
    while (inFile.getChar(&c) && c != '\n')
        ;
    {
    QTextStream in(&inFile);
    in >> nlabel;
    if (in.status() != QTextStream::Ok) {
        qCritical("Could not read the number of labelled points.");
        goto out;
    }
    sel.resize(nlabel);
    for (k = 0; k < nlabel; k++) {
        in >> p >> fdum >> fdum >> fdum >> fdum;
        if (in.status() != QTextStream::Ok) {
            qCritical("Could not read label point # %d",k+1);
            goto out;
        }
        sel[k] = p;
    }
    res = OK;
    }

out : {
        if (res != OK) {
            sel.resize(0);
        }
        return res;
    }
}

//=============================================================================================================

int MNESourceSpace::writeVolumeInfo(FiffStream::SPtr& stream, bool selected_only) const
{
    int ntot,nvert;
    int nneigh;
    int k,p;

    if (type != FIFFV_MNE_SPACE_VOLUME)
        return OK;
    if (neighbor_vert.empty() || nneighbor_vert.size() == 0)
        return OK;

    Eigen::VectorXi nneighbors;
    Eigen::VectorXi neighbors;

    if (selected_only) {
        Eigen::VectorXi inuse_map = Eigen::VectorXi::Constant(np, -1);
        for (k = 0,p = 0, ntot = 0; k < np; k++) {
            if (inuse[k]) {
                ntot += nneighbor_vert[k];
                inuse_map[k] = p++;
            }
        }
        nneighbors.resize(nuse);
        neighbors.resize(ntot);
        for (k = 0, nvert = 0, ntot = 0; k < np; k++) {
            if (inuse[k]) {
                const Eigen::VectorXi& neigh = neighbor_vert[k];
                nneigh = nneighbor_vert[k];
                nneighbors[nvert++] = nneigh;
                for (p = 0; p < nneigh; p++)
                    neighbors[ntot++] = neigh[p] < 0 ? -1 : inuse_map[neigh[p]];
            }
        }
    }
    else {
        for (k = 0, ntot = 0; k < np; k++)
            ntot += nneighbor_vert[k];
        nneighbors.resize(np);
        neighbors.resize(ntot);
        nvert     = np;
        for (k = 0, ntot = 0; k < np; k++) {
            const Eigen::VectorXi& neigh = neighbor_vert[k];
            nneigh = nneighbor_vert[k];
            nneighbors[k] = nneigh;
            for (p = 0; p < nneigh; p++)
                neighbors[ntot++] = neigh[p];
        }
    }

    stream->write_int(FIFF_MNE_SOURCE_SPACE_NNEIGHBORS,nneighbors.data(),nvert);
    stream->write_int(FIFF_MNE_SOURCE_SPACE_NEIGHBORS,neighbors.data(),ntot);

    if (!selected_only) {
        if (voxel_surf_RAS_t && !voxel_surf_RAS_t->isEmpty()) {
            stream->write_coord_trans(*voxel_surf_RAS_t);
            stream->write_int(FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS,vol_dims,3);
        }
        if (interpolator && !MRI_volume.isEmpty()) {
            stream->start_block(FIFFB_MNE_PARENT_MRI_FILE);
            if (MRI_surf_RAS_RAS_t && !MRI_surf_RAS_RAS_t->isEmpty())
                stream->write_coord_trans(*MRI_surf_RAS_RAS_t);
            if (MRI_voxel_surf_RAS_t && !MRI_voxel_surf_RAS_t->isEmpty())
                stream->write_coord_trans(*MRI_voxel_surf_RAS_t);
            stream->write_string(FIFF_MNE_FILE_NAME,MRI_volume);
            if (interpolator)
                stream->write_float_sparse_rcs(FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, interpolator->toEigenSparse().cast<float>());
            if (MRI_vol_dims[0] > 0 && MRI_vol_dims[1] > 0 && MRI_vol_dims[2] > 0) {
                stream->write_int(FIFF_MRI_WIDTH,&MRI_vol_dims[0]);
                stream->write_int(FIFF_MRI_HEIGHT,&MRI_vol_dims[1]);
                stream->write_int(FIFF_MRI_DEPTH,&MRI_vol_dims[2]);
            }
            stream->end_block(FIFFB_MNE_PARENT_MRI_FILE);
        }
    }
    else {
        if (interpolator && !MRI_volume.isEmpty()) {
            stream->write_string(FIFF_MNE_SOURCE_SPACE_MRI_FILE,MRI_volume);
            qCritical("Cannot write the interpolator for selection yet");
            return FAIL;
        }
    }
    return OK;
}

//=============================================================================================================

int MNESourceSpace::writeToStream(FiffStream::SPtr& stream, bool selected_only) const
{
    int p, pp;

    if (np <= 0) {
        qCritical("No points in the source space being saved");
        return FIFF_FAIL;
    }

    stream->start_block(FIFFB_MNE_SOURCE_SPACE);

    if (type != FIFFV_MNE_SPACE_UNKNOWN)
        stream->write_int(FIFF_MNE_SOURCE_SPACE_TYPE, &type);
    if (id != FIFFV_MNE_SURF_UNKNOWN)
        stream->write_int(FIFF_MNE_SOURCE_SPACE_ID, &id);
    if (!subject.isEmpty() && subject.size() > 0) {
        QString subj(subject);
        stream->write_string(FIFF_SUBJ_HIS_ID, subj);
    }

    stream->write_int(FIFF_MNE_COORD_FRAME, &coord_frame);

    if (selected_only) {
        if (nuse == 0) {
            qCritical("No vertices in use. Cannot write active-only vertices from this source space");
            return FIFF_FAIL;
        }

        Eigen::MatrixXf sel(nuse, 3);
        stream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &nuse);

        for (p = 0, pp = 0; p < np; p++) {
            if (inuse[p]) {
                sel.row(pp) = rr.row(p);
                pp++;
            }
        }
        stream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_POINTS, sel);

        for (p = 0, pp = 0; p < np; p++) {
            if (inuse[p]) {
                sel.row(pp) = nn.row(p);
                pp++;
            }
        }
        stream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_NORMALS, sel);
    }
    else {
        stream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &np);
        stream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_POINTS, Eigen::MatrixXf(rr));
        stream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_NORMALS, Eigen::MatrixXf(nn));

        if (nuse > 0 && inuse.size() > 0) {
            stream->write_int(FIFF_MNE_SOURCE_SPACE_SELECTION, inuse.data(), np);
            stream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE, &nuse);
        }

        if (ntri > 0) {
            stream->write_int(FIFF_MNE_SOURCE_SPACE_NTRI, &ntri);
            Eigen::MatrixXi file_tris = itris.array() + 1;
            stream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_TRIANGLES, file_tris);
        }

        if (nuse_tri > 0) {
            stream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE_TRI, &nuse_tri);
            Eigen::MatrixXi file_use_tris = use_itris.array() + 1;
            stream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, file_use_tris);
        }

        if (!nearest.empty()) {
            Eigen::VectorXi nearest_v(np);
            Eigen::VectorXf nearest_dist_v(np);

            std::sort(const_cast<std::vector<MNENearest>&>(nearest).begin(),
                      const_cast<std::vector<MNENearest>&>(nearest).end(),
                      [](const MNENearest& a, const MNENearest& b) { return a.vert < b.vert; });
            for (p = 0; p < np; p++) {
                nearest_v[p] = nearest[p].nearest;
                nearest_dist_v[p] = nearest[p].dist;
            }

            stream->write_int(FIFF_MNE_SOURCE_SPACE_NEAREST, nearest_v.data(), np);
            stream->write_float(FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, nearest_dist_v.data(), np);
        }

        if (!dist.is_empty()) {
            auto m = dist.pickLowerTriangleRcs();
            if (!m)
                return FIFF_FAIL;
            stream->write_float_sparse_rcs(FIFF_MNE_SOURCE_SPACE_DIST, m->toEigenSparse().cast<float>());
            stream->write_float(FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, &dist_limit);
        }
    }

    if (writeVolumeInfo(stream, selected_only) != OK)
        return FIFF_FAIL;

    stream->end_block(FIFFB_MNE_SOURCE_SPACE);
    return FIFF_OK;
}

