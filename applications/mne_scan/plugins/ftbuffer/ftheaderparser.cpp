//=============================================================================================================
/**
 * @file     ftheaderparser.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 * @since    0.1.9
 * @date     July, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Contains the definition of the FtHeaderParser class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftheaderparser.h"
#include "fiff/fiff_ch_info.h"
#include "fiff/fiff_constants.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtEndian>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;
using FTBUFFERPLUGIN::FtHeaderParser;
using FTBUFFERPLUGIN::MetaData;

//=============================================================================================================
// DEFINE FREE FUNCTIONS
//=============================================================================================================
namespace {

//=============================================================================================================
/**
 * Return a list of names for each channel from the input buffer.
 * @param [in] buffer   Buffer from where to extract the channel names spearated by '\0' characters.
 * @return A list with each channel's label name.
 */
QStringList extractChannelNamesFromBuffer(QBuffer& buffer)
{
    QString singleStringAllNames(buffer.buffer().replace('\0','\n'));
    QStringList channelNames(singleStringAllNames.split('\n'));
    channelNames.removeAll(channelNames[channelNames.size()-1]);
    return channelNames;
}

//=============================================================================================================
/**
 * Parses header chunk FT_CHUNK_CHANNEL_NAMES = 1
 *
 * @param[in, out] data         MetaData object that gets updated with measurment info from header
 * @param[in] neuromagBuffer    Buffer containing the data portion of the neuromag header chunk
 */
void parseChannelNamesHeader( MetaData& data, QBuffer& channelNamesBuffer)
{
    QStringList channelNames(extractChannelNamesFromBuffer(channelNamesBuffer));

//    qInfo() << channelNames;
    FIFFLIB::FiffInfo info;

    info.nchan = static_cast<FIFFLIB::fiff_int_t>(channelNames.size());

    QList<FIFFLIB::FiffChInfo> chanList;
    for ( auto& name : channelNames)
    {
        FIFFLIB::FiffChInfo chanInfo;
        chanInfo.ch_name = name;
        chanInfo.kind = FIFFV_MEG_CH;
        chanInfo.unit = FIFF_UNIT_T;
        chanInfo.unit_mul = FIFF_UNITM_NONE;
        chanList.append(chanInfo);
    }
    info.chs = chanList;
    info.ch_names = channelNames;

    data.setFiffinfo(info);
}

//=============================================================================================================
/**
 * Parses header chunk FT_CHUNK_NEUROMAG_HEADER = 8
 *
 * @param[in, out] data         MetaData object that gets updated with measurment info from header
 * @param[in] neuromagBuffer    Buffer containing the data portion of the neuromag header chunk
 */
void parseNeuromagHeader(MetaData& data, QBuffer& neuromagBuffer)
{
    //Pad buffer because the fiff file we receive is missing an end tag
    char cCharFromInt[sizeof (qint32)];
    qint32_be iIntToChar(-1);
    memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
    neuromagBuffer.write(cCharFromInt, sizeof(quint32));

    neuromagBuffer.reset();

    FIFFLIB::FiffStream::SPtr pStream(new FIFFLIB::FiffStream(&neuromagBuffer));
    pStream->setByteOrder(QDataStream::LittleEndian);

    if(pStream->open()){
        FIFFLIB::FiffInfo FifInfo;
        FIFFLIB::FiffDirNode::SPtr DirNode;
        if(pStream->read_meas_info(pStream->dirtree(), FifInfo, DirNode)){
            data.setFiffinfo(FifInfo);
        }
    }
}

//=============================================================================================================
/**
 * Parses headr chunk FT_CHUNK_NEUROMAG_ISOTRAK = 9
 *
 * @param[in, out] data         MetaData object that gets updated with measurment info from header
 * @param [in] isotrakBuffer    Buffer containing the data portion of the isotrak header chunk
 */
void parseIsotrakHeader(MetaData& data, QBuffer& isotrakBuffer)
{
    isotrakBuffer.reset();

    FIFFLIB::FiffStream stream(&isotrakBuffer);
    FIFFLIB::FiffDigitizerData digData;

    if(stream.open()){
        stream.read_digitizer_data(stream.dirtree(), digData);
        stream.close();

        data.setFiffDigitizerData(digData);
    }
}

} //namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtHeaderParser::FtHeaderParser()
{
    registerMembers();
}

//=============================================================================================================

MetaData FtHeaderParser::parseExtendedHeader(QBuffer &buffer)
{
    MetaData data;
    while(!buffer.atEnd()){
        processChunk(data, buffer);
    }

    return data;
}

//=============================================================================================================

void FtHeaderParser::registerMembers()
{
    chunkParsersMap[HeaderChunkType::FT_CHUNK_CHANNEL_NAMES] = parseChannelNamesHeader;
    chunkParsersMap[HeaderChunkType::FT_CHUNK_NEUROMAG_HEADER] = parseNeuromagHeader;
    chunkParsersMap[HeaderChunkType::FT_CHUNK_NEUROMAG_ISOTRAK] = parseIsotrakHeader;
}

//=============================================================================================================

void FtHeaderParser::processChunk(MetaData& data , QBuffer& buffer)
{
    FtHeaderParser::Chunk chunk = getChunk(buffer);

    auto chunkParser = chunkParsersMap.find(chunk.type);

    if (chunkParser != chunkParsersMap.end()) {
        chunkParser->second(data, *chunk.data);
    }
}

//=============================================================================================================

FtHeaderParser::Chunk FtHeaderParser::getChunk(QBuffer &buffer)
{
    FtHeaderParser::Chunk outChunk;

    char cType[sizeof(quint32)];
    buffer.read(cType, sizeof(quint32));
    std::memcpy(&outChunk.type, cType, sizeof(quint32));

    char cSize[sizeof(quint32)];
    buffer.read(cSize, sizeof(quint32));
    std::memcpy(&outChunk.size, cSize, sizeof(quint32));

    outChunk.data = QSharedPointer<QBuffer>(new QBuffer);

    outChunk.data->open(QIODevice::ReadWrite);
    outChunk.data->write(buffer.read(outChunk.size));

    return outChunk;
}
