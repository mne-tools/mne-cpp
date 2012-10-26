//=============================================================================================================
/**
* @file     fiff_stream.h
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
* @brief    Contains the FiffStream class declaration
*
*/

#ifndef FIFF_STREAM_H
#define FIFF_STREAM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_dir_entry.h"
#include "fiff_coord_trans.h"
#include "fiff_proj.h"
#include "fiff_ctf_comp.h"
#include "fiff_ch_info.h"
#include "fiff_dig_point.h"
#include "fiff_info.h"
#include "fiff_raw_data.h"
#include "fiff_cov.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../3rdParty/Eigen/Core"


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
class FiffStream;
class FiffTag;
class FiffRawData;

static FiffId defaultFiffId;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* FiffStream provides an interface for reading from and writing to fiff files
*
* @brief FIFF File I/O routines.
**/

class FIFFSHARED_EXPORT FiffStream : public QDataStream {

public:
    //=========================================================================================================
    /**
    * ctor
    *
    * @param[in] p_pIODevice    A fiff IO device like a fiff QFile or QTcpSocket
    */
    FiffStream(QIODevice* p_pIODevice);

    //=========================================================================================================
    /**
    * Destroys the FiffInfo.
    */
    ~FiffStream();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_end_block function
    *
    * Writes a FIFF_BLOCK_END tag
    *
    * @param[in] kind The block kind to end
    */
    void end_block(fiff_int_t kind);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_end_file function
    *
    * Writes the closing tags to a fif file and closes the file
    *
    */
    void end_file();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_finish_writing_raw function
    *
    * Finishes a raw file by writing all necessary end tags.
    *
    */
    void finish_writing_raw();

    //=========================================================================================================
    /**
    * QFile::open
    *
    * unmask base class open function
    */
//    using QFile::open;

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
    bool open(FiffDirTree*& p_pTree, QList<FiffDirEntry>*& p_pDir);

    //=========================================================================================================
    /**
    * fiff_read_bad_channels
    *
    * ### MNE toolbox root function ###
    *
    * Reads the bad channel list from a node if it exists
    * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirTree
    *       class, that's why a tree object doesn't need to be handed to the function.
    *
    * @param[in] p_pTree The node of interest
    *
    * @return the bad channel list
    */
    QStringList read_bad_channels(FiffDirTree* p_pTree);

    //=========================================================================================================
    /**
    * mne_read_cov
    *
    * ### MNE toolbox root function ###
    *
    * Reads a covariance matrix from a fiff file
    *
    * @param [in] node          look for the matrix in here
    * @param [in] cov_kind      what kind of a covariance matrix do we want?
    * @param [out] p_covData    the read covariance matrix
    *
    * @return true if succeeded, false otherwise
    */
    bool read_cov(FiffDirTree* node, fiff_int_t cov_kind, FiffCov*& p_covData);

    //=========================================================================================================
    /**
    * fiff_read_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * Read the CTF software compensation data from the given node
    *
    * @param[in] p_pTree    The node of interest
    * @param[in] chs        channels with the calibration info
    *
    * @return the CTF software compensation data
    */
    QList<FiffCtfComp*> read_ctf_comp( FiffDirTree* p_pNode, QList<FiffChInfo>& chs);

    //=========================================================================================================
    /**
    * fiff_read_meas_info
    *
    * ### MNE toolbox root function ###
    *
    * Read the measurement info
    * Source is assumed to be an open fiff file.
    *
    * @param[in] p_pTree    The node of interest
    * @param[out] info      the read measurement info
    *
    * @return the to measurement corresponding fiff_dir_tree.
    */
    FiffDirTree* read_meas_info(FiffDirTree* p_pTree, FiffInfo*& info);

    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Reads a named matrix.
    *
    * @param[in] p_pTree    The node of interest
    * @param[in] matkind    The matrix kind to look for
    * @param[out] mat       The named matrix
    *
    * @return true if succeeded, false otherwise
    */
    bool read_named_matrix(FiffDirTree* p_pTree, fiff_int_t matkind, FiffNamedMatrix*& mat);

    //=========================================================================================================
    /**
    * fiff_read_proj
    *
    * ### MNE toolbox root function ###
    *
    * [ projdata ] = fiff_read_proj(fid,node)
    *
    * Read the SSP data under a given directory node
    *
    * @param[in] p_pTree    The node of interest
    *
    * @return a list of SSP projectors
    */
    QList<FiffProj*> read_proj(FiffDirTree* p_pTree);

    //=========================================================================================================
    /**
    * fiff_setup_read_raw
    *
    * ### MNE toolbox root function ###
    *
    * Read information about raw data file
    *
    * @param[in] p_pIODevice        An fiff IO device like a fiff QFile or QTcpSocket
    * @param[out] data              The raw data information - contains the opened fiff file
    * @param[in] allow_maxshield    Accept unprocessed MaxShield data
    *
    * @return true if succeeded, false otherwise
    */
    static bool setup_read_raw(QIODevice* p_pIODevice, FiffRawData*& data, bool allow_maxshield = false);

    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Splits a string by looking for seperator ":"
    *
    * @param[in] p_sNameList    string to split
    *
    * @return the splitted string list
    */
    static QStringList split_name_list(QString p_sNameList);

    //=========================================================================================================
    /**
    * fiff_start_block
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffStream start_block member function
    *
    * Writes a FIFF_BLOCK_START tag
    *
    * @param[in] kind       The block kind to start
    */
    void start_block(fiff_int_t kind);

