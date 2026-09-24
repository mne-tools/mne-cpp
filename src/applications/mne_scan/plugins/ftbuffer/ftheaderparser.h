//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftheaderparser.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     July, 2021
 * @brief    Contains the declaration of the FtHeaderParser class.
 */

#ifndef FTHEADERPARSER_H
#define FTHEADERPARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_global.h"

#include "ftbuffertypes.h"

#include <fiff/fiff_tag.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_digitizer_data.h>

#include <unordered_map>
#include <functional>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBuffer>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//=============================================================================================================
// DEFINE STRUCTS
//=============================================================================================================

//=============================================================================================================
/**
 * Struct that holds metadata for a measurement.
 */
struct FTBUFFER_EXPORT MetaData{
    bool bFiffInfo = false;
    FIFFLIB::FiffInfo info;
    bool bFiffDigitizerData = false;
    FIFFLIB::FiffDigitizerData dig;

    void setFiffinfo(const FIFFLIB::FiffInfo& newinfo) {info = newinfo;
                                                        bFiffInfo = true;};

    void setFiffDigitizerData(const FIFFLIB::FiffDigitizerData& newdig) {dig = newdig;
                                                                         bFiffDigitizerData = true;};
};

//=============================================================================================================
// DEFINE FREE FUNCTIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Parses header chunk FT_CHUNK_NEUROMAG_HEADER = 8
 *
 * @param[in, out] data         MetaData object that gets updated with measurment info from header
 * @param[in] neuromagBuffer    Buffer containing the data portion of the neuromag header chunk
 */
FTBUFFER_EXPORT void parseNeuromagHeader(MetaData& data, QBuffer& neuromagBuffer);

//=============================================================================================================
/**
 * Parses headr chunk FT_CHUNK_NEUROMAG_ISOTRAK = 9
 *
 * @param[in, out] data         MetaData object that gets updated with measurment info from header
 * @param [in] isotrakBuffer    Buffer containing the data portion of the isotrak header chunk
 */
FTBUFFER_EXPORT void parseIsotrakHeader(MetaData& data, QBuffer& isotrakBuffer);

//=============================================================================================================
/**
 * This class parses the extended header chunks of a fieldtrip buffer and returns measurement metadata.
 *
 * Extended header chunks have three components. The chunk type, the data size, and the data. The first two
 * have fixed sizes, both being unsigned 32-bit integers. The size gives the size of the third component, the
 * data. The member functions within read from a QBuffer of extended headers that was returned by the fieldtrip
 * buffer; thse functions expect the 'read head' of the QBuffer to be at the correct location for the component
 * they are trying to read.
 */
class FTBUFFER_EXPORT FtHeaderParser{
public:
    //=========================================================================================================
    /**
     * Constructs a FtHeaderParser object.
     */
    FtHeaderParser();

    //=========================================================================================================
    /**
     * Parses entire extended header and returns the relevant metadata.
     *
     * @param[in] buffer    Buffer containing all of the extended header chunks.
     *
     * @return Returns MetaData object containing buffer/measurement metadata.
     */
    MetaData parseHeader(QBuffer& buffer);

private:
    //=========================================================================================================
    /**
     * Initialize the function map with entreis for the chunk headers we care about.
     */
    void registerMembers();

    //=========================================================================================================
    /**
     * Processes a single filedtrip extended header chunk and updates the MetaData, advances buffer to next chunk.
     *
     * @param[in, out] data     Metadata updated by processing each chunk.
     * @param[in] buffer        Buffer of all extended header chunks. Buffer needs to be at the beggining of a chunk.
     */
    void processChunk(MetaData& data, QBuffer& buffer);

    //=========================================================================================================
    /**
     * Retreives just the data portion of the extended header chunk and places it in a new buffer.
     *
     * @param[in] source        Buffer with all etended header chunks. Buffer needs to be at size of a chunk.
     * @param[in, out] dest     Destination buffer where chunk will be copied.
     */
    void getSingleHeaderChunk(QBuffer& source, QBuffer& dest);

    //=========================================================================================================
    /**
     * Returns type of chunk next up in the buffer.
     *
     * @param[in] buffer    Buffer of all extended header chunks. Buffer needs to be at start of a chunk.
     *
     * @return  Returns enum of the cooresponding header chuk type.
     */
    HeaderChunk getChunkType(QBuffer& buffer);

    std::unordered_map<HeaderChunk, std::function<void(MetaData&, QBuffer&)>> functionMap;  /**< Map of functions to parse header chunks. */
};

}//namespace
#endif // FTHEADERPARSER_H
