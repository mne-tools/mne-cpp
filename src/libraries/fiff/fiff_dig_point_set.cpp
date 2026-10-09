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
#include <QRegularExpression>

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
: m_qListDigPoint()
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(const FiffDigPointSet& p_FiffDigPointSet)
: m_qListDigPoint(p_FiffDigPointSet.m_qListDigPoint)
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(QList<FIFFLIB::FiffDigPoint> pointList)
: m_qListDigPoint(pointList)
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(QIODevice& p_IODevice) //const FiffDigPointSet &p_FiffDigPointSet
{
    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if (!FiffDigPointSet::readFromStream(t_pStream, *this)) {
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

bool FiffDigPointSet::readFromStream(FiffStream::SPtr& p_pStream, FiffDigPointSet& p_Dig)
{
    //
    //   Open the file, create directory
    //
    bool open_here = false;

    if (!p_pStream->device()->isOpen()) {
        QString t_sFileName = p_pStream->streamName();

        if (!p_pStream->open())
            return false;

        qInfo("Opening header data %s...\n", t_sFileName.toUtf8().constData());

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

    if (isotrak.size() == 1) {
        for (k = 0; k < isotrak[0]->nent(); ++k) {
            kind = isotrak[0]->dir[k]->kind;
            pos = isotrak[0]->dir[k]->pos;
            if (kind == FIFF_DIG_POINT) {
                p_pStream->read_tag(t_pTag, pos);
                p_Dig.m_qListDigPoint.append(t_pTag->toDigPoint());
            } else {
                if (kind == FIFF_MNE_COORD_FRAME) {
                    p_pStream->read_tag(t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                } else if (kind == FIFF_COORD_TRANS) {
                    p_pStream->read_tag(t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for (k = 0; k < p_Dig.size(); ++k) {
        p_Dig[k].coord_frame = coord_frame;
    }

    //
    //   All kinds of auxliary stuff
    //
    if (open_here) {
        p_pStream->close();
    }
    return true;
}

//=============================================================================================================

namespace
{

// Metres per file unit, as mne's _check_unit_and_get_scaling; 0 for an unknown unit.
double unitScale(const QString& unit)
{
    if (unit == QLatin1String("m"))
        return 1.0;
    if (unit == QLatin1String("cm"))
        return 1e-2;
    if (unit == QLatin1String("mm"))
        return 1e-3;
    qWarning() << "[FiffDigPointSet] unknown unit" << unit << "(use m, cm or mm)";
    return 0.0;
}

// Whitespace-separated numbers; false if any token is not one.
bool parseNumbers(QStringView text, QList<double>& values)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    for (const QStringView token : text.split(whitespace, Qt::SkipEmptyParts)) {
        bool ok = false;
        values.append(token.toDouble(&ok));
        if (!ok)
            return false;
    }
    return true;
}

// The fiducials (nasion, LPA, RPA) followed by the points, as mne's _read_isotrak_elp_points.
QList<Vector3d> readIsotrakElp(const QString& text)
{
    static const QRegularExpression coordinates(
        QStringLiteral("(-?\\d+\\.?\\d*e?-?\\d*)\\s+(-?\\d+\\.?\\d*e?-?\\d*)\\s+(-?\\d+\\.?\\d*e?-?\\d*)\\s*$"),
        QRegularExpression::MultilineOption);
    QList<Vector3d> points;
    for (QRegularExpressionMatchIterator it = coordinates.globalMatch(text); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        points.append(Vector3d(match.captured(1).toDouble(), match.captured(2).toDouble(), match.captured(3).toDouble()));
    }
    return points;
}

// The same for an .hsp/.eeg file, as mne's _read_isotrak_hsp_points; empty on a malformed file.
QList<Vector3d> readIsotrakHsp(const QString& text)
{
    const QStringList lines = text.split(QLatin1Char('\n'));
    qsizetype line = 0;
    while (line < lines.size() && !lines[line].toLower().contains(QLatin1String("position of fiducials")))
        ++line;
    QList<Vector3d> points;
    for (int i = 1; i <= 3; ++i) {
        QList<double> values;
        if (line + i >= lines.size() || !parseNumbers(QString(lines[line + i]).remove(QLatin1String("%F")), values) || values.size() != 3)
            return {};
        points.append(Vector3d(values[0], values[1], values[2]));
    }
    // A comment line, then "<number of points> <number of columns>" and the points
    const qsizetype header = line + 5;
    if (header >= lines.size())
        return points;
    QList<double> shape;
    if (!parseNumbers(lines[header], shape) || shape.size() != 2 || shape[1] != 3.0)
        return {};
    QList<double> values;
    if (!parseNumbers(lines.mid(header + 1).join(QLatin1Char('\n')), values) || values.size() != 3 * shape[0])
        return {};
    for (qsizetype i = 0; i + 2 < values.size(); i += 3)
        points.append(Vector3d(values[i], values[i + 1], values[i + 2]));
    return points;
}

} // namespace

//=============================================================================================================

bool FiffDigPointSet::readPolhemusIsotrak(const QString& path, FiffDigPointSet& dig, const QStringList& chNames, const QString& unit)
{
    const QString extension = QLatin1Char('.') + QFileInfo(path).suffix();
    const double scale = unitScale(unit);
    QFile file(path);
    if (scale == 0.0 || !QStringList{".hsp", ".elp", ".eeg"}.contains(extension) || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FiffDigPointSet::readPolhemusIsotrak] cannot read" << path;
        return false;
    }
    const QString text = QString::fromUtf8(file.readAll());
    const QList<Vector3d> points = extension == QLatin1String(".elp") ? readIsotrakElp(text) : readIsotrakHsp(text);
    const qsizetype count = points.size() - 3;
    if (count < 0 || (!chNames.isEmpty() && chNames.size() != count)) {
        qWarning() << "[FiffDigPointSet::readPolhemusIsotrak]" << path << "holds" << qMax(count, qsizetype(0)) << "points besides the fiducials, but"
                   << chNames.size() << "names were given";
        return false;
    }

    // Electrodes keep the number at the end of their names when every name has one
    QList<int> idents;
    for (const QString& name : chNames) {
        bool ok = false;
        idents.append(name.right(3).trimmed().toInt(&ok));
        if (!ok) {
            idents.clear();
            break;
        }
    }

    QList<FiffDigPoint> result;
    const auto append = [&](int kind, int ident, const Vector3d& r) {
        FiffDigPoint point;
        point.kind = kind;
        point.ident = ident;
        point.coord_frame = FIFFV_COORD_UNKNOWN;
        for (int c = 0; c < 3; ++c)
            point.r[c] = static_cast<float>(scale * r[c]);
        result.append(point);
    };
    append(FIFFV_POINT_CARDINAL, FIFFV_POINT_LPA, points[1]);
    append(FIFFV_POINT_CARDINAL, FIFFV_POINT_NASION, points[0]);
    append(FIFFV_POINT_CARDINAL, FIFFV_POINT_RPA, points[2]);
    const int kind = !chNames.isEmpty() ? FIFFV_POINT_EEG : (extension == QLatin1String(".elp") ? FIFFV_POINT_HPI : FIFFV_POINT_EXTRA);
    for (qsizetype i = 0; i < count; ++i)
        append(kind, idents.isEmpty() ? static_cast<int>(i) + 1 : idents[i], points[i + 3]);
    dig = FiffDigPointSet(result);
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::readPolhemusFastscan(const QString& path, MatrixX3d& points, const QString& unit, bool requireHeader)
{
    const double scale = unitScale(unit);
    QFile file(path);
    if (scale == 0.0 || QFileInfo(path).suffix() != QLatin1String("txt") || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FiffDigPointSet::readPolhemusFastscan] cannot read" << path;
        return false;
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));

    QString header;
    for (qsizetype i = 0; i < lines.size() && lines[i].startsWith(QLatin1Char('%')); ++i)
        header += lines[i];
    if (requireHeader && !header.contains(QLatin1String("FastSCAN"))) {
        qWarning() << "[FiffDigPointSet::readPolhemusFastscan]" << path << "does not contain a valid Polhemus FastSCAN header";
        return false;
    }

    QList<double> values;
    for (const QString& line : lines) {
        QList<double> row;
        if (!parseNumbers(line.left(line.indexOf(QLatin1Char('%'))), row) || (!row.isEmpty() && row.size() != 3)) {
            qWarning() << "[FiffDigPointSet::readPolhemusFastscan]" << path << "has a row that is not three numbers:" << line;
            return false;
        }
        values.append(row);
    }
    points.resize(values.size() / 3, 3);
    for (qsizetype i = 0; i < values.size(); ++i)
        points(i / 3, i % 3) = scale * values[i];
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::write(QIODevice& p_IODevice)
{
    FiffStream::SPtr t_pStream = FiffStream::start_file(p_IODevice);
    if (!t_pStream) {
        return false;
    }
    qInfo("Write Digitizer Points in %s...\n", t_pStream->streamName().toUtf8().constData());
    this->writeToStream(t_pStream.data());
    t_pStream->end_file();
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::write(const QString& filePath, QString* errorMessage)
{
    if (filePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Output path is empty.");
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

    for (qint32 h = 0; h < m_qListDigPoint.size(); ++h) {
        p_pStream->write_dig_point(m_qListDigPoint[h]);
    }

    qInfo("\t%lld digitizer points written\n", static_cast<long long>(m_qListDigPoint.size()));
    p_pStream->end_block(FIFFB_ISOTRAK);
    p_pStream->end_block(FIFFB_MEAS_INFO);
    p_pStream->end_block(FIFFB_MEAS);
}

//=============================================================================================================

const FiffDigPoint& FiffDigPointSet::operator[](qint32 idx) const
{
    if (idx >= m_qListDigPoint.length()) {
        qWarning("Warning: Required DigPoint doesn't exist! Returning DigPoint '0'.");
        idx = 0;
    }
    return m_qListDigPoint[idx];
}

//=============================================================================================================

FiffDigPoint& FiffDigPointSet::operator[](qint32 idx)
{
    if (idx >= m_qListDigPoint.length()) {
        qWarning("Warning: Required DigPoint doesn't exist! Returning DigPoint '0'.");
        idx = 0;
    }
    return m_qListDigPoint[idx];
}

//=============================================================================================================

FiffDigPointSet FiffDigPointSet::pickTypes(QList<int> includeTypes) const
{
    FiffDigPointSet pickedSet;

    for (int i = 0; i < m_qListDigPoint.size(); ++i) {
        if (includeTypes.contains(m_qListDigPoint[i].kind)) {
            pickedSet << m_qListDigPoint[i];
        }
    }

    return pickedSet;
}

//=============================================================================================================

FiffDigPointSet& FiffDigPointSet::operator<<(const FiffDigPoint& dig)
{
    this->m_qListDigPoint.append(dig);
    return *this;
}

//=============================================================================================================

FiffDigPointSet& FiffDigPointSet::operator<<(const FiffDigPoint* dig)
{
    this->m_qListDigPoint.append(*dig);
    return *this;
}

//=============================================================================================================

void FiffDigPointSet::applyTransform(const FiffCoordTrans& coordTrans, bool bApplyInverse)
{
    Vector4f tempvec;
    for (int i = 0; i < m_qListDigPoint.size(); ++i) {
        tempvec(0) = m_qListDigPoint.at(i).r[0];
        tempvec(1) = m_qListDigPoint.at(i).r[1];
        tempvec(2) = m_qListDigPoint.at(i).r[2];
        tempvec(3) = 1.0f;
        if (bApplyInverse) {
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
