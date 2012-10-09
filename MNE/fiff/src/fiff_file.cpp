
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
        qDebug() << "close File " << this->fileName();
        this->close();
    }
}


//*************************************************************************************************************

void FiffFile::end_block(fiff_int_t kind)
{
    this->write_int(FIFF_BLOCK_END,&kind);
}


//*************************************************************************************************************

void FiffFile::end_file()
{
    fiff_int_t datasize = 0;

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)FIFF_NOP;
    out << (qint32)FIFFT_VOID;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_NONE;
}


//*************************************************************************************************************

void FiffFile::finish_writing_raw()
{
    this->end_block(FIFFB_RAW_DATA);
    this->end_block(FIFFB_MEAS);
    this->end_file();
}


//*************************************************************************************************************

bool FiffFile::open(FiffDirTree*& p_pTree, QList<FiffDirEntry>*& p_pDir)
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
    p_pDir = new QList<FiffDirEntry>;

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
        FiffDirEntry t_fiffDirEntry;
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

    printf("[done]\n");

    //
    //   Back to the beginning
    //
    this->seek(0); //fseek(fid,0,'bof');
    return true;
}


//*************************************************************************************************************

bool FiffFile::setup_read_raw(QString t_sFileName, FiffRawData*& data, bool allow_maxshield)
{
    if(data)
        delete data;
    data = NULL;

    //
    //   Open the file
    //
    printf("Opening raw data file %s...\n",t_sFileName.toUtf8().constData());

    FiffFile* p_pFile = new FiffFile(t_sFileName);
    FiffDirTree* t_pTree = NULL;
    QList<FiffDirEntry>* t_pDir = NULL;

    if(!p_pFile->open(t_pTree, t_pDir))
    {
        if(t_pTree)
            delete t_pTree;

        if(t_pDir)
            delete t_pDir;

        return false;
    }

    //
    //   Read the measurement info
    //
//        [ info, meas ] = fiff_read_meas_info(fid,tree);
    FiffInfo* info = NULL;
    FiffDirTree* meas = t_pTree->read_meas_info(p_pFile, info);

    if (!meas)
        return false; //ToDo garbage collecting

    //
    //   Locate the data of interest
    //
    QList<FiffDirTree*> raw = meas->dir_tree_find(FIFFB_RAW_DATA);
    if (raw.size() == 0)
    {
        raw = meas->dir_tree_find(FIFFB_CONTINUOUS_DATA);
        if(allow_maxshield)
        {
            for (qint32 i = 0; i < raw.size(); ++i)
                if(raw[i])
                    delete raw[i];
            raw = meas->dir_tree_find(FIFFB_SMSH_RAW_DATA);
            if (raw.size() == 0)
            {
                printf("No raw data in %s\n", t_sFileName.toUtf8().constData());
                return false;
            }
        }
        else
        {
            if (raw.size() == 0)
            {
                printf("No raw data in %s\n", t_sFileName.toUtf8().constData());
                return false;
            }
        }
    }

    //
    //   Set up the output structure
    //
    info->filename   = t_sFileName;

    data = new FiffRawData();
    data->file = p_pFile;// fid;
    data->info       = info;
    data->first_samp = 0;
    data->last_samp  = 0;
    //
    //   Process the directory
    //
    QList<FiffDirEntry> dir = raw.at(0)->dir;
    fiff_int_t nent = raw.at(0)->nent;
    fiff_int_t nchan = info->nchan;
    fiff_int_t first = 0;
    fiff_int_t first_samp = 0;
    fiff_int_t first_skip   = 0;
    //
    //  Get first sample tag if it is there
    //
    FiffTag* t_pTag = NULL;
    if (dir.at(first).kind == FIFF_FIRST_SAMPLE)
    {
        FiffTag::read_tag(p_pFile, t_pTag, dir.at(first).pos);
        first_samp = *t_pTag->toInt();
        ++first;
    }
    //
    //  Omit initial skip
    //
    if (dir.at(first).kind == FIFF_DATA_SKIP)
    {
        //
        //  This first skip can be applied only after we know the buffer size
        //
        FiffTag::read_tag(p_pFile, t_pTag, dir.at(first).pos);
        first_skip = *t_pTag->toInt();
        ++first;
    }
    data->first_samp = first_samp;
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
        FiffDirEntry ent = dir.at(k);
        if (ent.kind == FIFF_DATA_SKIP)
        {
            FiffTag::read_tag(p_pFile, t_pTag, ent.pos);
            nskip = *t_pTag->toInt();
        }
        else if(ent.kind == FIFF_DATA_BUFFER)
        {
            //
            //   Figure out the number of samples in this buffer
            //
            switch(ent.type)
            {
                case FIFFT_DAU_PACK16:
                    nsamp = ent.size/(2*nchan);
                    break;
                case FIFFT_SHORT:
                    nsamp = ent.size/(2*nchan);
                    break;
                case FIFFT_FLOAT:
                    nsamp = ent.size/(4*nchan);
                    break;
                case FIFFT_INT:
                    nsamp = ent.size/(4*nchan);
                    break;
                default:
                    printf("Cannot handle data buffers of type %d\n",ent.type);
                    return false;
            }
            //
            //  Do we have an initial skip pending?
            //
            if (first_skip > 0)
            {
                first_samp += nsamp*first_skip;
                data->first_samp = first_samp;
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
            t_RawDir.ent   = ent;
            t_RawDir.first = first_samp;
            t_RawDir.last  = first_samp + nsamp - 1;//ToDo -1 right or is that MATLAB syntax
            t_RawDir.nsamp = nsamp;
            rawdir.append(t_RawDir);
            first_samp += nsamp;
            ++ndir;
        }
    }
    data->last_samp  = first_samp - 1;//ToDo -1 right or is that MATLAB syntax
    //
    //   Add the calibration factors
    //
    MatrixXf cals(1,data->info->nchan);
    cals.setZero();
    for (int k = 0; k < data->info->nchan; ++k)
        cals(0,k) = data->info->chs.at(k).range*data->info->chs.at(k).cal;
    //
    data->cals       = cals;
    data->rawdir     = rawdir;
    //data->proj       = [];
    //data.comp       = [];
    //
    printf("\tRange : %d ... %d  =  %9.3f ... %9.3f secs\n",
           data->first_samp,data->last_samp,
           (double)data->first_samp/data->info->sfreq,
           (double)data->last_samp/data->info->sfreq);
    printf("Ready.\n");
    data->file->close();
    return true;
}


