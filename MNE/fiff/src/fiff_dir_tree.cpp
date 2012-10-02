//=============================================================================================================
/**
* @file     fiff_dir_tree.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the FiffDirTree Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../include/fiff_dir_tree.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDirTree::FiffDirTree()
: block(-1)
, nent(-1)
, nent_tree(-1)
, nchild(-1)
{
}


//*************************************************************************************************************

FiffDirTree::~FiffDirTree()
{
    QList<FiffDirTree*>::iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        if (*i)
            delete *i;
}


//*************************************************************************************************************

bool FiffDirTree::copy_tree(FiffFile* fidin, FiffId& in_id, QList<FiffDirTree*>& nodes, FiffFile* fidout)
{
    if(nodes.size() <= 0)
        return false;

    qint32 k, p;

    for(k = 0; k < nodes.size(); ++k)
    {
        fidout->start_block(nodes[k]->block);//8
        if (nodes[k]->id.version != -1)
        {
            if (in_id.version != -1)
                fidout->write_id(FIFF_PARENT_FILE_ID, in_id);//9

            fidout->write_id(FIFF_BLOCK_ID);//10
            fidout->write_id(FIFF_PARENT_BLOCK_ID, nodes[k]->id);//11
        }
        for (p = 0; p < nodes[k]->nent; ++p)
        {
            //
            //   Do not copy these tags
            //
            if(nodes[k]->dir[p].kind == FIFF_BLOCK_ID || nodes[k]->dir[p].kind == FIFF_PARENT_BLOCK_ID || nodes[k]->dir[p].kind == FIFF_PARENT_FILE_ID)
                continue;

            //
            //   Read and write tags, pass data through transparently
            //
            if (!fidin->seek(nodes[k]->dir[p].pos)) //fseek(fidin, nodes(k).dir(p).pos, 'bof') == -1
            {
                printf("Could not seek to the tag\n");
                return false;
            }

            FiffTag tag;

            QDataStream in(fidin);
            in.setByteOrder(QDataStream::BigEndian);

            in >> tag.kind;// = fread(fidin, 1, 'int32');
            in >> tag.type;// = fread(fidin, 1, 'uint32');
            in >> tag.size;// = fread(fidin, 1, 'int32');
            in >> tag.next;// = fread(fidin, 1, 'int32');


            if (tag.size > 0)
            {
                if (tag.data == NULL)
                    tag.data = malloc(tag.size + ((tag.type == FIFFT_STRING) ? 1 : 0));
                else
                    tag.data = realloc(tag.data,tag.size + ((tag.type == FIFFT_STRING) ? 1 : 0));

                if (tag.data == NULL) {
                    printf("fiff_read_tag: memory allocation failed.\n");//consider throw
                    return false;
                }
                char *t_pCharData = static_cast< char* >(tag.data);
                in.readRawData(t_pCharData, tag.size);
                if (tag.type == FIFFT_STRING)
                    t_pCharData[tag.size] = NULL;//make sure that char ends with NULL
                FiffTag::convert_tag_data(&tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
            } //tag.data = fread(fidin, tag.size, 'uchar');


            QDataStream out(fidout);
            out.setByteOrder(QDataStream::BigEndian);

            out << (qint32)tag.kind;
            out << (qint32)tag.type;
            out << (qint32)tag.size;
            out << (qint32)FIFFV_NEXT_SEQ;

            out.writeRawData(static_cast< const char* >(tag.data),tag.size);
        }
        for(p = 0; p < nodes[k]->nchild; ++p)
        {
            QList<FiffDirTree*> childList;
            childList << nodes[k]->children[p];
            FiffDirTree::copy_tree(fidin, in_id, childList, fidout);
        }
        fidout->end_block(nodes[k]->block);
    }
    return true;
}


//*************************************************************************************************************

qint32 FiffDirTree::make_dir_tree(FiffFile* p_pFile, QList<FiffDirEntry>* p_pDir, FiffDirTree*& p_pTree, qint32 start)
{
    if (p_pTree != NULL)
        delete p_pTree;
    p_pTree = new FiffDirTree();

    FIFFLIB::FiffTag* t_pTag = NULL;

    qint32 block;
    if(p_pDir->at(start).kind == FIFF_BLOCK_START)
    {
        FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(start).pos);
        block = *t_pTag->toInt();
    }
    else
    {
        block = 0;
    }

//    qDebug() << "start { " << p_pTree->block;

    int current = start;

    p_pTree->block = block;
    p_pTree->nent = 0;
    p_pTree->nchild = 0;

    while (current < p_pDir->size())
    {
        if (p_pDir->at(current).kind == FIFF_BLOCK_START)
        {
            if (current != start)
            {
                FiffDirTree* p_pChildTree = NULL;
                current = FiffDirTree::make_dir_tree(p_pFile,p_pDir,p_pChildTree, current);
                ++p_pTree->nchild;
                p_pTree->children.append(p_pChildTree);
            }
        }
        else if(p_pDir->at(current).kind == FIFF_BLOCK_END)
        {
            FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(start).pos);
            if (*t_pTag->toInt() == p_pTree->block)
                break;
        }
        else
        {
            ++p_pTree->nent;
            p_pTree->dir.append(p_pDir->at(current));

            //
            //  Add the id information if available
            //
            if (block == 0)
            {
                if (p_pDir->at(current).kind == FIFF_FILE_ID)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(current).pos);
                    p_pTree->id = t_pTag->toFiffID();
                }
            }
            else
            {
                if (p_pDir->at(current).kind == FIFF_BLOCK_ID)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(current).pos);
                    p_pTree->id = t_pTag->toFiffID();
                }
                else if (p_pDir->at(current).kind == FIFF_PARENT_BLOCK_ID)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(current).pos);
                    p_pTree->parent_id = t_pTag->toFiffID();
                }
            }
        }
        ++current;
    }

    //
    // Eliminate the empty directory
    //
    if(p_pTree->nent == 0)
        p_pTree->dir.clear();

//    qDebug() << "block =" << p_pTree->block << "nent =" << p_pTree->nent << "nchild =" << p_pTree->nchild;
//    qDebug() << "end } " << block;

    delete t_pTag;

    return current;
}


//*************************************************************************************************************

QList<FiffDirTree*> FiffDirTree::dir_tree_find(fiff_int_t kind)
{
    QList<FiffDirTree*> nodes;
    if(this->block == kind)
        nodes.append(this);

    QList<FiffDirTree*>::iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        nodes.append((*i)->dir_tree_find(kind));

    return nodes;
}


//*************************************************************************************************************

bool FiffDirTree::find_tag(FiffFile* p_pFile, fiff_int_t findkind, FiffTag*& p_pTag)
{
    for (int p = 0; p < this->nent; ++p)
    {
       if (this->dir.at(p).kind == findkind)
       {
          FiffTag::read_tag(p_pFile,p_pTag,this->dir.at(p).pos);
          return true;
       }
    }
    if (p_pTag != NULL)
    {
        delete p_pTag;
        p_pTag = NULL;
    }
    return false;
}


//*************************************************************************************************************

bool FiffDirTree::has_tag(fiff_int_t findkind)
{
    for(int p = 0; p < this->nent; ++p)
        if(this->dir.at(p).kind == findkind)
            return true;
   return false;
}


//*************************************************************************************************************

QStringList FiffDirTree::read_bad_channels(FiffFile* p_pFile)
{
    QList<FiffDirTree*> node = this->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    FIFFLIB::FiffTag* t_pTag = NULL;

    QStringList bads;

    if (node.size() > 0)
        if(node.at(0)->find_tag(p_pFile, FIFF_MNE_CH_NAME_LIST, t_pTag))
            bads = FiffDirTree::split_name_list(t_pTag->toString());

    return bads;
}


//*************************************************************************************************************

QList<FiffCtfComp*> FiffDirTree::read_ctf_comp(FiffFile* p_pFile, QList<FiffChInfo>& chs)
{
    FiffDirTree* p_pNode = this;

    QList<FiffCtfComp*> compdata;
    QList<FiffDirTree*> t_qListComps = p_pNode->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);

    qint32 i, k, p, col, row;
    fiff_int_t kind, pos;
    FiffTag* t_pTag = NULL;
    for (k = 0; k < t_qListComps.size(); ++k)
    {
        FiffDirTree* node = t_qListComps.at(k);
        //
        //   Read the data we need
        //
        FiffNamedMatrix* mat = NULL;
        node->read_named_matrix(p_pFile, FIFF_MNE_CTF_COMP_DATA, mat);
        for(p = 0; p < node->nent; ++p)
        {
            kind = node->dir.at(p).kind;
            pos  = node->dir.at(p).pos;
            if (kind == FIFF_MNE_CTF_COMP_KIND)
            {
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                break;
            }
        }
        if (!t_pTag)
        {
            printf("Compensation type not found\n");
            return compdata;
        }
        //
        //   Get the compensation kind and map it to a simple number
        //
        FiffCtfComp* one = new FiffCtfComp();
        one->ctfkind = *t_pTag->toInt();
        delete t_pTag;
        t_pTag = NULL;

        one->kind   = -1;
        if (one->ctfkind == 1194410578) //hex2dec('47314252')
            one->kind = 1;
        else if (one->ctfkind == 1194476114) //hex2dec('47324252')
            one->kind = 2;
        else if (one->ctfkind == 1194541650) //hex2dec('47334252')
            one->kind = 3;
        else
            one->kind = one->ctfkind;

        for (p = 0; p < node->nent; ++p)
        {
            kind = node->dir.at(p).kind;
            pos  = node->dir.at(p).pos;
            if (kind == FIFF_MNE_CTF_COMP_CALIBRATED)
            {
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                break;
            }
        }
        bool calibrated;
        if (!t_pTag)
            calibrated = false;
        else
            calibrated = (bool)*t_pTag->toInt();

        one->save_calibrated = calibrated;
        one->rowcals = MatrixXf::Ones(1,mat->data.rows());//ones(1,size(mat.data,1));
        one->colcals = MatrixXf::Ones(1,mat->data.cols());//ones(1,size(mat.data,2));
        if (!calibrated)
        {
            //
            //   Calibrate...
            //
            //
            //   Do the columns first
            //
            QStringList ch_names;
            for (p  = 0; p < chs.size(); ++p)
                ch_names.append(chs.at(p).ch_name);

            qint32 count;
            MatrixXf col_cals(mat->data.cols(), 1);
            col_cals.setZero();
            for (col = 0; col < mat->data.cols(); ++col)
            {
                count = 0;
                for (i = 0; i < ch_names.size(); ++i)
                {
                    if (QString::compare(mat->col_names.at(col),ch_names.at(i)) == 0)
                    {
                        count += 1;
                        p = i;
                    }
                }
                if (count == 0)
                {
                    printf("Channel %s is not available in data",mat->col_names.at(col).toUtf8().constData());
                    delete t_pTag;
                    return compdata;
                }
                else if (count > 1)
                {
                    printf("Ambiguous channel %s",mat->col_names.at(col).toUtf8().constData());
                    delete t_pTag;
                    return compdata;
                }
                col_cals(col,0) = 1.0f/(chs.at(p).range*chs.at(p).cal);
            }
            //
            //    Then the rows
            //
            MatrixXf row_cals(mat->data.rows(), 1);
            row_cals.setZero();
            for (row = 0; row < mat->data.rows(); ++row)
            {
                count = 0;
                for (i = 0; i < ch_names.size(); ++i)
                {
                    if (QString::compare(mat->row_names.at(row),ch_names.at(i)) == 0)
                    {
                        count += 1;
                        p = i;
                    }
                }

                if (count == 0)
                {
                    printf("Channel %s is not available in data",mat->row_names.at(row).toUtf8().constData());
                    delete t_pTag;
                    return compdata;
                }
                else if (count > 1)
                {
                    printf("Ambiguous channel %s",mat->row_names.at(row).toUtf8().constData());
                    delete t_pTag;
                    return compdata;
                }

                row_cals(row, 0) = chs.at(p).range*chs.at(p).cal;
            }
            mat->data            = row_cals.asDiagonal()*mat->data*col_cals.asDiagonal();
            one->rowcals         = row_cals;
            one->colcals         = col_cals;
        }
        one->data       = mat;
        compdata.append(one);
    }

    if (compdata.size() > 0)
        printf("\tRead %d compensation matrices\n",compdata.size());

    if(t_pTag)
        delete t_pTag;
    return compdata;
}


//*************************************************************************************************************

QStringList FiffDirTree::split_name_list(QString p_sNameList)
{
    return p_sNameList.split(":");
}


//*************************************************************************************************************

FiffDirTree* FiffDirTree::read_meas_info(FiffFile* p_pFile, FiffInfo*& info)
{
    FiffDirTree* p_pTree = this;

    if (info)
        delete info;
    info = NULL;
    //
    //   Find the desired blocks
    //
    QList<FiffDirTree*> meas = p_pTree->dir_tree_find(FIFFB_MEAS);

    if (meas.size() == 0)
    {
        printf("Could not find measurement data\n");
        return NULL;
    }
    //
    QList<FiffDirTree*> meas_info = meas.at(0)->dir_tree_find(FIFFB_MEAS_INFO);
    if (meas_info.count() == 0)
    {
        printf("Could not find measurement info\n");
        delete meas[0];
        return NULL;
    }
    //
    //   Read measurement info
    //
    FiffTag* t_pTag = NULL;

    fiff_int_t nchan = -1;
    float sfreq = -1.0f;
    QList<FiffChInfo> chs;
    float lowpass = -1.0f;
    float highpass = -1.0f;

    FiffChInfo t_chInfo;

    FiffCoordTrans cand;
    FiffCoordTrans dev_head_t;
    FiffCoordTrans ctf_head_t;

    fiff_int_t meas_date[2];
    meas_date[0] = -1;
    meas_date[1] = -1;

    fiff_int_t kind = -1;
    fiff_int_t pos = -1;

    for (qint32 k = 0; k < meas_info.at(0)->nent; ++k)
    {
        kind = meas_info.at(0)->dir.at(k).kind;
        pos  = meas_info.at(0)->dir.at(k).pos;
        switch (kind)
        {
            case FIFF_NCHAN:
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                nchan = *t_pTag->toInt();
                break;
            case FIFF_SFREQ:
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                sfreq = *t_pTag->toFloat();
                break;
            case FIFF_CH_INFO:
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                chs.append( t_pTag->toChInfo() );
                break;
            case FIFF_LOWPASS:
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                lowpass = *t_pTag->toFloat();
                break;
            case FIFF_HIGHPASS:
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                highpass = *t_pTag->toFloat();
                break;
            case FIFF_MEAS_DATE:
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                meas_date[0] = t_pTag->toInt()[0];
                meas_date[1] = t_pTag->toInt()[1];
                break;
            case FIFF_COORD_TRANS:
                //ToDo: This has to be debugged!!
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                cand = t_pTag->toCoordTrans();
                if(cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
                    dev_head_t = cand;
                else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
                    ctf_head_t = cand;
                break;
        }
    }
    //
    //   Check that we have everything we need
    //
    if (nchan < 0)
    {
        printf("Number of channels in not defined\n");
        delete meas[0];
        return NULL;
    }
    if (sfreq < 0)
    {
        printf("Sampling frequency is not defined\n");
        delete meas[0];
        return NULL;
    }
    if (chs.size() == 0)
    {
        printf("Channel information not defined\n");
        delete meas[0];
        return NULL;
    }
    if (chs.size() != nchan)
    {
        printf("Incorrect number of channel definitions found\n");
        delete meas[0];
        return NULL;
    }

    if ((dev_head_t.from == -1) || (ctf_head_t.from == -1)) //if isempty(dev_head_t) || isempty(ctf_head_t)
    {
        QList<FiffDirTree*> hpi_result = meas_info.at(0)->dir_tree_find(FIFFB_HPI_RESULT);
        if (hpi_result.size() == 1)
        {
            for( qint32 k = 0; k < hpi_result.at(0)->nent; ++k)
            {
                kind = hpi_result.at(0)->dir.at(k).kind;
                pos  = hpi_result.at(0)->dir.at(k).pos;
                if (kind == FIFF_COORD_TRANS)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    cand = t_pTag->toCoordTrans();
                    if (cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
                        dev_head_t = cand;
                    else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
                        ctf_head_t = cand;
                }
            }
        }
    }
    //
    //   Locate the Polhemus data
    //
    QList<FiffDirTree*> isotrak = meas_info.at(0)->dir_tree_find(FIFFB_ISOTRAK);

    QList<fiff_dig_point_t> dig;// = struct('kind',{},'ident',{},'r',{},'coord_frame',{});
    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    qint32 k = 0;

    if (isotrak.size() == 1)
    {
        for (k = 0; k < isotrak.at(0)->nent; ++k)
        {
            kind = isotrak.at(0)->dir.at(k).kind;
            pos  = isotrak.at(0)->dir.at(k).pos;
            if (kind == FIFF_DIG_POINT)
            {
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                dig.append(t_pTag->toDigPoint());
            }
            else
            {
                if (kind == FIFF_MNE_COORD_FRAME)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                }
                else if (kind == FIFF_COORD_TRANS)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for(k = 0; k < dig.size(); ++k)
        dig[k].coord_frame = coord_frame;

    if (dig_trans.from != -1) //if exist('dig_trans','var')
    {
        if (dig_trans.from != coord_frame && dig_trans.to != coord_frame)
        {
            FiffCoordTrans tmpEmptyTrans;
            dig_trans = tmpEmptyTrans; //clear('dig_trans');

        }
    }

    //
    //   Locate the acquisition information
    //
    QList<FiffDirTree*> acqpars = meas_info.at(0)->dir_tree_find(FIFFB_DACQ_PARS);
    QString acq_pars;
    QString acq_stim;
    if (acqpars.size() == 1)
    {
        for( k = 0; k < acqpars.at(0)->nent; ++k)
        {
            kind = acqpars.at(0)->dir.at(k).kind;
            pos  = acqpars.at(0)->dir.at(k).pos;
            if (kind == FIFF_DACQ_PARS)
            {
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                acq_pars = t_pTag->toString();
            }
            else if (kind == FIFF_DACQ_STIM)
            {
                FiffTag::read_tag(p_pFile, t_pTag, pos);
                acq_stim = t_pTag->toString();
            }
        }
    }
    //
    //   Load the SSP data
    //
    QList<FiffProj*> projs = meas_info.at(0)->read_proj(p_pFile);//ToDo Member Function
    //
    //   Load the CTF compensation data
    //
    QList<FiffCtfComp*> comps = meas_info.at(0)->read_ctf_comp(p_pFile, chs);//ToDo Member Function
    //
    //   Load the bad channel list
    //
    QStringList bads = p_pTree->read_bad_channels(p_pFile);
    //
    //   Put the data together
    //
    info = new FiffInfo();
    if (p_pTree->id.version != -1)
        info->file_id = p_pTree->id;
    else
        info->file_id.version = -1;

    //
    //  Make the most appropriate selection for the measurement id
    //
    if (meas_info.at(0)->parent_id.version == -1)
    {
        if (meas_info.at(0)->id.version == -1)
        {
            if (meas.at(0)->id.version == -1)
            {
                if (meas.at(0)->parent_id.version == -1)
                    info->meas_id = info->file_id;
                else
                    info->meas_id = meas.at(0)->parent_id;
            }
            else
                info->meas_id = meas.at(0)->id;
        }
        else
            info->meas_id = meas_info.at(0)->id;
    }
    else
        info->meas_id = meas_info.at(0)->parent_id;

    if (meas_date[0] == -1)
    {
        info->meas_date[0] = info->meas_id.time.secs;
        info->meas_date[1] = info->meas_id.time.usecs;
    }
    else
    {
        info->meas_date[0] = meas_date[0];
        info->meas_date[1] = meas_date[1];
    }

    info->nchan  = nchan;
    info->sfreq  = sfreq;
    if (highpass != -1.0f)
        info->highpass = highpass;
    else
        info->highpass = 0.0f;

    if (lowpass != -1.0f)
        info->lowpass = lowpass;
    else
        info->lowpass = info->sfreq/2.0;

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    info->chs = chs;
    for (qint32 c = 0; c < info->nchan; ++c)
        info->ch_names << info->chs.at(c).ch_name;

    //
    //  Add the coordinate transformations
    //
    info->dev_head_t = dev_head_t;
    info->ctf_head_t = ctf_head_t;
    if ((info->dev_head_t.from != -1) && (info->ctf_head_t.from != -1)) //~isempty(info.dev_head_t) && ~isempty(info.ctf_head_t)
    {
        info->dev_ctf_t    = info->dev_head_t;
        info->dev_ctf_t.to = info->ctf_head_t.from;
        info->dev_ctf_t.trans = ctf_head_t.trans.inverse()*info->dev_ctf_t.trans;
    }
    else
        info->dev_ctf_t.from = -1;

    //
    //   All kinds of auxliary stuff
    //
    info->dig   = dig;
    if (dig_trans.from != -1)
        info->dig_trans = dig_trans;

    info->bads  = bads;
    info->projs = projs;
    info->comps = comps;
    info->acq_pars = acq_pars;
    info->acq_stim = acq_stim;

    return meas[0];
}


//*************************************************************************************************************

bool FiffDirTree::read_named_matrix(FiffFile* p_pFile, fiff_int_t matkind, FiffNamedMatrix*& mat)
{
    if (mat != NULL)
        delete mat;
    mat = new FiffNamedMatrix();

    FiffDirTree* node = this;
    //
    //   Descend one level if necessary
    //
    bool found_it = false;
    if (node->block != FIFFB_MNE_NAMED_MATRIX)
    {
        for (int k = 0; k < node->nchild; ++k)
        {
            if (node->children.at(k)->block == FIFFB_MNE_NAMED_MATRIX)
            {
                if(node->children.at(k)->has_tag(matkind))
                {
                    node = node->children.at(k);
                    found_it = true;
                    break;
                }
            }
       }
       if (!found_it)
       {
          printf("Fiff::read_named_matrix: Desired named matrix (kind = %d) not available\n",matkind);
          return false;
       }
    }
    else
    {
        if (!node->has_tag(matkind))
        {
            printf("Desired named matrix (kind = %d) not available",matkind);
            return false;
        }
    }

    FIFFLIB::FiffTag* t_pTag = NULL;
    //
    //   Read everything we need
    //
    if(!node->find_tag(p_pFile, matkind, t_pTag))
    {
        printf("Matrix data missing.\n");
        return false;
    }
    else
    {
        //qDebug() << "Is Matrix" << t_pTag->isMatrix() << "Special Type:" << t_pTag->getType();
        mat->data = t_pTag->toFloatMatrix().transpose();
    }

    mat->nrow = mat->data.rows();
    mat->ncol = mat->data.cols();

    if(node->find_tag(p_pFile, FIFF_MNE_NROW, t_pTag))
        if (*t_pTag->toInt() != mat->nrow)
        {
            printf("Number of rows in matrix data and FIFF_MNE_NROW tag do not match");
            return false;
        }
    if(node->find_tag(p_pFile, FIFF_MNE_NCOL, t_pTag))
        if (*t_pTag->toInt() != mat->ncol)
        {
            printf("Number of columns in matrix data and FIFF_MNE_NCOL tag do not match");
            return false;
        }

    QString row_names;
    if(node->find_tag(p_pFile, FIFF_MNE_ROW_NAMES, t_pTag))
        row_names = t_pTag->toString();

    QString col_names;
    if(node->find_tag(p_pFile, FIFF_MNE_COL_NAMES, t_pTag))
        col_names = t_pTag->toString();

    //
    //   Put it together
    //
    if (!row_names.isEmpty())
        mat->row_names = FiffDirTree::split_name_list(row_names);

    if (!col_names.isEmpty())
        mat->col_names = FiffDirTree::split_name_list(col_names);

    return true;
}


//*************************************************************************************************************

QList<FiffProj*> FiffDirTree::read_proj(FiffFile* p_pFile)
{
    FiffDirTree* p_pNode = this;

    QList<FiffProj*> projdata;// = struct('kind',{},'active',{},'desc',{},'data',{});
    //
    //   Locate the projection data
    //
    QList<FiffDirTree*> t_qListNodes = p_pNode->dir_tree_find(FIFFB_PROJ);
    if ( t_qListNodes.size() == 0 )
        return projdata;


    FIFFLIB::FiffTag* t_pTag = NULL;
    t_qListNodes.at(0)->find_tag(p_pFile, FIFF_NCHAN, t_pTag);
    fiff_int_t global_nchan;
    if (t_pTag)
        global_nchan = *t_pTag->toInt();


    fiff_int_t nchan;
    QList<FiffDirTree*> t_qListItems = t_qListNodes.at(0)->dir_tree_find(FIFFB_PROJ_ITEM);
    for ( qint32 i = 0; i < t_qListItems.size(); ++i)
    {
        //
        //   Find all desired tags in one item
        //
        FiffDirTree* t_pFiffDirTreeItem = t_qListItems[i];
        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_NCHAN, t_pTag);
        if (t_pTag)
            nchan = *t_pTag->toInt();
        else
            nchan = global_nchan;

        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_DESCRIPTION, t_pTag);
        QString desc; // maybe, in some cases this has to be a struct.
        if (t_pTag)
        {
            qDebug() << "read_proj: this has to be debugged";
            desc = t_pTag->toString();
        }
        else
        {
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_NAME, t_pTag);
            if (t_pTag)
                desc = t_pTag->toString();
            else
            {
                printf("Projection item description missing\n");
                return projdata;
            }
        }
//            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag);
//            QString namelist;
//            if (t_pTag)
//            {
//                namelist = t_pTag->toString();
//            }
//            else
//            {
//                printf("Projection item channel list missing\n");
//                return projdata;
//            }
        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_KIND, t_pTag);
        fiff_int_t kind;
        if (t_pTag)
        {
            kind = *t_pTag->toInt();
        }
        else
        {
            printf("Projection item kind missing");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_NVEC, t_pTag);
        fiff_int_t nvec;
        if (t_pTag)
        {
            nvec = *t_pTag->toInt();
        }
        else
        {
            printf("Number of projection vectors not specified\n");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag);
        QStringList names;
        if (t_pTag)
        {
            names = split_name_list(t_pTag->toString());
        }
        else
        {
            printf("Projection item channel list missing\n");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_VECTORS, t_pTag);
        MatrixXf data;
        if (t_pTag)
        {
            data = t_pTag->toFloatMatrix().transpose();
        }
        else
        {
            printf("Projection item data missing\n");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_MNE_PROJ_ITEM_ACTIVE, t_pTag);
        bool active;
        if (t_pTag)
            active = *t_pTag->toInt();
        else
            active = false;

        if (data.cols() != names.size())
        {
            printf("Number of channel names does not match the size of data matrix\n");
            return projdata;
        }



        //
        //   create a named matrix for the data
        //
        QStringList defaultList;
        FiffNamedMatrix* t_fiffNamedMatrix = new FiffNamedMatrix(nvec, nchan, defaultList, names, data);

        FiffProj* one = new FiffProj(kind, active, desc, t_fiffNamedMatrix);
        //
        projdata.append(one);
    }

    if (projdata.size() > 0)
    {
        printf("\tRead a total of %d projection items:\n", projdata.size());
        for(qint32 k = 0; k < projdata.size(); ++k)
        {
            printf("\t\t%s (%d x %d)",projdata.at(k)->desc.toUtf8().constData(), projdata.at(k)->data->nrow, projdata.at(k)->data->ncol);
            if (projdata.at(k)->active)
                printf(" active\n");
            else
                printf(" idle\n");
        }
    }

    if (t_pTag)
        delete t_pTag;

    return projdata;
}
