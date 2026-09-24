//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftheaderparser.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     July, 2021
 * @brief    Contains the definition of the FtHeaderParser class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftheaderparser.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

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