//*************************************************************************************************************

void FiffFile::start_block(fiff_int_t kind)
{
    this->write_int(FIFF_BLOCK_START,&kind);
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
    p_pFile->write_id(FIFF_FILE_ID);//1
    qint32 data = -1;
    p_pFile->write_int(FIFF_DIR_POINTER,&data);//2
    p_pFile->write_int(FIFF_FREE_LIST,&data);//3
    //
    //   Ready for more
    //
    return p_pFile;
}


//*************************************************************************************************************

FiffFile* FiffFile::start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXf*& cals, MatrixXi sel)
{
    //
    //   We will always write floats
    //
    fiff_int_t data_type = 4;
    qint32 k;

    if(sel.cols() == 0)
    {
        sel.resize(1,info->nchan);
        for (k = 0; k < info->nchan; ++k)
            sel(0, k) = k; //+1 when MATLAB notation
    }

    QList<FiffChInfo> chs;

    for(k = 0; k < sel.cols(); ++k)
        chs << info->chs.at(sel(0,k));

    fiff_int_t nchan = chs.size();
    //
    //  Create the file and save the essentials
    //

    FiffFile* t_pFile = start_file(p_sFileName);//1, 2, 3
    t_pFile->start_block(FIFFB_MEAS);//4
    t_pFile->write_id(FIFF_BLOCK_ID);//5
    if(info->meas_id.version != -1)
    {
        t_pFile->write_id(FIFF_PARENT_BLOCK_ID,info->meas_id);//6
    }
    //
    //
    //    Measurement info
    //
    t_pFile->start_block(FIFFB_MEAS_INFO);//7
    //
    //    Blocks from the original
    //
    QList<fiff_int_t> blocks;
    blocks << FIFFB_SUBJECT << FIFFB_HPI_MEAS << FIFFB_HPI_RESULT << FIFFB_ISOTRAK << FIFFB_PROCESSING_HISTORY;
    bool have_hpi_result = false;
    bool have_isotrak    = false;
    if (blocks.size() > 0 && !info->filename.isEmpty())
    {
        FiffFile* t_pFile2 = new FiffFile(info->filename);

        FiffDirTree* t_pTree = NULL;
        QList<FiffDirEntry>* t_pDir = NULL;
        t_pFile2->open(t_pTree, t_pDir);

        for(qint32 k = 0; k < blocks.size(); ++k)
        {
            QList<FiffDirTree*> nodes = t_pTree->dir_tree_find(blocks.at(k));
            FiffDirTree::copy_tree(t_pFile2,t_pTree->id,nodes,t_pFile);
            if(blocks[k] == FIFFB_HPI_RESULT && nodes.size() > 0)
                have_hpi_result = true;

            if(blocks[k] == FIFFB_ISOTRAK && nodes.size() > 0)
                have_isotrak = true;
        }

        delete t_pDir;
        delete t_pTree;
        delete t_pFile2;
        t_pFile2 = NULL;
    }
    //
    //    megacq parameters
    //
    if (!info->acq_pars.isEmpty() || !info->acq_stim.isEmpty())
    {
        t_pFile->start_block(FIFFB_DACQ_PARS);
        if (!info->acq_pars.isEmpty())
            t_pFile->write_string(FIFF_DACQ_PARS, info->acq_pars);

        if (!info->acq_stim.isEmpty())
            t_pFile->write_string(FIFF_DACQ_STIM, info->acq_stim);

        t_pFile->end_block(FIFFB_DACQ_PARS);
    }
    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result)
    {
        if (info->dev_head_t.from != -1)
            t_pFile->write_coord_trans(info->dev_head_t);

        if (info->ctf_head_t.from != -1)
            t_pFile->write_coord_trans(info->ctf_head_t);
    }
    //
    //    Polhemus data
    //
    if (info->dig.size() > 0 && !have_isotrak)
    {
        t_pFile->start_block(FIFFB_ISOTRAK);
        for (qint32 k = 0; k < info->dig.size(); ++k)
            t_pFile->write_dig_point(info->dig[k]);

        t_pFile->end_block(FIFFB_ISOTRAK);
    }
    //
    //    Projectors
    //
    t_pFile->write_proj(info->projs);
    //
    //    CTF compensation info
    //
    t_pFile->write_ctf_comp(info->comps);
    //
    //    Bad channels
    //
    if (info->bads.size() > 0)
    {
        t_pFile->start_block(FIFFB_MNE_BAD_CHANNELS);
        t_pFile->write_name_list(FIFF_MNE_CH_NAME_LIST,info->bads);
        t_pFile->end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //    General
    //
    t_pFile->write_float(FIFF_SFREQ,&info->sfreq);
    t_pFile->write_float(FIFF_HIGHPASS,&info->highpass);
    t_pFile->write_float(FIFF_LOWPASS,&info->lowpass);
    t_pFile->write_int(FIFF_NCHAN,&nchan);
    t_pFile->write_int(FIFF_DATA_PACK,&data_type);
    if (info->meas_date[0] != -1)
        t_pFile->write_int(FIFF_MEAS_DATE,info->meas_date, 2);
    //
    //    Channel info
    //
    if (cals)
        delete cals;
    cals = new MatrixXf(1,nchan);

    for(k = 0; k < nchan; ++k)
    {
        //
        //    Scan numbers may have been messed up
        //
        chs[k].scanno = k+1;//+1 because
        chs[k].range  = 1.0f;
        (*cals)(0,k) = chs[k].cal;
        t_pFile->write_ch_info(&chs[k]);
    }
    //
    //
    t_pFile->end_block(FIFFB_MEAS_INFO);
    //
    // Start the raw data
    //
    t_pFile->start_block(FIFFB_RAW_DATA);

    return t_pFile;
}


