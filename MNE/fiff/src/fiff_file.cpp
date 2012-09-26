
//=============================================================================================================
/**
* @file     fiff_file.cpp
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
* @brief    Contains the implementation of the FiffFile Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../include/fiff_file.h"
#include "../include/fiff_tag.h"
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

FiffFile::FiffFile(QString& p_sFilename)
: QFile(p_sFilename)
{

}


//*************************************************************************************************************

FiffFile::~FiffFile()
{
    if(this->isOpen())
    {
        this->close();
    }
}


//*************************************************************************************************************

bool FiffFile::open(FiffDirTree*& p_pTree, QList<fiff_dir_entry_t>*& p_pDir)
{

    if (!this->open(QIODevice::ReadOnly))
    {
        printf("Cannot open file %s\n", this->fileName().toUtf8().constData());//consider throw
        return false;
    }

    FIFFLIB::FiffTag* t_pTag = NULL;
    FiffTag::read_tag_info(this, t_pTag);

    if (t_pTag->kind != FIFF_FILE_ID)
    {
        printf("Fiff::open: file does not start with a file id tag");//consider throw
        return false;
    }

    if (t_pTag->type != FIFFT_ID_STRUCT)
    {
        printf("Fiff::open: file does not start with a file id tag");//consider throw
        return false;
    }
    if (t_pTag->size != 20)
    {
        printf("Fiff::open: file does not start with a file id tag");//consider throw
        return false;
    }

    FiffTag::read_tag(this, t_pTag);

    if (t_pTag->kind != FIFF_DIR_POINTER)
    {
        printf("Fiff::open: file does have a directory pointer");//consider throw
        return false;
    }

    //
    //   Read or create the directory tree
    //
    printf("\nCreating tag directory for %s...\n", this->fileName().toUtf8().constData());

    if (p_pDir)
        delete p_pDir;
    p_pDir = new QList<fiff_dir_entry_t>;

    qint32 dirpos = *t_pTag->toInt();
    if (dirpos > 0)
    {
        FiffTag::read_tag(this, t_pTag, dirpos);
        *p_pDir = t_pTag->toDirEntry();
    }
    else
    {
        int k = 0;
        this->seek(0);//fseek(fid,0,'bof');
        //dir = struct('kind',{},'type',{},'size',{},'pos',{});
        fiff_dir_entry_t t_fiffDirEntry;
        while (t_pTag->next >= 0)
        {
            t_fiffDirEntry.pos = this->pos();//pos = ftell(fid);
            FiffTag::read_tag_info(this, t_pTag);
            ++k;
            t_fiffDirEntry.kind = t_pTag->kind;
            t_fiffDirEntry.type = t_pTag->type;
            t_fiffDirEntry.size = t_pTag->size;
            p_pDir->append(t_fiffDirEntry);
        }
    }
    delete t_pTag;
    //
    //   Create the directory tree structure
    //

    FiffDirTree::make_dir_tree(this, p_pDir, p_pTree);

//    qDebug() << "[done]\n";

    //
    //   Back to the beginning
    //
    this->seek(0); //fseek(fid,0,'bof');
    return true;
}


//*************************************************************************************************************

FiffFile* FiffFile::start_file(QString& p_sFilename)
{
    FiffFile* p_pFile = new FiffFile(p_sFilename);

    if(!p_pFile->open(QIODevice::WriteOnly))
    {
        printf("Cannot write to %s\n", p_pFile->fileName().toUtf8().constData());//consider throw
        delete p_pFile;
        return NULL;
    }

    //
    //   Write the compulsory items
    //
    p_pFile->write_id(FIFF_FILE_ID);
    qint32 data = -1;
    p_pFile->write_int(FIFF_DIR_POINTER,&data);
    p_pFile->write_int(FIFF_FREE_LIST,&data);
    //
    //   Ready for more
    //
    return p_pFile;
}


//*************************************************************************************************************

void FiffFile::write_coord_trans(FiffCoordTrans& trans)
{
    //?typedef struct _fiffCoordTransRec {
    //  fiff_int_t   from;                   /*!< Source coordinate system. */
    //  fiff_int_t   to;                     /*!< Destination coordinate system. */
    //  fiff_float_t rot[3][3];              /*!< The forward transform (rotation part) */
    //  fiff_float_t move[3];                /*!< The forward transform (translation part) */
    //  fiff_float_t invrot[3][3];           /*!< The inverse transform (rotation part) */
    //  fiff_float_t invmove[3];             /*!< The inverse transform (translation part) */
    //} *fiffCoordTrans, fiffCoordTransRec;  /*!< Coordinate transformation descriptor */

    fiff_int_t datasize = 4*2*12 + 4*2;

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)FIFF_COORD_TRANS;
    out << (qint32)FIFFT_COORD_TRANS_STRUCT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

