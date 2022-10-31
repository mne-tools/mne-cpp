//=============================================================================================================
/**
 * @file     mne_sss_data.cpp
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
 * @brief    Definition of the MNE SSS Data (MneSssData) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_sss_data.h"
#include <fiff/fiff_file.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

//============================= dot.h =============================

#define X 0
#define Y 1
#define Z 2

#define VEC_COPY(to,from) {\
    (to)[X] = (from)[X];\
    (to)[Y] = (from)[Y];\
    (to)[Z] = (from)[Z];\
    }

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSssData::MneSssData()
: job(FIFFV_SSS_JOB_NOTHING)
, coord_frame(FIFFV_COORD_UNKNOWN)
, nchan(0)
, out_order(0)
, in_order(0)
, comp_info(NULL)
, ncomp(0)
, in_nuse(0)
, out_nuse(0)
{
}

//=============================================================================================================

MneSssData::MneSssData(const MneSssData& p_MneSssData)
: job(p_MneSssData.job)
, coord_frame(p_MneSssData.coord_frame)
, nchan(p_MneSssData.nchan)
, out_order(p_MneSssData.out_order)
, in_order(p_MneSssData.in_order)
, ncomp(p_MneSssData.ncomp)
, in_nuse(p_MneSssData.in_nuse)
, out_nuse(p_MneSssData.out_nuse)
{
    origin[0] = p_MneSssData.origin[0];
    origin[1] = p_MneSssData.origin[1];
    origin[2] = p_MneSssData.origin[2];

    comp_info = (int *)malloc(ncomp*sizeof(int));

    for (int k = 0; k < ncomp; k++)
        comp_info[k] = p_MneSssData.comp_info[k];
}

//=============================================================================================================

MneSssData::~MneSssData()
{
    if ((char *)(comp_info) != NULL)
        free((char *)(comp_info));
}

//=============================================================================================================

MneSssData *MneSssData::read_sss_data(const QString &name)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    MneSssData* s  = NULL;

    if(stream->open())
        s = read_sss_data_from_node(stream,stream->dirtree());

    stream->close();
    return s;
}

//=============================================================================================================

MneSssData *MneSssData::read_sss_data_from_node(QSharedPointer<FiffStream> &stream, const QSharedPointer<FiffDirNode> &start)
{
    MneSssData* s  = new MneSssData();
    QList<FiffDirNode::SPtr> sss;
    FiffDirNode::SPtr node;
    FiffTag::SPtr t_pTag;
    float       *r0;
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
        if (!node->find_tag(stream, FIFF_SSS_JOB, t_pTag))
            goto bad;
        s->job = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_FRAME, t_pTag))
            goto bad;
        s->coord_frame = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_ORIGIN, t_pTag))
            goto bad;
        r0 = t_pTag->toFloat();
        VEC_COPY(s->origin,r0);

        if (!node->find_tag(stream, FIFF_SSS_ORD_IN, t_pTag))
            goto bad;
        s->in_order = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_ORD_OUT, t_pTag))
            goto bad;
        s->out_order = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_NMAG, t_pTag))
            goto bad;
        s->nchan = *t_pTag->toInt();

        if (!node->find_tag(stream, FIFF_SSS_COMPONENTS, t_pTag))
            goto bad;
        qDebug() << "ToDo: Check whether comp_info contains the right stuff!!! - use VectorXi instead";
        s->comp_info = t_pTag->toInt();
        s->ncomp     = t_pTag->size()/sizeof(fiff_int_t);

        if (s->ncomp != (s->in_order*(2+s->in_order) + s->out_order*(2+s->out_order))) {
            printf("Number of SSS components does not match the expansion orders listed in the file");
            goto bad;
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

bad : {
        /*
            * Not entirely happy
            */
        if (s)
            delete s;
        return NULL;
    }
}

//=============================================================================================================
namespace MNELIB
{

typedef struct {
    int frame;
    const char *name;
} frameNameRec_1;

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

void MneSssData::print(FILE *f) const
{
    int j,p,q,n;

    fprintf(f,"job         : %d\n",this->job);
    fprintf(f,"coord frame : %s\n",mne_coord_frame_name_1(this->coord_frame));
    fprintf(f,"origin      : %6.1f %6.1f %6.1f mm\n",1000*this->origin[0],1000*this->origin[1],1000*this->origin[2]);
    fprintf(f,"in order    : %d\n",this->in_order);
    fprintf(f,"out order   : %d\n",this->out_order);
    fprintf(f,"nchan       : %d\n",this->nchan);
    fprintf(f,"ncomp       : %d\n",this->ncomp);
    fprintf(f,"in nuse     : %d\n",this->in_nuse);
    fprintf(f,"out nuse    : %d\n",this->out_nuse);
    fprintf(f,"comps       : ");
    /*
   * This produces the same output as maxfilter
   */
    for (j = 0, n = 3, p = 0; j < this->in_order; j++, n = n + 2) {
        if (j > 0)
            fprintf(f,";");
        for (q = 0; q < n; q++, p++)
            fprintf(f,"%1d",this->comp_info[p]);
    }
    fprintf(f,"//");
    for (j = 0, n = 3; j < this->out_order; j++, n = n + 2) {
        if (j > 0)
            fprintf(f,";");
        for (q = 0; q < n; q++, p++)
            fprintf(f,"%1d",this->comp_info[p]);
    }
    fprintf(f,"\n");
    return;
}
