//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_sss_data.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref MNELIB::MNESssData.
 *
 * Implements FIFF read of the @c FIFFB_SSS_INFO sub-blocks, parsing the
 * @c FIFF_SSS_FRAME, @c FIFF_SSS_JOB, @c FIFF_SSS_ORD_IN /
 * @c _ORD_OUT tags into the in-memory descriptor.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_sss_data.h"
#include <fiff/fiff_file.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESssData::MNESssData()
: job(FIFFV_SSS_JOB_NOTHING)
, coord_frame(FIFFV_COORD_UNKNOWN)
, nchan(0)
, out_order(0)
, in_order(0)
, in_nuse(0)
, out_nuse(0)
{
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
}

//=============================================================================================================

MNESssData::MNESssData(const MNESssData& p_MneSssData)
: job(p_MneSssData.job)
, coord_frame(p_MneSssData.coord_frame)
, nchan(p_MneSssData.nchan)
, out_order(p_MneSssData.out_order)
, in_order(p_MneSssData.in_order)
, comp_info(p_MneSssData.comp_info)
, in_nuse(p_MneSssData.in_nuse)
, out_nuse(p_MneSssData.out_nuse)
{
    origin[0] = p_MneSssData.origin[0];
    origin[1] = p_MneSssData.origin[1];
    origin[2] = p_MneSssData.origin[2];
}

//=============================================================================================================

MNESssData::~MNESssData() = default;

//=============================================================================================================

std::unique_ptr<MNESssData> MNESssData::read(const QString &name)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    std::unique_ptr<MNESssData> s;

    if(stream->open())
        s = read_from_node(stream,stream->dirtree());

    stream->close();
    return s;
}

//=============================================================================================================

std::unique_ptr<MNESssData> MNESssData::read_from_node(QSharedPointer<FiffStream> &stream, const QSharedPointer<FiffDirNode> &start)
{
    auto s = std::make_unique<MNESssData>();
    QList<FiffDirNode::SPtr> sss;
    FiffDirNode::SPtr node;
    FiffTag::UPtr t_pTag;
    const float *r0;
    int j,p,q,n;
    /*
        * Locate the SSS information
        */
    sss = start->dir_tree_find(FIFFB_SSS_INFO);
    if (sss.size() > 0) {
        node = sss[0];
        /*
            * Read the SSS information, require all tags to be present
            */
        if (!node->find_tag(stream, FIFF_SSS_JOB, t_pTag)) {
            return nullptr;
        }
        s->job = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_FRAME, t_pTag)) {
            return nullptr;
        }
        s->coord_frame = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_ORIGIN, t_pTag)) {
            return nullptr;
        }
        r0 = t_pTag->toFloat();
        Eigen::Map<Eigen::Vector3f>(s->origin) = Eigen::Map<const Eigen::Vector3f>(r0);

        if (!node->find_tag(stream, FIFF_SSS_ORD_IN, t_pTag)) {
            return nullptr;
        }
        s->in_order = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_ORD_OUT, t_pTag)) {
            return nullptr;
        }
        s->out_order = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_NMAG, t_pTag)) {
            return nullptr;
        }
        s->nchan = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_COMPONENTS, t_pTag)) {
            return nullptr;
        }
        {
            int ncomp = t_pTag->size()/sizeof(fiff_int_t);
            int *raw  = t_pTag->toInt();
            s->comp_info = Eigen::VectorXi::Map(raw, ncomp);

            if (ncomp != (s->in_order*(2+s->in_order) + s->out_order*(2+s->out_order))) {
                qCritical("Number of SSS components does not match the expansion orders listed in the file");
                return nullptr;
            }
        }
        /*
            * Count the components in use
            */
        for (j = 0, n = 3, p = 0; j < s->in_order; j++, n = n + 2) {
            for (q = 0; q < n; q++, p++)
                if (s->comp_info[p])
                    s->in_nuse++;
        }
        for (j = 0, n = 3; j < s->out_order; j++, n = n + 2) {
            for (q = 0; q < n; q++, p++)
                s->out_nuse++;
        }
    }
    /*
        * There it is!
        */
    return s;
}

//=============================================================================================================
namespace MNELIB
{

/** @brief Lookup record mapping a FIFF coordinate frame integer code to its human-readable name. */
struct frameNameRec_1 {
    int frame;
    const char *name;
};

}

//=============================================================================================================

const char *mne_coord_frame_name_1(int frame)
{
    static frameNameRec_1 frames[] = {
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

void MNESssData::print(QTextStream &out) const
{
    int j,p,q,n;

    out << "job         : " << this->job << "\n";
    out << "coord frame : " << mne_coord_frame_name_1(this->coord_frame) << "\n";
    out << "origin      : " << qSetFieldWidth(6) << qSetRealNumberPrecision(1) << Qt::fixed
        << 1000*this->origin[0] << " " << 1000*this->origin[1] << " " << 1000*this->origin[2] << qSetFieldWidth(0) << " mm\n";
    out << "in order    : " << this->in_order << "\n";
    out << "out order   : " << this->out_order << "\n";
    out << "nchan       : " << this->nchan << "\n";
    out << "ncomp       : " << this->comp_info.size() << "\n";
    out << "in nuse     : " << this->in_nuse << "\n";
    out << "out nuse    : " << this->out_nuse << "\n";
    out << "comps       : ";
    /*
   * This produces the same output as maxfilter
   */
    for (j = 0, n = 3, p = 0; j < this->in_order; j++, n = n + 2) {
        if (j > 0)
            out << ";";
        for (q = 0; q < n; q++, p++)
            out << this->comp_info[p];
    }
    out << "//";
    for (j = 0, n = 3; j < this->out_order; j++, n = n + 2) {
        if (j > 0)
            out << ";";
        for (q = 0; q < n; q++, p++)
            out << this->comp_info[p];
    }
    out << "\n";
    return;
}
