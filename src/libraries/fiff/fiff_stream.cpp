//=============================================================================================================
/**
 * @file     fiff_stream.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffStream Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"
#include "fiff_ctf_comp.h"
#include "fiff_info.h"
#include "fiff_info_base.h"
#include "fiff_raw_data.h"
#include "fiff_cov.h"
#include "fiff_coord_trans.h"
#include "fiff_ch_info.h"
#include "fiff_ch_pos.h"
#include "fiff_dig_point.h"
#include "fiff_id.h"
#include "c/fiff_digitizer_data.h"
#include "fiff_dig_point.h"

#include <utils/mnemath.h>
#include <utils/ioutils.h>

#define MALLOC_54(x,t) (t *)malloc((x)*sizeof(t))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include <iostream>
#include <time.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/LU>
#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTcpSocket>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStream::FiffStream(QIODevice *p_pIODevice)
: QDataStream(p_pIODevice)
{
    this->setFloatingPointPrecision(QDataStream::SinglePrecision);
    this->setByteOrder(QDataStream::BigEndian);
    this->setVersion(QDataStream::Qt_5_0);
}

//=============================================================================================================

FiffStream::FiffStream(QByteArray * a,
                       QIODevice::OpenMode mode)
: QDataStream(a, mode)
{
    this->setFloatingPointPrecision(QDataStream::SinglePrecision);
    this->setByteOrder(QDataStream::BigEndian);
    this->setVersion(QDataStream::Qt_5_0);
}

//=============================================================================================================

QString FiffStream::streamName()
{
    QFile* t_pFile = qobject_cast<QFile*>(this->device());
    QString p_sFileName;
    if(t_pFile)
        p_sFileName = t_pFile->fileName();
    else
        p_sFileName = QString("TCPSocket");

    return p_sFileName;
}

//=============================================================================================================

FiffId FiffStream::id() const
{
    return m_id;
}

//=============================================================================================================

QList<FiffDirEntry::SPtr>& FiffStream::dir()
{
    return m_dir;
}

//=============================================================================================================

const QList<FiffDirEntry::SPtr>& FiffStream::dir() const
{
    return m_dir;
}

//=============================================================================================================

int FiffStream::nent() const
{
    return m_dir.size();
}

//=============================================================================================================

const FiffDirNode::SPtr& FiffStream::dirtree() const
{
    return m_dirtree;
}

//=============================================================================================================

fiff_long_t FiffStream::end_block(fiff_int_t kind, fiff_int_t next)
{
    return this->write_int(FIFF_BLOCK_END,&kind,1,next);
}

//=============================================================================================================

void FiffStream::end_file()
{
    fiff_int_t datasize = 0;

     *this << (qint32)FIFF_NOP;
     *this << (qint32)FIFFT_VOID;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_NONE;
}

//=============================================================================================================

void FiffStream::finish_writing_raw()
{
    this->end_block(FIFFB_RAW_DATA);
    this->end_block(FIFFB_MEAS);
    this->end_file();
    this->close();
}

//=============================================================================================================

bool FiffStream::get_evoked_entries(const QList<FiffDirNode::SPtr> &evoked_node, QStringList &comments, QList<fiff_int_t> &aspect_kinds, QString &t)
{
    comments.clear();
    aspect_kinds.clear();
    QList<FiffDirNode::SPtr>::ConstIterator ev;

    FiffTag::SPtr t_pTag;
    qint32 kind, pos, k;

    for(ev = evoked_node.begin(); ev != evoked_node.end(); ++ev)
    {
        for(k = 0; k < (*ev)->nent(); ++k)
        {
            kind = (*ev)->dir[k]->kind;
            pos = (*ev)->dir[k]->pos;
            if (kind == FIFF_COMMENT)
            {
                this->read_tag(t_pTag,pos);
                comments.append(t_pTag->toString());
            }
        }
        FiffDirNode::SPtr my_aspect = (*ev)->dir_tree_find(FIFFB_ASPECT)[0];
        for(k = 0; k < my_aspect->nent(); ++k)
        {
            kind = my_aspect->dir[k]->kind;
            pos = my_aspect->dir[k]->pos;
            if (kind == FIFF_ASPECT_KIND)
            {
                this->read_tag(t_pTag,pos);
                aspect_kinds.append(*t_pTag->toInt());
            }
        }
    }

    if(comments.size() != aspect_kinds.size() || comments.size() == 0)
    {
        qWarning("Dataset names in FIF file could not be found.");
        return false;
    }

    t = QString();
    for(k = 0; k < aspect_kinds.size(); ++k)
    {
        t += QString("%1 - \"%2\" (").arg(k).arg(comments[k]);
        if(aspect_kinds[k] == FIFFV_ASPECT_AVERAGE)
            t += QString("FIFFV_ASPECT_AVERAGE)\n");
        else if(aspect_kinds[k] == FIFFV_ASPECT_STD_ERR)
            t += QString("FIFFV_ASPECT_STD_ERR)\n");
        else
            t += QString("unknown)\n");
    }

    return true;
}

//=============================================================================================================

bool FiffStream::open(QIODevice::OpenModeFlag mode)
{
    QString t_sFileName = this->streamName();
    FiffTag::SPtr t_pTag;

    /*
     * Try to open...
     */
    if (!this->device()->open(mode))
    {
        qCritical("Cannot open %s\n", t_sFileName.toUtf8().constData());//consider throw
        return false;
    }

    if(!check_beginning(t_pTag)) // Supposed to get the id already in the beginning - read once approach - for TCP/IP support
        return false;

    /*
     * Read id and directory pointer
     */
    if (t_pTag->kind != FIFF_FILE_ID) {
        qCritical("FIFF file should start with FIFF_FILE_ID!");//consider throw
        this->device()->close();
        return false;
    }
    m_id = t_pTag->toFiffID();

    this->read_tag(t_pTag);
    if (t_pTag->kind != FIFF_DIR_POINTER) {
        qWarning("Fiff::open: file does have a directory pointer");//consider throw
        this->device()->close();
        return false;
    }

    //
    //   Read or create the directory tree
    //
    qInfo("Creating tag directory for %s...", t_sFileName.toUtf8().constData());
    m_dir.clear();
    qint32 dirpos = *t_pTag->toInt();
    /*
     * Do we have a directory or not?
     */
    if (dirpos <= 0) {  /* Must do it in the hard way... */
        bool ok = false;
        m_dir = this->make_dir(&ok);
        if (!ok) {
          qCritical ("Could not create tag directory!");
          return false;
        }
    }
    else {              /* Just read the directory */
        if(!this->read_tag(t_pTag, dirpos)) {
            qCritical("Could not read the tag directory (file probably damaged)!");
            return false;
        }
        m_dir = t_pTag->toDirEntry();
    }

    /*
     * Check for a mistake
     */
    if (m_dir[m_dir.size()-2]->kind == FIFF_DIR) {
        m_dir.removeLast();
        m_dir[m_dir.size()-1]->kind = -1;
        m_dir[m_dir.size()-1]->type = -1;
        m_dir[m_dir.size()-1]->size = -1;
        m_dir[m_dir.size()-1]->pos  = -1;
    }

    //
    //   Create the directory tree structure
    //
    if((this->m_dirtree = this->make_subtree(m_dir)) == NULL)
        return false;
    else
        this->m_dirtree->parent.clear();

    //
    //   Back to the beginning
    //
    this->device()->seek(SEEK_SET); //fseek(fid,0,'bof');
    return true;
}

//=============================================================================================================

bool FiffStream::close()
{
    if(this->device()->isOpen())
        this->device()->close();

    return true;
}

//=============================================================================================================

FiffDirNode::SPtr FiffStream::make_subtree(QList<FiffDirEntry::SPtr> &dentry)
{
    FiffDirNode::SPtr defaultNode;
    FiffDirNode::SPtr node = FiffDirNode::SPtr(new FiffDirNode);
    FiffDirNode::SPtr child;
    FiffTag::SPtr t_pTag;
    QList<FiffDirEntry::SPtr> dir;
    qint32 current = 0;

    node->dir_tree    = dentry;
    node->nent_tree   = 1;
    node->parent      = FiffDirNode::SPtr();
    node->type = FIFFB_ROOT;

    if (dentry[current]->kind == FIFF_BLOCK_START) {
        if (!this->read_tag(t_pTag,dentry[current]->pos))
            return defaultNode;
        else
            node->type = *t_pTag->toInt();
    }
    else {
        node->id = this->id();
    }

    ++current;
    int level = 0;
    for (; current < dentry.size(); ++current) {
        ++node->nent_tree;
        if (dentry[current]->kind == FIFF_BLOCK_START) {
            level++;
            if (level == 1) {
                QList<FiffDirEntry::SPtr> sub_dentry = dentry.mid(current);
                if (!(child = this->make_subtree(sub_dentry)))
                    return defaultNode;
                child->parent = node;
                node->children.append(child);
            }
        }
        else if (dentry[current]->kind == FIFF_BLOCK_END) {
            level--;
            if (level < 0)
                break;
        }
        else if (dentry[current]->kind == -1)
            break;
        else if (level == 0) {
            /*
            * Take the node id from the parent block id,
            * block id, or file id. Let the block id
            * take precedence over parent block id and file id
            */
            if (((dentry[current]->kind == FIFF_PARENT_BLOCK_ID || dentry[current]->kind == FIFF_FILE_ID) && node->id.isEmpty()) || dentry[current]->kind == FIFF_BLOCK_ID) {
                if (!this->read_tag(t_pTag,dentry[current]->pos))
                    return defaultNode;
                node->id = t_pTag->toFiffID();
            }
            dir.append(FiffDirEntry::SPtr(new FiffDirEntry(*dentry[current])));//Memcopy necessary here - or is a pointer fine?
        }
    }
    /*
     * Strip unused entries
     */
    node->dir = dir;
    return node;
}

//=============================================================================================================

QStringList FiffStream::read_bad_channels(const FiffDirNode::SPtr& p_Node)
{
    QList<FiffDirNode::SPtr> node = p_Node->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    FiffTag::SPtr t_pTag;

    QStringList bads;

    if (node.size() > 0)
        if(node[0]->find_tag(this, FIFF_MNE_CH_NAME_LIST, t_pTag))
            bads = split_name_list(t_pTag->toString());

    return bads;
}

//=============================================================================================================

