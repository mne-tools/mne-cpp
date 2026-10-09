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
#include <QHash>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <optional>

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

// The points of an mne DigMontage in metres, before they become dig points.
struct Montage
{
    std::optional<Vector3d> nasion, lpa, rpa;
    QList<Vector3d> hpi, extra;
    QStringList chNames;
    QList<Vector3d> chPos;

    // A repeated name keeps its first place and its last position, as mne's _check_dupes_odict.
    void addElectrode(const QString& name, const Vector3d& r)
    {
        const qsizetype index = chNames.indexOf(name);
        if (index >= 0) {
            qWarning() << "[FiffDigPointSet] duplicate electrode" << name << "- the last position is used";
            chPos[index] = r;
            return;
        }
        chNames.append(name);
        chPos.append(r);
    }
};

// The dig points of mne's make_dig_montage: cardinals, HPI coils, head shape, then electrodes,
// which keep the number at the end of their names when every name has one (1..n otherwise).
FiffDigPointSet makeDigMontage(const Montage& montage, QStringList* chNames)
{
    QList<FiffDigPoint> points;
    const auto append = [&points](int kind, int ident, const Vector3d& r) {
        FiffDigPoint point;
        point.kind = kind;
        point.ident = ident;
        point.coord_frame = FIFFV_COORD_UNKNOWN;
        for (int c = 0; c < 3; ++c)
            point.r[c] = static_cast<float>(r[c]);
        points.append(point);
    };
    if (montage.lpa)
        append(FIFFV_POINT_CARDINAL, FIFFV_POINT_LPA, *montage.lpa);
    if (montage.nasion)
        append(FIFFV_POINT_CARDINAL, FIFFV_POINT_NASION, *montage.nasion);
    if (montage.rpa)
        append(FIFFV_POINT_CARDINAL, FIFFV_POINT_RPA, *montage.rpa);
    for (qsizetype i = 0; i < montage.hpi.size(); ++i)
        append(FIFFV_POINT_HPI, static_cast<int>(i) + 1, montage.hpi[i]);
    for (qsizetype i = 0; i < montage.extra.size(); ++i)
        append(FIFFV_POINT_EXTRA, static_cast<int>(i) + 1, montage.extra[i]);

    QList<int> idents;
    for (const QString& name : montage.chNames) {
        bool ok = false;
        idents.append(name.right(3).trimmed().toInt(&ok));
        if (!ok) {
            idents.clear();
            break;
        }
    }
    for (qsizetype i = 0; i < montage.chPos.size(); ++i)
        append(FIFFV_POINT_EEG, idents.isEmpty() ? static_cast<int>(i) + 1 : idents[i], montage.chPos[i]);
    if (chNames)
        *chNames = montage.chNames;
    return FiffDigPointSet(points);
}

// The text of a file, or nothing with a warning.
std::optional<QString> readText(const QString& path, const char* reader)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[FiffDigPointSet::" << reader << "] cannot read" << path;
        return std::nullopt;
    }
    return QString::fromUtf8(file.readAll());
}

// The text of each child element of every <element> in an XML file (namespaces ignored).
QList<QHash<QString, QString>> readXmlRecords(const QString& path, const QString& element)
{
    QFile file(path);
    QList<QHash<QString, QString>> records;
    if (!file.open(QIODevice::ReadOnly))
        return records;
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement || xml.name() != element)
            continue;
        QHash<QString, QString> record;
        while (xml.readNextStartElement()) {
            const QString key = xml.name().toString();
            record.insert(key, xml.readElementText(QXmlStreamReader::SkipChildElements));
        }
        records.append(record);
    }
    if (xml.hasError()) {
        qWarning() << "[FiffDigPointSet]" << path << xml.errorString();
        records.clear();
    }
    return records;
}

// A position from the x/y/z fields of an XML record.
bool recordPosition(const QHash<QString, QString>& record, const QStringList& keys, double scale, Vector3d& r)
{
    for (int c = 0; c < 3; ++c) {
        bool ok = false;
        r[c] = scale * record.value(keys[c]).toDouble(&ok);
        if (!ok)
            return false;
    }
    return true;
}

} // namespace

