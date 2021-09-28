//=============================================================================================================
/**
 * @file     fiff_dig_point.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffDigPoint Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point.h"
#include "fiff_coord_trans.h"
#include "fiff_stream.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigPoint::FiffDigPoint()
: kind(-1)
, ident(-1)
, coord_frame(-1)
{
    for(qint32 i = 0; i < 3; ++i)
        r[i] = -1;
}

//=============================================================================================================

FiffDigPoint::FiffDigPoint(const FiffDigPoint& p_FiffDigPoint)
: kind(p_FiffDigPoint.kind)
, ident(p_FiffDigPoint.ident)
, coord_frame(p_FiffDigPoint.coord_frame)
{
    for(qint32 i = 0; i < 3; ++i)
        r[i] = p_FiffDigPoint.r[i];
}

//=============================================================================================================

FiffDigPoint::~FiffDigPoint()
{
}

//=============================================================================================================

FiffDigPoint& FiffDigPoint::operator=(FiffDigPoint& rhs)
{
    kind = rhs.kind;
    ident = rhs.ident;
    coord_frame = rhs.coord_frame;
    for(int i = 0; i < 3; ++i )
    {
        r[i] = rhs.r[i];
    }
    return *this;
}

//=============================================================================================================

FiffDigPoint& FiffDigPoint::operator=(FiffDigPoint rhs)
{
    kind = rhs.kind;
    ident = rhs.ident;
    coord_frame = rhs.coord_frame;
    for(int i = 0; i < 3; ++i )
    {
        r[i] = rhs.r[i];
    }
    return *this;
}
