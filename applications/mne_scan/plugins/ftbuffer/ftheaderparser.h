#ifndef FTHEADERPARSER_H
#define FTHEADERPARSER_H


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffertypes.h"

#include <fiff/fiff_tag.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/c/fiff_digitizer_data.h>

#include <unordered_map>
#include <functional>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBuffer>


namespace FTBUFFERPLUGIN
{

struct MetaData{
    bool bFiffInfo = false;
    FIFFLIB::FiffInfo info;
    bool bFiffDigitizerData = false;
    FIFFLIB::FiffDigitizerData dig;

    void setFiffinfo(FIFFLIB::FiffInfo newinfo) {info = newinfo;
                                                 bFiffInfo = true;};

    void setFiffDigitizerData(FIFFLIB::FiffDigitizerData newdig) {dig = newdig;
                                                                  bFiffDigitizerData = true;};
};

void parseNeuromagHeader(MetaData&, QBuffer&);
void parseIsotrakHeader(MetaData&, QBuffer&);
void parseUnkownType(MetaData&, QBuffer&);

class FtHeaderParser{
public:
    FtHeaderParser();

    MetaData parseHeader(QBuffer&);

private:
    void registerMembers();

    void processChunk(MetaData&, QBuffer&);

    void getSingleHeaderChunk(QBuffer& source, QBuffer& dest);

    HeaderChunk getChunkType(QBuffer&);

    std::unordered_map<HeaderChunk, std::function<void(MetaData&, QBuffer&)>> functionMap;
};

}//namespace
#endif // FTHEADERPARSER_H
