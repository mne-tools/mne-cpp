//=============================================================================================================
/**
* @file     fiff_file.h
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
* @brief    Contains the FiffFile class declaration.
*
*/

#ifndef FIFF_FILE_H
#define FIFF_FILE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_coord_trans.h"
#include "fiff_proj.h"
#include "fiff_ctf_comp.h"
#include "fiff_ch_info.h"
#include "fiff_info.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../../../include/3rdParty/Eigen/Core"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffDirTree;

static MatrixXi defaultFileMatrixXi(0,0);
static FiffId defaultFiffId;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS FiffFile
*
*
* @brief The FiffFile class provides...
**/

class FIFFSHARED_EXPORT FiffFile : public QFile {

public:
    //=========================================================================================================
    /**
    * ctor
    *
    * @param[in] p_sFileName file name of the file to open
    */
    FiffFile(QString& p_sFilename);

    //=========================================================================================================
    /**
    * Destroys the FiffInfo.
    */
    ~FiffFile();

    //=========================================================================================================
    /**
    * fiff_end_block
    *
    * fiff_end_block(fid, kind)
    *
    * Writes a FIFF_BLOCK_END tag
    *
    *     fid           An open fif file descriptor
    *     kind          The block kind to end
    *
    */
    void end_block(fiff_int_t kind)
    {
        this->write_int(FIFF_BLOCK_END,&kind);
    }


    //=========================================================================================================
    /**
    * fiff_end_file
    *
    * fiff_end_file(fid)
    *
    * Writes the closing tags to a fif file and closes the file
    *
    *     fid           An open fif file descriptor
    *
    */
    void end_file()
    {
        fiff_int_t datasize = 0;

        QDataStream out(this);   // we will serialize the data into the file
        out.setByteOrder(QDataStream::BigEndian);

        out << (qint32)FIFF_NOP;
        out << (qint32)FIFFT_VOID;
        out << (qint32)datasize;
        out << (qint32)FIFFV_NEXT_NONE;

//        count = fwrite(fid, int32(FIFF.FIFF_NOP), 'int32');
//        if count ~= 1
//            error(me, 'write failed');
//        end
//        count = fwrite(fid, int32(FIFF.FIFFT_VOID), 'int32');
//        if count ~= 1
//            error(me, 'write failed');
//        end
//        count = fwrite(fid, int32(datasize), 'int32');
//        if count ~= 1
//            error(me, 'write failed');
//        end
//        count = fwrite(fid, int32(FIFF.FIFFV_NEXT_NONE), 'int32');
//        if count ~= 1
//            error(me, 'write failed');
//        end
    }









    //=========================================================================================================
    /**
    * fiff_finish_writing_raw
    *
    * function fiff_finish_writing_raw(fid)
    %
    % fid        of an open raw data file
    %
    *
    */
    void finish_writing_raw()
    {
        this->end_block(FIFFB_RAW_DATA);
        this->end_block(FIFFB_MEAS);
        this->end_file();
    }


    //=========================================================================================================
    /**
    * QFile::open
    *
    * unmask base class open function
    */
    using QFile::open;

    //=========================================================================================================
    /**
    * fiff_open
    *
    * ### MNE toolbox root function ###
    *
    * Opens a fif file and provides the directory of tags
    *
    * @param[out] p_pTree tag directory organized into a tree
    * @param[out] p_pDir the sequential tag directory
    *
    * @return true if succeeded, false otherwise
    */
    bool open(FiffDirTree*& p_pTree, QList<fiff_dir_entry_t>*& p_pDir);




    //=========================================================================================================
    /**
    * fiff_start_block
    *
    * fiff_start_block(fid,kind)
    *
    * Writes a FIFF_BLOCK_START tag
    *
    *     fid           An open fif file descriptor
    *     kind          The block kind to start
    *
    */
    void start_block(fiff_int_t kind)
    {
        this->write_int(FIFF_BLOCK_START,&kind);
    }

    //=========================================================================================================
    /**
    * fiff_start_file
    *
    * ### MNE toolbox root function ###
    *
    * [fid] = fiff_start_file(name)
    *
    * Opens a fiff file for writing and writes the compulsory header tags
    *
    *     name           The name of the file to open. It is recommended
    *                    that the name ends with .fif
    *
    */
    static FiffFile* start_file(QString& p_sFilename);


    //=========================================================================================================
    /**
    * fiff_start_writing_raw
    *
    * ### MNE toolbox root function ###
    *
    * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
    *
    * name       filename
    * info       The measurement info block of the source file
    * sel        Which channels will be included in the output file (optional)
    *
    */
    static FiffFile* start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXf*& cals, MatrixXi sel = defaultFileMatrixXi);


    //=========================================================================================================
    /**
    * fiff_write_ch_info
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_ch_info(fid,ch)
    *
    * Writes a channel information record to a fif file
    *
    *     fid           An open fif file descriptor
    *     ch            The channel information structure to write
    *
    *     The type, cal, unit, and pos members are explained in Table 9.5
    *     of the MNE manual
    *
    */
    void write_ch_info(FiffChInfo* ch)
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
//        count = fwrite(fid,int32(FIFF_CH_INFO),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(FIFFT_CH_INFO_STRUCT),'int32');
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
        //   Start writing fiffChInfoRec
        //
        out << (qint32)ch->scanno;
        out << (qint32)ch->logno;
        out << (qint32)ch->kind;
        out << (float)ch->range;
        out << (float)ch->cal;
//        count = fwrite(fid,int32(ch.scanno),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(ch.logno),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(ch.kind),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(ch.range),'single');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(ch.cal),'single');
//        if count ~= 1
//            error(me,'write failed');
//        end
        //
        //   fiffChPosRec follows
        //
        out << (qint32)ch->coil_type;
        qint32 i;
        for(i = 0; i < 12; ++i)
            out << ch->loc(i,0);
