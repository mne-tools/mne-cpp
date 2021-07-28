#include "ftheaderparser.h"

#include <QtEndian>


using namespace FTBUFFERPLUGIN;

//=============================================================================================================

FtHeaderParser::FtHeaderParser()
{

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

HeaderChunk FtHeaderParser::getChunkType(QBuffer &buffer)
{
    qint32 iType;
    char cType[sizeof(qint32)];

    buffer.read(cType, sizeof(qint32));
    std::memcpy(&iType, cType, sizeof(qint32));

    std::cout << "Read header of type" << iType << "\n";
    return static_cast<HeaderChunk>(iType);
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

void parseNeuromagHeader(MetaData& data, QBuffer& neuromagBuffer)
{
    qint32_be iIntToChar;
    char cCharFromInt[sizeof (qint32)];

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

void parseIsotrakHeader(MetaData& data, QBuffer& isotrakBuffer)
{
    isotrakBuffer.reset();

    FIFFLIB::FiffStream stream(&isotrakBuffer);
    FIFFLIB::FiffDigitizerData digData;

    stream.open();
    stream.read_digitizer_data(stream.dirtree(), digData);
    stream.close();

    data.setFiffDigitizerData(digData);
}

//=============================================================================================================

void FtHeaderParser::registerMembers()
{
    functionMap = {{HeaderChunk::FT_CHUNK_NEUROMAG_HEADER, parseNeuromagHeader},
                   {HeaderChunk::FT_CHUNK_NEUROMAG_ISOTRAK, parseIsotrakHeader}};
}
