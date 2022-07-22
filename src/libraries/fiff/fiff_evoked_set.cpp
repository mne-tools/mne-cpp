//=============================================================================================================
/**
 * @file     fiff_evoked_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffEvokedSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_evoked_set.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"
#include "fiff_stream.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvokedSet::FiffEvokedSet()
{
    qRegisterMetaType<FIFFLIB::FiffEvokedSet>("FIFFLIB::FiffEvokedSet");
    qRegisterMetaType<FIFFLIB::FiffEvokedSet::SPtr>("FIFFLIB::FiffEvokedSet::SPtr");
}

//=============================================================================================================

FiffEvokedSet::FiffEvokedSet(QIODevice& p_IODevice)
{
    qRegisterMetaType<FIFFLIB::FiffEvokedSet>("FIFFLIB::FiffEvokedSet");
    qRegisterMetaType<FIFFLIB::FiffEvokedSet::SPtr>("FIFFLIB::FiffEvokedSet::SPtr");

    if(!FiffEvokedSet::read(p_IODevice, *this))
    {
        printf("\tFiff evoked data set not found.\n");//ToDo Throw here
        return;
    }
}

//=============================================================================================================

FiffEvokedSet::FiffEvokedSet(const FiffEvokedSet& p_FiffEvokedSet)
: info(p_FiffEvokedSet.info)
, evoked(p_FiffEvokedSet.evoked)
{
}

//=============================================================================================================

FiffEvokedSet::~FiffEvokedSet()
{
}

//=============================================================================================================

void FiffEvokedSet::clear()
{
    info.clear();
    evoked.clear();
}

//=============================================================================================================

FiffEvokedSet FiffEvokedSet::pick_channels(const QStringList& include,
                                           const QStringList& exclude) const
{
    FiffEvokedSet res;
    res.info = this->info;
    QList<FiffEvoked>::ConstIterator ev;
    for(ev = evoked.begin(); ev != evoked.end(); ++ev)
        res.evoked.push_back(ev->pick_channels(include, exclude));

    return res;

    //### OLD MATLAB oriented implementation ###

//    if(include.size() == 0 && exclude.size() == 0)
//        return FiffEvokedDataSet(*this);

//    RowVectorXi sel = FiffInfo::pick_channels(this->info.ch_names, include, exclude);
//    if (sel.cols() == 0)
//    {
//        qWarning("Warning : No channels match the selection.\n");
//        return FiffEvokedDataSet(*this);
//    }

//    FiffEvokedDataSet res(*this);
//    //
//    //   Modify the measurement info
//    //
////    if (res->info)
////        delete res->info;
//    res.info = FiffInfo(res.info.pick_info(sel));
//    //
//    //   Create the reduced data set
//    //
//    MatrixXd selBlock(1,1);
//    qint32 k, l;
//    for(k = 0; k < res.evoked.size(); ++k)
//    {
//        if(selBlock.rows() != sel.cols() || selBlock.cols() != res.evoked[k]->epochs.cols())
//            selBlock.resize(sel.cols(), res.evoked[k]->epochs.cols());
//        for(l = 0; l < sel.cols(); ++l)
//        {
//            selBlock.block(l,0,1,selBlock.cols()) = res.evoked[k]->epochs.block(sel(0,l),0,1,selBlock.cols());
//        }
//        res.evoked[k]->epochs.resize(sel.cols(), res.evoked[k]->epochs.cols());
//        res.evoked[k]->epochs = selBlock;
//    }

//    return res;
}

//=============================================================================================================

bool FiffEvokedSet::compensate_to(FiffEvokedSet& p_FiffEvokedSet,
                                  fiff_int_t to) const
{
    qint32 now = p_FiffEvokedSet.info.get_current_comp();
    FiffCtfComp ctf_comp;

    if(now == to)
    {
        printf("Data is already compensated as desired.\n");
        return false;
    }

    //Make the compensator and apply it to all data sets
    p_FiffEvokedSet.info.make_compensator(now,to,ctf_comp);

    for(qint16 i=0; i < p_FiffEvokedSet.evoked.size(); ++i)
    {
        p_FiffEvokedSet.evoked[i].data = ctf_comp.data->data*p_FiffEvokedSet.evoked[i].data;
    }

    //Update the compensation info in the channel descriptors
    p_FiffEvokedSet.info.set_current_comp(to);

    return true;
}

//=============================================================================================================

bool FiffEvokedSet::find_evoked(const FiffEvokedSet& p_FiffEvokedSet) const
{
    if(!p_FiffEvokedSet.evoked.size()) {
        printf("No evoked response data sets in %s\n",p_FiffEvokedSet.info.filename.toUtf8().constData());
        return false;
    }
    else
        printf("\nFound %lld evoked response data sets in %s :\n",p_FiffEvokedSet.evoked.size(),p_FiffEvokedSet.info.filename.toUtf8().constData());

    for(qint32 i = 0; i < p_FiffEvokedSet.evoked.size(); ++i) {
        printf("%s (%s)\n",p_FiffEvokedSet.evoked.at(i).comment.toUtf8().constData(),p_FiffEvokedSet.evoked.at(i).aspectKindToString().toUtf8().constBegin());
    }

    return true;
}

//=============================================================================================================

bool FiffEvokedSet::read(QIODevice& p_IODevice,
                         FiffEvokedSet& p_FiffEvokedSet, QPair<float,float> baseline,
                         bool proj)
{
    p_FiffEvokedSet.clear();

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = t_pStream->streamName();

    printf("Exploring %s ...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open())
        return false;
    //
    //   Read the measurement info
    //
    FiffDirNode::SPtr meas;
    if(!t_pStream->read_meas_info(t_pStream->dirtree(), p_FiffEvokedSet.info, meas))
        return false;
    p_FiffEvokedSet.info.filename = t_sFileName; //move fname storage to read_meas_info member function
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

    QStringList comments;
    QList<fiff_int_t> aspect_kinds;
    QString t;
    if(!t_pStream->get_evoked_entries(evoked_node, comments, aspect_kinds, t))
        t = QString("None found, must use integer");
    printf("\tFound %lld datasets\n", evoked_node.size());

    for(qint32 i = 0; i < comments.size(); ++i)
    {
        QFile t_file(p_FiffEvokedSet.info.filename);
        printf(">> Processing %s <<\n", comments[i].toUtf8().constData());
        FiffEvoked t_FiffEvoked;
        if(FiffEvoked::read(t_file, t_FiffEvoked, i, baseline, proj))
            p_FiffEvokedSet.evoked.push_back(t_FiffEvoked);
    }

    return true;

    //### OLD MATLAB oriented implementation ###

//    data.clear();

//    if (setno < 0)
//    {
//        printf("Data set selector must be positive\n");
//        return false;
//    }
//    //
//    //   Open the file
//    //
//    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
//    QString t_sFileName = t_pStream->streamName();

//    printf("Reading %s ...\n",t_sFileName.toUtf8().constData());
//    FiffDirNode t_Tree;
//    QList<FiffDirEntry> t_Dir;

//    if(!t_pStream->open(t_Tree, t_Dir))
//    {
//        if(t_pStream)
//            delete t_pStream;
//        return false;
//    }
//    //
//    //   Read the measurement info
//    //
//    FiffInfo info;// = NULL;
//    FiffDirNode meas;
//    if(!t_pStream->read_meas_info(t_Tree, info, meas))
//        return false;
//    info.filename = t_sFileName; //move fname storage to read_meas_info member function
//    //
//    //   Locate the data of interest
//    //
//    QList<FiffDirNode> processed = meas.dir_tree_find(FIFFB_PROCESSED_DATA);
//    if (processed.size() == 0)
//    {
//        if(t_pStream)
//            delete t_pStream;
//        printf("Could not find processed data\n");
//        return false;
//    }
//    //
//    QList<FiffDirNode> evoked = meas.dir_tree_find(FIFFB_EVOKED);
//    if (evoked.size() == 0)
//    {
//        if(t_pStream)
//            delete t_pStream;
//        printf("Could not find evoked data");
//        return false;
//    }
//    //
//    //   Identify the aspects
//    //
//    fiff_int_t naspect = 0;
//    fiff_int_t nsaspects = 0;
//    qint32 oldsize = 0;
//    MatrixXi is_smsh(1,0);
//    QList< QList<FiffDirNode> > sets_aspects;
//    QList< qint32 > sets_naspect;
//    QList<FiffDirNode> saspects;
//    qint32 k;
//    for (k = 0; k < evoked.size(); ++k)
//    {
////            sets(k).aspects = fiff_dir_tree_find(evoked(k),FIFF.FIFFB_ASPECT);
////            sets(k).naspect = length(sets(k).aspects);

//        sets_aspects.append(evoked[k].dir_tree_find(FIFFB_ASPECT));
//        sets_naspect.append(sets_aspects[k].size());

//        if (sets_naspect[k] > 0)
//        {
//            oldsize = is_smsh.cols();
//            is_smsh.conservativeResize(1, oldsize + sets_naspect[k]);
//            is_smsh.block(0, oldsize, 1, sets_naspect[k]) = MatrixXi::Zero(1, sets_naspect[k]);
//            naspect += sets_naspect[k];
//        }
//        saspects  = evoked[k].dir_tree_find(FIFFB_SMSH_ASPECT);
//        nsaspects = saspects.size();
//        if (nsaspects > 0)
//        {
//            sets_naspect[k] += nsaspects;
//            sets_aspects[k].append(saspects);

//            oldsize = is_smsh.cols();
//            is_smsh.conservativeResize(1, oldsize + sets_naspect[k]);
//            is_smsh.block(0, oldsize, 1, sets_naspect[k]) = MatrixXi::Ones(1, sets_naspect[k]);
//            naspect += nsaspects;
//        }
//    }
//    printf("\t%d evoked data sets containing a total of %d data aspects in %s\n",evoked.size(),naspect,t_sFileName.toUtf8().constData());
//    if (setno >= naspect || setno < 0)
//    {
//        if(t_pStream)
//            delete t_pStream;
//        printf("Data set selector out of range\n");
//        return false;
//    }
//    //
//    //   Next locate the evoked data set
//    //
//    qint32 p = 0;
//    qint32 a = 0;
//    bool goon = true;
//    FiffDirNode my_evoked;
//    FiffDirNode my_aspect;
//    for(k = 0; k < evoked.size(); ++k)
//    {
//        for (a = 0; a < sets_naspect[k]; ++a)
//        {
//            if(p == setno)
//            {
//                my_evoked = evoked[k];
//                my_aspect = sets_aspects[k][a];
//                goon = false;
//                break;
//            }
//            ++p;
//        }
//        if (!goon)
//            break;
//    }
//    //
//    //   The desired data should have been found but better to check
//    //
//    if (my_evoked.isEmpty() || my_aspect.isEmpty())
//    {
//        if(t_pStream)
//            delete t_pStream;
//        printf("Desired data set not found\n");
//        return false;
//    }
//    //
//    //   Now find the data in the evoked block
//    //
//    fiff_int_t nchan = 0;
//    float sfreq = -1.0f;
//    QList<FiffChInfo> chs;
//    fiff_int_t kind, pos, first, last;
//    FiffTag* t_pTag = NULL;
//    QString comment("");
//    for (k = 0; k < my_evoked.nent; ++k)
//    {
//        kind = my_evoked.dir[k].kind;
//        pos  = my_evoked.dir[k].pos;
//        switch (kind)
//        {
//            case FIFF_COMMENT:
//                FiffTag::read_tag(t_pStream,t_pTag,pos);
//                comment = t_pTag->toString();
//                break;
//            case FIFF_FIRST_SAMPLE:
//                FiffTag::read_tag(t_pStream,t_pTag,pos);
//                first = *t_pTag->toInt();
//                break;
//            case FIFF_LAST_SAMPLE:
//                FiffTag::read_tag(t_pStream,t_pTag,pos);
//                last = *t_pTag->toInt();
//                break;
//            case FIFF_NCHAN:
//                FiffTag::read_tag(t_pStream,t_pTag,pos);
//                nchan = *t_pTag->toInt();
//                break;
//            case FIFF_SFREQ:
//                FiffTag::read_tag(t_pStream,t_pTag,pos);
//                sfreq = *t_pTag->toFloat();
//                break;
//            case FIFF_CH_INFO:
//                FiffTag::read_tag(t_pStream, t_pTag, pos);
//                chs.append( t_pTag->toChInfo() );
//                break;
//        }
//    }
//    if (comment.isEmpty())
//        comment = QString("No comment");
//    //
//    //   Local channel information?
//    //
//    if (nchan > 0)
//    {
//        if (chs.size() == 0)
//        {
//            if(t_pStream)
//                delete t_pStream;
//            printf("Local channel information was not found when it was expected.\n");
//            return false;
//        }
//        if (chs.size() != nchan)
//        {
//            if(t_pStream)
//                delete t_pStream;
//            printf("Number of channels and number of channel definitions are different\n");
//            return false;
//        }
//        info.chs   = chs;
//        info.nchan = nchan;
//        printf("\tFound channel information in evoked data. nchan = %d\n",nchan);
//        if (sfreq > 0.0f)
//            info.sfreq = sfreq;
//    }
//    qint32 nsamp = last-first+1;
//    printf("\tFound the data of interest:\n");
//    printf("\t\tt = %10.2f ... %10.2f ms (%s)\n", 1000*(float)first/info.sfreq, 1000*(float)last/info.sfreq,comment.toUtf8().constData());
//    if (info.comps.size() > 0)
//        printf("\t\t%d CTF compensation matrices available\n", info.comps.size());
//    //
//    // Read the data in the aspect block
//    //
//    fiff_int_t nepoch = 0;
//    fiff_int_t aspect_kind = -1;
//    fiff_int_t nave = -1;
//    QList<FiffTag*> epoch;
//    for (k = 0; k < my_aspect.nent; ++k)
//    {
//        kind = my_aspect.dir[k].kind;
//        pos  = my_aspect.dir[k].pos;

//        switch (kind)
//        {
//            case FIFF_COMMENT:
//                FiffTag::read_tag(t_pStream, t_pTag, pos);
//                comment = t_pTag->toString();
//                break;
//            case FIFF_ASPECT_KIND:
//                FiffTag::read_tag(t_pStream, t_pTag, pos);
//                aspect_kind = *t_pTag->toInt();
//                break;
//            case FIFF_NAVE:
//                FiffTag::read_tag(t_pStream, t_pTag, pos);
//                nave = *t_pTag->toInt();
//                break;
//            case FIFF_EPOCH:
//                FiffTag::read_tag(t_pStream, t_pTag, pos);
//                epoch.append(new FiffTag(t_pTag));
//                ++nepoch;
//                break;
//        }
//    }
//    if (nave == -1)
//        nave = 1;
//    printf("\t\tnave = %d aspect type = %d\n", nave, aspect_kind);
//    if (nepoch != 1 && nepoch != info.nchan)
//    {
//        if(t_pStream)
//            delete t_pStream;
//        printf("Number of epoch tags is unreasonable (nepoch = %d nchan = %d)\n", nepoch, info.nchan);
//        return false;
//    }
//    //
//    MatrixXd all;// = NULL;
//    if (nepoch == 1)
//    {
//        //
//        //   Only one epoch
//        //
//        all = epoch[0]->toFloatMatrix();
//        all.transposeInPlace();
//        //
//        //   May need a transpose if the number of channels is one
//        //
//        if (all.cols() == 1 && info.nchan == 1)
//            all.transposeInPlace();
//    }
//    else
//    {
//        //
//        //   Put the old style epochs together
//        //
//        all = epoch[0]->toFloatMatrix();
//        all.transposeInPlace();

//        for (k = 2; k < nepoch; ++k)
//        {
//            oldsize = all.rows();
//            MatrixXd tmp = epoch[k]->toFloatMatrix();
//            tmp.transposeInPlace();
//            all.conservativeResize(oldsize+tmp.rows(), all.cols());
//            all.block(oldsize, 0, tmp.rows(), tmp.cols()) = tmp;
//        }
//    }
//    if (all.cols() != nsamp)
//    {
//        if(t_pStream)
//            delete t_pStream;
//        printf("Incorrect number of samples (%d instead of %d)", (int)all.cols(), nsamp);
//        return false;
//    }
//    //
//    //   Calibrate
//    //
//    typedef Eigen::Triplet<double> T;
//    std::vector<T> tripletList;
//    tripletList.reserve(info.nchan);
//    for(k = 0; k < info.nchan; ++k)
//        tripletList.push_back(T(k, k, info.chs[k].cal));

//    SparseMatrix<double> cals(info.nchan, info.nchan);
//    cals.setFromTriplets(tripletList.begin(), tripletList.end());

//    all = cals * all;
//    //
//    //   Put it all together
//    //
//    data.info = info;

////        if(data.evoked)
////            delete data.evoked;
//    data.evoked.append(FiffEvokedData::SDPtr(new FiffEvokedData()));
//    data.evoked[0]->aspect_kind = aspect_kind;
//    data.evoked[0]->is_smsh     = is_smsh(0,setno);
//    if (nave != -1)
//        data.evoked[0]->nave = nave;
//    else
//        data.evoked[0]->nave  = 1;

//    data.evoked[0]->first = first;
//    data.evoked[0]->last  = last;
//    if (!comment.isEmpty())
//        data.evoked[0]->comment = comment;
//    //
//    //   Times for convenience and the actual epoch data
//    //

//    data.evoked[0]->times = MatrixXd(1, last-first+1);

//    for (k = 0; k < data.evoked[0]->times.cols(); ++k)
//        data.evoked[0]->times(0, k) = ((float)(first+k)) / info.sfreq;

////    if(data.evoked[0].epochs)
////        delete data.evoked[0]->epochs;
//    data.evoked[0]->epochs = all;

//    if(t_pStream)
//        delete t_pStream;

//    return true;
}
