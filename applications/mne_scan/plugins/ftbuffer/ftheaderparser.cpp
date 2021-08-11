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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtEndian>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using FTBUFFERPLUGIN::FtHeaderParser;
using FTBUFFERPLUGIN::MetaData;

//=============================================================================================================
// DEFINE FREE FUNCTIONS
//=============================================================================================================

void FTBUFFERPLUGIN::parseNeuromagHeader(MetaData& data, QBuffer& neuromagBuffer)
{
    qint32_be iIntToChar;
    char cCharFromInt[sizeof (qint32)];

    //Pad buffer because the fiff file we receive is missing an end tag
    iIntToChar = -1;
    memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
    neuromagBuffer.write(cCharFromInt);
    neuromagBuffer.write(cCharFromInt);
    neuromagBuffer.write(cCharFromInt);
    neuromagBuffer.write(cCharFromInt);

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

void FTBUFFERPLUGIN::parseIsotrakHeader(MetaData& data, QBuffer& isotrakBuffer)
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

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtHeaderParser::FtHeaderParser()
{
    registerMembers();
}

//=============================================================================================================

MetaData FtHeaderParser::parseHeader(QBuffer &buffer)
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
    functionMap[HeaderChunk::FT_CHUNK_NEUROMAG_HEADER] = parseNeuromagHeader;
    functionMap[HeaderChunk::FT_CHUNK_NEUROMAG_ISOTRAK] = parseIsotrakHeader;
}

//=============================================================================================================

void FtHeaderParser::processChunk(MetaData& data , QBuffer& buffer)
{
    auto chunkType = getChunkType(buffer);
    auto function = functionMap.find(chunkType);

    QBuffer headerChunk;
    getSingleHeaderChunk(buffer, headerChunk);

    if (function != functionMap.end()){
        function->second(data, headerChunk);
        data.lHeaderList.push_back(chunkType);
    }
}

//=============================================================================================================

void FtHeaderParser::getSingleHeaderChunk(QBuffer &source, QBuffer &dest)
{
    qint32 iSize;
    char cSize[sizeof(qint32)];

    //read size of chunk
    source.read(cSize, sizeof(qint32));
    std::memcpy(&iSize, cSize, sizeof(qint32));

    //Read relevant chunk info
    dest.open(QIODevice::ReadWrite);
    dest.write(source.read(iSize));
}

//=============================================================================================================

HeaderChunk FtHeaderParser::getChunkType(QBuffer &buffer)
{
    qint32 iType;
    char cType[sizeof(qint32)];

    buffer.read(cType, sizeof(qint32));
    std::memcpy(&iType, cType, sizeof(qint32));

    std::cout << "Read header of type" << iType << "\n";
    return static_cast<HeaderChunk>(iType);
}

