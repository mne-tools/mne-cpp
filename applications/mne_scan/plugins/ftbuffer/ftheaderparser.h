//=============================================================================================================
/**
 * @file     ftheaderparser.h
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
 * @brief    Contains the declaration of the FtHeaderParser class.
 *
 */

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


//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//=============================================================================================================
// DEFINE STRUCTS
//=============================================================================================================

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

//=============================================================================================================
// DEFINE FREE FUNCTIONS
//=============================================================================================================

void parseNeuromagHeader(MetaData&, QBuffer&);
void parseIsotrakHeader(MetaData&, QBuffer&);
void parseUnkownType(MetaData&, QBuffer&);

//=============================================================================================================
/**
 * This class parses the extended header chunks of a fieldtrip buffer and returns measurement metadata.
 */
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
