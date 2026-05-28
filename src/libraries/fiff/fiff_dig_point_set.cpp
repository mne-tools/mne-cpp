//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     fiff_dig_point_set.cpp
 * @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     July 2016
 * @brief    Implementation of @ref FiffDigPointSet: parses an entire FIFFB_ISOTRAK block into a list of @ref FiffDigPoint records.
 *
 * Mirrors the @c info['dig'] list in MNE-Python. Used by the
 * registration GUIs and by the head-shape coregistration paths.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point_set.h"

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"
#include "fiff_types.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <stdexcept>
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigPointSet::FiffDigPointSet()
    :m_qListDigPoint()
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(const FiffDigPointSet &p_FiffDigPointSet)
: m_qListDigPoint(p_FiffDigPointSet.m_qListDigPoint)
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(QList<FIFFLIB::FiffDigPoint> pointList)
: m_qListDigPoint(pointList)
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(QIODevice &p_IODevice)   //const FiffDigPointSet &p_FiffDigPointSet
{
    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if(!FiffDigPointSet::readFromStream(t_pStream, *this)) {
        t_pStream->close();
        throw std::runtime_error("Could not read the FiffDigPointSet");
    }

    qInfo("[FiffDigPointSet::FiffDigPointSet] %i digitizer Points read from file.", this->size());
}

//=============================================================================================================

FiffDigPointSet::~FiffDigPointSet()
{
}

//=============================================================================================================

bool FiffDigPointSet::readFromStream(FiffStream::SPtr &p_pStream, FiffDigPointSet &p_Dig)
{
    //
    //   Open the file, create directory
    //
    bool open_here = false;

    if (!p_pStream->device()->isOpen()) {
        QString t_sFileName = p_pStream->streamName();

        if(!p_pStream->open())
            return false;

        qInfo("Opening header data %s...\n",t_sFileName.toUtf8().constData());

        open_here = true;
    }

    //
    //   Read the measurement info
    //
    //read_hpi_info(p_pStream,p_Tree, info);
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;
    FiffTag::UPtr t_pTag;

    //
    //   Locate the Electrodes
    //
    QList<FiffDirNode::SPtr> isotrak = p_pStream->dirtree()->dir_tree_find(FIFFB_ISOTRAK);

    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    qint32 k = 0;

    if (isotrak.size() == 1)
    {
        for (k = 0; k < isotrak[0]->nent(); ++k)
        {
            kind = isotrak[0]->dir[k]->kind;
            pos  = isotrak[0]->dir[k]->pos;
            if (kind == FIFF_DIG_POINT)
            {
                p_pStream->read_tag(t_pTag, pos);
                p_Dig.m_qListDigPoint.append(t_pTag->toDigPoint());
            }
            else
            {
                if (kind == FIFF_MNE_COORD_FRAME)
                {
                    p_pStream->read_tag(t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                }
                else if (kind == FIFF_COORD_TRANS)
                {
                    p_pStream->read_tag(t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for(k = 0; k < p_Dig.size(); ++k)
    {
        p_Dig[k].coord_frame = coord_frame;
    }

    //
    //   All kinds of auxliary stuff
    //
    if(open_here)
    {
        p_pStream->close();
    }
    return true;
}

//=============================================================================================================

void FiffDigPointSet::write(QIODevice &p_IODevice)
{
    //
    //   Open the file, create directory
    //

    // Create the file and save the essentials
    FiffStream::SPtr t_pStream = FiffStream::start_file(p_IODevice);
    qInfo("Write Digitizer Points in %s...\n", t_pStream->streamName().toUtf8().constData());
    this->writeToStream(t_pStream.data());
    t_pStream->end_file();
}

//=============================================================================================================

bool FiffDigPointSet::write(const QString& filePath, QString* errorMessage)
{
    if (filePath.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Output path is empty.");
        return false;
    }

    const QFileInfo info(filePath);
    if (!info.dir().exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Destination directory '%1' does not exist.")
                                .arg(info.dir().absolutePath());
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot open '%1' for writing: %2")
                                .arg(filePath, file.errorString());
        }
        return false;
    }

    write(file);
    file.close();

    if (!QFileInfo::exists(filePath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("FiffDigPointSet::write produced no file at '%1'.")
                                .arg(filePath);
        }
        return false;
    }
    return true;
}

//=============================================================================================================

void FiffDigPointSet::writeToStream(FiffStream* p_pStream)
{
    p_pStream->start_block(FIFFB_MEAS);
    p_pStream->start_block(FIFFB_MEAS_INFO);
    p_pStream->start_block(FIFFB_ISOTRAK);

    for(qint32 h = 0; h < m_qListDigPoint.size(); ++h)
    {
        p_pStream->write_dig_point(m_qListDigPoint[h]);
    }

    qInfo("\t%lld digitizer points written\n", m_qListDigPoint.size());
    p_pStream->end_block(FIFFB_ISOTRAK);
    p_pStream->end_block(FIFFB_MEAS_INFO);
    p_pStream->end_block(FIFFB_MEAS);
}

//=============================================================================================================

const FiffDigPoint& FiffDigPointSet::operator[] (qint32 idx) const
{
    if (idx>=m_qListDigPoint.length())
    {
        qWarning("Warning: Required DigPoint doesn't exist! Returning DigPoint '0'.");
        idx=0;
    }
    return m_qListDigPoint[idx];
}

//=============================================================================================================

FiffDigPoint& FiffDigPointSet::operator[] (qint32 idx)
{
    if (idx >= m_qListDigPoint.length())
    {
        qWarning("Warning: Required DigPoint doesn't exist! Returning DigPoint '0'.");
        idx = 0;
    }
    return m_qListDigPoint[idx];
}

//=============================================================================================================

FiffDigPointSet FiffDigPointSet::pickTypes(QList<int> includeTypes) const
{
    FiffDigPointSet pickedSet;

    for(int i = 0; i < m_qListDigPoint.size(); ++i) {
        if(includeTypes.contains(m_qListDigPoint[i].kind)) {
            pickedSet << m_qListDigPoint[i];
        }
    }

    return pickedSet;
}

//=============================================================================================================

FiffDigPointSet &FiffDigPointSet::operator<<(const FiffDigPoint &dig)
{
    this->m_qListDigPoint.append(dig);
    return *this;
}

//=============================================================================================================

FiffDigPointSet &FiffDigPointSet::operator<<(const FiffDigPoint *dig)
{
    this->m_qListDigPoint.append(*dig);
    return *this;
}

//=============================================================================================================

void FiffDigPointSet::applyTransform(const FiffCoordTrans& coordTrans, bool bApplyInverse)
{
    Vector4f tempvec;
    for(int i = 0; i < m_qListDigPoint.size(); ++i) {
        tempvec(0) = m_qListDigPoint.at(i).r[0];
        tempvec(1) = m_qListDigPoint.at(i).r[1];
        tempvec(2) = m_qListDigPoint.at(i).r[2];
        tempvec(3) = 1.0f;
        if(bApplyInverse) {
            tempvec = coordTrans.invtrans * tempvec;
        } else {
            tempvec = coordTrans.trans * tempvec;
        }
        m_qListDigPoint[i].r[0] = tempvec(0);
        m_qListDigPoint[i].r[1] = tempvec(1);
        m_qListDigPoint[i].r[2] = tempvec(2);
    }
}

//=============================================================================================================

QList<FiffDigPoint> FiffDigPointSet::getList()
{
    return m_qListDigPoint;
}
