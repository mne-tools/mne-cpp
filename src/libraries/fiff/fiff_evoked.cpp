//=============================================================================================================
/**
 * @file     fiff_evoked.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FIFFEvoked Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_evoked.h"
#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"

#include <utils/mnemath.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvoked::FiffEvoked()
: nave(-1)
, aspect_kind(-1)
, first(-1)
, last(-1)
, baseline(qMakePair(-1.0f, -1.0f))
{

}

//=============================================================================================================

FiffEvoked::FiffEvoked(QIODevice& p_IODevice,
                       QVariant setno,
                       QPair<float,float> t_baseline,
                       bool proj,
                       fiff_int_t p_aspect_kind)
{
    if(!FiffEvoked::read(p_IODevice, *this, setno, t_baseline, proj, p_aspect_kind))
    {
        baseline = t_baseline;

        printf("\tFiff evoked data not found.\n");//ToDo Throw here
        return;
    }
}

//=============================================================================================================

FiffEvoked::FiffEvoked(const FiffEvoked& p_FiffEvoked)
: info(p_FiffEvoked.info)
, nave(p_FiffEvoked.nave)
, aspect_kind(p_FiffEvoked.aspect_kind)
, first(p_FiffEvoked.first)
, last(p_FiffEvoked.last)
, comment(p_FiffEvoked.comment)
, times(p_FiffEvoked.times)
, data(p_FiffEvoked.data)
, proj(p_FiffEvoked.proj)
, baseline(p_FiffEvoked.baseline)
{

}

//=============================================================================================================

FiffEvoked::~FiffEvoked()
{

}

//=============================================================================================================

void FiffEvoked::clear()
{
    info.clear();
    nave = -1;
    aspect_kind = -1;
    first = -1;
    last = -1;
    comment = QString("");
    times = RowVectorXf();
    data = MatrixXd();
    proj = MatrixXd();
}

//=============================================================================================================

FiffEvoked FiffEvoked::pick_channels(const QStringList& include,
                                     const QStringList& exclude) const
{
    if(include.size() == 0 && exclude.size() == 0)
        return FiffEvoked(*this);

    RowVectorXi sel = FiffInfo::pick_channels(this->info.ch_names, include, exclude);
    if (sel.cols() == 0)
    {
        qWarning("Warning : No channels match the selection.\n");
        return FiffEvoked(*this);
    }

    FiffEvoked res(*this);
    //
    //   Modify the measurement info
    //
    res.info = FiffInfo(res.info.pick_info(sel));
    //
    //   Create the reduced data set
    //
    MatrixXd selBlock(1,1);

    if(selBlock.rows() != sel.cols() || selBlock.cols() != res.data.cols())
        selBlock.resize(sel.cols(), res.data.cols());
    for(qint32 l = 0; l < sel.cols(); ++l)
    {
        if(sel(0,l) <= res.data.rows()) {
            selBlock.block(l,0,1,selBlock.cols()) = res.data.block(sel(0,l),0,1,selBlock.cols());
        } else {
            qWarning("FiffEvoked::pick_channels - Warning : Selected channel index out of bound.\n");
        }
    }
    res.data.resize(sel.cols(), res.data.cols());
    res.data = selBlock;

    return res;
}

//=============================================================================================================

bool FiffEvoked::read(QIODevice& p_IODevice,
                      FiffEvoked& p_FiffEvoked,
                      QVariant setno,
                      QPair<float,float> t_baseline,
                      bool proj,
                      fiff_int_t p_aspect_kind)
{
    p_FiffEvoked.clear();

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = t_pStream->streamName();

    printf("Reading %s ...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open())
        return false;
    //
    //   Read the measurement info
    //
    FiffInfo info;
    FiffDirNode::SPtr meas;
    if(!t_pStream->read_meas_info(t_pStream->dirtree(), info, meas))
        return false;
    info.filename = t_sFileName; //move fname storage to read_meas_info member function
    //
    //   Locate the data of interest
    //
    QList<FiffDirNode::SPtr> processed = meas->dir_tree_find(FIFFB_PROCESSED_DATA);
    if (processed.size() == 0)
    {
        qWarning("Could not find processed data");
        return false;
    }
    //
    QList<FiffDirNode::SPtr> evoked_node = meas->dir_tree_find(FIFFB_EVOKED);
    if (evoked_node.size() == 0)
    {
        qWarning("Could not find evoked data");
        return false;
    }

    // convert setno to an integer
    if(!setno.isValid())
    {
        if (evoked_node.size() > 1)
        {
            QStringList comments;
            QList<fiff_int_t> aspect_kinds;
            QString t;
            if(!t_pStream->get_evoked_entries(evoked_node, comments, aspect_kinds, t))
                t = QString("None found, must use integer");
            qWarning("%d datasets present, setno parameter must be set. Candidate setno names:\n%s", (int)evoked_node.size(), t.toUtf8().constData());
            return false;
        }
        else
            setno = 0;
    }
    else
    {
        // find string-based entry
        bool t_bIsInteger = true;
        setno.toInt(&t_bIsInteger);
        if(!t_bIsInteger)
        {
            if(p_aspect_kind != FIFFV_ASPECT_AVERAGE && p_aspect_kind != FIFFV_ASPECT_STD_ERR)
            {
                qWarning("kindStat must be \"FIFFV_ASPECT_AVERAGE\" or \"FIFFV_ASPECT_STD_ERR\"");
                return false;
            }

            QStringList comments;
            QList<fiff_int_t> aspect_kinds;
            QString t;
            t_pStream->get_evoked_entries(evoked_node, comments, aspect_kinds, t);

            bool found = false;
            for(qint32 i = 0; i < comments.size(); ++i)
            {
                if(comments[i].compare(setno.toString()) == 0 && p_aspect_kind == aspect_kinds[i])
                {
                    setno = i;
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                qWarning() << "setno " << setno << " (" << p_aspect_kind << ") not found, out of found datasets:\n " << t;
                return false;
            }
        }
    }

    if (setno.toInt() >= evoked_node.size() || setno.toInt() < 0)
    {
        qWarning("Data set selector out of range");
        return false;
    }

    FiffDirNode::SPtr my_evoked = evoked_node[setno.toInt()];

    //
    //   Identify the aspects
    //
    QList<FiffDirNode::SPtr> aspects = my_evoked->dir_tree_find(FIFFB_ASPECT);

    if(aspects.size() > 1)
        printf("\tMultiple (%d) aspects found. Taking first one.\n", (int)aspects.size());

    FiffDirNode::SPtr my_aspect = aspects[0];

    //
    //   Now find the data in the evoked block
    //
    fiff_int_t nchan = 0;
    float sfreq = -1.0f;
    QList<FiffChInfo> chs;
    fiff_int_t kind, pos, first=0, last=0;
    FiffTag::SPtr t_pTag;
    QString comment("");
    qint32 k;
    for (k = 0; k < my_evoked->nent(); ++k)
    {
        kind = my_evoked->dir[k]->kind;
        pos  = my_evoked->dir[k]->pos;
        switch (kind)
        {
            case FIFF_COMMENT:
                t_pStream->read_tag(t_pTag,pos);
                comment = t_pTag->toString();
                break;
            case FIFF_FIRST_SAMPLE:
                t_pStream->read_tag(t_pTag,pos);
                first = *t_pTag->toInt();
                break;
            case FIFF_LAST_SAMPLE:
                t_pStream->read_tag(t_pTag,pos);
                last = *t_pTag->toInt();
                break;
            case FIFF_NCHAN:
                t_pStream->read_tag(t_pTag,pos);
                nchan = *t_pTag->toInt();
                break;
            case FIFF_SFREQ:
                t_pStream->read_tag(t_pTag,pos);
                sfreq = *t_pTag->toFloat();
                break;
            case FIFF_CH_INFO:
                t_pStream->read_tag(t_pTag, pos);
                chs.append( t_pTag->toChInfo() );
                break;
        }
    }
    if (comment.isEmpty())
        comment = QString("No comment");

    //
    //   Local channel information?
    //
    if (nchan > 0)
    {
        if (chs.size() == 0)
        {
            qWarning("Local channel information was not found when it was expected.");
            return false;
        }
        if (chs.size() != nchan)
        {
            qWarning("Number of channels and number of channel definitions are different.");
            return false;
        }
        info.chs   = chs;
        info.nchan = nchan;
        printf("\tFound channel information in evoked data. nchan = %d\n",nchan);
        if (sfreq > 0.0f)
            info.sfreq = sfreq;
    }
    qint32 nsamp = last-first+1;
    printf("\tFound the data of interest:\n");
    printf("\t\tt = %10.2f ... %10.2f ms (%s)\n", 1000*(float)first/info.sfreq, 1000*(float)last/info.sfreq,comment.toUtf8().constData());
    if (info.comps.size() > 0)
        printf("\t\t%d CTF compensation matrices available\n", (int)info.comps.size());

    //
    // Read the data in the aspect block
    //
    fiff_int_t aspect_kind = -1;
    fiff_int_t nave = -1;
    QList<FiffTag> epoch;
    for (k = 0; k < my_aspect->nent(); ++k)
    {
        kind = my_aspect->dir[k]->kind;
        pos  = my_aspect->dir[k]->pos;

        switch (kind)
        {
            case FIFF_COMMENT:
                t_pStream->read_tag(t_pTag, pos);
                comment = t_pTag->toString();
                break;
            case FIFF_ASPECT_KIND:
                t_pStream->read_tag(t_pTag, pos);
                aspect_kind = *t_pTag->toInt();
                break;
            case FIFF_NAVE:
                t_pStream->read_tag(t_pTag, pos);
                nave = *t_pTag->toInt();
                break;
            case FIFF_EPOCH:
                t_pStream->read_tag(t_pTag, pos);
                epoch.append(FiffTag(t_pTag.data()));
                break;
        }
    }
    if (nave == -1)
        nave = 1;
    printf("\t\tnave = %d - aspect type = %d\n", nave, aspect_kind);

    qint32 nepoch = epoch.size();
    MatrixXd all_data;
    if (nepoch == 1)
    {
        //
        //   Only one epoch
        //
        all_data = epoch[0].toFloatMatrix().cast<double>();
        all_data.transposeInPlace();
        //
        //   May need a transpose if the number of channels is one
        //
        if (all_data.cols() == 1 && info.nchan == 1)
            all_data.transposeInPlace();
    }
    else
    {
        //
        //   Put the old style epochs together
        //
        all_data = epoch[0].toFloatMatrix().cast<double>();
        all_data.transposeInPlace();
        qint32 oldsize;
        for (k = 1; k < nepoch; ++k)
        {
            oldsize = all_data.rows();
            MatrixXd tmp = epoch[k].toFloatMatrix().cast<double>();
            tmp.transposeInPlace();
            all_data.conservativeResize(oldsize+tmp.rows(), all_data.cols());
            all_data.block(oldsize, 0, tmp.rows(), tmp.cols()) = tmp;
        }
    }
    if (all_data.cols() != nsamp)
    {
        qWarning("Incorrect number of samples (%d instead of %d)", (int) all_data.cols(), nsamp);
        return false;
    }

    //
    //   Calibrate
    //
    printf("\n\tPreprocessing...\n");
    printf("\t%d channels remain after picking\n",info.nchan);

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(info.nchan);
    for(k = 0; k < info.nchan; ++k)
        tripletList.push_back(T(k, k, info.chs[k].cal));
    SparseMatrix<double> cals(info.nchan, info.nchan);
    cals.setFromTriplets(tripletList.begin(), tripletList.end());

    all_data = cals * all_data;

    RowVectorXf times = RowVectorXf(last-first+1);
    for (k = 0; k < times.size(); ++k)
        times[k] = ((float)(first+k)) / info.sfreq;

    //
    // Set up projection
    //
    if(info.projs.size() == 0 || !proj)
    {
        printf("\tNo projector specified for these data.\n");
        p_FiffEvoked.proj = MatrixXd();
    }
    else
    {
        //   Create the projector
        MatrixXd projection;
        qint32 nproj = info.make_projector(projection);
        if(nproj == 0)
        {
            printf("\tThe projection vectors do not apply to these channels\n");
            p_FiffEvoked.proj = MatrixXd();
        }
        else
        {
            printf("\tCreated an SSP operator (subspace dimension = %d)\n", nproj);
            p_FiffEvoked.proj = projection;
        }

        //   The projection items have been activated
        FiffProj::activate_projs(info.projs);
    }

    if(p_FiffEvoked.proj.rows() > 0)
    {
        all_data = p_FiffEvoked.proj * all_data;
        printf("\tSSP projectors applied to the evoked data\n");
    }

    // Put it all together
    p_FiffEvoked.info = info;
    p_FiffEvoked.nave = nave;
    p_FiffEvoked.aspect_kind = aspect_kind;
    p_FiffEvoked.first = first;
    p_FiffEvoked.last = last;
    p_FiffEvoked.comment = comment;
    p_FiffEvoked.times = times;
    p_FiffEvoked.data = all_data;

    // Run baseline correction
    p_FiffEvoked.applyBaselineCorrection(t_baseline);

    return true;
}

//=============================================================================================================

void FiffEvoked::setInfo(const FiffInfo &p_info,
                         bool proj)
{
    info = p_info;
    //
    // Set up projection
    //
    if(info.projs.size() == 0 || !proj)
    {
        printf("\tNo projector specified for these data.\n");
        this->proj = MatrixXd();
    }
    else
    {
        //   Create the projector
        MatrixXd projection;
        qint32 nproj = info.make_projector(projection);
        if(nproj == 0)
        {
            printf("\tThe projection vectors do not apply to these channels\n");
            this->proj = MatrixXd();
        }
        else
        {
            printf("\tCreated an SSP operator (subspace dimension = %d)\n", nproj);
            this->proj = projection;
        }

        //   The projection items have been activated
        FiffProj::activate_projs(info.projs);
    }
}

//=============================================================================================================

FiffEvoked & FiffEvoked::operator+=(const MatrixXd &newData)
{
    //Init matrix if necessary
    if(nave == -1 || nave == 0) {
        data = MatrixXd::Zero(newData.rows(),newData.cols());
    }

    if(data.cols() == newData.cols() && data.rows() == newData.rows()) {
        //Revert old averaging
        data = data*nave;

        //Do new averaging
        data += newData;
        if(nave <= 0) {
            nave = 1;
        } else {
            nave++;
        }

        data /= nave;
    }

    return *this;
}

//=============================================================================================================

void FiffEvoked::applyBaselineCorrection(QPair<float, float>& p_baseline)
{
    // Run baseline correction
    printf("Applying baseline correction ... (mode: mean)\n");
    this->data = MNEMath::rescale(this->data, this->times, p_baseline, QString("mean"));
    this->baseline = p_baseline;
}