bool FiffStream::read_cov(const FiffDirNode::SPtr& p_Node, fiff_int_t cov_kind, FiffCov& p_covData)
{
    p_covData.clear();

    //
    //   Find all covariance matrices
    //
    QList<FiffDirNode::SPtr> covs = p_Node->dir_tree_find(FIFFB_MNE_COV);
    if (covs.size() == 0)
    {
        qWarning("No covariance matrices found");
        return false;
    }
    //
    //   Is any of the covariance matrices a noise covariance
    //
    qint32 p = 0;
    FiffTag::SPtr tag;
    bool success = false;
    fiff_int_t dim, nfree, nn;
    QStringList names;
    bool diagmat = false;
    VectorXd eig;
    MatrixXd eigvec;
    VectorXd cov_diag;
    MatrixXd cov;
    VectorXd cov_sparse;
    QStringList bads;
    for(p = 0; p < covs.size(); ++p)
    {
        success = covs[p]->find_tag(this, FIFF_MNE_COV_KIND, tag);
        if (success && *tag->toInt() == cov_kind)
        {
            FiffDirNode::SPtr current = covs[p];
            //
            //   Find all the necessary data
            //
            if (!current->find_tag(this, FIFF_MNE_COV_DIM, tag))
            {
                qWarning("Covariance matrix dimension not found.\n");
                return false;
            }
            dim = *tag->toInt();
            if (!current->find_tag(this, FIFF_MNE_COV_NFREE, tag))
                nfree = -1;
            else
                nfree = *tag->toInt();

            if (current->find_tag(this, FIFF_MNE_ROW_NAMES, tag))
            {
                names = FiffStream::split_name_list(tag->toString());
                if (names.size() != dim)
                {
                    qWarning("Number of names does not match covariance matrix dimension\n");
                    return false;
                }
            }
            if (!current->find_tag(this, FIFF_MNE_COV, tag))
            {
                if (!current->find_tag(this, FIFF_MNE_COV_DIAG, tag))
                {
                    qWarning("No covariance matrix data found\n");
                    return false;
                }
                else
                {
                    //
                    //   Diagonal is stored
                    //
                    if (tag->type == FIFFT_DOUBLE)
                    {
                        cov_diag = Map<VectorXd>(tag->toDouble(),dim);
                    }
                    else if (tag->type == FIFFT_FLOAT)
                    {
                        cov_diag = Map<VectorXf>(tag->toFloat(),dim).cast<double>();
                    }
                    else {
                        qCritical("Illegal data type for covariance matrix\n");
                        return false;
                    }

                    diagmat = true;
                    qInfo("\t%d x %d diagonal covariance (kind = %d) found.\n", dim, dim, cov_kind);
                }
            }
            else
            {
                VectorXd vals;
                nn = dim*(dim+1)/2;
                if (tag->type == FIFFT_DOUBLE)
                {
                    vals =  Map<VectorXd>(tag->toDouble(),nn);
                }
                else if (tag->type == FIFFT_FLOAT)
                {
                    vals = Map<VectorXf>(tag->toFloat(),nn).cast<double>();
                }
                else
                {
                    qDebug() << tag->getInfo();
                    return false;
                }

                if(!MNEMath::issparse(vals))
                {
                    //
                    //   Lower diagonal is stored
                    //
                    cov = MatrixXd::Zero(dim,dim);

                    // XXX : should remove for loops
                    qint32 q = 0;
                    for(qint32 j = 0; j < dim; ++j)
                    {
                        for(qint32 k = 0; k <= j; ++k)
                        {
                            cov(j,k) = vals(q);
                            ++q;
                        }
                    }
                    for(qint32 j = 0; j < dim; ++j)
                        for(qint32 k = j+1; k < dim; ++k)
                            cov(j,k) = cov(k,j);

                    diagmat = false;
                    qInfo("\t%d x %d full covariance (kind = %d) found.\n", dim, dim, cov_kind);

                }
                else
                {
                    diagmat = false;
                    qDebug() << "ToDo: FiffStream::read_cov - this needs to be debugged.\n";
                    cov = vals;
                    qInfo("\t%d x %d sparse covariance (kind = %d) found.\n", dim, dim, cov_kind);
                }
//MATLAB
//                    if ~issparse(tag.data)
//                        //
//                        //   Lower diagonal is stored
//                        //
//                        qDebug() << tag->getInfo();
//                        vals = tag.data;
//                        data = zeros(dim,dim);
//                        % XXX : should remove for loops
//                        q = 1;
//                        for j = 1:dim
//                            for k = 1:j
//                                data(j,k) = vals(q);
//                                q = q + 1;
//                            end
//                        end
//                        for j = 1:dim
//                            for k = j+1:dim
//                                data(j,k) = data(k,j);
//                            end
//                        end
//                        diagmat = false;
//                        fprintf('\t%d x %d full covariance (kind = %d) found.\n',dim,dim,cov_kind);
//                    else
//                        diagmat = false;
//                        data = tag.data;
//                        fprintf('\t%d x %d sparse covariance (kind = %d) found.\n',dim,dim,cov_kind);
//                    end
//MATLAB END
            }
            //
            //   Read the possibly precomputed decomposition
            //
            FiffTag::SPtr tag1;
            FiffTag::SPtr tag2;
            if (current->find_tag(this, FIFF_MNE_COV_EIGENVALUES, tag1) && current->find_tag(this, FIFF_MNE_COV_EIGENVECTORS, tag2))
            {
                eig = VectorXd(Map<VectorXd>(tag1->toDouble(),dim));
                eigvec = tag2->toFloatMatrix().cast<double>();
                eigvec.transposeInPlace();
            }
            //
            //   Read the projection operator
            //
            QList<FiffProj> projs = this->read_proj(current);
            //
            //   Read the bad channel list
            //
            bads = this->read_bad_channels(current);
            //
            //   Put it together
            //
            p_covData.clear();

            p_covData.kind   = cov_kind;
            p_covData.diag   = diagmat;
            p_covData.dim    = dim;
            p_covData.names  = names;

            if(cov_diag.size() > 0)
                p_covData.data   = cov_diag;
            else if(cov.size() > 0)
                p_covData.data   = cov;
            else if(cov_sparse.size() > 0)
                p_covData.data   = cov_sparse;

            p_covData.projs  = projs;
            p_covData.bads   = bads;
            p_covData.nfree  = nfree;
            p_covData.eig    = eig;
            p_covData.eigvec = eigvec;

            //
            return true;
        }
    }

    qInfo("Did not find the desired covariance matrix\n");
    return false;
}

//=============================================================================================================

QList<FiffCtfComp> FiffStream::read_ctf_comp(const FiffDirNode::SPtr& p_Node, const QList<FiffChInfo>& p_Chs)
{
    QList<FiffCtfComp> compdata;
    QList<FiffDirNode::SPtr> t_qListComps = p_Node->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);

    qint32 i, k, p, col, row;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;
    for (k = 0; k < t_qListComps.size(); ++k)
    {
        FiffDirNode::SPtr node = t_qListComps[k];
        //
        //   Read the data we need
        //
        FiffNamedMatrix::SDPtr mat(new FiffNamedMatrix());
        this->read_named_matrix(node, FIFF_MNE_CTF_COMP_DATA, *mat.data());
        for(p = 0; p < node->nent(); ++p)
        {
            kind = node->dir[p]->kind;
            pos  = node->dir[p]->pos;
            if (kind == FIFF_MNE_CTF_COMP_KIND)
            {
                this->read_tag(t_pTag, pos);
                break;
            }
        }
        if (!t_pTag)
        {
            qWarning("Compensation type not found\n");
            return compdata;
        }
        //
        //   Get the compensation kind and map it to a simple number
        //
        FiffCtfComp one;
        one.ctfkind = *t_pTag->toInt();

        t_pTag.clear();

        one.kind   = -1;
        if (one.ctfkind == 1194410578) //hex2dec('47314252')
            one.kind = 1;
        else if (one.ctfkind == 1194476114) //hex2dec('47324252')
            one.kind = 2;
        else if (one.ctfkind == 1194541650) //hex2dec('47334252')
            one.kind = 3;
        else if (one.ctfkind == 1194479433)
            one.kind = 4;
        else if (one.ctfkind == 1194544969)
            one.kind = 5;
        else
            one.kind = one.ctfkind;

        for (p = 0; p < node->nent(); ++p)
        {
            kind = node->dir[p]->kind;
            pos  = node->dir[p]->pos;
            if (kind == FIFF_MNE_CTF_COMP_CALIBRATED)
            {
                this->read_tag(t_pTag, pos);
                break;
            }
        }
        bool calibrated;
        if (!t_pTag)
            calibrated = false;
        else
            calibrated = (bool)*t_pTag->toInt();

        one.save_calibrated = calibrated;
        one.rowcals = MatrixXd::Ones(1,mat->data.rows());//ones(1,size(mat.data,1));
        one.colcals = MatrixXd::Ones(1,mat->data.cols());//ones(1,size(mat.data,2));
        if (!calibrated)
        {
            //
            //   Calibrate...
            //
            //
            //   Do the columns first
            //
            QStringList ch_names;
            for (p  = 0; p < p_Chs.size(); ++p)
                ch_names.append(p_Chs[p].ch_name);

            qint32 count;
            MatrixXd col_cals(mat->data.cols(), 1);
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
                    qWarning("Channel %s is not available in data",mat->col_names.at(col).toUtf8().constData());
                    return compdata;
                }
                else if (count > 1)
                {
                    qWarning("Ambiguous channel %s",mat->col_names.at(col).toUtf8().constData());
                    return compdata;
                }
                col_cals(col,0) = 1.0f/(p_Chs[p].range*p_Chs[p].cal);
            }
            //
            //    Then the rows
            //
            MatrixXd row_cals(mat->data.rows(), 1);
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
                    qWarning("Channel %s is not available in data",mat->row_names.at(row).toUtf8().constData());
                    return compdata;
                }
                else if (count > 1)
                {
                    qWarning("Ambiguous channel %s",mat->row_names.at(row).toUtf8().constData());
                    return compdata;
                }

                row_cals(row, 0) = p_Chs[p].range*p_Chs[p].cal;
            }
            mat->data           = row_cals.asDiagonal()* mat->data *col_cals.asDiagonal();
            one.rowcals         = row_cals;
            one.colcals         = col_cals;
        }
        one.data       = mat;
        compdata.append(one);
    }

    if (compdata.size() > 0)
        qInfo("\tRead %d compensation matrices\n",(int)compdata.size());

    return compdata;
}

//=============================================================================================================

bool FiffStream::read_digitizer_data(const FiffDirNode::SPtr& p_Node, FiffDigitizerData& p_digData)
{
    p_digData.coord_frame = FIFFV_COORD_UNKNOWN;
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;
    int npoint = 0;
    FiffTag::SPtr t_pTag;

    QList<FiffDirNode::SPtr> t_qListDigData = p_Node->dir_tree_find(FIFFB_ISOTRAK);

    //Check if digitizer data is available
    if(t_qListDigData.isEmpty()) {
        t_qListDigData = p_Node->dir_tree_find(FIFFB_MRI_SET);

        if(t_qListDigData.isEmpty()) {
            printf( "No Isotrak data found in %s", this->streamName().toLatin1().data());
            return false;
        } else {
            p_digData.coord_frame = FIFFV_COORD_MRI;
        }
    } else {
        p_digData.coord_frame = FIFFV_COORD_HEAD;
    }

    // Read actual data and store it
    for (int k = 0; k < t_qListDigData.first()->nent(); ++k) {
        kind = t_qListDigData.first()->dir[k]->kind;
        pos  = t_qListDigData.first()->dir[k]->pos;

        switch(kind) {
        case FIFF_DIG_POINT:
            this->read_tag(t_pTag, pos);
            p_digData.points.append(t_pTag->toDigPoint());
            break;
        case FIFF_MNE_COORD_FRAME:
            this->read_tag(t_pTag, pos);
            p_digData.coord_frame = *t_pTag->toInt();
            break;
        }
    }

    npoint = p_digData.points.size();

    if (npoint == 0) {
        printf( "No digitizer data in %s", this->streamName().toLatin1().data());
        return false;
    }

    for (auto& dig : p_digData.points){
        dig.coord_frame = p_digData.coord_frame;
    }

    //Add other information as default
    p_digData.filename    = this->streamName();
    p_digData.npoint      = npoint;

    for (int k = 0; k < p_digData.npoint; k++) {
        p_digData.active.append(TRUE);
        p_digData.discard.append(FALSE);
    }

    return true;
}

//=============================================================================================================