//        count = fwrite(fid,int32(FIFF_COORD_TRANS),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFT_COORD_TRANS_STRUCT),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(datasize),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFV_NEXT_SEQ),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
    //
    //   Start writing fiffCoordTransRec
    //
    out << (qint32)trans.from;
    out << (qint32)trans.to;
//        count = fwrite(fid,int32(trans.from),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(trans.to),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end

    //
    //   The transform...
    //
    qint32 r, c;
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            out << (float)trans.trans(r,c);
    for (r = 0; r < 3; ++r)
        out << (float)trans.trans(r,3);

//        rot = trans.trans(1:3,1:3)';
//        move = trans.trans(1:3,4)';
//        count = fwrite(fid,single(rot),'single');
//        if count ~= 9
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(move),'single');
//        if count ~= 3
//            error(me,'write failed');
//        end

    //
    //   ...and its inverse
    //
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            out << (float)trans.invtrans(r,c);
    for (r = 0; r < 3; ++r)
        out << (float)trans.invtrans(r,3);

//        trans_inv=inv(trans.trans);
//        rot=trans_inv(1:3,1:3)';
//        move=trans_inv(1:3,4)';
//        count = fwrite(fid,single(rot),'single');
//        if count ~= 9
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(move),'single');
//        if count ~= 3
//            error(me,'write failed');
//        end
}


//*************************************************************************************************************

void FiffFile::write_dig_point(fiff_dig_point_t& dig)
{
    //?typedef struct _fiffDigPointRec {
    //  fiff_int_t kind;               /*!< FIFF_POINT_CARDINAL,
    //                                  *   FIFF_POINT_HPI, or
    //                                  *   FIFF_POINT_EEG */
    //  fiff_int_t ident;              /*!< Number identifying this point */
    //  fiff_float_t r[3];             /*!< Point location */
    //} *fiffDigPoint,fiffDigPointRec; /*!< Digitization point description */

    fiff_int_t datasize = 5*4;

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)FIFF_DIG_POINT;
    out << (qint32)FIFFT_DIG_POINT_STRUCT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

//        count = fwrite(fid,int32(FIFF_DIG_POINT),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFT_DIG_POINT_STRUCT),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(datasize),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFV_NEXT_SEQ),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
    //
    //   Start writing fiffDigPointRec
    //
    out << (qint32)dig.kind;
    out << (qint32)dig.ident;
    for(qint32 i = 0; i < 3; ++i)
        out << dig.r[i];
//        count = fwrite(fid,int32(dig.kind),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(dig.ident),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(dig.r(1:3)),'single');
//        if count ~= 3
//            error(me,'write failed');
//        end
}


//*************************************************************************************************************

void FiffFile::write_float(fiff_int_t kind, float* data, fiff_int_t nel)
{
    qint32 datasize = nel * 4;

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)kind;
    out << (qint32)FIFFT_FLOAT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

    for(qint32 i = 0; i < nel; ++i)
        out << data[i];

//    count = fwrite(fid,int32(kind),'int32');
//    if count ~= 1
//        error(me,'write failed');
//    end
//    count = fwrite(fid,int32(FIFFT_FLOAT),'int32');
//    if count ~= 1
//        error(me,'write failed');
//    end
//    count = fwrite(fid,int32(datasize),'int32');
//    if count ~= 1
//        error(me,'write failed');
//    end
//    count = fwrite(fid,int32(FIFFV_NEXT_SEQ),'int32');
//    if count ~= 1
//        error(me,'write failed');
//    end
//    count = fwrite(fid,single(data),'single');
//    if count ~= nel
//        error(me,'write failed');
//    end
}


//*************************************************************************************************************

void FiffFile::write_float_matrix(fiff_int_t kind, MatrixXf& mat)
{
    qint32 FIFFT_MATRIX = 1 << 30;
    qint32 FIFFT_MATRIX_FLOAT = FIFFT_FLOAT | FIFFT_MATRIX;

    qint32 numel = mat.rows()*mat.cols();

    fiff_int_t datasize = 4*numel + 4*3;


    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)kind;
    out << (qint32)FIFFT_MATRIX_FLOAT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

    qint32 i;
    for(i = 0; i < numel; ++i)
        out << mat.data()[i];

//        count = fwrite(fid,int32(kind),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFT_MATRIX_FLOAT),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(datasize),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFV_NEXT_SEQ),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(mat'),'single');
//        if count ~= numel(mat)
//            error(me,'write failed');
//        end

    qint32 dims[3];
    dims[0] = mat.cols();
    dims[1] = mat.rows();
    dims[2] = 2;

    for(i = 0; i < 3; ++i)
        out << dims[i];