//=============================================================================================================

bool FiffDigPointSet::readPolhemusIsotrak(const QString& path, FiffDigPointSet& dig, const QStringList& chNames, const QString& unit)
{
    const QString extension = QLatin1Char('.') + QFileInfo(path).suffix();
    const double scale = unitScale(unit);
    if (scale == 0.0 || !QStringList{".hsp", ".elp", ".eeg"}.contains(extension)) {
        qWarning() << "[FiffDigPointSet::readPolhemusIsotrak] not an Isotrak file:" << path;
        return false;
    }
    const std::optional<QString> text = readText(path, "readPolhemusIsotrak");
    if (!text)
        return false;
    QList<Vector3d> points = extension == QLatin1String(".elp") ? readIsotrakElp(*text) : readIsotrakHsp(*text);
    const qsizetype count = points.size() - 3;
    if (count < 0 || (!chNames.isEmpty() && chNames.size() != count)) {
        qWarning() << "[FiffDigPointSet::readPolhemusIsotrak]" << path << "holds" << qMax(count, qsizetype(0)) << "points besides the fiducials, but"
                   << chNames.size() << "names were given";
        return false;
    }
    for (Vector3d& point : points)
        point *= scale;

    Montage montage;
    montage.nasion = points[0];
    montage.lpa = points[1];
    montage.rpa = points[2];
    const QList<Vector3d> rest = points.mid(3);
    if (!chNames.isEmpty()) {
        for (qsizetype i = 0; i < count; ++i)
            montage.addElectrode(chNames[i], rest[i]);
    } else if (extension == QLatin1String(".elp")) {
        montage.hpi = rest;
    } else {
        montage.extra = rest;
    }
    dig = makeDigMontage(montage, nullptr);
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::readPolhemusFastscan(const QString& path, MatrixX3d& points, const QString& unit, bool requireHeader)
{
    const double scale = unitScale(unit);
    if (scale == 0.0 || QFileInfo(path).suffix() != QLatin1String("txt")) {
        qWarning() << "[FiffDigPointSet::readPolhemusFastscan] not a FastSCAN file:" << path;
        return false;
    }
    const std::optional<QString> text = readText(path, "readPolhemusFastscan");
    if (!text)
        return false;
    const QStringList lines = text->split(QLatin1Char('\n'));

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

bool FiffDigPointSet::readCaptrak(const QString& path, FiffDigPointSet& dig, QStringList* chNames)
{
    Montage montage;
    for (const QHash<QString, QString>& electrode : readXmlRecords(path, QStringLiteral("CapTrakElectrode"))) {
        Vector3d r;
        if (!recordPosition(electrode, {"X", "Y", "Z"}, 1e-3, r)) {
            qWarning() << "[FiffDigPointSet::readCaptrak] electrode without a position in" << path;
            return false;
        }
        const QString name = electrode.value(QStringLiteral("Name"));
        if (name == QLatin1String("Nasion"))
            montage.nasion = r;
        else if (name == QLatin1String("LPA"))
            montage.lpa = r;
        else if (name == QLatin1String("RPA"))
            montage.rpa = r;
        else
            montage.addElectrode(name, r);
    }
    if (!montage.nasion || !montage.lpa || !montage.rpa) {
        qWarning() << "[FiffDigPointSet::readCaptrak]" << path << "lacks the Nasion, LPA or RPA electrode";
        return false;
    }
    dig = makeDigMontage(montage, chNames);
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::readEgi(const QString& path, FiffDigPointSet& dig, QStringList* chNames)
{
    Montage montage;
    for (const QHash<QString, QString>& sensor : readXmlRecords(path, QStringLiteral("sensor"))) {
        Vector3d r;
        bool ok = false;
        const int number = sensor.value(QStringLiteral("number")).toInt(&ok);
        if (!ok || !recordPosition(sensor, {"x", "y", "z"}, 1e-2, r)) {
            qWarning() << "[FiffDigPointSet::readEgi] sensor without a number or position in" << path;
            return false;
        }
        const QString name = sensor.value(QStringLiteral("name"));
        switch (sensor.value(QStringLiteral("type")).toInt()) {
            case 0: // EEG
                montage.addElectrode(QStringLiteral("EEG %1").arg(number, 3, 10, QLatin1Char('0')), r);
                break;
            case 1: // reference, numbered after the electrodes before it
                montage.addElectrode(QStringLiteral("EEG %1").arg(montage.chNames.size() + 1, 3, 10, QLatin1Char('0')), r);
                break;
            case 2:
                if (name == QLatin1String("Nasion"))
                    montage.nasion = r;
                else if (name == QLatin1String("Left periauricular point"))
                    montage.lpa = r;
                else if (name == QLatin1String("Right periauricular point"))
                    montage.rpa = r;
                break;
            default:
                qWarning() << "[FiffDigPointSet::readEgi] skipping sensor" << number << "of unknown type";
        }
    }
    if (!montage.nasion || !montage.lpa || !montage.rpa) {
        qWarning() << "[FiffDigPointSet::readEgi]" << path << "lacks the nasion or a periauricular point";
        return false;
    }
    dig = makeDigMontage(montage, chNames);
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::readLocalite(const QString& path,
                                   FiffDigPointSet& dig,
                                   QStringList* chNames,
                                   const QString& nasion,
                                   const QString& lpa,
                                   const QString& rpa)
{
    const std::optional<QString> text = readText(path, "readLocalite");
    if (!text)
        return false;
    const QStringList lines = text->split(QLatin1Char('\n'));
    Montage montage;
    for (qsizetype i = 1; i < lines.size(); ++i) {
        if (lines[i].trimmed().isEmpty())
            continue;
        const QStringList fields = lines[i].split(QLatin1Char(','));
        Vector3d r;
        bool ok = fields.size() == 5;
        for (int c = 0; ok && c < 3; ++c)
            r[c] = fields[c + 2].toDouble(&ok) / 1000.0;
        if (!ok) {
            qWarning() << "[FiffDigPointSet::readLocalite]" << path << "line" << i + 1 << "is not '#,name,x,y,z':" << lines[i];
            return false;
        }
        montage.addElectrode(fields[1], r);
    }
    // The named fiducials leave the electrodes
    const auto takeFiducial = [&montage](const QString& name, std::optional<Vector3d>& fiducial) {
        if (name.isEmpty())
            return true;
        const qsizetype index = montage.chNames.indexOf(name);
        if (index < 0)
            return false;
        fiducial = montage.chPos.takeAt(index);
        montage.chNames.removeAt(index);
        return true;
    };
    if (!takeFiducial(nasion, montage.nasion) || !takeFiducial(lpa, montage.lpa) || !takeFiducial(rpa, montage.rpa)) {
        qWarning() << "[FiffDigPointSet::readLocalite]" << path << "has no point named" << nasion << lpa << rpa;
        return false;
    }
    dig = makeDigMontage(montage, chNames);
    return true;
}

//=============================================================================================================

bool FiffDigPointSet::readNeuroscanDat(const QString& path, FiffDigPointSet& dig, QStringList* chNames)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    const std::optional<QString> text = readText(path, "readNeuroscanDat");
    if (!text)
        return false;
    const QStringList lines = text->split(QLatin1Char('\n'));
    Montage montage;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        const QStringList items = lines[i].split(whitespace, Qt::SkipEmptyParts);
        if (items.isEmpty())
            continue;
        Vector3d r;
        bool ok = items.size() == 5;
        for (int c = 0; ok && c < 3; ++c)
            r[c] = items[c + 2].toDouble(&ok);
        if (!ok) {
            qWarning() << "[FiffDigPointSet::readNeuroscanDat]" << path << "line" << i << "is not 'name number x y z':" << lines[i];
            return false;
        }
        // The point number marks the fiducials (78 nasion, 76 LPA, 82 RPA) and the centroid (67)
        const QString& number = items[1];
        if (number == QLatin1String("78"))
            montage.nasion = r;
        else if (number == QLatin1String("76"))
            montage.lpa = r;
        else if (number == QLatin1String("82"))
            montage.rpa = r;
        else if (number != QLatin1String("67"))
            montage.addElectrode(items[0], r);
    }
    dig = makeDigMontage(montage, chNames);
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