    //=========================================================================================================
    /**
    * fiff_start_file
    *
    * ### MNE toolbox root function ###
    *
    * Opens a fiff file for writing and writes the compulsory header tags
    *
    * @param[in] p_pIODevice    The IODevice (like QFile or QTCPSocket) to open. It is recommended that the name ends with .fif
    *
    * @return The opened file.
    */
    static FiffStream* start_file(QIODevice* p_pIODevice);

    //=========================================================================================================
    /**
    * fiff_start_writing_raw
    *
    * ### MNE toolbox root function ###
    *
    * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
    *
    * @param[in] p_pIODevice    A fiff IO device like a fiff QFile or QTcpSocket
    * @param[in] info           The measurement info block of the source file
    * @param[out] cals          Thecalibration matrix
    * @param[in] sel            Which channels will be included in the output file (optional)
    *
    * @return the started fiff file
    */
    static FiffStream* start_writing_raw(QIODevice* p_pIODevice, FiffInfo* info, MatrixXd*& cals, MatrixXi sel = defaultMatrixXi);

    //=========================================================================================================
    /**
    * Get the stream name
    *
    * @return the name of the current stream
    */
    QString streamName();

    //=========================================================================================================
    /**
    * fiff_write_ch_info
    *
    * ### MNE toolbox root function ###
    *
    * Writes a channel information record to a fif file
    * The type, cal, unit, and pos members are explained in Table 9.5
    * of the MNE manual
    *
    * @param[in] ch     The channel information structure to write
    */
    void write_ch_info(FiffChInfo* ch);

    //=========================================================================================================
    /**
    * fiff_write_coord_trans
    *
    * ### MNE toolbox root function ###
    *
    * Writes a coordinate transformation structure
    *
    * @param[in] trans  The coordinate transfomation structure
    */
    void write_coord_trans(FiffCoordTrans* trans);

    //=========================================================================================================
    /**
    * fiff_write_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * Writes the CTF compensation data into a fif file
    *
    * @param[in] comps  The compensation data to write
    */
    void write_ctf_comp(QList<FiffCtfComp*>& comps);

    //=========================================================================================================
    /**
    * fiff_write_dig_point
    *
    * ### MNE toolbox root function ###
    *
    * Writes a digitizer data point into a fif file
    *
    * @param[in] p_pStream    An open fif file
    * @param[in] dig        The point to write
    */
    void write_dig_point(FiffDigPoint& dig);

    //=========================================================================================================
    /**
    * fiff_write_id
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffStream write_id member function
    *
    * Writes fiff id
    * If the id argument is missing it will be generated here
    *
    * @param[in] kind       The tag kind
    * @param[in] id         The id to write
    */
    void write_id(fiff_int_t kind, FiffId& id = defaultFiffId);

    //=========================================================================================================
    /**
    * fiff_write_int
    *
    * ### MNE toolbox root function ###
    *
    * Writes a 32-bit integer tag to a fif file
    *
    * @param[in] p_pStream    An open fif file
    * @param[in] kind       Tag kind
    * @param[in] data       The integer data pointer
    * @param[in] nel        Number of integers to write (default = 1)
    */
    void write_int(fiff_int_t kind, fiff_int_t* data, fiff_int_t nel = 1);

    //=========================================================================================================
    /**
    * fiff_write_float
    *
    * ### MNE toolbox root function ###
    *
    * Writes a single-precision floating point tag to a fif file
    *
    * @param[in] kind       Tag kind
    * @param[in] data       The float data pointer
    * @param[in] nel        Number of floats to write (default = 1)
    */
    void write_float(fiff_int_t kind, float* data, fiff_int_t nel = 1);

    //=========================================================================================================
    /**
    * fiff_write_float_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Writes a single-precision floating-point matrix tag
    *
    * @param[in] kind       The tag kind
    * @param[in] mat        The data matrix
    */
    void write_float_matrix(fiff_int_t kind, const MatrixXd* mat);


    //=========================================================================================================
    /**
    * fiff_write_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Writes a colon-separated list of names
    *
    * @param[in] kind       The tag kind
    * @param[in] data       An array of names to create the list from
    */
    void write_name_list(fiff_int_t kind,QStringList& data);

    //=========================================================================================================
    /**
    * fiff_write_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Writes a named single-precision floating-point matrix
    *
    * @param[in] kind       The tag kind to use for the data
    * @param[in] data       The data matrix
    */
    void write_named_matrix(fiff_int_t kind,FiffNamedMatrix* mat);

    //=========================================================================================================
    /**
    * fiff_write_proj
    *
    * ### MNE toolbox root function ###
    *
    * Writes the projection data into a fif stream (file)
    *
    * @param[in] projs      The compensation data to write
    */
    void write_proj(QList<FiffProj*>& projs);

    //=========================================================================================================
    /**
    * fiff_write_raw_buffer
    *
    * ### MNE toolbox root function ###
    *
    * Writes a raw buffer.
    *
    * @param[in] buf        the buffer to write
    * @param[in] cals       calibration factors
    *
    * @return true if succeeded, false otherwise
    */
    bool write_raw_buffer(MatrixXd* buf, MatrixXd* cals);

    //=========================================================================================================
    /**
    * fiff_write_string
    *
    * ### MNE toolbox root function ###
    *
    * Writes a string tag
    *
    * @param[in] kind       The tag kind
    * @param[in] data       The string data to write
    */
    void write_string(fiff_int_t kind, QString& data);

};

} // NAMESPACE

#endif // FIFF_STREAM_H