//        count = fwrite(fid,int32(dims),'int32');
//        if count ~= 3
//            error(me,'write failed');
//        end
}


//*************************************************************************************************************

void FiffFile::write_id(fiff_int_t kind, FiffId& id)
{
    if(id.version == -1)
    {
        /* initialize random seed: */
        srand ( time(NULL) );
        double rand_1 = (double)(rand() % 100);rand_1 /= 100;
        double rand_2 = (double)(rand() % 100);rand_2 /= 100;

        time_t seconds;
        seconds = time (NULL);

        //fiff_int_t timezone = 5;      //   Matlab does not know the timezone
        id.version   = (1 << 16) | 2;   //   Version (1 << 16) | 2
        id.machid[0] = 65536*rand_1;    //   Machine id is random for now
        id.machid[1] = 65536*rand_2;    //   Machine id is random for now
        id.time.secs = (int)seconds;    //seconds since January 1, 1970 //3600*(24*(now-datenum(1970,1,1,0,0,0))+timezone);
        id.time.usecs = 0;              //   Do not know how we could get this
    }

    //
    //
    fiff_int_t datasize = 5*4;                       //   The id comprises five integers

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)kind;
    out << (qint32)FIFFT_ID_STRUCT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;
    //
    // Collect the bits together for one write
    //
    qint32 data[5];
    data[0] = id.version;
    data[1] = id.machid[0];
    data[2] = id.machid[1];
    data[3] = id.time.secs;
    data[4] = id.time.usecs;

    for(qint32 i = 0; i < 5; ++i)
        out << data[i];

//        //DEBUG
//        this->close();
//        this->open(QIODevice::ReadOnly);
//        QDataStream in(this);    // read the data serialized from the file

//        qint32 a;
//        for(int i = 0; i < 9; ++i)
//        {
//            in >> a;           // extract "the answer is" and 42
//            qDebug() << a;
//        }
//        //DEBUG
}


//*************************************************************************************************************

void FiffFile::write_int(fiff_int_t kind, fiff_int_t* data, fiff_int_t nel)
{
    fiff_int_t datasize = nel * 4;

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)kind;
    out << (qint32)FIFFT_INT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

    for(qint32 i = 0; i < nel; ++i)
        out << data[i];

//        //DEBUG
//        this->close();
//        this->open(QIODevice::ReadOnly);
//        QDataStream in(this);    // read the data serialized from the file

//        qint32 a;
//        for(int i = 0; i < 9+5; ++i)
//        {
//            in >> a;           // extract "the answer is" and 42
//            qDebug() << a;
//        }
//        //DEBUG
}


//*************************************************************************************************************

void FiffFile::write_name_list(fiff_int_t kind,QStringList& data)
{
    QString all = data.join(":");
    this->write_string(kind,all);
}


//*************************************************************************************************************

void FiffFile::write_proj(QList<FiffProj*>& projs)
{
    if (projs.size() <= 0)
        return;

    this->start_block(FIFFB_PROJ);

    for(qint32 k = 0; k < projs.size(); ++k)
    {
        this->start_block(FIFFB_PROJ_ITEM);
        this->write_string(FIFF_NAME,projs[k]->desc);
        this->write_int(FIFF_PROJ_ITEM_KIND,&projs[k]->kind);
        if (projs[k]->kind == FIFFV_PROJ_ITEM_FIELD)
        {
            float fValue = 0.0f;
            this->write_float(FIFF_PROJ_ITEM_TIME, &fValue);
        }

        this->write_int(FIFF_NCHAN, &projs[k]->data->ncol);
        this->write_int(FIFF_PROJ_ITEM_NVEC, &projs[k]->data->nrow);
        qint32 bValue = (qint32)projs[k]->active;
        this->write_int(FIFF_MNE_PROJ_ITEM_ACTIVE, &bValue);
        this->write_name_list(FIFF_PROJ_ITEM_CH_NAME_LIST, projs[k]->data->col_names);
        this->write_float_matrix(FIFF_PROJ_ITEM_VECTORS, projs[k]->data->data);
        this->end_block(FIFFB_PROJ_ITEM);
    }
    this->end_block(FIFFB_PROJ);
}


//*************************************************************************************************************

void FiffFile::write_string(fiff_int_t kind, QString& data)
{
    fiff_int_t datasize = data.size();//size(data,2);

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)kind;
    out << (qint32)FIFFT_STRING;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

    const char* dataString = data.toUtf8().constData();
    for(qint32 i = 0; i < datasize; ++i)
        out << dataString[i];

//        count = fwrite(fid,int32(kind),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFT_STRING),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(datasize),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFV_NEXT_SEQ),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,data,'uchar');
//        if count ~= datasize
//            error(me,'write failed');
//        end
}
