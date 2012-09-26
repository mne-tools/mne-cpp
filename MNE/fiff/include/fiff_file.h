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


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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



static FiffId defaultFiffId;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



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
    * ToDo make this part of the FiffFile classs
    *
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
    bool start_file()
    {
        if(!this->open(QIODevice::WriteOnly))
        {
            printf("Cannot write to %s\n", this->fileName().toUtf8().constData());//consider throw
            return false;
        }

        //
        //   Write the compulsory items
        //
        //FIFF_FILE_ID = 100;
        //FIFF_DIR_POINTER=101;
        //FIFF_FREE_LIST=106;

        this->write_id(FIFF_FILE_ID);
//        fiff_write_int(fid,FIFF_DIR_POINTER,-1);
//        fiff_write_int(fid,FIFF_FREE_LIST,-1);
//        //
        //   Ready for more
        //
        return true;
    }




    //
    // fiff_write_id(fid,kind,id)
    //
    // Writes fiff id
    //
    //     fid           An open fif file descriptor
    //     kind          The tag kind
    //     id            The id to write
    //
    // If the id argument is missing it will be generated here
    //

    void write_id(fiff_int_t kind, FiffId& id = defaultFiffId)
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
//        count = fwrite(fid,int32(kind),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end

        out << (qint32)FIFFT_ID_STRUCT;
//        count = fwrite(fid,int32(FIFFT_ID_STRUCT),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end

        out << (qint32)datasize;
//        count = fwrite(fid,int32(datasize),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end

        out << (qint32)FIFFV_NEXT_SEQ;
//        count = fwrite(fid,int32(FIFFV_NEXT_SEQ),'int32');
//        if count ~= 1
//            error(me,'write failed');
//        end
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
//        count = fwrite(fid,int32(data),'int32');
//        if count ~= 5
//            error(me,'write failed');
//        end


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











public:

};

} // NAMESPACE

#endif // FIFF_FILE_H
