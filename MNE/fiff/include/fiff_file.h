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
    void write_ctf_comp(QList<FiffCtfComp*>& comps)
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
//ToDo            this->write_named_matrix(FIFF_MNE_CTF_COMP_DATA,comp->data);
            this->end_block(FIFFB_MNE_CTF_COMP_DATA);

            delete comp;
        }
        this->end_block(FIFFB_MNE_CTF_COMP);

        return;

    }





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
