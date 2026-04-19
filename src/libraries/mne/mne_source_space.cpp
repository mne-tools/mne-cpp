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
#include "mne_hemisphere.h"
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
#include <QDebug>

#include <cstring>
#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

using FIFFLIB::FiffCoordTrans;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

constexpr int FAIL = -1;
constexpr int OK   =  0;

constexpr int NNEIGHBORS = 26;

constexpr int CURVATURE_FILE_MAGIC_NUMBER = 16777215;

constexpr int TAG_OLD_MGH_XFORM     = 30;
constexpr int TAG_OLD_COLORTABLE   = 1;
constexpr int TAG_OLD_USEREALRAS   = 2;
constexpr int TAG_USEREALRAS      = 4;

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
        qCritical("Illegal vertex number %d (max %d).",no,maxno);
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

std::unique_ptr<MNEVolGeom> read_vol_geom(QFile &fp)
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

    auto vg = std::make_unique<MNEVolGeom>();

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
                vg->filename = QString::fromUtf8(buf);
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
        vg = std::make_unique<MNEVolGeom>();
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
    size_t snbytes = nbytes;

     val = nullptr;
    if (nbytes > 0) {
        auto dum = std::make_unique<unsigned char[]>(nbytes+1);
        if (fp.read(reinterpret_cast<char*>(dum.get()), nbytes) != static_cast<qint64>(snbytes)) {
            qCritical("Failed to read %d bytes of tag data",static_cast<int>(nbytes));
            return FAIL;
        }
        dum[nbytes] = '\0'; /* Ensure null termination */
        val     = dum.release();
        nbytesp = nbytes;
    }
    else {			/* Need to handle special cases */
        if (tag == TAG_OLD_SURF_GEOM) {
            auto g = read_vol_geom(fp);
            if (!g)
                return FAIL;
            /*
             * Serialize MNEVolGeom as POD fields followed by a
             * null-terminated UTF-8 filename.  We must NOT memcpy the
             * entire MNEVolGeom because it contains a QString member
             * whose internal pointers become dangling when *g is
             * destroyed at the end of this block.
             */
            struct VolGeomPOD {
                int   valid;
                int   width, height, depth;
                float xsize, ysize, zsize;
                float x_ras[3], y_ras[3], z_ras[3];
                float c_ras[3];
            };
            QByteArray fn = g->filename.toUtf8();
            size_t totalSize = sizeof(VolGeomPOD) + fn.size() + 1;
            auto buf = std::make_unique<unsigned char[]>(totalSize);
            VolGeomPOD pod;
            pod.valid  = g->valid;
            pod.width  = g->width;  pod.height = g->height; pod.depth = g->depth;
            pod.xsize  = g->xsize;  pod.ysize  = g->ysize;  pod.zsize = g->zsize;
            std::memcpy(pod.x_ras, g->x_ras, 3 * sizeof(float));
            std::memcpy(pod.y_ras, g->y_ras, 3 * sizeof(float));
            std::memcpy(pod.z_ras, g->z_ras, 3 * sizeof(float));
            std::memcpy(pod.c_ras, g->c_ras, 3 * sizeof(float));
            std::memcpy(buf.get(), &pod, sizeof(VolGeomPOD));
            std::memcpy(buf.get() + sizeof(VolGeomPOD), fn.constData(), fn.size() + 1);
            val     = buf.release();
            nbytesp = static_cast<long long>(totalSize);
        }
        else if (tag == TAG_OLD_USEREALRAS || tag == TAG_USEREALRAS) {
            auto vi = std::make_unique<int[]>(1);
            if (read_int(fp, vi[0]) == FAIL)
                return FAIL;
            val = reinterpret_cast<unsigned char *>(vi.release());
            nbytesp = sizeof(int);
        }
        else {
            qWarning("Encountered an unknown tag with no length specification : %d\n",tag);
            val     = nullptr;
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
        curv.resize(0); return FAIL;
    }
    if (read_int3(fp,magic) != 0) {
        qCritical() << "Bad magic in" << fname;
        curv.resize(0); return FAIL;
    }
    if (magic == CURVATURE_FILE_MAGIC_NUMBER) {	    /* A new-style curvature file */
        /*
 * How many and faces
 */
        if (read_int(fp,ncurv) != 0) {
            curv.resize(0); return FAIL;
        }
        if (read_int(fp,nface) != 0) {
            curv.resize(0); return FAIL;
        }
#ifdef DEBUG
        qInfo("nvert = %d nface = %d\n",ncurv,nface);
#endif
        if (read_int(fp,val_pervert) != 0) {
            curv.resize(0); return FAIL;
        }
        if (val_pervert != 1) {
            qCritical("Values per vertex not equal to one.");
            curv.resize(0); return FAIL;
        }
        /*
 * Read the curvature values
 */
        curv.resize(ncurv);
        curvmin = curvmax = 0.0;
        for (k = 0; k < ncurv; k++) {
            if (read_float(fp,fval) != 0) {
                curv.resize(0); return FAIL;
            }
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
        if (read_int3(fp,nface) != 0) {
            curv.resize(0); return FAIL;
        }
#ifdef DEBUG
        qInfo("nvert = %d nface = %d\n",ncurv,nface);
#endif
        /*
 * Read the curvature values
 */
        curv.resize(ncurv);
        curvmin = curvmax = 0.0;
        for (k = 0; k < ncurv; k++) {
            if (read_int2(fp,val) != 0) {
                curv.resize(0); return FAIL;
            }
            curv[k] = static_cast<float>(val)/100.0;
            if (curv[k] > curvmax)
                curvmax = curv[k];
            if (curv[k] < curvmin)
                curvmin = curv[k];

        }
    }
#ifdef DEBUG
    qInfo("Curvature range: %f...%f\n",curvmin,curvmax);
#endif
    return OK;
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
        return FAIL;
    }
    if (read_int3(fp,magic) != 0) {
        qCritical() << "Bad magic in" << fname;
        return FAIL;
    }
    if (magic != TRIANGLE_FILE_MAGIC_NUMBER &&
            magic != QUAD_FILE_MAGIC_NUMBER &&
            magic != NEW_QUAD_FILE_MAGIC_NUMBER) {
        qCritical() << "Bad magic in" << fname;
        return FAIL;
    }
    if (magic == TRIANGLE_FILE_MAGIC_NUMBER) {
        /*
     * Get the comment
     */
        qInfo("Triangle file : ");
        for (fp.getChar(&c); c != '\n'; fp.getChar(&c)) {
            if (fp.atEnd()) {
                qCritical()<<"Bad triangle file.";
                return FAIL;
            }
            putc(c,stderr);
        }
        fp.getChar(&c);
        /*
     * How many vertices and triangles?
     */
        if (read_int(fp,nvert) != 0)
            return FAIL;
        if (read_int(fp,ntri) != 0)
            return FAIL;
        qInfo(" nvert = %d ntri = %d\n",nvert,ntri);
        vert.resize(nvert, 3);
        tri.resize(ntri, 3);
        /*
     * Read the vertices
     */
        for (k = 0; k < nvert; k++) {
            if (read_float(fp,vert(k,0)) != 0)
                return FAIL;
            if (read_float(fp,vert(k,1)) != 0)
                return FAIL;
            if (read_float(fp,vert(k,2)) != 0)
                return FAIL;
        }
        /*
     * Read the triangles
     */
        for (k = 0; k < ntri; k++) {
            if (read_int(fp,tri(k,0)) != 0)
                return FAIL;
            if (check_vertex(tri(k,0),nvert) != OK)
                return FAIL;
            if (read_int(fp,tri(k,1)) != 0)
                return FAIL;
            if (check_vertex(tri(k,1),nvert) != OK)
                return FAIL;
            if (read_int(fp,tri(k,2)) != 0)
                return FAIL;
            if (check_vertex(tri(k,2),nvert) != OK)
                return FAIL;
        }
    }
    else if (magic == QUAD_FILE_MAGIC_NUMBER ||
             magic == NEW_QUAD_FILE_MAGIC_NUMBER) {
        if (read_int3(fp,nvert) != 0)
            return FAIL;
        if (read_int3(fp,nquad) != 0)
            return FAIL;
        qInfo("%s file : nvert = %d nquad = %d\n",
                magic == QUAD_FILE_MAGIC_NUMBER ? "Quad" : "New quad",
                nvert,nquad);
        vert.resize(nvert, 3);
        if (magic == QUAD_FILE_MAGIC_NUMBER) {
            for (k = 0; k < nvert; k++) {
                if (read_int2(fp,val) != 0)
                    return FAIL;
                vert(k,0) = val/100.0;
                if (read_int2(fp,val) != 0)
                    return FAIL;
                vert(k,1) = val/100.0;
                if (read_int2(fp,val) != 0)
                    return FAIL;
                vert(k,2) = val/100.0;
            }
        }
        else {			/* NEW_QUAD_FILE_MAGIC_NUMBER */
            for (k = 0; k < nvert; k++) {
                if (read_float(fp,vert(k,0)) != 0)
                    return FAIL;
                if (read_float(fp,vert(k,1)) != 0)
                    return FAIL;
                if (read_float(fp,vert(k,2)) != 0)
                    return FAIL;
            }
        }
        ntri = 2*nquad;
        tri.resize(ntri, 3);
        for (k = 0, ntri = 0; k < nquad; k++) {
            for (p = 0; p < 4; p++) {
                if (read_int3(fp,quad[p]) != 0)
                    return FAIL;
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
    qInfo("%f ",sqrt(1.9*quad[0]) + sqrt(3.5*quad[1]));
     */

            if (EVEN(which)) {
                tri(ntri,0) = quad[0];
                tri(ntri,1) = quad[1];
                tri(ntri,2) = quad[3];
                ntri++;

                tri(ntri,0) = quad[2];
                tri(ntri,1) = quad[3];
                tri(ntri,2) = quad[1];
                ntri++;
            }
            else {
                tri(ntri,0) = quad[0];
                tri(ntri,1) = quad[1];
                tri(ntri,2) = quad[2];
                ntri++;

                tri(ntri,0) = quad[0];
                tri(ntri,1) = quad[2];
                tri(ntri,2) = quad[3];
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
            return FAIL;
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
}

//=========================================================================
// get_volume_geom_from_tag
//=========================================================================

std::optional<MNEVolGeom> get_volume_geom_from_tag(const MNEMghTagGroup *tagsp)
{
    if (!tagsp)
        return std::nullopt;

    struct VolGeomPOD {
        int   valid;
        int   width, height, depth;
        float xsize, ysize, zsize;
        float x_ras[3], y_ras[3], z_ras[3];
        float c_ras[3];
    };

    for (const auto &t : tagsp->tags) {
        if (t->tag == TAG_OLD_SURF_GEOM) {
            if (t->len < static_cast<long long>(sizeof(VolGeomPOD)))
                return std::nullopt;

            const unsigned char *d = reinterpret_cast<const unsigned char *>(t->data.constData());
            VolGeomPOD pod;
            std::memcpy(&pod, d, sizeof(VolGeomPOD));

            MNEVolGeom result;
            result.valid  = pod.valid;
            result.width  = pod.width;  result.height = pod.height; result.depth = pod.depth;
            result.xsize  = pod.xsize;  result.ysize  = pod.ysize;  result.zsize = pod.zsize;
            std::memcpy(result.x_ras, pod.x_ras, 3 * sizeof(float));
            std::memcpy(result.y_ras, pod.y_ras, 3 * sizeof(float));
            std::memcpy(result.z_ras, pod.z_ras, 3 * sizeof(float));
            std::memcpy(result.c_ras, pod.c_ras, 3 * sizeof(float));

            if (t->len > static_cast<long long>(sizeof(VolGeomPOD)))
                result.filename = QString::fromUtf8(
                    reinterpret_cast<const char *>(d + sizeof(VolGeomPOD)));

            return result;
        }
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

    cm[0] = cm[1] = cm[2] = 0.0;
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
        inuse[k] = 1;
    nuse = np;
    return;
}

//=============================================================================================================

bool MNESourceSpace::is_left_hemi() const
/*
 * Left or right hemisphere?
 */
{
    int k;
    float xave;

    for (k = 0, xave = 0.0; k < np; k++)
        xave += rr(k,0);
    if (xave < 0.0)
        return true;
    else
        return false;
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
        qCritical("Coordinate transformation does not match with the source space coordinate system.");
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

    qInfo("Computing patch statistics...\n");
    if (neighbor_tri.empty())
        if (add_geometry_info(false) != OK)
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
    qInfo("\tareas, average normals, and mean deviations...");
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
            if (q < nuse && vertno[q] == nearest_data[p-1].nearest) { /* Some source space points may have been omitted since
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
    if (q < nuse && vertno[q] == nearest_data[p-1].nearest) {
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
    qInfo(" %d/%d [done]\n",q,nuse);

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
    return load_surface_geom(surf_file,curv_file,true,true);
}

//=============================================================================================================

std::unique_ptr<MNESourceSpace> MNESourceSpace::load_surface_geom(const QString& surf_file,
                                                                   const QString& curv_file,
                                                                   bool add_geometry,
                                                                   bool check_too_many_neighbors)
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
        return nullptr;

    if (!curv_file.isEmpty()) {
        if (read_curvature_file(curv_file, curvs) == -1)
            return nullptr;
        if (curvs.size() != verts.rows()) {
            qCritical()<<"Incorrect number of vertices in the curvature file.";
            return nullptr;
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
            if (s->add_geometry_info(true) != OK)
                return nullptr;
        }
        else {
            if (s->add_geometry_info2(true) != OK)
                return nullptr;
        }
    }
    else if (s->nn.rows() == 0) {			/* Normals only */
        if (s->add_vertex_normals() != OK)
            return nullptr;
    }
    else
        s->add_triangle_data();
    s->nuse   = s->np;
    s->inuse  = Eigen::VectorXi::Ones(s->np);
    s->vertno = Eigen::VectorXi::LinSpaced(s->np, 0, s->np - 1);
    s->mgh_tags = std::move(tags);
    s->vol_geom = get_volume_geom_from_tag(s->mgh_tags ? &(*s->mgh_tags) : nullptr);

    return s;
}

//=============================================================================================================

static std::optional<FiffCoordTrans> make_voxel_ras_trans(const Eigen::Vector3f& r0,
                                                  const Eigen::Vector3f& x_ras,
                                                  const Eigen::Vector3f& y_ras,
                                                  const Eigen::Vector3f& z_ras,
                                                  const Eigen::Vector3f& voxel_size)
{
    Eigen::Matrix3f rot;
    rot.row(0) = x_ras.transpose() * voxel_size[0];
    rot.row(1) = y_ras.transpose() * voxel_size[1];
    rot.row(2) = z_ras.transpose() * voxel_size[2];

    return FiffCoordTrans(FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_COORD_MRI, rot, r0);
}

MNESourceSpace* MNESourceSpace::make_volume_source_space(const MNESurface& surf, float grid, float exclude, float mindist)
/*
     * Make a source space which covers the volume bounded by surf
     */
{
    Eigen::Vector3f minV, maxV, cm;
    int   minn[3],maxn[3];
    float maxdist,dist;
    int   k,c;
    std::unique_ptr<MNESourceSpace> sp;
    int np,nplane,nrow;
    int nneigh;
    int x,y,z;
    /*
        * Figure out the grid size
        */
    cm.setZero();
    minV = maxV = surf.rr.row(0).transpose();

    for (k = 0; k < surf.np; k++) {
        Eigen::Vector3f node = surf.rr.row(k).transpose();
        cm += node;
        minV = minV.cwiseMin(node);
        maxV = maxV.cwiseMax(node);
    }
    cm /= static_cast<float>(surf.np);
    /*
       * Define the sphere which fits the surface
       */
    maxdist = 0.0;
    for (k = 0; k < surf.np; k++) {
        dist = (surf.rr.row(k).transpose() - cm).norm();
        if (dist > maxdist)
            maxdist = dist;
    }
    qInfo("FsSurface CM = (%6.1f %6.1f %6.1f) mm\n",
           1000*cm[X], 1000*cm[Y], 1000*cm[Z]);
    qInfo("FsSurface fits inside a sphere with radius %6.1f mm\n",1000*maxdist);
    qInfo("FsSurface extent:\n"
           "\tx = %6.1f ... %6.1f mm\n"
           "\ty = %6.1f ... %6.1f mm\n"
           "\tz = %6.1f ... %6.1f mm\n",
           1000*minV[X],1000*maxV[X],
           1000*minV[Y],1000*maxV[Y],
           1000*minV[Z],1000*maxV[Z]);
    for (c = 0; c < 3; c++) {
        if (maxV[c] > 0)
            maxn[c] = floor(std::fabs(maxV[c])/grid)+1;
        else
            maxn[c] = -floor(std::fabs(maxV[c])/grid)-1;
        if (minV[c] > 0)
            minn[c] = floor(std::fabs(minV[c])/grid)+1;
        else
            minn[c] = -floor(std::fabs(minV[c])/grid)-1;
    }
    qInfo("Grid extent:\n"
           "\tx = %6.1f ... %6.1f mm\n"
           "\ty = %6.1f ... %6.1f mm\n"
           "\tz = %6.1f ... %6.1f mm\n",
           1000*(minn[0]*grid),1000*(maxn[0]*grid),
           1000*(minn[1]*grid),1000*(maxn[1]*grid),
           1000*(minn[2]*grid),1000*(maxn[2]*grid));
    /*
       * Now make the initial grid
       */
    np = 1;
    for (c = 0; c < 3; c++)
        np = np*(maxn[c]-minn[c]+1);
    nplane = (maxn[0]-minn[0]+1)*(maxn[1]-minn[1]+1);
    nrow   = (maxn[0]-minn[0]+1);
    sp = MNESourceSpace::create_source_space(np);
    sp->type = MNE_SOURCE_SPACE_VOLUME;
    sp->nneighbor_vert = Eigen::VectorXi::Constant(sp->np, NNEIGHBORS);
    sp->neighbor_vert.resize(sp->np);
    for (k = 0; k < sp->np; k++) {
        sp->inuse[k]  = 1;
        sp->vertno[k] = k;
        sp->nn(k,0) = sp->nn(k,1) = 0.0; /* Source orientation is immaterial */
        sp->nn(k,2) = 1.0;
        sp->neighbor_vert[k] = Eigen::VectorXi::Constant(NNEIGHBORS, -1);
        sp->nuse++;
    }
    for (k = 0, z = minn[2]; z <= maxn[2]; z++) {
        for (y = minn[1]; y <= maxn[1]; y++) {
            for (x = minn[0]; x <= maxn[0]; x++, k++) {
                sp->rr(k,0) = x*grid;
                sp->rr(k,1) = y*grid;
                sp->rr(k,2) = z*grid;
                /*
             * Figure out the neighborhood:
             * 6-neighborhood first
             */
                Eigen::VectorXi& neigh = sp->neighbor_vert[k];
                if (z > minn[2])
                    neigh[0]  = k - nplane;
                if (x < maxn[0])
                    neigh[1] = k + 1;
                if (y < maxn[1])
                    neigh[2] = k + nrow;
                if (x > minn[0])
                    neigh[3] = k - 1;
                if (y > minn[1])
                    neigh[4] = k - nrow;
                if (z < maxn[2])
                    neigh[5] = k + nplane;
                /*
             * Then the rest to complete the 26-neighborhood
             * First the plane below
             */
                if (z > minn[2]) {
                    if (x < maxn[0]) {
                        neigh[6] = k + 1 - nplane;
                        if (y < maxn[1])
                            neigh[7] = k + 1 + nrow - nplane;
                    }
                    if (y < maxn[1])
                        neigh[8] = k + nrow - nplane;
                    if (x > minn[0]) {
                        if (y < maxn[1])
                            neigh[9] = k - 1 + nrow - nplane;
                        neigh[10] = k - 1 - nplane;
                        if (y > minn[1])
                            neigh[11] = k - 1 - nrow - nplane;
                    }
                    if (y > minn[1]) {
                        neigh[12] = k - nrow - nplane;
                        if (x < maxn[0])
                            neigh[13] = k + 1 - nrow - nplane;
                    }
                }
                /*
             * Then the same plane
             */
                if (x < maxn[0] && y < maxn[1])
                    neigh[14] = k + 1 + nrow;
                if (x > minn[0]) {
                    if (y < maxn[1])
                        neigh[15] = k - 1 + nrow;
                    if (y > minn[1])
                        neigh[16] = k - 1 - nrow;
                }
                if (y > minn[1] && x < maxn[0])
                    neigh[17] = k + 1 - nrow - nplane;
                /*
             * Finally one plane above
             */
                if (z < maxn[2]) {
                    if (x < maxn[0]) {
                        neigh[18] = k + 1 + nplane;
                        if (y < maxn[1])
                            neigh[19] = k + 1 + nrow + nplane;
                    }
                    if (y < maxn[1])
                        neigh[20] = k + nrow + nplane;
                    if (x > minn[0]) {
                        if (y < maxn[1])
                            neigh[21] = k - 1 + nrow + nplane;
                        neigh[22] = k - 1 + nplane;
                        if (y > minn[1])
                            neigh[23] = k - 1 - nrow + nplane;
                    }
                    if (y > minn[1]) {
                        neigh[24] = k - nrow + nplane;
                        if (x < maxn[0])
                            neigh[25] = k + 1 - nrow + nplane;
                    }
                }
            }
        }
    }
    qInfo("%d sources before omitting any.\n",sp->nuse);
    /*
       * Exclude infeasible points
       */
    for (k = 0; k < sp->np; k++) {
        dist = (sp->rr.row(k).transpose() - cm).norm();
        if (dist < exclude || dist > maxdist) {
            sp->inuse[k] = 0;
            sp->nuse--;
        }
    }
    qInfo("%d sources after omitting infeasible sources.\n",sp->nuse);
    {
        std::vector<std::unique_ptr<MNESourceSpace>> sp_vec;
        sp_vec.push_back(std::move(sp));
        if (filter_source_spaces(surf,mindist,FiffCoordTrans(),sp_vec,nullptr) != OK) {
            return nullptr;
        }
        sp = std::move(sp_vec[0]);
    }
    qInfo("%d sources remaining after excluding the sources outside the surface and less than %6.1f mm inside.\n",sp->nuse,1000*mindist);
    /*
       * Omit unused vertices from the neighborhoods
       */
    qInfo("Adjusting the neighborhood info...");
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
    qInfo("[done]\n");
    /*
     * Set up the volume data (needed for creating the interpolation matrix)
     */
    {
        Eigen::Vector3f r0(minn[0]*grid, minn[1]*grid, minn[2]*grid);
        Eigen::Vector3f voxel_size(grid, grid, grid);
        Eigen::Vector3f x_ras = Eigen::Vector3f::UnitX();
        Eigen::Vector3f y_ras = Eigen::Vector3f::UnitY();
        Eigen::Vector3f z_ras = Eigen::Vector3f::UnitZ();
        int width  = (maxn[0]-minn[0]+1);
        int height = (maxn[1]-minn[1]+1);
        int depth  = (maxn[2]-minn[2]+1);

        sp->voxel_surf_RAS_t = make_voxel_ras_trans(r0,x_ras,y_ras,z_ras,voxel_size);
        if (!sp->voxel_surf_RAS_t || sp->voxel_surf_RAS_t->isEmpty())
            return nullptr;

        sp->vol_dims[0] = width;
        sp->vol_dims[1] = height;
        sp->vol_dims[2] = depth;
        Eigen::Map<Eigen::Vector3f>(sp->voxel_size) = voxel_size;
    }

    return sp.release();
}

//=============================================================================================================

int MNESourceSpace::filter_source_spaces(const MNESurface& surf, float limit, const FiffCoordTrans& mri_head_t, std::vector<std::unique_ptr<MNESourceSpace>>& spaces, QTextStream *filtered)   /* Provide a list of filtered points here */
/*
     * Remove all source space points closer to the surface than a given limit
     */
{
    MNESourceSpace* s;
    int k,p1,p2;
    Eigen::Vector3f r1;
    float mindist,dist;
    int   minnode;
    int   omit,omit_outside;
    double tot_angle;
    int nspace = static_cast<int>(spaces.size());

    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD && mri_head_t.isEmpty()) {
        qCritical("Source spaces are in head coordinates and no coordinate transform was provided!");
        return FAIL;
    }
    /*
        * How close are the source points to the surface?
        */
    qInfo("Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        qInfo("head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        qInfo("MRI coordinates.\n");
    else
        qWarning("unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    qInfo("Checking that the sources are inside the bounding surface ");
    if (limit > 0.0)
        qInfo("and at least %6.1f mm away",1000*limit);
    qInfo(" (will take a few...)\n");
    omit         = 0;
    omit_outside = 0;
    for (k = 0; k < nspace; k++) {
        s = spaces[k].get();
        for (p1 = 0; p1 < s->np; p1++)
            if (s->inuse[p1]) {
                r1 = s->rr.row(p1).transpose();	/* Transform the point to MRI coordinates */
                if (s->coord_frame == FIFFV_COORD_HEAD)
                    FiffCoordTrans::apply_inverse_trans(r1.data(),mri_head_t,FIFFV_MOVE);
                /*
                * Check that the source is inside the inner skull surface
                */
                tot_angle = surf.sum_solids(r1)/(4*M_PI);
                if (std::fabs(tot_angle-1.0) > 1e-5) {
                    omit_outside++;
                    s->inuse[p1] = 0;
                    s->nuse--;
                    if (filtered)
                        *filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                  << 1000*r1[X] << " " << 1000*r1[Y] << " " << 1000*r1[Z] << "\n" << qSetFieldWidth(0);
                }
                else if (limit > 0.0) {
                    /*
                        * Check the distance limit
                        */
                    mindist = 1.0;
                    minnode = 0;
                    for (p2 = 0; p2 < surf.np; p2++) {
                        dist = (surf.rr.row(p2).transpose() - r1).norm();
                        if (dist < mindist) {
                            mindist = dist;
                            minnode = p2;
                        }
                    }
                    if (mindist < limit) {
                        omit++;
                        s->inuse[p1] = 0;
                        s->nuse--;
                        if (filtered)
                            *filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                      << 1000*r1[X] << " " << 1000*r1[Y] << " " << 1000*r1[Z] << "\n" << qSetFieldWidth(0);
                    }
                }
            }
    }
    (void)minnode; // squash compiler warning, this is unused
    if (omit_outside > 0)
        qInfo("%d source space points omitted because they are outside the inner skull surface.\n",
               omit_outside);
    if (omit > 0)
        qInfo("%d source space points omitted because of the %6.1f-mm distance limit.\n",
               omit,1000*limit);
    qInfo("Thank you for waiting.\n");
    return OK;
}

//=============================================================================================================

void MNESourceSpace::filter_source_space(FilterThreadArg *arg)
{
    FilterThreadArg* a = arg;
    int    p1,p2;
    double tot_angle;
    int    omit,omit_outside;
    Eigen::Vector3f r1;
    float  mindist,dist;
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
            r1 = a->s->rr.row(p1).transpose();	/* Transform the point to MRI coordinates */
            if (a->s->coord_frame == FIFFV_COORD_HEAD) {
                Q_ASSERT(a->mri_head_t);
                FiffCoordTrans::apply_inverse_trans(r1.data(),*a->mri_head_t,FIFFV_MOVE);
            }
            /*
           * Check that the source is inside the inner skull surface
           */
            tot_angle = surf->sum_solids(r1)/(4*M_PI);
            if (std::fabs(tot_angle-1.0) > 1e-5) {
                omit_outside++;
                a->s->inuse[p1] = 0;
                a->s->nuse--;
                if (a->filtered)
                    *a->filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                 << 1000*r1[X] << " " << 1000*r1[Y] << " " << 1000*r1[Z] << "\n" << qSetFieldWidth(0);
            }
            else if (a->limit > 0.0) {
                /*
         * Check the distance limit
         */
                mindist = 1.0;
                minnode = 0;
                for (p2 = 0; p2 < surf->np; p2++) {
                    dist = (surf->rr.row(p2).transpose() - r1).norm();
                    if (dist < mindist) {
                        mindist = dist;
                        minnode = p2;
                    }
                }
                if (mindist < a->limit) {
                    omit++;
                    a->s->inuse[p1] = 0;
                    a->s->nuse--;
                    if (a->filtered)
                        *a->filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                     << 1000*r1[X] << " " << 1000*r1[Y] << " " << 1000*r1[Z] << "\n" << qSetFieldWidth(0);
                }
            }
        }
    }
    (void)minnode; // squash compiler warning, set but unused
    if (omit_outside > 0)
        qInfo("%d source space points omitted because they are outside the inner skull surface.\n",
                omit_outside);
    if (omit > 0)
        qInfo("%d source space points omitted because of the %6.1f-mm distance limit.\n",
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
    int nspace = static_cast<int>(spaces.size());

    if (bemfile.isEmpty())
        return OK;

    {
        auto rawSurf = MNESurface::read_bem_surface(bemfile,FIFFV_BEM_SURF_ID_BRAIN,false);
        if (!rawSurf) {
            qCritical("BEM model does not have the inner skull triangulation!");
            return FAIL;
        }
        surf.reset(rawSurf.release());
    }
    /*
     * How close are the source points to the surface?
     */
    qInfo("Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        qInfo("head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        qInfo("MRI coordinates.\n");
    else
        qWarning("unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    qInfo("Checking that the sources are inside the inner skull ");
    if (limit > 0.0)
        qInfo("and at least %6.1f mm away",1000*limit);
    qInfo(" (will take a few...)\n");
    if (nproc < 2 || nspace == 1 || !use_threads) {
        /*
        * This is the conventional calculation
        */
        for (k = 0; k < nspace; k++) {
            auto a_ptr = std::make_unique<FilterThreadArg>();
            a_ptr->s = spaces[k].get();
            a_ptr->mri_head_t = std::make_unique<FiffCoordTrans>(mri_head_t);
            a_ptr->surf = surf;
            a_ptr->limit = limit;
            a_ptr->filtered = filtered;
            filter_source_space(a_ptr.get());
            spaces[k]->rearrange_source_space();
        }
    }
    else {
        /*
        * Calculate all (both) source spaces simultaneously
        */
        QList<FilterThreadArg*> args;

        std::vector<std::unique_ptr<FilterThreadArg>> arg_owners;
        for (k = 0; k < nspace; k++) {
            auto a_ptr = std::make_unique<FilterThreadArg>();
            a_ptr->s = spaces[k].get();
            a_ptr->mri_head_t = std::make_unique<FiffCoordTrans>(mri_head_t);
            a_ptr->surf = surf;
            a_ptr->limit = limit;
            a_ptr->filtered = filtered;
            args.append(a_ptr.get());
            arg_owners.push_back(std::move(a_ptr));
        }
        /*
        * Ready to start the threads & Wait for them to complete
        */
        QtConcurrent::blockingMap(args, filter_source_space);

        for (k = 0; k < nspace; k++) {
            spaces[k]->rearrange_source_space();
        }
    }
    qInfo("Thank you for waiting.\n\n");

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

    if(!stream->open()) {
        stream->close();
        return FIFF_FAIL;
    }

    sources = stream->dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
    if (sources.size() == 0) {
        qCritical("No source spaces available here");
        stream->close();
        return FIFF_FAIL;
    }
    for (j = 0; j < sources.size(); j++) {
        new_space = MNESourceSpace::create_source_space(0);
        node = sources[j];
        /*
            * Get the mandatory data first
            */
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag)) {
            stream->close();
            return FIFF_FAIL;
        }
        new_space->np = *t_pTag->toInt();
        if (new_space->np == 0) {
            qCritical("No points in this source space");
            stream->close();
            return FIFF_FAIL;
        }
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_POINTS, t_pTag)) {
            stream->close();
            return FIFF_FAIL;
        }
        MatrixXf tmp_rr = t_pTag->toFloatMatrix().transpose();
        new_space->rr = tmp_rr;
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag)) {
            stream->close();
            return FIFF_FAIL;
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
            new_space->subject = t_pTag->toString();
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
                    stream->close();
                    return FIFF_FAIL;
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
                stream->close();
                return FIFF_FAIL;
            }

            {
                Eigen::Map<Eigen::VectorXi> inuseMap(t_pTag->toInt(), new_space->np);
                new_space->inuse = inuseMap;
            }
            if (new_space->nuse > 0) {
                new_space->vertno = Eigen::VectorXi::Zero(new_space->nuse);
                for (k = 0, p = 0; k < new_space->np; k++) {
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
                    stream->close();
                    return FIFF_FAIL;
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
                Eigen::Map<Eigen::VectorXi> nearestMap(t_pTag->toInt(), new_space->np);
                new_space->nearest.resize(new_space->np);
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].vert = k;
                    new_space->nearest[k].nearest = nearestMap[k];
                    new_space->nearest[k].patch = nullptr;
                }

                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, t_pTag)) {
                    stream->close();
                    return FIFF_FAIL;
                }
                Eigen::Map<const Eigen::VectorXf> nearestDistMap(t_pTag->toFloat(), new_space->np);
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].dist = nearestDistMap[k];
                }
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
                        stream->close();
                        return FIFF_FAIL;
                    }
                    auto dist_full = dist_lower->mne_add_upper_triangle_rcs();
                    if (!dist_full) {
                        stream->close();
                        return FIFF_FAIL;
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

            Eigen::VectorXi neighborsVec;
            Eigen::VectorXi nneighborsVec;
            ntot = nvert = 0;
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEIGHBORS, t_pTag)) {
                ntot = t_pTag->size()/sizeof(fiff_int_t);
                neighborsVec = Eigen::Map<Eigen::VectorXi>(t_pTag->toInt(), ntot);
            }
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NNEIGHBORS, t_pTag)) {
                nvert = t_pTag->size()/sizeof(fiff_int_t);
                nneighborsVec = Eigen::Map<Eigen::VectorXi>(t_pTag->toInt(), nvert);
            }
            if (neighborsVec.size() > 0 && nneighborsVec.size() > 0) {
                if (nvert != new_space->np) {
                    qCritical("Inconsistent neighborhood data in file.");
                    stream->close();
                    return FIFF_FAIL;
                }
                for (k = 0, ntot_count = 0; k < nvert; k++)
                    ntot_count += nneighborsVec[k];
                if (ntot_count != ntot) {
                    qCritical("Inconsistent neighborhood data in file.");
                    stream->close();
                    return FIFF_FAIL;
                }
                new_space->nneighbor_vert = Eigen::VectorXi::Zero(nvert);
                new_space->neighbor_vert.resize(nvert);
                for (k = 0, q = 0; k < nvert; k++) {
                    new_space->nneighbor_vert[k] = nneigh = nneighborsVec[k];
                    new_space->neighbor_vert[k] = Eigen::VectorXi(nneigh);
                    for (p = 0; p < nneigh; p++,q++)
                        new_space->neighbor_vert[k][p] = neighborsVec[q];
                }
            }
            /*
                * There might be a coordinate transformation and dimensions
                */
            new_space->voxel_surf_RAS_t   = FiffCoordTrans(FiffCoordTrans::readTransformFromNode(stream, node, FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS));
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS, t_pTag)) {
                Eigen::Map<Eigen::Vector3i> volDimsMap(t_pTag->toInt());
                Eigen::Map<Eigen::Vector3i>(new_space->vol_dims) = volDimsMap;
            }
            {
                QList<FiffDirNode::SPtr>  mris = node->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);

                if (mris.size() == 0) { /* The old way */
                    new_space->MRI_surf_RAS_RAS_t = FiffCoordTrans(FiffCoordTrans::readTransformFromNode(stream, node, FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS));
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_MRI_FILE, t_pTag)) {
                        new_space->MRI_volume = t_pTag->toString();
                    }
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator = std::move(*FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag));
                    }
                }
                else {
                    if (node->find_tag(stream, FIFF_MNE_FILE_NAME, t_pTag)) {
                        new_space->MRI_volume = t_pTag->toString();
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
                    qCritical("Could not transform a source space because of transformation incompatibility.");
                    return FAIL;
                }
            }
            else {
                qCritical("Could not transform a source space because of missing coordinate transformation.");
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
    MNESourceSpace* lh = nullptr;
    MNESourceSpace* rh = nullptr;
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
            qWarning("\tWarning: cannot assign label file %s to a hemisphere.\n",labels[k].toUtf8().constData());
            continue;
        }
        if (sp) {
            if (read_label(labels[k],sel) == FAIL)
                return FAIL;
            for (p = 0; p < sel.size(); p++) {
                if (sel[p] >= 0 && sel[p] < sp->np)
                    (*inuse)[sel[p]] = sp->inuse[sel[p]];
                else
                    qWarning("vertex number out of range in %s (%d vs %d)\n",
                           labels[k].toUtf8().constData(),sel[p],sp->np);
            }
            qInfo("Processed label file %s\n",labels[k].toUtf8().constData());
        }
    }
    if (lh) lh->update_inuse(std::move(lh_inuse));
    if (rh) rh->update_inuse(std::move(rh_inuse));
    return OK;
}