//*************************************************************************************************************

void FiffFile::write_ch_info(FiffChInfo* ch)
{
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

    QDataStream out(this);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::BigEndian);

    out << (qint32)FIFF_CH_INFO;
    out << (qint32)FIFFT_CH_INFO_STRUCT;
    out << (qint32)datasize;
    out << (qint32)FIFFV_NEXT_SEQ;

    //
    //   Start writing fiffChInfoRec
    //
    out << (qint32)ch->scanno;
    out << (qint32)ch->logno;
    out << (qint32)ch->kind;

    int iData = 0;
    iData = *(int *)&ch->range;
    out << iData;
    iData = *(int *)&ch->cal;
    out << iData;

    //
    //   fiffChPosRec follows
    //
    out << (qint32)ch->coil_type;
    qint32 i;
    for(i = 0; i < 12; ++i)
    {
        iData = *(int *)&ch->loc(i,0);
        out << iData;
    }

    //
    //   unit and unit multiplier
    //
    out << (qint32)ch->unit;
    out << (qint32)ch->unit_mul;

    //
    //   Finally channel name
    //
    fiff_int_t len = ch->ch_name.size();
    QString ch_name;
    if(len > 15)
    {
        ch_name = ch->ch_name.mid(0, 15);
    }
    else
        ch_name = ch->ch_name;

    len = ch_name.size();


    out.writeRawData(ch_name.toUtf8().constData(),len);

    if (len < 16)
    {
        const char* chNull = "";
        for(i = 0; i < 16-len; ++i)
            out.writeRawData(chNull,1);
    }
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

    //
    //   Start writing fiffCoordTransRec
    //
    out << (qint32)trans.from;
    out << (qint32)trans.to;

    //
    //   The transform...
    //
    qint32 r, c;
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            out << (float)trans.trans(r,c);
    for (r = 0; r < 3; ++r)
        out << (float)trans.trans(r,3);

    //
    //   ...and its inverse
    //
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            out << (float)trans.invtrans(r,c);
    for (r = 0; r < 3; ++r)
        out << (float)trans.invtrans(r,3);
}


