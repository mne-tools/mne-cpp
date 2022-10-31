//=============================================================================================================
/**
 * @file     mne_surface.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNESurface class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <QtCore/QtDebug>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_tag.h>

#include "mne_surface.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESurface::MNESurface()
{
    id = FIFFV_BEM_SURF_ID_UNKNOWN;
    sigma = 1.0;
    np = 0;
    ntri = 0;
    coord_frame = FIFFV_COORD_MRI;
}

//=============================================================================================================

bool MNESurface::read(QIODevice& p_IODevice, QList<MNESurface::SPtr>& surfaces)
{
    FiffStream::SPtr fiffStream(new FiffStream(&p_IODevice));

    if(!fiffStream->open())
    {
        qCritical() << "Could not open FIFF stream!";
        return false;
    }

    return read(fiffStream, false, fiffStream->dirtree(), surfaces);
}

//=============================================================================================================

bool MNESurface::read(FiffStream::SPtr& p_pStream, bool add_geom, const FiffDirNode::SPtr& p_Tree, QList<MNESurface::SPtr>& surfaces)
{
    if(add_geom)
    {
        // TODO Implement complete_surface_info from mne-matlab.
        qWarning() << "add_geom flag is not yet implemented!";
    }

    QList<FiffDirNode::SPtr>bem = p_Tree->dir_tree_find(FIFFB_BEM);
    if(bem.isEmpty())
    {
        qCritical() << "No BEM block found!";
        return false;
    }

    QList<FiffDirNode::SPtr>bemsurf = p_Tree->dir_tree_find(FIFFB_BEM_SURF);
    if(bemsurf.isEmpty())
    {
        qCritical() << "No BEM surfaces found!";
        return false;
    }

    FiffTag::SPtr tag(new FiffTag());
    fiff_int_t coord_frame;
    if(bem[0]->find_tag(p_pStream, FIFF_BEM_COORD_FRAME, tag))
    {
        coord_frame = *tag->toInt();
    }else
    {
        qWarning() << "No FIFF_BEM_COORD_FRAME found!";
        coord_frame = FIFFV_COORD_MRI;
    }

    QList<FiffDirNode::SPtr>::Iterator itBemsurf;
    for(itBemsurf = bemsurf.begin(); itBemsurf != bemsurf.end(); ++itBemsurf)
    {
        MNESurface::SPtr surf;
        if(read(p_pStream, *itBemsurf, coord_frame, surf))
        {
            surfaces.append(surf);
        }else
        {
            qWarning() << "Could not read surface!";
        }
    }

    return true;
}

//=============================================================================================================

bool MNESurface::read(FIFFLIB::FiffStream::SPtr& fiffStream,
        const FIFFLIB::FiffDirNode::SPtr& dir, const fiff_int_t def_coord_frame,
        MNESurface::SPtr& surf)
{
    surf.reset(new MNESurface);
    FiffTag::SPtr tag;

    // Read attributes //
    //-----------------//

    if(dir->find_tag(fiffStream, FIFF_BEM_SURF_ID, tag))
    {
        surf->id = *tag->toInt();
    }else
    {
        surf->id = FIFFV_BEM_SURF_ID_UNKNOWN;
        qWarning() << "ID not found! Default: " << surf->id;
    }

    if(dir->find_tag(fiffStream, FIFF_BEM_SIGMA, tag))
    {
        surf->sigma = *tag->toFloat();
    }else
    {
        surf->sigma = 1.0;
        qWarning() << "sigma not found! Default: " << surf->sigma;
    }

    if(dir->find_tag(fiffStream, FIFF_BEM_SURF_NNODE, tag))
    {
        surf->np = *tag->toInt();
    }else
    {
        qCritical() << "np not found!";
        return false;
    }

    if(dir->find_tag(fiffStream, FIFF_BEM_SURF_NTRI, tag))
    {
        surf->ntri = *tag->toInt();
    }else
    {
        qCritical() << "ntri not found!";
        return false;
    }

    if(dir->find_tag(fiffStream, FIFF_MNE_COORD_FRAME, tag))
    {
        surf->coord_frame = *tag->toInt();
    }else
    {
        qWarning()
                << "FIFF_MNE_COORD_FRAME not found, trying FIFF_BEM_COORD_FRAME.";
        if(dir->find_tag(fiffStream, FIFF_BEM_COORD_FRAME, tag))
        {
            surf->coord_frame = *tag->toInt();
        }else
        {
            surf->coord_frame = def_coord_frame;
            qWarning() << "FIFF_BEM_COORD_FRAME not found! Default: "
                    << surf->coord_frame;
        }
    }

    // Read data //
    //-----------//

    if(dir->find_tag(fiffStream, FIFF_BEM_SURF_NODES, tag) && surf->np > 0)
    {
        //ToDo: rows, cols are exchanged - usually 3 cols -> ToDo: transpose
        surf->rr.resize(3, surf->np);
        surf->rr = tag->toFloatMatrix();
    }else
    {
        qCritical() << "Vertices not found!";
        return false;
    }

    if(dir->find_tag(fiffStream, FIFF_MNE_SOURCE_SPACE_NORMALS, tag)
            && surf->np > 0)
    {
        //ToDo: rows, cols are exchanged - usually 3 cols -> ToDo: transpose
        surf->nn.resize(3, surf->np);
        surf->nn = tag->toFloatMatrix();
    }else
    {
        qWarning() << "Vertex normals not found!";
    }

    if(dir->find_tag(fiffStream, FIFF_BEM_SURF_TRIANGLES, tag) && surf->ntri > 0)
    {
        //ToDo: rows, cols are exchanged - usually 3 cols -> ToDo: transpose
        surf->tris.resize(3, surf->ntri);
        surf->tris = tag->toIntMatrix();
    }else
    {
        qCritical() << "Triangulation not found!";
        return false;
    }

    return true;
}