//=============================================================================================================

int MNESourceSpace::read_label(const QString& label, Eigen::VectorXi& sel)
/*
          * Find the source points within a label
          */
{
    int k,p,nlabel;
    char c;
    float fdum;
    /*
       * Read the label file
       */
    QFile inFile(label);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << label;//err_set_sys_error(label);
        sel.resize(0);
        return FAIL;
    }
    inFile.getChar(&c);
    if (c !='#') {
        qCritical("FsLabel file does not start correctly.");
        sel.resize(0);
        return FAIL;
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
        sel.resize(0);
        return FAIL;
    }
    sel.resize(nlabel);
    for (k = 0; k < nlabel; k++) {
        in >> p >> fdum >> fdum >> fdum >> fdum;
        if (in.status() != QTextStream::Ok) {
            qCritical("Could not read label point # %d",k+1);
            sel.resize(0);
            return FAIL;
        }
        sel[k] = p;
    }
    }

    return OK;
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

//=============================================================================================================

/**
 * Helper: generate vertices of a unit icosahedron subdivided to the given grade.
 * Grade 0 = 12 vertices (icosahedron), grade N = 10 * 4^N + 2 vertices.
 */
static Eigen::MatrixX3f generateIcoVertices(int grade)
{
    // Base icosahedron vertices
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    std::vector<Eigen::Vector3f> verts = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };
    // Normalize to unit sphere
    for (auto& v : verts)
        v.normalize();

    // Base icosahedron faces
    std::vector<std::array<int,3>> faces = {
        {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
        {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
        {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
        {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1}
    };

    // Subdivide
    for (int g = 0; g < grade; ++g) {
        std::map<std::pair<int,int>, int> midpointCache;
        std::vector<std::array<int,3>> newFaces;

        auto getMidpoint = [&](int i1, int i2) -> int {
            auto key = std::make_pair(std::min(i1, i2), std::max(i1, i2));
            auto it = midpointCache.find(key);
            if (it != midpointCache.end())
                return it->second;
            Eigen::Vector3f mid = (verts[i1] + verts[i2]).normalized();
            int idx = static_cast<int>(verts.size());
            verts.push_back(mid);
            midpointCache[key] = idx;
            return idx;
        };

        for (const auto& f : faces) {
            int a = getMidpoint(f[0], f[1]);
            int b = getMidpoint(f[1], f[2]);
            int c = getMidpoint(f[2], f[0]);
            newFaces.push_back({f[0], a, c});
            newFaces.push_back({f[1], b, a});
            newFaces.push_back({f[2], c, b});
            newFaces.push_back({a, b, c});
        }
        faces = newFaces;
    }

    Eigen::MatrixX3f result(static_cast<int>(verts.size()), 3);
    for (int i = 0; i < static_cast<int>(verts.size()); ++i)
        result.row(i) = verts[i];
    return result;
}

//=============================================================================================================

MNEHemisphere MNESourceSpace::icoDownsample(const MNEHemisphere& hemi, int icoGrade)
{
    MNEHemisphere result(hemi);

    if (hemi.np <= 0 || hemi.rr.rows() == 0) {
        qWarning("MNESourceSpace::icoDownsample - Hemisphere has no vertices.");
        return result;
    }

    // Generate icosahedral surface at the requested grade
    Eigen::MatrixX3f icoVerts = generateIcoVertices(icoGrade);

    // Project hemisphere vertices to unit sphere for matching
    Eigen::MatrixX3f hemiNorm(hemi.np, 3);
    for (int i = 0; i < hemi.np; ++i) {
        Eigen::Vector3f v = hemi.rr.row(i);
        float len = v.norm();
        if (len > 0.0f)
            hemiNorm.row(i) = (v / len).transpose();
        else
            hemiNorm.row(i) = v.transpose();
    }

    // Clear inuse
    result.inuse = Eigen::VectorXi::Zero(result.np);

    // For each icosahedral vertex, find the nearest hemisphere vertex
    for (int i = 0; i < icoVerts.rows(); ++i) {
        Eigen::Vector3f icoV = icoVerts.row(i);
        float bestDist = std::numeric_limits<float>::max();
        int bestIdx = -1;
        for (int j = 0; j < hemi.np; ++j) {
            float d = (hemiNorm.row(j).transpose() - icoV).squaredNorm();
            if (d < bestDist) {
                bestDist = d;
                bestIdx = j;
            }
        }
        if (bestIdx >= 0)
            result.inuse[bestIdx] = 1;
    }

    // Recount nuse and rebuild vertno
    result.nuse = result.inuse.sum();
    result.vertno.resize(result.nuse);
    int k = 0;
    for (int i = 0; i < result.np; ++i) {
        if (result.inuse[i])
            result.vertno[k++] = i;
    }

    return result;
}