//*************************************************************************************************************

void FiffFile::write_ctf_comp(QList<FiffCtfComp*>& comps)
{
    if (comps.size() <= 0)
        return;
    //
    //  This is very simple in fact
    //
    this->start_block(FIFFB_MNE_CTF_COMP);
    for(qint32 k = 0; k < comps.size(); ++k)
    {
        FiffCtfComp* comp = new FiffCtfComp(comps[k]);
        this->start_block(FIFFB_MNE_CTF_COMP_DATA);
        //
        //    Write the compensation kind
        //
        this->write_int(FIFF_MNE_CTF_COMP_KIND, &comp->ctfkind);
        qint32 save_calibrated = comp->save_calibrated;
        this->write_int(FIFF_MNE_CTF_COMP_CALIBRATED, &save_calibrated);
        //
        //    Write an uncalibrated or calibrated matrix
        //
        comp->data->data = (comp->rowcals.diagonal()).inverse()*comp->data->data*(comp->colcals.diagonal()).inverse();
        this->write_named_matrix(FIFF_MNE_CTF_COMP_DATA,comp->data);
        this->end_block(FIFFB_MNE_CTF_COMP_DATA);

        delete comp;
    }
    this->end_block(FIFFB_MNE_CTF_COMP);

    return;
}


//*************************************************************************************************************

void FiffFile::write_dig_point(FiffDigPoint& dig)
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

    //
    //   Start writing fiffDigPointRec
    //
    out << (qint32)dig.kind;
    out << (qint32)dig.ident;
    for(qint32 i = 0; i < 3; ++i)
        out << dig.r[i];
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

    int iData = 0;
    for(qint32 i = 0; i < nel; ++i)
    {
        iData = *(int *)&data[i];
        out << iData;
    }
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
    int iData = 0;

    for(i = 0; i < numel; ++i)
    {
        iData = *(int *)&mat.data()[i];
        out << iData;
    }

    qint32 dims[3];
    dims[0] = mat.cols();
    dims[1] = mat.rows();
    dims[2] = 2;

    for(i = 0; i < 3; ++i)
        out << dims[i];
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
}


//*************************************************************************************************************

void FiffFile::write_name_list(fiff_int_t kind,QStringList& data)
{
    QString all = data.join(":");
    this->write_string(kind,all);
}


//*************************************************************************************************************

void FiffFile::write_named_matrix(fiff_int_t kind,FiffNamedMatrix* mat)
{
    this->start_block(FIFFB_MNE_NAMED_MATRIX);
    this->write_int(FIFF_MNE_NROW, &mat->nrow);
    this->write_int(FIFF_MNE_NCOL, &mat->ncol);
    if (mat->row_names.size() > 0)
       this->write_name_list(FIFF_MNE_ROW_NAMES,mat->row_names);
    if (mat->col_names.size() > 0)
       this->write_name_list(FIFF_MNE_COL_NAMES,mat->col_names);
    this->write_float_matrix(kind,mat->data);
    this->end_block(FIFFB_MNE_NAMED_MATRIX);
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

bool FiffFile::write_raw_buffer(MatrixXf* buf, MatrixXf* cals)
{
    if (buf->rows() != cals->cols())
    {
        printf("buffer and calibration sizes do not match\n");
        return false;
    }

    SparseMatrix<float> inv_calsMat(cals->cols(), cals->cols());

    for(qint32 i = 0; i < cals->cols(); ++i)
        inv_calsMat.insert(i, i) = 1.0/(*cals)(0,i);

    MatrixXf tmp = inv_calsMat*(*buf);
    this->write_float(FIFF_DATA_BUFFER,tmp.data(),tmp.rows()*tmp.cols());
    return true;
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

    out.writeRawData(data.toUtf8().constData(),datasize);
}