bool FiffStream::read_meas_info_base(const FiffDirNode::SPtr& p_Node, FiffInfoBase& p_InfoForward)
{
    p_InfoForward.clear();

    //
    //   Find the desired blocks
    //
    QList<FiffDirNode::SPtr> parent_meg = p_Node->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (parent_meg.size() == 0)
    {
        qWarning("No parent MEG information found in operator\n");
        return false;
    }

    FiffTag::SPtr t_pTag;

    QList<FiffChInfo> chs;
    FiffCoordTrans cand;
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;

    for (qint32 k = 0; k < parent_meg[0]->nent(); ++k)
    {
        kind = parent_meg[0]->dir[k]->kind;
        pos  = parent_meg[0]->dir[k]->pos;
        if (kind == FIFF_CH_INFO)
        {
            this->read_tag(t_pTag, pos);
            chs.append( t_pTag->toChInfo() );
        }
    }

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    p_InfoForward.chs = chs;
    for (qint32 c = 0; c < p_InfoForward.chs.size(); ++c)
        p_InfoForward.ch_names << p_InfoForward.chs[c].ch_name;

    p_InfoForward.nchan = chs.size();

    //
    //   Get the MEG device <-> head coordinate transformation
    //
    if(parent_meg[0]->find_tag(this, FIFF_COORD_TRANS, t_pTag))
    {
        cand = t_pTag->toCoordTrans();
        if(cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
            p_InfoForward.dev_head_t = cand;
        else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
            p_InfoForward.ctf_head_t = cand;
        else
            qWarning("MEG device/head coordinate transformation not found");
    }
    else
        qWarning("MEG/head coordinate transformation not found.\n");

    //
    //   Load the bad channel list
    //
    p_InfoForward.bads = this->read_bad_channels(p_Node);

    return true;
}

//=============================================================================================================

bool FiffStream::read_meas_info(const FiffDirNode::SPtr& p_Node, FiffInfo& info, FiffDirNode::SPtr& p_NodeInfo)
{
//    if (info)
//        delete info;
    info.clear();
    //
    //   Find the desired blocks
    //
    QList<FiffDirNode::SPtr> meas = p_Node->dir_tree_find(FIFFB_MEAS);

    if (meas.size() == 0)
    {
        qWarning("Could not find measurement data\n");
        return false;
    }
    //
    QList<FiffDirNode::SPtr> meas_info = meas[0]->dir_tree_find(FIFFB_MEAS_INFO);
    if (meas_info.count() == 0) {
        qWarning("Could not find measurement info\n");
//        delete meas[0];
        return false;
    }
    //
    //   Read measurement info
    //
    FiffTag::SPtr t_pTag;

    fiff_int_t nchan = -1;
    float sfreq = -1.0f;
    float linefreq = -1.0f;
    QList<FiffChInfo> chs;
    float lowpass = -1.0f;
    float highpass = -1.0f;

    int proj_id = 0;
    QString proj_name = "";
    QString xplotter_layout = "";

    QString utc_offset = "";
    fiff_int_t gantry_angle = -1;

    QString experimenter = "";
    QString description = "";

    FiffChInfo t_chInfo;

    FiffCoordTrans cand;// = NULL;
    FiffCoordTrans dev_head_t;// = NULL;
    FiffCoordTrans ctf_head_t;// = NULL;

    fiff_int_t meas_date[2];
    meas_date[0] = -1;
    meas_date[1] = -1;

    fiff_int_t kind = -1;
    fiff_int_t pos = -1;

    for (qint32 k = 0; k < meas_info[0]->nent(); ++k)
    {
        kind = meas_info[0]->dir[k]->kind;
        pos  = meas_info[0]->dir[k]->pos;
        switch (kind)
        {
            case FIFF_NCHAN:
                this->read_tag(t_pTag, pos);
                nchan = *t_pTag->toInt();
                break;
            case FIFF_SFREQ:
                this->read_tag(t_pTag, pos);
                sfreq = *t_pTag->toFloat();
                break;
            case FIFF_LINE_FREQ:
                this->read_tag(t_pTag, pos);
                linefreq = *t_pTag->toFloat();
                break;
            case FIFF_CH_INFO:
                this->read_tag(t_pTag, pos);
                chs.append( t_pTag->toChInfo() );
                break;
            case FIFF_LOWPASS:
                this->read_tag(t_pTag, pos);
                lowpass = *t_pTag->toFloat();
                break;
            case FIFF_HIGHPASS:
                this->read_tag(t_pTag, pos);
                highpass = *t_pTag->toFloat();
                break;
            case FIFF_MEAS_DATE:
                this->read_tag(t_pTag, pos);
                meas_date[0] = t_pTag->toInt()[0];
                meas_date[1] = t_pTag->toInt()[1];
                break;
            case FIFF_COORD_TRANS:
                //ToDo: This has to be debugged!!
                this->read_tag(t_pTag, pos);
                cand = t_pTag->toCoordTrans();
                if(cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
                    dev_head_t = cand;
                else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
                    ctf_head_t = cand;
                break;
            case FIFF_PROJ_ID:
                this->read_tag(t_pTag,pos);
                proj_id = *t_pTag->toInt();
                break;
            case FIFF_PROJ_NAME:
                this->read_tag(t_pTag,pos);
                proj_name = t_pTag->toString();
                break;
            case FIFF_XPLOTTER_LAYOUT:
                this->read_tag(t_pTag,pos);
                xplotter_layout = t_pTag->toString();
                break;
            case FIFF_EXPERIMENTER:
                this->read_tag(t_pTag,pos);
                experimenter = t_pTag->toString();
                break;
            case FIFF_DESCRIPTION:
                this->read_tag(t_pTag,pos);
                description = t_pTag->toString();
                break;
            case FIFF_GANTRY_ANGLE:
                this->read_tag(t_pTag,pos);
                gantry_angle = *t_pTag->toInt();
                break;
            case FIFF_UTC_OFFSET:
                this->read_tag(t_pTag,pos);
                utc_offset = t_pTag->toString();
                break;
        }
    }
    //
    //   Check that we have everything we need
    //
    if (nchan < 0)
    {
        qWarning("Number of channels in not defined\n");
        return false;
    }
    if (sfreq < 0)
    {
        qWarning("Sampling frequency is not defined\n");
        return false;
    }
    if (linefreq < 0)
    {
        qWarning("Line frequency is not defined\n");
    }
    if (chs.size() == 0)
    {
        qWarning("Channel information not defined\n");
        return false;
    }
    if (chs.size() != nchan)
    {
        qWarning("Incorrect number of channel definitions found\n");
        return false;
    }

    if (dev_head_t.isEmpty() || ctf_head_t.isEmpty())
    {
        QList<FiffDirNode::SPtr> hpi_result = meas_info[0]->dir_tree_find(FIFFB_HPI_RESULT);
        if (hpi_result.size() == 1)
        {
            for( qint32 k = 0; k < hpi_result[0]->nent(); ++k)
            {
                kind = hpi_result[0]->dir[k]->kind;
                pos  = hpi_result[0]->dir[k]->pos;
                if (kind == FIFF_COORD_TRANS)
                {
                    this->read_tag(t_pTag, pos);
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
    QList<FiffDirNode::SPtr> isotrak = meas_info[0]->dir_tree_find(FIFFB_ISOTRAK);

    QList<FiffDigPoint> dig;
    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    qint32 k = 0;

    if (isotrak.size() == 1)
    {
        for (k = 0; k < isotrak[0]->nent(); ++k)
        {
            kind = isotrak[0]->dir[k]->kind;
            pos  = isotrak[0]->dir[k]->pos;
            if (kind == FIFF_DIG_POINT)
            {
                this->read_tag(t_pTag, pos);
                dig.append(t_pTag->toDigPoint());
            }
            else
            {
                if (kind == FIFF_MNE_COORD_FRAME)
                {
                    this->read_tag(t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                }
                else if (kind == FIFF_COORD_TRANS)
                {
                    this->read_tag(t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for(k = 0; k < dig.size(); ++k)
        dig[k].coord_frame = coord_frame;

    if (!dig_trans.isEmpty()) //if exist('dig_trans','var')
        if (dig_trans.from != coord_frame && dig_trans.to != coord_frame)
            dig_trans.clear();

    //
    //   Locate the acquisition information
    //
    QList<FiffDirNode::SPtr> acqpars = meas_info[0]->dir_tree_find(FIFFB_DACQ_PARS);
    QString acq_pars;
    QString acq_stim;
    if (acqpars.size() == 1)
    {
        for( k = 0; k < acqpars[0]->nent(); ++k)
        {
            kind = acqpars[0]->dir[k]->kind;
            pos  = acqpars[0]->dir[k]->pos;
            if (kind == FIFF_DACQ_PARS)
            {
                this->read_tag(t_pTag, pos);
                acq_pars = t_pTag->toString();
            }
            else if (kind == FIFF_DACQ_STIM)
            {
                this->read_tag(t_pTag, pos);
                acq_stim = t_pTag->toString();
            }
        }
    }
    //
    //   Load the SSP data
    //
    QList<FiffProj> projs = this->read_proj(meas_info[0]);//ToDo Member Function
    //
    //   Load the CTF compensation data
    //
    QList<FiffCtfComp> comps = this->read_ctf_comp(meas_info[0], chs);//ToDo Member Function
    //
    //   Load the bad channel list
    //
    QStringList bads = this->read_bad_channels(p_Node);
    //
    //   Put the data together
    //
//    info = new FiffInfo();
    if (p_Node->id.version != -1)
        info.file_id = p_Node->id;
    else
        info.file_id.version = -1;

    //
    //  Make the most appropriate selection for the measurement id
    //
    if (meas_info[0]->parent_id.version == -1)
    {
        if (meas_info[0]->id.version == -1)
        {
            if (meas[0]->id.version == -1)
            {
                if (meas[0]->parent_id.version == -1)
                    info.meas_id = info.file_id;
                else
                    info.meas_id = meas[0]->parent_id;
            }
            else
                info.meas_id = meas[0]->id;
        }
        else
            info.meas_id = meas_info[0]->id;
    }
    else
        info.meas_id = meas_info[0]->parent_id;

    if (meas_date[0] == -1)
    {
        info.meas_date[0] = info.meas_id.time.secs;
        info.meas_date[1] = info.meas_id.time.usecs;
    }
    else
    {
        info.meas_date[0] = meas_date[0];
        info.meas_date[1] = meas_date[1];
    }

    info.nchan  = nchan;
    info.sfreq  = sfreq;
    info.linefreq = linefreq;

    if (highpass != -1.0f)
        info.highpass = highpass;
    else
        info.highpass = 0.0f;

    if (lowpass != -1.0f)
        info.lowpass = lowpass;
    else
        info.lowpass = info.sfreq/2.0;

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    info.chs = chs;
    for (qint32 c = 0; c < info.nchan; ++c)
        info.ch_names << info.chs[c].ch_name;

    //
    //  Add the coordinate transformations
    //
    info.dev_head_t = dev_head_t;
    info.ctf_head_t = ctf_head_t;
    if ((!info.dev_head_t.isEmpty()) && (!info.ctf_head_t.isEmpty())) //~isempty(info.dev_head_t) && ~isempty(info.ctf_head_t)
    {
        info.dev_ctf_t     = info.dev_head_t;
        info.dev_ctf_t.to = info.ctf_head_t.from;
        info.dev_ctf_t.trans = ctf_head_t.trans.inverse()*info.dev_ctf_t.trans;
    }
    else
        info.dev_ctf_t.clear();

    //
    //   All kinds of auxliary stuff
    //
    info.dig   = dig;
    if (!dig_trans.isEmpty())
        info.dig_trans = dig_trans;

    info.experimenter = experimenter;
    info.description = description;
    info.proj_id = proj_id;
    info.proj_name = proj_name;
    info.xplotter_layout = xplotter_layout;
    info.gantry_angle = gantry_angle;
    info.utc_offset = utc_offset;

    info.bads  = bads;
    info.projs = projs;
    info.comps = comps;
    info.acq_pars = acq_pars;
    info.acq_stim = acq_stim;

    p_NodeInfo = meas[0];

    return true;
}

//=============================================================================================================

bool FiffStream::read_named_matrix(const FiffDirNode::SPtr& p_Node, fiff_int_t matkind, FiffNamedMatrix& mat)
{
    mat.clear();

    FiffDirNode::SPtr node = p_Node;
    //
    //   Descend one level if necessary
    //
    bool found_it = false;
    if (node->type != FIFFB_MNE_NAMED_MATRIX)
    {
        for (int k = 0; k < node->nchild(); ++k)
        {
            if (node->children[k]->type == FIFFB_MNE_NAMED_MATRIX)
            {
                if(node->children[k]->has_tag(matkind))
                {
                    node = node->children[k];
                    found_it = true;
                    break;
                }
            }
       }
       if (!found_it)
       {
          qWarning("Fiff::read_named_matrix: Desired named matrix (kind = %d) not available\n",matkind);
          return false;
       }
    }
    else
    {
        if (!node->has_tag(matkind))
        {
            qWarning("Desired named matrix (kind = %d) not available",matkind);
            return false;
        }
    }

    FiffTag::SPtr t_pTag;
    //
    //   Read everything we need
    //
    if(!node->find_tag(this, matkind, t_pTag))
    {
        qWarning("Matrix data missing.\n");
        return false;
    }
    else
    {
        //qDebug() << "Is Matrix" << t_pTag->isMatrix() << "Special Type:" << t_pTag->getType();
        mat.data = t_pTag->toFloatMatrix().cast<double>();
        mat.data.transposeInPlace();
    }

    mat.nrow = mat.data.rows();
    mat.ncol = mat.data.cols();

    if(node->find_tag(this, FIFF_MNE_NROW, t_pTag))
        if (*t_pTag->toInt() != mat.nrow)
        {
            qWarning("Number of rows in matrix data and FIFF_MNE_NROW tag do not match");
            return false;
        }
    if(node->find_tag(this, FIFF_MNE_NCOL, t_pTag))
        if (*t_pTag->toInt() != mat.ncol)
        {
            qWarning("Number of columns in matrix data and FIFF_MNE_NCOL tag do not match");
            return false;
        }

    QString row_names;
    if(node->find_tag(this, FIFF_MNE_ROW_NAMES, t_pTag))
        row_names = t_pTag->toString();

    QString col_names;
    if(node->find_tag(this, FIFF_MNE_COL_NAMES, t_pTag))
        col_names = t_pTag->toString();

    //
    //   Put it together
    //
    if (!row_names.isEmpty())
        mat.row_names = split_name_list(row_names);

    if (!col_names.isEmpty())
        mat.col_names = split_name_list(col_names);

    if (mat.row_names.size() != mat.nrow)
    {
        qWarning("FiffStream::read_named_matrix - Number of rows in matrix data and row names do not match\n");
    }

    if (mat.col_names.size() != mat.ncol)
    {
        qWarning("FiffStream::read_named_matrix - Number of columns in matrix data and column names do not match\n");
    }

    return true;
}

//=============================================================================================================

QList<FiffProj> FiffStream::read_proj(const FiffDirNode::SPtr& p_Node)
{
    QList<FiffProj> projdata;// = struct('kind',{},'active',{},'desc',{},'data',{});
    //
    //   Locate the projection data
    //
    QList<FiffDirNode::SPtr> t_qListNodes = p_Node->dir_tree_find(FIFFB_PROJ);
    if ( t_qListNodes.size() == 0 )
        return projdata;

    FiffTag::SPtr t_pTag;
    t_qListNodes[0]->find_tag(this, FIFF_NCHAN, t_pTag);
    fiff_int_t global_nchan = 0;
    if (t_pTag)
        global_nchan = *t_pTag->toInt();

    fiff_int_t nchan;
    QList<FiffDirNode::SPtr> t_qListItems = t_qListNodes[0]->dir_tree_find(FIFFB_PROJ_ITEM);
    for ( qint32 i = 0; i < t_qListItems.size(); ++i)
    {
        //
        //   Find all desired tags in one item
        //
        FiffDirNode::SPtr t_pFiffDirTreeItem = t_qListItems[i];
        t_pFiffDirTreeItem->find_tag(this, FIFF_NCHAN, t_pTag);
        if (t_pTag)
            nchan = *t_pTag->toInt();
        else
            nchan = global_nchan;

        t_pFiffDirTreeItem->find_tag(this, FIFF_DESCRIPTION, t_pTag);
        QString desc; // maybe, in some cases this has to be a struct.
        if (t_pTag)
        {
            qDebug() << "read_proj: this has to be debugged";
            desc = t_pTag->toString();
        }
        else
        {
            t_pFiffDirTreeItem->find_tag(this, FIFF_NAME, t_pTag);
            if (t_pTag)
                desc = t_pTag->toString();
            else
            {
                qWarning("Projection item description missing\n");
                return projdata;
            }
        }
//            t_pFiffDirTreeItem->find_tag(this, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag);
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
        t_pFiffDirTreeItem->find_tag(this, FIFF_PROJ_ITEM_KIND, t_pTag);
        fiff_int_t kind;
        if (t_pTag)
        {
            kind = *t_pTag->toInt();
        }
        else
        {
            qWarning("Projection item kind missing");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(this, FIFF_PROJ_ITEM_NVEC, t_pTag);
        fiff_int_t nvec;
        if (t_pTag)
        {
            nvec = *t_pTag->toInt();
        }
        else
        {
            qWarning("Number of projection vectors not specified\n");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(this, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag);
        QStringList names;
        if (t_pTag)
        {
            names = split_name_list(t_pTag->toString());
        }
        else
        {
            qWarning("Projection item channel list missing\n");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(this, FIFF_PROJ_ITEM_VECTORS, t_pTag);
        MatrixXd data;// = NULL;
        if (t_pTag)
        {
            data = t_pTag->toFloatMatrix().cast<double>();
            data.transposeInPlace();
        }
        else
        {
            qWarning("Projection item data missing\n");
            return projdata;
        }
        t_pFiffDirTreeItem->find_tag(this, FIFF_MNE_PROJ_ITEM_ACTIVE, t_pTag);
        bool active;
        if (t_pTag)
            active = *t_pTag->toInt();
        else
            active = false;

        if (data.cols() != names.size())
        {
            qWarning("Number of channel names does not match the size of data matrix\n");
            return projdata;
        }

        //
        //   create a named matrix for the data
        //
        QStringList defaultList;
        FiffNamedMatrix t_fiffNamedMatrix(nvec, nchan, defaultList, names, data);

        FiffProj one(kind, active, desc, t_fiffNamedMatrix);
        //
        projdata.append(one);
    }

    if (projdata.size() > 0)
    {
        printf("\tRead a total of %d projection items:\n", (int)projdata.size());
        for(qint32 k = 0; k < projdata.size(); ++k)
        {
            printf("\t\t%s (%d x %d)",projdata[k].desc.toUtf8().constData(), projdata[k].data->nrow, projdata[k].data->ncol);
            if (projdata[k].active)
                printf(" active\n");
            else
                printf(" idle\n");
        }
    }

    return projdata;
}

//=============================================================================================================

bool FiffStream::read_tag_data(FiffTag::SPtr &p_pTag, fiff_long_t pos)
{
    if(pos >= 0)
    {
        this->device()->seek(pos);
    }

    if(!p_pTag)
        return false;

    //
    // Read data when available
    //
    if (p_pTag->size() > 0)
    {
        this->readRawData(p_pTag->data(), p_pTag->size());
        FiffTag::convert_tag_data(p_pTag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
    }

    if (p_pTag->next != FIFFV_NEXT_SEQ)
        this->device()->seek(p_pTag->next);//fseek(fid,tag.next,'bof');

    return true;
}

//=============================================================================================================

fiff_long_t FiffStream::read_tag_info(FiffTag::SPtr &p_pTag, bool p_bDoSkip)
{
    fiff_long_t pos = this->device()->pos();

    p_pTag = FiffTag::SPtr(new FiffTag());

    //Option 1
//    t_DataStream.readRawData((char *)p_pTag, FIFFC_TAG_INFO_SIZE);
//    p_pTag->kind = Fiff::swap_int(p_pTag->kind);
//    p_pTag->type = Fiff::swap_int(p_pTag->type);
//    p_pTag->size = Fiff::swap_int(p_pTag->size);
//    p_pTag->next = Fiff::swap_int(p_pTag->next);

    //Option 2
     *this  >> p_pTag->kind;
     *this  >> p_pTag->type;
    qint32 size;
     *this  >> size;
    p_pTag->resize(size);
     *this  >> p_pTag->next;

//    qDebug() << "read_tag_info" << "  Kind:" << p_pTag->kind << "  Type:" << p_pTag->type << "  Size:" << p_pTag->size() << "  Next:" << p_pTag->next;

    if (p_bDoSkip)
    {
        QTcpSocket* t_qTcpSocket = qobject_cast<QTcpSocket*>(this->device());
        if(t_qTcpSocket)
        {
            this->skipRawData(p_pTag->size());
        }
        else
        {
            if (p_pTag->next > 0)
            {
                if(!this->device()->seek(p_pTag->next)) {
                    qCritical("fseek"); //fseek(fid,tag.next,'bof');
                    pos = -1;
                }
            }
            else if (p_pTag->size() > 0 && p_pTag->next == FIFFV_NEXT_SEQ)
            {
                if(!this->device()->seek(this->device()->pos()+p_pTag->size())) {
                    qCritical("fseek"); //fseek(fid,tag.size,'cof');
                    pos = -1;
                }
            }
        }
    }
    return pos;
}

//=============================================================================================================

bool FiffStream::read_rt_tag(FiffTag::SPtr &p_pTag)
{
    while(this->device()->bytesAvailable() < 16)
        this->device()->waitForReadyRead(10);

//    if(!this->read_tag_info(p_pTag, false))
//        return false;
    this->read_tag_info(p_pTag, false);

    while(this->device()->bytesAvailable() < p_pTag->size())
        this->device()->waitForReadyRead(10);

    if(!this->read_tag_data(p_pTag))
        return false;

    return true;
}

//=============================================================================================================

bool FiffStream::read_tag(FiffTag::SPtr &p_pTag,
                          fiff_long_t pos)
{
    if (pos >= 0) {
        this->device()->seek(pos);
    }

    p_pTag = FiffTag::SPtr(new FiffTag());

    //
    // Read fiff tag header from stream
    //
     *this  >> p_pTag->kind;
     *this  >> p_pTag->type;
    qint32 size;
     *this  >> size;
    p_pTag->resize(size);
     *this  >> p_pTag->next;

//    qDebug() << "read_tag" << "  Kind:" << p_pTag->kind << "  Type:" << p_pTag->type << "  Size:" << p_pTag->size() << "  Next:" << p_pTag->next;

    //
    // Read data when available
    //
    int endian;
    if (this->byteOrder() == QDataStream::LittleEndian){
        endian = FIFFV_LITTLE_ENDIAN;
        //printf("Endian: Little\n");
    } else {
        endian = FIFFV_BIG_ENDIAN;
        //printf("Endian: Big\n");
    }

    if (p_pTag->size() > 0)
    {
        this->readRawData(p_pTag->data(), p_pTag->size());
        //FiffTag::convert_tag_data(p_pTag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
        FiffTag::convert_tag_data(p_pTag,endian,FIFFV_NATIVE_ENDIAN);
    }

    if (p_pTag->next != FIFFV_NEXT_SEQ)
        this->device()->seek(p_pTag->next);//fseek(fid,tag.next,'bof');

    return true;
}

//=============================================================================================================

bool FiffStream::setup_read_raw(QIODevice &p_IODevice,
                                FiffRawData& data,
                                bool allow_maxshield,
                                bool is_littleEndian)
{
    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = t_pStream->streamName();

    if(is_littleEndian){
        //printf("Setting stream to little Endian\n");
        t_pStream->setByteOrder(QDataStream::LittleEndian);
    }

    qInfo("Opening raw data %s...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open()){
        return false;
    }

    //
    //   Read the measurement info
    //
    FiffInfo info;// = NULL;
    FiffDirNode::SPtr meas;
    if(!t_pStream->read_meas_info(t_pStream->dirtree(), info, meas))
        return false;

    //
    //   Locate the data of interest
    //
    QList<FiffDirNode::SPtr> raw = meas->dir_tree_find(FIFFB_RAW_DATA);
    if (raw.size() == 0)
    {
        raw = meas->dir_tree_find(FIFFB_CONTINUOUS_DATA);
        if(allow_maxshield)
        {
//            for (qint32 i = 0; i < raw.size(); ++i)
//                if(raw[i])
//                    delete raw[i];
            raw = meas->dir_tree_find(FIFFB_SMSH_RAW_DATA);
            qInfo("Maxshield data found\n");

            if (raw.size() == 0)
            {
                qWarning("No raw data in %s\n", t_sFileName.toUtf8().constData());
                return false;
            }
        }
        else
        {
            if (raw.size() == 0)
            {
                qWarning("No raw data in %s\n", t_sFileName.toUtf8().constData());
                return false;
            }
        }
    }
    //
    //   Set up the output structure
    //
    info.filename   = t_sFileName;

    data.clear();
    data.file = t_pStream;// fid;
    data.info = info;
    data.first_samp = 0;
    data.last_samp  = 0;
    //
    //   Process the directory
    //
    QList<FiffDirEntry::SPtr> dir = raw[0]->dir;
    fiff_int_t nent = raw[0]->nent();
    fiff_int_t nchan = info.nchan;
    fiff_int_t first = 0;
    fiff_int_t first_samp = 0;
    fiff_int_t first_skip = 0;
    //
    //  Get first sample tag if it is there
    //
    FiffTag::SPtr t_pTag;
    if (dir[first]->kind == FIFF_FIRST_SAMPLE)
    {
        t_pStream->read_tag(t_pTag, dir[first]->pos);
        first_samp = *t_pTag->toInt();
        ++first;
    }
    //
    //  Omit initial skip
    //
    if (dir[first]->kind == FIFF_DATA_SKIP)
    {
        //
        //  This first skip can be applied only after we know the buffer size
        //
        t_pStream->read_tag(t_pTag, dir[first]->pos);
        first_skip = *t_pTag->toInt();
        ++first;
    }
    data.first_samp = first_samp;
    //
    //   Go through the remaining tags in the directory
    //
    QList<FiffRawDir> rawdir;
//        rawdir = struct('ent',{},'first',{},'last',{},'nsamp',{});
    fiff_int_t nskip = 0;
    fiff_int_t ndir  = 0;
    fiff_int_t nsamp = 0;
    for (qint32 k = first; k < nent; ++k)
    {
        FiffDirEntry::SPtr ent = dir[k];
        if (ent->kind == FIFF_DATA_SKIP)
        {
            t_pStream->read_tag(t_pTag, ent->pos);
            nskip = *t_pTag->toInt();
        }
        else if(ent->kind == FIFF_DATA_BUFFER)
        {
            //
            //   Figure out the number of samples in this buffer
            //
            switch(ent->type)
            {
                case FIFFT_DAU_PACK16:
                    nsamp = ent->size/(2*nchan);
                    break;
                case FIFFT_SHORT:
                    nsamp = ent->size/(2*nchan);
                    break;
                case FIFFT_FLOAT:
                    nsamp = ent->size/(4*nchan);
                    break;
                case FIFFT_INT:
                    nsamp = ent->size/(4*nchan);
                    break;
                default:
                    qWarning("Cannot handle data buffers of type %d\n",ent->type);
                    return false;
            }
            //
            //  Do we have an initial skip pending?
            //
            if (first_skip > 0)
            {
                first_samp += nsamp*first_skip;
                data.first_samp = first_samp;
                first_skip = 0;
            }
            //
            //  Do we have a skip pending?
            //
            if (nskip > 0)
            {
                FiffRawDir t_RawDir;
                t_RawDir.first = first_samp;
                t_RawDir.last  = first_samp + nskip*nsamp - 1;//ToDo -1 right or is that MATLAB syntax
                t_RawDir.nsamp = nskip*nsamp;
                rawdir.append(t_RawDir);
                first_samp = first_samp + nskip*nsamp;
                nskip = 0;
                ++ndir;
            }
            //
            //  Add a data buffer
            //
            FiffRawDir t_RawDir;
            t_RawDir.ent  = ent;
            t_RawDir.first = first_samp;
            t_RawDir.last  = first_samp + nsamp - 1;//ToDo -1 right or is that MATLAB syntax
            t_RawDir.nsamp = nsamp;
            rawdir.append(t_RawDir);
            first_samp += nsamp;
            ++ndir;
        }
    }
    data.last_samp  = first_samp - 1;//ToDo -1 right or is that MATLAB syntax
    //
    //   Add the calibration factors
    //
    RowVectorXd cals(data.info.nchan);
    cals.setZero();
    for (qint32 k = 0; k < data.info.nchan; ++k)
        cals[k] = data.info.chs[k].range*data.info.chs[k].cal;
    //
    data.cals       = cals;
    data.rawdir     = rawdir;
    //data->proj       = [];
    //data.comp       = [];
    //
    qInfo("\tRange : %d ... %d  =  %9.3f ... %9.3f secs",
           data.first_samp,data.last_samp,
           (double)data.first_samp/data.info.sfreq,
           (double)data.last_samp/data.info.sfreq);
    qInfo("Ready.");
    data.file->close();

    return true;
}

//=============================================================================================================

QStringList FiffStream::split_name_list(QString p_sNameList)
{
    return p_sNameList.replace(" ","").split(":");
}

//=============================================================================================================

fiff_long_t FiffStream::start_block(fiff_int_t kind)
{
    return this->write_int(FIFF_BLOCK_START,&kind);
}

//=============================================================================================================

FiffStream::SPtr FiffStream::start_file(QIODevice& p_IODevice)
{
    FiffStream::SPtr p_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = p_pStream->streamName();

    if(!p_pStream->device()->open(QIODevice::WriteOnly))
    {
        qWarning("Cannot write to %s\n", t_sFileName.toUtf8().constData());//consider throw
        FiffStream::SPtr p_pEmptyStream;
        return p_pEmptyStream;
    }

    //
    //   Write the compulsory items
    //
    p_pStream->write_id(FIFF_FILE_ID);//1
    int null_pointer = FIFFV_NEXT_NONE;
    p_pStream->write_int(FIFF_DIR_POINTER,&null_pointer);//2
    p_pStream->write_int(FIFF_FREE_LIST,&null_pointer);//3
    //
    //   Ready for more
    //
    return p_pStream;
}

//=============================================================================================================

FiffStream::SPtr FiffStream::open_update(QIODevice &p_IODevice)
{

    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = t_pStream->streamName();

    /*
     * Try to open...
     */
    if(!t_pStream->open(QIODevice::ReadWrite)) {
        qCritical("Cannot open %s\n", t_sFileName.toUtf8().constData());//consider throw
        return FiffStream::SPtr();
    }

    FiffTag::SPtr t_pTag;
    long dirpos,pointerpos;

    QFile *file = qobject_cast<QFile *>(t_pStream->device());

    if (file != NULL) {
        /*
        * Ensure that the last tag in the directory has next set to FIFF_NEXT_NONE
        */
        pointerpos = t_pStream->dir()[t_pStream->nent()-2]->pos;
        if(!t_pStream->read_tag(t_pTag,pointerpos)){
            qCritical("Could not read last tag in the directory list!");
            t_pStream->close();
            return FiffStream::SPtr();
        }
        if (t_pTag->next != FIFFV_NEXT_NONE) {
            t_pTag->next = FIFFV_NEXT_NONE;
            t_pStream->write_tag(t_pTag,pointerpos);
        }
        /*
        * Read directory pointer
        */
        pointerpos = t_pStream->dir()[1]->pos;
        if(!t_pStream->read_tag(t_pTag,pointerpos)){
            qCritical("Could not read directory pointer!");
            t_pStream->close();
            return FiffStream::SPtr();
        }
        /*
        * Do we have a directory?
        */
        dirpos = *t_pTag->toInt();
        if (dirpos > 0) {
            /*
            * Yes! We will ignore it.
            */
            t_pStream->write_dir_pointer(-1, pointerpos);
            /*
            * Clean up the trailing end
            */
            file->resize(dirpos);//truncate file to new size
        }
        /*
        * Seek to end for writing
        */
        t_pStream->device()->seek(file->size());//SEEK_END
    }
    return t_pStream;
}

//=============================================================================================================

FiffStream::SPtr FiffStream::start_writing_raw(QIODevice &p_IODevice,
                                               const FiffInfo& info,
                                               RowVectorXd& cals,
                                               MatrixXi sel,
                                               bool bResetRange)
{
    //
    //   We will always write floats
    //
    fiff_int_t data_type = 4;
    qint32 k;

    if(sel.cols() == 0)
    {
        sel.resize(1,info.nchan);
        for (k = 0; k < info.nchan; ++k)
            sel(0, k) = k; //+1 when MATLAB notation
    }

    QList<FiffChInfo> chs;

    for(k = 0; k < sel.cols(); ++k)
        chs << info.chs.at(sel(0,k));

    fiff_int_t nchan = chs.size();

    //
    //  Create the file and save the essentials
    //
    FiffStream::SPtr t_pStream = start_file(p_IODevice);//1, 2, 3
    t_pStream->start_block(FIFFB_MEAS);//4
    t_pStream->write_id(FIFF_BLOCK_ID);//5
    if(info.meas_id.version != -1)
    {
        t_pStream->write_id(FIFF_PARENT_BLOCK_ID,info.meas_id);//6
    }
    //
    //
    //    Measurement info
    //
    t_pStream->start_block(FIFFB_MEAS_INFO);//7

    #ifndef WASMBUILD
    //
    //    Blocks from the original
    //
    QList<fiff_int_t> blocks;
    blocks << FIFFB_SUBJECT << FIFFB_HPI_MEAS << FIFFB_HPI_RESULT << FIFFB_HPI_SUBSYSTEM << FIFFB_ISOTRAK << FIFFB_PROCESSING_HISTORY << FIFFB_DACQ_PARS << FIFFB_EVENTS;
    bool have_hpi_result = false;
    bool have_isotrak    = false;
    if (blocks.size() > 0 && !info.filename.isEmpty())
    {
        QFile t_qFile(info.filename);//ToDo this has to be adapted for TCPSocket
        FiffStream::SPtr t_pStream2(new FiffStream(&t_qFile));

        if(!t_pStream2->open()){
            qDebug() << "Failed to open file. Returning early";
            return t_pStream;
        }

        for(qint32 k = 0; k < blocks.size(); ++k)
        {
            QList<FiffDirNode::SPtr> nodes = t_pStream2->dirtree()->dir_tree_find(blocks[k]);
            FiffDirNode::copy_tree(t_pStream2,t_pStream2->dirtree()->id,nodes,t_pStream);
            if(blocks[k] == FIFFB_HPI_RESULT && nodes.size() > 0)
                have_hpi_result = true;

            if(blocks[k] == FIFFB_ISOTRAK && nodes.size() > 0)
                have_isotrak = true;
        }

        t_pStream2 = FiffStream::SPtr();
    }
    #endif

    //
    //    megacq parameters
    //
    if (!info.acq_pars.isEmpty() || !info.acq_stim.isEmpty())
    {
        t_pStream->start_block(FIFFB_DACQ_PARS);
        if (!info.acq_pars.isEmpty())
            t_pStream->write_string(FIFF_DACQ_PARS, info.acq_pars);

        if (!info.acq_stim.isEmpty())
            t_pStream->write_string(FIFF_DACQ_STIM, info.acq_stim);

        t_pStream->end_block(FIFFB_DACQ_PARS);
    }

    #ifndef WASMBUILD
    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result)
    {
        if (!info.dev_head_t.isEmpty())
            t_pStream->write_coord_trans(info.dev_head_t);

        if (!info.ctf_head_t.isEmpty())
            t_pStream->write_coord_trans(info.ctf_head_t);
    }
    //
    //    Polhemus data
    //
    if (info.dig.size() > 0 && !have_isotrak)
    {
        t_pStream->start_block(FIFFB_ISOTRAK);
        for (qint32 k = 0; k < info.dig.size(); ++k)
            t_pStream->write_dig_point(info.dig[k]);

        t_pStream->end_block(FIFFB_ISOTRAK);
    }
    #endif

    //
    //    Projectors
    //
    t_pStream->write_proj(info.projs);
    //
    //    CTF compensation info
    //
    t_pStream->write_ctf_comp(info.comps);
    //
    //    Bad channels
    //
    if (info.bads.size() > 0)
    {
        t_pStream->start_block(FIFFB_MNE_BAD_CHANNELS);
        t_pStream->write_name_list(FIFF_MNE_CH_NAME_LIST,info.bads);
        t_pStream->end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //    General
    //
    t_pStream->write_float(FIFF_SFREQ,&info.sfreq);
    t_pStream->write_float(FIFF_HIGHPASS,&info.highpass);
    t_pStream->write_float(FIFF_LOWPASS,&info.lowpass);
    t_pStream->write_float(FIFF_LINE_FREQ,&info.linefreq);
    t_pStream->write_int(FIFF_GANTRY_ANGLE,&info.gantry_angle);
    t_pStream->write_int(FIFF_NCHAN,&nchan);
    t_pStream->write_int(FIFF_DATA_PACK,&data_type);
    t_pStream->write_int(FIFF_PROJ_ID,&info.proj_id);
    t_pStream->write_string(FIFF_EXPERIMENTER,info.experimenter);
    t_pStream->write_string(FIFF_DESCRIPTION,info.description);
    t_pStream->write_string(FIFF_PROJ_NAME,info.proj_name);
    t_pStream->write_string(FIFF_XPLOTTER_LAYOUT,info.xplotter_layout);
    if (info.meas_date[0] != -1)
        t_pStream->write_int(FIFF_MEAS_DATE,info.meas_date, 2);
    //
    //    Channel info
    //
    cals = RowVectorXd(nchan);
    for(k = 0; k < nchan; ++k)
    {
        //
        //    Scan numbers may have been messed up
        //
        chs[k].scanNo = k+1;
        if(bResetRange) {
            chs[k].range = 1.0; // Reset to 1.0 because we always write floats.
        }
        cals[k] = chs[k].cal;
        t_pStream->write_ch_info(chs[k]);
    }
    //
    //
    t_pStream->end_block(FIFFB_MEAS_INFO);
    //
    // Start the raw data
    //
    t_pStream->start_block(FIFFB_RAW_DATA);

    return t_pStream;
}

//=============================================================================================================

fiff_long_t FiffStream::write_tag(const QSharedPointer<FiffTag> &p_pTag, fiff_long_t pos)
{
    /*
     * Write tag to specified position
     */
    if (pos >= 0) {
        this->device()->seek(pos);
    }
    else { //SEEK_END
        QFile* file = qobject_cast<QFile*> (this->device());
        if(file)
            this->device()->seek(file->size());
    }
    pos = this->device()->pos();

    fiff_int_t datasize = p_pTag->size();

     *this << static_cast<qint32>(p_pTag->kind);
     *this << static_cast<qint32>(p_pTag->type);
     *this << static_cast<qint32>(datasize);
     *this << static_cast<qint32>(p_pTag->next);

    /*
     * Do we have data?
     */
    if (datasize > 0) {
        /*
        * Data exists...
        */
        this->writeRawData(p_pTag->data(),datasize);
    }

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_ch_info(const FiffChInfo& ch)
{
    fiff_long_t pos = this->device()->pos();

    //typedef struct _fiffChPosRec {
    //  fiff_int_t   coil_type;          /*!< What kind of coil. */
    //  fiff_float_t r0[3];              /*!< Coil coordinate system origin */
    //  fiff_float_t ex[3];              /*!< Coil coordinate system x-axis unit vector */
    //  fiff_float_t ey[3];              /*!< Coil coordinate system y-axis unit vector */
    //  fiff_float_t ez[3];             /*!< Coil coordinate system z-axis unit vector */
    //} fiffChPosRec,*fiffChPos;        /*!< Measurement channel position and coil type */

    //typedef struct _fiffChInfoRec {
    //  fiff_int_t    scanNo;        /*!< Scanning order # */
    //  fiff_int_t    logNo;         /*!< Logical channel # */
    //  fiff_int_t    kind;          /*!< Kind of channel */
    //  fiff_float_t  range;         /*!< Voltmeter range (only applies to raw data ) */
    //  fiff_float_t  cal;           /*!< Calibration from volts to... */
    //  fiff_ch_pos_t chpos;         /*!< Channel location */
    //  fiff_int_t    unit;          /*!< Unit of measurement */
    //  fiff_int_t    unit_mul;      /*!< Unit multiplier exponent */
    //  fiff_char_t   ch_name[16];   /*!< Descriptive name for the channel */
    //} fiffChInfoRec,*fiffChInfo;   /*!< Description of one channel */
    fiff_int_t datasize= 4*13 + 4*7 + 16;

     *this << (qint32)FIFF_CH_INFO;
     *this << (qint32)FIFFT_CH_INFO_STRUCT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    //
    //   Start writing fiffChInfoRec
    //
     *this << (qint32)ch.scanNo;
     *this << (qint32)ch.logNo;
     *this << (qint32)ch.kind;

     *this << ch.range;
     *this << ch.cal;

    //
    //   FiffChPos follows
    //
    write_ch_pos(ch.chpos);

    //
    //   unit and unit multiplier
    //
     *this << (qint32)ch.unit;
     *this << (qint32)ch.unit_mul;

    //
    //   Finally channel name
    //
    fiff_int_t len = ch.ch_name.size();
    QString ch_name;
    if(len > 15)
        ch_name = ch.ch_name.mid(0, 15);
    else
        ch_name = ch.ch_name;

    len = ch_name.size();

    this->writeRawData(ch_name.toUtf8().constData(),len);

    if (len < 16) {
        const char* chNull = "";
        for(qint32 i = 0; i < 16-len; ++i)
            this->writeRawData(chNull,1);
    }

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_ch_pos(const FiffChPos &chpos)
{
    fiff_long_t pos = this->device()->pos();

    //
    //   FiffChPos
    //
     *this << (qint32)chpos.coil_type;

    qint32 i;
    // r0
    for(i = 0; i < 3; ++i)
        *this << chpos.r0[i];
    // ex
    for(i = 0; i < 3; ++i)
        *this << chpos.ex[i];
    // ey
    for(i = 0; i < 3; ++i)
        *this << chpos.ey[i];
    // ez
    for(i = 0; i < 3; ++i)
        *this << chpos.ez[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_coord_trans(const FiffCoordTrans& trans)
{
    fiff_long_t pos = this->device()->pos();

    //?typedef struct _fiffCoordTransRec {
    //  fiff_int_t   from;                   /*!< Source coordinate system. */
    //  fiff_int_t   to;                     /*!< Destination coordinate system. */
    //  fiff_float_t rot[3][3];              /*!< The forward transform (rotation part) */
    //  fiff_float_t move[3];                /*!< The forward transform (translation part) */
    //  fiff_float_t invrot[3][3];           /*!< The inverse transform (rotation part) */
    //  fiff_float_t invmove[3];             /*!< The inverse transform (translation part) */
    //} *fiffCoordTrans, fiffCoordTransRec;  /*!< Coordinate transformation descriptor */
    fiff_int_t datasize = 4*2*12 + 4*2;

     *this << (qint32)FIFF_COORD_TRANS;
     *this << (qint32)FIFFT_COORD_TRANS_STRUCT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    //
    //   Start writing fiffCoordTransRec
    //
     *this << (qint32)trans.from;
     *this << (qint32)trans.to;

    //
    //   The transform...
    //
    qint32 r, c;
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            *this << (float)trans.trans(r,c);
    for (r = 0; r < 3; ++r)
        *this << (float)trans.trans(r,3);

    //
    //   ...and its inverse
    //
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            *this << (float)trans.invtrans(r,c);
    for (r = 0; r < 3; ++r)
        *this << (float)trans.invtrans(r,3);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_cov(const FiffCov &p_FiffCov)
{
    fiff_long_t pos = this->device()->pos();

    this->start_block(FIFFB_MNE_COV);

    //
    //   Dimensions etc.
    //
    this->write_int(FIFF_MNE_COV_KIND, &p_FiffCov.kind);
    this->write_int(FIFF_MNE_COV_DIM, &p_FiffCov.dim);
    if (p_FiffCov.nfree > 0)
        this->write_int(FIFF_MNE_COV_NFREE, &p_FiffCov.nfree);
    //
    //   Channel names
    //
    if(p_FiffCov.names.size() > 0)
        this->write_name_list(FIFF_MNE_ROW_NAMES, p_FiffCov.names);
    //
    //   Data
    //
    if(p_FiffCov.diag)
        this->write_double(FIFF_MNE_COV_DIAG, p_FiffCov.data.col(0).data(), p_FiffCov.data.rows());
    else
    {
//        if issparse(cov.data)
//           fiff_write_float_sparse_rcs(fid,FIFF.FIFF_MNE_COV,cov.data);
//        else
//        {
            // Store only lower part of covariance matrix
            qint32 dim = p_FiffCov.dim;
            qint32 n = ((dim*dim) - dim)/2;

            VectorXd vals(n);
            qint32 count = 0;
            for(qint32 i = 1; i < dim; ++i)
                for(qint32 j = 0; j < i; ++j)
                    vals(count) = p_FiffCov.data(i,j);

            this->write_double(FIFF_MNE_COV, vals.data(), vals.size());
//        }
    }
    //
    //   Eigenvalues and vectors if present
    //
    if(p_FiffCov.eig.size() > 0 && p_FiffCov.eigvec.size() > 0)
    {
        this->write_float_matrix(FIFF_MNE_COV_EIGENVECTORS, p_FiffCov.eigvec.cast<float>());
        this->write_double(FIFF_MNE_COV_EIGENVALUES, p_FiffCov.eig.data(), p_FiffCov.eig.size());
    }
    //
    //   Projection operator
    //
    this->write_proj(p_FiffCov.projs);
    //
    //   Bad channels
    //
    if(p_FiffCov.bads.size() > 0)
    {
        this->start_block(FIFFB_MNE_BAD_CHANNELS);
        this->write_name_list(FIFF_MNE_CH_NAME_LIST, p_FiffCov.bads);
        this->end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //   Done!
    //
    this->end_block(FIFFB_MNE_COV);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_ctf_comp(const QList<FiffCtfComp>& comps)
{
    fiff_long_t pos = this->device()->pos();

    if (comps.size() <= 0)
        return -1;
    //
    //  This is very simple in fact
    //
    this->start_block(FIFFB_MNE_CTF_COMP);
    for(qint32 k = 0; k < comps.size(); ++k)
    {
        FiffCtfComp comp(comps[k]);
        this->start_block(FIFFB_MNE_CTF_COMP_DATA);
        //
        //    Write the compensation kind
        //
        this->write_int(FIFF_MNE_CTF_COMP_KIND, &comp.ctfkind);
        qint32 save_calibrated = comp.save_calibrated;
        this->write_int(FIFF_MNE_CTF_COMP_CALIBRATED, &save_calibrated);
        //
        //  Write an uncalibrated or calibrated matrix
        //  If compensators where calibrated undo this here
        //
        if(comps[k].save_calibrated) {
            comp.data->data = (comp.rowcals.asDiagonal()).inverse()* comp.data->data * (comp.colcals.asDiagonal()).inverse();
        }

        this->write_named_matrix(FIFF_MNE_CTF_COMP_DATA,*comp.data.data());
        this->end_block(FIFFB_MNE_CTF_COMP_DATA);
    }
    this->end_block(FIFFB_MNE_CTF_COMP);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_dig_point(const FiffDigPoint& dig)
{
    fiff_long_t pos = this->device()->pos();

    //?typedef struct _fiffDigPointRec {
    //  fiff_int_t kind;               /*!< FIFF_POINT_CARDINAL,
    //                                  *   FIFF_POINT_HPI, or
    //                                  *   FIFF_POINT_EEG */
    //  fiff_int_t ident;              /*!< Number identifying this point */
    //  fiff_float_t r[3];             /*!< Point location */
    //} *fiffDigPoint,fiffDigPointRec; /*!< Digitization point description */
    fiff_int_t datasize = 5*4;

     *this << (qint32)FIFF_DIG_POINT;
     *this << (qint32)FIFFT_DIG_POINT_STRUCT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    //
    //   Start writing fiffDigPointRec
    //
     *this << (qint32)dig.kind;
     *this << (qint32)dig.ident;
    for(qint32 i = 0; i < 3; ++i)
        *this << dig.r[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_dir_pointer(fiff_int_t dirpos, fiff_long_t pos, fiff_int_t next)
{
    /*
     * Write entires to specified position
     */
    if (pos >= 0) {
        this->device()->seek(pos);
    }
    else { //SEEK_END
        QFile* file = qobject_cast<QFile*> (this->device());
        if(file)
            this->device()->seek(file->size());
    }
    pos = this->device()->pos();

    fiff_int_t datasize = 1 * 4;

     *this << (qint32)FIFF_DIR_POINTER;
     *this << (qint32)FIFFT_INT;
     *this << (qint32)datasize;
     *this << (qint32)next;

     *this << dirpos;

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_dir_entries(const QList<FiffDirEntry::SPtr> &dir, fiff_long_t pos)
{
//    /** Directories are composed of these structures. *
//     typedef struct _fiffDirEntryRec {
//      fiff_int_t  kind;		/**< Tag number *
//      fiff_int_t  type;		/**< Data type *
//      fiff_int_t  size;		/**< How many bytes *
//      fiff_int_t  pos;		/**< Location in file
//                      * Note: the data is located at pos +
//                      * FIFFC_DATA_OFFSET *
//     } fiffDirEntryRec,*fiffDirEntry;/**< Directory is composed of these *

    /*
     * Write entires to specified position
     */
    if (pos >= 0) {
        this->device()->seek(pos);
    }
    else { //SEEK_END
        QFile* file = qobject_cast<QFile*> (this->device());
        if(file)
            this->device()->seek(file->size());
    }

    pos = this->device()->pos();

    fiff_int_t nent = dir.size();
    fiff_int_t datasize = nent * (fiff_int_t)sizeof(FiffDirEntry);

     *this << (qint32)FIFF_DIR;
     *this << (qint32)FIFFT_DIR_ENTRY_STRUCT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_NONE;

    //
    //   Start writing FiffDirEntries
    //
    for(qint32 i = 0; i < nent; ++i) {
        *this << (qint32)dir[i]->kind;
        *this << (qint32)dir[i]->type;
        *this << (qint32)dir[i]->size;
        *this << (qint32)dir[i]->pos;
    }

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_double(fiff_int_t kind, const double* data, fiff_int_t nel)
{
    fiff_long_t pos = this->device()->pos();

    qint32 datasize = nel * 8;

    *this << (qint32)kind;
    *this << (qint32)FIFFT_DOUBLE;
    *this << (qint32)datasize;
    *this << (qint32)FIFFV_NEXT_SEQ;

    for(qint32 i = 0; i < nel; ++i)
        *this << data[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float(fiff_int_t kind, const float* data, fiff_int_t nel)
{
    fiff_long_t pos = this->device()->pos();

    qint32 datasize = nel * 4;

    *this << (qint32)kind;
    *this << (qint32)FIFFT_FLOAT;
    *this << (qint32)datasize;
    *this << (qint32)FIFFV_NEXT_SEQ;

    for(qint32 i = 0; i < nel; ++i)
        *this << data[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float_matrix(fiff_int_t kind, const MatrixXf& mat)
{
    fiff_long_t pos = this->device()->pos();

    qint32 numel = mat.rows() * mat.cols();

    fiff_int_t datasize = 4*numel + 4*3;

     *this << (qint32)kind;
     *this << (qint32)FIFFT_MATRIX_FLOAT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    qint32 i, j;
    // Storage order: row-major
    for(i = 0; i < mat.rows(); ++i)
        for(j = 0; j < mat.cols(); ++j)
            *this << mat(i,j);

    qint32 dims[3];
    dims[0] = mat.cols();
    dims[1] = mat.rows();
    dims[2] = 2;

    for(i = 0; i < 3; ++i)
        *this << dims[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float_sparse_ccs(fiff_int_t kind, const SparseMatrix<float>& mat)
{
    fiff_long_t pos = this->device()->pos();

    //
    //   nnz values
    //   nnz row indices
    //   ncol+1 pointers
    //   dims
    //   nnz
    //   ndim
    //
    qint32 nnzm = mat.nonZeros();
    qint32 ncol = mat.cols();
    fiff_int_t datasize = 4*nnzm + 4*nnzm + 4*(ncol+1) + 4*4;
    //
    //   Nonzero entries
    //
    typedef Eigen::Triplet<float> T;
    std::vector<T> s;
    s.reserve(mat.nonZeros());
    for (int k=0; k < mat.outerSize(); ++k)
        for (SparseMatrix<float>::InnerIterator it(mat,k); it; ++it)
            s.push_back(T(it.row(), it.col(), it.value()));

    s = MNEMath::sortrows<float>(s, 1);

    //[ rows, starts ] = unique(s(:,1),'first');
    std::vector<qint32> cols, starts;
    qint32 v_old = -1;
    quint32 i;
    for(i = 0; i < s.size(); ++i)
    {
        if((signed) s[i].col() != v_old)
        {
            v_old = s[i].col();
            cols.push_back(s[i].col());
            starts.push_back(i);
        }
    }

     *this << (qint32)kind;
     *this << (qint32)FIFFT_CCS_MATRIX_FLOAT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    //
    //  The data values
    //
    for(i = 0; i < s.size(); ++i)
        *this << s[i].value();

    //
    //  Row indices
    //
    for(i = 0; i < s.size(); ++i)
        *this << s[i].row();

    //
    //  Pointers
    //
    RowVectorXi ptrs = RowVectorXi::Ones(ncol+1);
    ptrs.array() *= -1;
    quint32 k;
    for(k = 0; k < cols.size(); ++k)
        ptrs[cols[k]] = starts[k];
    ptrs[ncol] = nnzm;
    //
    //  Fill in pointers for empty columns
    //
    for(k = ncol; k >= 1; --k)
       if(ptrs[k-1] < 0)
          ptrs[k-1] = ptrs[k];
    //
    for(i = 0; i < (quint32)ptrs.size(); ++i)
        *this << ptrs[i];
    //
    //   Dimensions
    //
    qint32 dims[4];
    dims[0] = mat.nonZeros();
    dims[1] = mat.rows();
    dims[2] = mat.cols();
    dims[3] = 2;

    for(i = 0; i < 4; ++i)
        *this << dims[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float_sparse_rcs(fiff_int_t kind, const SparseMatrix<float>& mat)
{
    fiff_long_t pos = this->device()->pos();

    //
    //   nnz values
    //   nnz column indices
    //   nrow+1 pointers
    //   dims
    //   nnz
    //   ndim
    //
    qint32 nnzm = mat.nonZeros();
    qint32 nrow = mat.rows();
    fiff_int_t datasize = 4*nnzm + 4*nnzm + 4*(nrow+1) + 4*4;
    //
    //   Nonzero entries
    //
    typedef Eigen::Triplet<float> T;
    std::vector<T> s;
    s.reserve(mat.nonZeros());
    for (int k=0; k < mat.outerSize(); ++k)
        for (SparseMatrix<float>::InnerIterator it(mat,k); it; ++it)
            s.push_back(T(it.row(), it.col(), it.value()));

    s = MNEMath::sortrows<float>(s);

    //[ rows, starts ] = unique(s(:,1),'first');
    std::vector<qint32> rows, starts;
    qint32 v_old = -1;
    quint32 i;
    for(i = 0; i < s.size(); ++i)
    {
        if((signed) s[i].row() != v_old)
        {
            v_old = s[i].row();
            rows.push_back(s[i].row());
            starts.push_back(i);
        }
    }

    //
    // Write tag info header
    //
     *this << (qint32)kind;
     *this << (qint32)FIFFT_RCS_MATRIX_FLOAT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    //
    //  The data values
    //
    for(i = 0; i < s.size(); ++i)
        *this << s[i].value();

    //
    //  Column indices
    //
    for(i = 0; i < s.size(); ++i)
        *this << s[i].col();

    //
    //  Pointers
    //
    RowVectorXi ptrs = RowVectorXi::Ones(nrow+1);
    ptrs.array() *= -1;
    quint32 k;
    for(k = 0; k < rows.size(); ++k)
        ptrs[rows[k]] = starts[k];
    ptrs[nrow] = nnzm;

    //
    //  Fill in pointers for empty rows
    //
    for(k = nrow; k >= 1; --k)
       if(ptrs[k-1] < 0)
          ptrs[k-1] = ptrs[k];

    //
    for(i = 0; i < (quint32)ptrs.size(); ++i)
        *this << ptrs[i];

    //
    //  Dimensions
    //
    qint32 dims[4];
    dims[0] = mat.nonZeros();
    dims[1] = mat.rows();
    dims[2] = mat.cols();
    dims[3] = 2;

    for(i = 0; i < 4; ++i)
        *this << dims[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_id(fiff_int_t kind, const FiffId& id)
{
    fiff_long_t pos = this->device()->pos();

    FiffId t_id = id;

    if(t_id.isEmpty()) {
        //
        // Create a new one
        //
        t_id = FiffId::new_file_id();
    }

    //
    //
    fiff_int_t datasize = 5*4;                       //   The id comprises five integers

     *this << (qint32)kind;
     *this << (qint32)FIFFT_ID_STRUCT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;
    //
    // Collect the bits together for one write
    //
    qint32 data[5];
    data[0] = t_id.version;
    data[1] = t_id.machid[0];
    data[2] = t_id.machid[1];
    data[3] = t_id.time.secs;
    data[4] = t_id.time.usecs;

    for(qint32 i = 0; i < 5; ++i)
        *this << data[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_info_base(const FiffInfoBase & p_FiffInfoBase)
{
    fiff_long_t pos = this->device()->pos();

    //
    // Information from the MEG file
    //
    this->start_block(FIFFB_MNE_PARENT_MEAS_FILE);
    this->write_string(FIFF_MNE_FILE_NAME, p_FiffInfoBase.filename);
    if(!p_FiffInfoBase.meas_id.isEmpty())
        this->write_id(FIFF_PARENT_BLOCK_ID, p_FiffInfoBase.meas_id);

    //
    //    General
    //
    this->write_int(FIFF_NCHAN,&p_FiffInfoBase.nchan);

    //
    //    Channel info
    //
    qint32 k;
    QList<FiffChInfo> chs;
    for(k = 0; k < p_FiffInfoBase.nchan; ++k)
        chs << p_FiffInfoBase.chs[k];

    for(k = 0; k < p_FiffInfoBase.nchan; ++k)
    {
        //
        //    Scan numbers may have been messed up
        //
        chs[k].scanNo = k+1;//+1 because
        this->write_ch_info(chs[k]);
    }

    //
    //    Blocks from the original -> skip this
    //
    bool have_hpi_result = false;

    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result)
    {
        if (!p_FiffInfoBase.dev_head_t.isEmpty())
            this->write_coord_trans(p_FiffInfoBase.dev_head_t);
        if (!p_FiffInfoBase.ctf_head_t.isEmpty())
            this->write_coord_trans(p_FiffInfoBase.ctf_head_t);
    }

    //
    //    Bad channels
    //
    if (p_FiffInfoBase.bads.size() > 0)
    {
        this->start_block(FIFFB_MNE_BAD_CHANNELS);
        this->write_name_list(FIFF_MNE_CH_NAME_LIST,p_FiffInfoBase.bads);
        this->end_block(FIFFB_MNE_BAD_CHANNELS);
    }

    this->end_block(FIFFB_MNE_PARENT_MEAS_FILE);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_int(fiff_int_t kind, const fiff_int_t* data, fiff_int_t nel, fiff_int_t next)
{
    fiff_long_t pos = this->device()->pos();

    fiff_int_t datasize = nel * 4;

     *this << (qint32)kind;
     *this << (qint32)FIFFT_INT;
     *this << (qint32)datasize;
     *this << (qint32)next;

    for(qint32 i = 0; i < nel; ++i)
        *this << data[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_int_matrix(fiff_int_t kind, const MatrixXi& mat)
{
    fiff_long_t pos = this->device()->pos();

//    qint32 FIFFT_MATRIX = 1 << 30;
//    qint32 FIFFT_MATRIX_INT = FIFFT_INT | FIFFT_MATRIX;

    qint32 numel = mat.rows() * mat.cols();

    fiff_int_t datasize = 4*numel + 4*3;

     *this << (qint32)kind;
     *this << (qint32)FIFFT_MATRIX_INT;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    qint32 i, j;
    // Storage order: row-major
    for(i = 0; i < mat.rows(); ++i)
        for(j = 0; j < mat.cols(); ++j)
            *this << mat(i,j);

    qint32 dims[3];
    dims[0] = mat.cols();
    dims[1] = mat.rows();
    dims[2] = 2;

    for(i = 0; i < 3; ++i)
        *this << dims[i];

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_name_list(fiff_int_t kind, const QStringList& data)
{
    QString all = data.join(":");
    return this->write_string(kind,all);
}

//=============================================================================================================

fiff_long_t FiffStream::write_named_matrix(fiff_int_t kind, const FiffNamedMatrix& mat)
{
    fiff_long_t pos = this->device()->pos();

    this->start_block(FIFFB_MNE_NAMED_MATRIX);
    this->write_int(FIFF_MNE_NROW, &mat.nrow);
    this->write_int(FIFF_MNE_NCOL, &mat.ncol);
    if (mat.row_names.size() > 0)
       this->write_name_list(FIFF_MNE_ROW_NAMES, mat.row_names);
    if (mat.col_names.size() > 0)
       this->write_name_list(FIFF_MNE_COL_NAMES, mat.col_names);
    this->write_float_matrix(kind,mat.data.cast<float>());
    this->end_block(FIFFB_MNE_NAMED_MATRIX);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_proj(const QList<FiffProj>& projs)
{
    fiff_long_t pos = this->device()->pos();

    if (projs.size() <= 0)
        return -1;

    this->start_block(FIFFB_PROJ);

    for(qint32 k = 0; k < projs.size(); ++k)
    {
        this->start_block(FIFFB_PROJ_ITEM);
        this->write_string(FIFF_NAME,projs[k].desc);
        this->write_int(FIFF_PROJ_ITEM_KIND,&projs[k].kind);
        if (projs[k].kind == FIFFV_PROJ_ITEM_FIELD)
        {
            float fValue = 0.0f;
            this->write_float(FIFF_PROJ_ITEM_TIME, &fValue);
        }

        this->write_int(FIFF_NCHAN, &projs[k].data->ncol);
        this->write_int(FIFF_PROJ_ITEM_NVEC, &projs[k].data->nrow);
        qint32 bValue = (qint32)projs[k].active;
        this->write_int(FIFF_MNE_PROJ_ITEM_ACTIVE, &bValue);
        this->write_name_list(FIFF_PROJ_ITEM_CH_NAME_LIST, projs[k].data->col_names);
        this->write_float_matrix(FIFF_PROJ_ITEM_VECTORS, projs[k].data->data.cast<float>());//rows == length(names)
        this->end_block(FIFFB_PROJ_ITEM);
    }
    this->end_block(FIFFB_PROJ);

    return pos;
}

//=============================================================================================================

bool FiffStream::write_raw_buffer(const MatrixXd& buf,
                                  const RowVectorXd& cals)
{
    if (buf.rows() != cals.cols())
    {
        qWarning("buffer and calibration sizes do not match\n");
        return false;
    }

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(cals.cols());
    for(qint32 i = 0; i < cals.cols(); ++i)
        tripletList.push_back(T(i, i, 1.0/cals[i]));

    SparseMatrix<double> inv_calsMat(cals.cols(), cals.cols());
    inv_calsMat.setFromTriplets(tripletList.begin(), tripletList.end());

    MatrixXf tmp = (inv_calsMat*buf).cast<float>();
    this->write_float(FIFF_DATA_BUFFER,tmp.data(),tmp.rows()*tmp.cols());
    return true;
}

//=============================================================================================================

bool FiffStream::write_raw_buffer(const MatrixXd& buf,
                                  const SparseMatrix<double>& mult)
{
    if (buf.rows() != mult.cols()) {
        qWarning("buffer and mult sizes do not match\n");
        return false;
    }

    SparseMatrix<double> inv_mult(mult.rows(), mult.cols());
    for (int k=0; k<inv_mult.outerSize(); ++k)
      for (SparseMatrix<double>::InnerIterator it(mult,k); it; ++it)
        inv_mult.coeffRef(it.row(),it.col()) = 1/it.value();

    MatrixXf tmp = (inv_mult*buf).cast<float>();
    this->write_float(FIFF_DATA_BUFFER,tmp.data(),tmp.rows()*tmp.cols());
    return true;
}

//=============================================================================================================

bool FiffStream::write_raw_buffer(const MatrixXd& buf)
{
    MatrixXf tmp = buf.cast<float>();
    this->write_float(FIFF_DATA_BUFFER,tmp.data(),tmp.rows()*tmp.cols());
    return true;
}

//=============================================================================================================

fiff_long_t FiffStream::write_string(fiff_int_t kind,
                                     const QString& data)
{
    fiff_long_t pos = this->device()->pos();

    fiff_int_t datasize = data.size();
     *this << (qint32)kind;
     *this << (qint32)FIFFT_STRING;
     *this << (qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;

    this->writeRawData(data.toUtf8().constData(),datasize);

    return pos;
}

//=============================================================================================================

void FiffStream::write_rt_command(fiff_int_t command, const QString& data)
{
    fiff_int_t datasize = data.size();
     *this << (qint32)FIFF_MNE_RT_COMMAND;
     *this << (qint32)FIFFT_VOID;
     *this << 4+(qint32)datasize;
     *this << (qint32)FIFFV_NEXT_SEQ;
     *this << command;

    this->writeRawData(data.toUtf8().constData(),datasize);
}

//=============================================================================================================

QList<FiffDirEntry::SPtr> FiffStream::make_dir(bool *ok)
{
    FiffTag::SPtr t_pTag;
    QList<FiffDirEntry::SPtr> dir;
    FiffDirEntry::SPtr t_pFiffDirEntry;
    fiff_long_t pos;
    if(ok) *ok = false;
    /*
     * Start from the very beginning...
     */
    if(!this->device()->seek(SEEK_SET))
        return dir;
    while ((pos = this->read_tag_info(t_pTag)) != -1) {
        /*
        * Check that we haven't run into the directory
        */
        if (t_pTag->kind == FIFF_DIR)
            break;
        /*
        * Put in the new entry
        */
        t_pFiffDirEntry = FiffDirEntry::SPtr(new FiffDirEntry);
        t_pFiffDirEntry->kind = t_pTag->kind;
        t_pFiffDirEntry->type = t_pTag->type;
        t_pFiffDirEntry->size = t_pTag->size();
        t_pFiffDirEntry->pos = (fiff_long_t)pos;

        //qDebug() << "Kind: " << t_pTag->kind << "| Type:" << t_pTag->type << "| Size" << t_pTag->size() << "| Next:" << t_pTag->next;

        dir.append(t_pFiffDirEntry);
        if (t_pTag->next < 0)
            break;
    }
    /*
     * Put in the new the terminating entry
     */
    t_pFiffDirEntry = FiffDirEntry::SPtr(new FiffDirEntry);
    t_pFiffDirEntry->kind = -1;
    t_pFiffDirEntry->type = -1;
    t_pFiffDirEntry->size = -1;
    t_pFiffDirEntry->pos  = -1;
    dir.append(t_pFiffDirEntry);

    if(ok) *ok = true;
    return dir;
}

//=============================================================================================================

bool FiffStream::check_beginning(FiffTag::SPtr &p_pTag)
{
    this->read_tag(p_pTag);

    if (p_pTag->kind != FIFF_FILE_ID)
    {
        qWarning("Fiff::open: file does not start with a file id tag\n");//consider throw
        return false;
    }

    if (p_pTag->type != FIFFT_ID_STRUCT)
    {
        qWarning("Fiff::open: file does not start with a file id tag\n");//consider throw
        return false;
    }
    if (p_pTag->size() != 20)
    {
        qWarning("Fiff::open: file does not start with a file id tag\n");//consider throw
        return false;
    }
    //do not rewind since the data is contained in the returned tag; -> done for TCP IP reasosn, no rewind possible there
    return true;
}