//        count = fwrite(fid,int32(ch.coil_type),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,single(ch.loc),'single');
//        if count ~= 12
//            error(me,'write failed');
//        end
        //
        //   unit and unit multiplier
        //
        out << (qint32)ch->unit;
        out << (qint32)ch->unit_mul;
//        count = fwrite(fid,int32(ch.unit),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
//        count = fwrite(fid,int32(ch.unit_mul),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
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



        const char* dataString = ch_name.toUtf8().constData();
        for(i = 0; i < len; ++i)
            out << dataString[i];
//        count = fwrite(fid,ch_name,'char');
//        if count ~= len
//            error(me,'write failed');
//        end

        if (len < 16)
        {
            char chNull = NULL;
            for(i = 0; i < 16-len; ++i)
                out << chNull;
//            dum=zeros(1,16-len);
//            count = fwrite(fid,uint8(dum),'uchar');
//            if count ~= 16-len
//                error(me,'write failed');
//            end
        }
    }


    //=========================================================================================================
    /**
    * fiff_write_coord_trans
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_coord_trans(fid,trans)
    *
    * Writes a coordinate transformation structure
    *
    *     fid           An open fif file descriptor
    *     trans         The coordinate transfomation structure
    *
    */
    void write_coord_trans(FiffCoordTrans& trans);



    //=========================================================================================================
    /**
    * fiff_write_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_ctf_comp(fid,comps)
    *
    * Writes the CTF compensation data into a fif file
    *
    *     fid           An open fif file descriptor
    *     comps         The compensation data to write
    *
    */
    void write_ctf_comp(QList<FiffCtfComp*>& comps);


    //=========================================================================================================
    /**
    * fiff_write_dig_point
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_dig_point(fid,dig)
    *
    * Writes a digitizer data point into a fif file
    *
    *     fid           An open fif file descriptor
    *     dig           The point to write
    *
    */
    void write_dig_point(fiff_dig_point_t& dig);


    //=========================================================================================================
    /**
    * fiff_write_id
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_id(fid,kind,id)
    *
    * Writes fiff id
    *
    *     fid           An open fif file descriptor
    *     kind          The tag kind
    *     id            The id to write
    *
    * If the id argument is missing it will be generated here
    *
    */
    void write_id(fiff_int_t kind, FiffId& id = defaultFiffId);


    //=========================================================================================================
    /**
    * fiff_write_int
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_int(fid,kind,data)
    *
    * Writes a 32-bit integer tag to a fif file
    *
    *     fid           An open fif file descriptor
    *     kind          Tag kind
    *     data          The integers to use as data
    *     nel           Zahl an Elementen in der data size
    */
    void write_int(fiff_int_t kind, fiff_int_t* data, fiff_int_t nel = 1);


    //=========================================================================================================
    /**
    * fiff_write_float
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_float(fid,kind,data)
    *
    * Writes a single-precision floating point tag to a fif file
    *
    *     fid           An open fif file descriptor
    *     kind          Tag kind
    *     data          The data
    *
    */
    void write_float(fiff_int_t kind, float* data, fiff_int_t nel = 1);


    //=========================================================================================================
    /**
    * fiff_write_float_matrix
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_float_matrix(fid,kind,mat)
    *
    * Writes a single-precision floating-point matrix tag
    *
    *     fid           An open fif file descriptor
    *     kind          The tag kind
    *     mat           The data matrix
    *
    */
    void write_float_matrix(fiff_int_t kind, MatrixXf& mat);


    //=========================================================================================================
    /**
    * fiff_write_name_list
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_name_list(fid,kind,mat)
    *
    * Writes a colon-separated list of names
    *
    *     fid           An open fif file descriptor
    *     kind          The tag kind
    *     data          An array of names to create the list from
    *
    */
    void write_name_list(fiff_int_t kind,QStringList& data);


    //=========================================================================================================
    /**
    * fiff_write_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_named_matrix(fid,kind,mat)
    *
    * Writes a named single-precision floating-point matrix
    *
    *     fid           An open fif file descriptor
    *     kind          The tag kind to use for the data
    *     mat           The data matrix
    */
    void write_named_matrix(fiff_int_t kind,FiffNamedMatrix* mat);


    //=========================================================================================================
    /**
    * fiff_write_proj
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_proj(fid,projs)
    *
    * Writes the projection data into a fif file
    *
    *     fid           An open fif file descriptor
    *     projs         The compensation data to write
    */
    void write_proj(QList<FiffProj*>& projs);




    //=========================================================================================================
    /**
    * fiff_write_raw_buffer
    *
    * ### MNE toolbox root function ###
    *
    * function fiff_write_raw_buffer(fid,info,buf)
    *
    * fid        of an open raw data file
    * buf        the buffer to write
    * cals       calibration factors
    *
    *
    */
    bool write_raw_buffer(MatrixXf* buf, MatrixXf* cals)
    {
        if (buf->rows() != cals->cols())
        {
            printf("buffer and calibration sizes do not match\n");
            return false;
        }

        MatrixXf calsMat(cals->transpose().asDiagonal());
        MatrixXf tmp = calsMat.inverse()*(*buf);

        this->write_float(FIFF_DATA_BUFFER,tmp.data(),tmp.rows()*tmp.cols()); // XXX why not diag(1./cals) ???
        return true;
    }

    //=========================================================================================================
    /**
    * fiff_write_string
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_string(fid,kind,data)
    *
    * Writes a string tag
    *
    *     fid           An open fif file descriptor
    *     kind          The tag kind
    *     data          The string data to write
    */
    void write_string(fiff_int_t kind, QString& data);
};

} // NAMESPACE

#endif // FIFF_FILE_H
