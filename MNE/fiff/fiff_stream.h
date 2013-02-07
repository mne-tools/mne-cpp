//=============================================================================================================
/**
* @file     fiff_stream.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief    FiffStream class declaration
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
#include "fiff_coord_trans.h"
#include "fiff_ch_info.h"
#include "fiff_dig_point.h"
#include "fiff_cov.h"
#include "fiff_dir_tree.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//*************************************************************************************************************
//=============================================================================================================
// Forward Declarations
//=============================================================================================================

class FiffStream;
class FiffTag;
class FiffCtfComp;
class FiffRawData;
class FiffInfo;


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

class FIFFSHARED_EXPORT FiffStream : public QDataStream
{
public:
    typedef QSharedPointer<FiffStream> SPtr;            /**< Shared pointer type for FiffStream. */
    typedef QSharedPointer<const FiffStream> ConstSPtr; /**< Const shared pointer type for FiffStream. */

    //=========================================================================================================
    /**
    * Constructs a fiff stream that uses the I/O device p_pIODevice.
    *
    * @param[in] p_pIODevice    A fiff IO device like a fiff QFile or QTCPSocket
    */
    explicit FiffStream(QIODevice* p_pIODevice);

    //=========================================================================================================
    /**
    * Constructs a fiff stream that operates on a byte array, a. The mode describes how the device is to be used.
    *
    * @param[in] a      The byte array
    * @param[in] mode   The open mode
    */
    explicit FiffStream(QByteArray * a, QIODevice::OpenMode mode);

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
    * @param[out] p_Tree    tag directory organized into a tree
    * @param[out] p_Dir     the sequential tag directory
    *
    * @return true if succeeded, false otherwise
    */
    bool open(FiffDirTree& p_Tree, QList<FiffDirEntry>& p_Dir);

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
    * @param[in] p_Node The node of interest
    *
    * @return the bad channel list
    */
    QStringList read_bad_channels(const FiffDirTree& p_Node);

    //=========================================================================================================
    /**
    * mne_read_cov - also for mne_read_noise_cov
    *
    * ### MNE toolbox root function ###
    *
    * Reads a covariance matrix from a fiff file
    *
    * @param [in] p_Node          look for the matrix in here
    * @param [in] cov_kind      what kind of a covariance matrix do we want?
    * @param [out] p_covData    the read covariance matrix
    *
    * @return true if succeeded, false otherwise
    */
    bool read_cov(const FiffDirTree& p_Node, fiff_int_t cov_kind, FiffCov& p_covData);

    //=========================================================================================================
    /**
    * fiff_read_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * Read the CTF software compensation data from the given node
    *
    * @param[in] p_Node The node of interest
    * @param[in] p_Chs  channels with the calibration info
    *
    * @return the CTF software compensation data
    */
    QList<FiffCtfComp> read_ctf_comp(const FiffDirTree& p_Node, const QList<FiffChInfo>& p_Chs);

    //=========================================================================================================
    /**
    * fiff_read_meas_info
    *
    * ### MNE toolbox root function ###
    *
    * Read the measurement info
    * Source is assumed to be an open fiff file.
    *
    * @param[in] p_Node       The node of interest
    * @param[out] p_Info      The read measurement info
    * @param[out] p_NodeInfo  The to measurement corresponding fiff_dir_tree.
    *
    * @return the to measurement corresponding fiff_dir_tree.
    */
    bool read_meas_info(const FiffDirTree& p_Node, FiffInfo& p_Info, FiffDirTree& p_NodeInfo);

    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Reads a named matrix.
    *
    * @param[in] p_Node     The node of interest
    * @param[in] matkind    The matrix kind to look for
    * @param[out] mat       The named matrix
    *
    * @return true if succeeded, false otherwise
    */
    bool read_named_matrix(const FiffDirTree& p_Node, fiff_int_t matkind, FiffNamedMatrix& mat);

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
    * @param[in] p_Node    The node of interest
    *
    * @return a list of SSP projectors
    */
    QList<FiffProj> read_proj(const FiffDirTree& p_Node);

    //=========================================================================================================
    /**
    * fiff_setup_read_raw
    *
    * ### MNE toolbox root function ###
    *
    * Read information about raw data file
    *
    * @param[in] p_pIODevice        An fiff IO device like a fiff QFile or QTCPSocket
    * @param[out] data              The raw data information - contains the opened fiff file
    * @param[in] allow_maxshield    Accept unprocessed MaxShield data
    *
    * @return true if succeeded, false otherwise
    */
    static bool setup_read_raw(QIODevice* p_pIODevice, FiffRawData& data, bool allow_maxshield = false);

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
    * @param[in] p_pIODevice    A fiff IO device like a fiff QFile or QTCPSocket
    * @param[in] info           The measurement info block of the source file
    * @param[out] cals          Thecalibration matrix
    * @param[in] sel            Which channels will be included in the output file (optional)
    *
    * @return the started fiff file
    */
    static FiffStream* start_writing_raw(QIODevice* p_pIODevice, const FiffInfo& info, MatrixXd& cals, MatrixXi sel = defaultMatrixXi);

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
    void write_coord_trans(const FiffCoordTrans& trans);

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
    void write_ctf_comp(const QList<FiffCtfComp>& comps);

    //=========================================================================================================
    /**
    * fiff_write_dig_point
    *
    * ### MNE toolbox root function ###
    *
    * Writes a digitizer data point into a fif file
    *
    * @param[in] dig        The point to write
    */
    void write_dig_point(const FiffDigPoint& dig);

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
    void write_id(fiff_int_t kind, const FiffId& id = defaultFiffId);

    //=========================================================================================================
    /**
    * fiff_write_int
    *
    * ### MNE toolbox root function ###
    *
    * Writes a 32-bit integer tag to a fif file
    *
    * @param[in] kind       Tag kind
    * @param[in] data       The integer data pointer
    * @param[in] nel        Number of integers to write (default = 1)
    */
    void write_int(fiff_int_t kind, const fiff_int_t* data, fiff_int_t nel = 1);

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
    void write_float(fiff_int_t kind, const float* data, fiff_int_t nel = 1);

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
    void write_float_matrix(fiff_int_t kind, const MatrixXd& mat);


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
    void write_name_list(fiff_int_t kind, const QStringList& data);

    //=========================================================================================================
    /**
    * fiff_write_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Writes a named single-precision floating-point matrix
    *
    * @param[in] kind       The tag kind to use for the data
    * @param[in] mat        The data matrix
    */
    void write_named_matrix(fiff_int_t kind, const FiffNamedMatrix& mat);

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
    void write_proj(const QList<FiffProj>& projs);

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
    bool write_raw_buffer(const MatrixXd& buf, const MatrixXd& cals);

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
    void write_string(fiff_int_t kind, const QString& data);

    //=========================================================================================================
    /**
    * Writes a real-time command
    *
    * @param[in] command    The real time command
    * @param[in] data       The string data to write
    */
    void write_rt_command(fiff_int_t command, const QString& data);
};

} // NAMESPACE

#endif // FIFF_STREAM_H
