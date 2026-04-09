//=============================================================================================================
/**
 * @file     bids_raw_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    BidsRawData class definition — read(), write(), clear(), and createReader().
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_raw_data.h"
#include "bids_channel.h"
#include "bids_dataset_description.h"
#include "bids_const.h"
#include "readers/bids_edf_reader.h"
#include "readers/bids_brain_vision_reader.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// ANONYMOUS HELPERS — read side
//=============================================================================================================

namespace
{

//=========================================================================================================
int bidsUnitToFiffUnit(const QString& sUnit)
{
    QString u = sUnit.toLower().trimmed();
    if(u == "v" || u == "\u00B5v" || u == "uv" || u == "mv" || u == "nv")
        return FIFF_UNIT_V;
    if(u == "t" || u == "ft" || u == "pt")
        return FIFF_UNIT_T;
    return FIFF_UNIT_NONE;
}

//=========================================================================================================
int bidsUnitToFiffUnitMul(const QString& sUnit)
{
    QString u = sUnit.toLower().trimmed();
    if(u == "\u00B5v" || u == "uv" || u == "\u00B5s" || u == "us")
        return FIFF_UNITM_MU;
    if(u == "mv")
        return FIFF_UNITM_M;
    if(u == "nv")
        return FIFF_UNITM_N;
    if(u == "ft")
        return FIFF_UNITM_F;
    if(u == "pt")
        return FIFF_UNITM_P;
    return FIFF_UNITM_NONE;
}

//=========================================================================================================
void applyChannelsTsv(FiffInfo& info, const QList<BidsChannel>& channels)
{
    if(channels.isEmpty())
        return;

    QMap<QString, int> bidsToFiff = bidsTypeToFiffKind();

    QMap<QString, const BidsChannel*> channelMap;
    for(const auto& ch : channels)
        channelMap[ch.name] = &ch;

    info.bads.clear();

    for(int i = 0; i < info.chs.size(); ++i) {
        FiffChInfo& fiffCh = info.chs[i];
        auto it = channelMap.find(fiffCh.ch_name);
        if(it == channelMap.end())
            continue;
        const BidsChannel* rec = it.value();

        QString typeUpper = rec->type.toUpper();
        if(bidsToFiff.contains(typeUpper))
            fiffCh.kind = bidsToFiff[typeUpper];

        if(!rec->units.isEmpty() && rec->units != "n/a") {
            fiffCh.unit = bidsUnitToFiffUnit(rec->units);
            fiffCh.unit_mul = bidsUnitToFiffUnitMul(rec->units);
        }

        if(rec->status.toLower() == "bad")
            info.bads.append(fiffCh.ch_name);
    }
}

//=========================================================================================================
void applyElectrodePositions(FiffInfo& info,
                             const QList<BidsElectrode>& electrodes,
                             const QString& coordSystemName,
                             const QString& coordUnits)
{
    if(electrodes.isEmpty())
        return;

    QMap<QString, int> coordMap = bidsCoordToFiffFrame();
    int coordFrame = FIFFV_COORD_UNKNOWN;
    if(coordMap.contains(coordSystemName))
        coordFrame = coordMap[coordSystemName];

    float scaleFactor = 1.0f;
    QString units = coordUnits.toLower();
    if(units == "mm")
        scaleFactor = 0.001f;
    else if(units == "cm")
        scaleFactor = 0.01f;

    QSet<QString> chNames;
    for(const auto& ch : info.chs)
        chNames.insert(ch.ch_name);

    info.dig.clear();
    int ident = 1;
    for(const auto& elec : electrodes) {
        if(elec.x == "n/a" || elec.y == "n/a" || elec.z == "n/a")
            continue;

        FiffDigPoint dp;
        dp.kind = chNames.contains(elec.name) ? FIFFV_POINT_EEG : FIFFV_POINT_EXTRA;
        dp.ident = ident++;
        dp.r[0] = elec.x.toFloat() * scaleFactor;
        dp.r[1] = elec.y.toFloat() * scaleFactor;
        dp.r[2] = elec.z.toFloat() * scaleFactor;
        dp.coord_frame = coordFrame;

        info.dig.append(dp);
    }
}

//=========================================================================================================
QJsonObject readJsonFile(const QString& sFilePath)
{
    QFile file(sFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if(error.error != QJsonParseError::NoError)
        return {};
    return doc.object();
}

//=========================================================================================================
bool writeJsonFile(const QString& sFilePath, const QJsonObject& json)
{
    QFile file(sFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

//=========================================================================================================
void readSidecarJson(const QString& sFilePath,
                     FiffInfo& info,
                     BidsRawData& data)
{
    QJsonObject json = readJsonFile(sFilePath);
    if(json.isEmpty())
        return;

    // Apply to FiffInfo
    double plf = json.value(QStringLiteral("PowerLineFrequency")).toDouble();
    if(plf > 0.0)
        info.linefreq = static_cast<float>(plf);

    double sf = json.value(QStringLiteral("SamplingFrequency")).toDouble();
    if(sf > 0.0 && std::abs(info.sfreq - static_cast<float>(sf)) > 0.5f)
        qWarning() << "[BidsRawData::read] Sampling frequency mismatch: raw ="
                    << info.sfreq << "sidecar =" << sf;

    // Store independent metadata fields on BidsRawData
    data.ieegReference         = json.value(QStringLiteral("iEEGReference")).toString();
    data.taskDescription       = json.value(QStringLiteral("TaskDescription")).toString();
    data.manufacturer          = json.value(QStringLiteral("Manufacturer")).toString();
    data.manufacturerModelName = json.value(QStringLiteral("ManufacturerModelName")).toString();
    data.softwareVersions      = json.value(QStringLiteral("SoftwareVersions")).toString();
    data.recordingType         = json.value(QStringLiteral("RecordingType")).toString();
}

//=============================================================================================================
// ANONYMOUS HELPERS — write side
//=============================================================================================================

//=========================================================================================================
QString fiffUnitToBidsString(int unit, int unitMul)
{
    if(unit == FIFF_UNIT_V) {
        switch(unitMul) {
        case FIFF_UNITM_MU: return QStringLiteral("\u00B5V");
        case FIFF_UNITM_M:  return QStringLiteral("mV");
        case FIFF_UNITM_N:  return QStringLiteral("nV");
        default:             return QStringLiteral("V");
        }
    }
    if(unit == FIFF_UNIT_T) {
        switch(unitMul) {
        case FIFF_UNITM_F:  return QStringLiteral("fT");
        case FIFF_UNITM_P:  return QStringLiteral("pT");
        default:             return QStringLiteral("T");
        }
    }
    return QStringLiteral("n/a");
}

//=========================================================================================================
QString channelTypeDescription(int kind)
{
    switch(kind) {
    case FIFFV_EEG_CH:  return QStringLiteral("ElectroEncephaloGram");
    case FIFFV_ECOG_CH: return QStringLiteral("Electrocorticography");
    case FIFFV_SEEG_CH: return QStringLiteral("StereoElectroEncephaloGram");
    case FIFFV_DBS_CH:  return QStringLiteral("DeepBrainStimulation");
    case FIFFV_MEG_CH:  return QStringLiteral("MagnetoEncephaloGram");
    case FIFFV_STIM_CH: return QStringLiteral("Trigger");
    case FIFFV_EOG_CH:  return QStringLiteral("ElectroOculoGram");
    case FIFFV_ECG_CH:  return QStringLiteral("ElectroCardioGram");
    case FIFFV_EMG_CH:  return QStringLiteral("ElectroMyoGram");
    case FIFFV_MISC_CH: return QStringLiteral("Miscellaneous");
    case FIFFV_RESP_CH: return QStringLiteral("Respiration");
    default:            return QStringLiteral("n/a");
    }
}

//=========================================================================================================
QList<BidsChannel> buildChannelRecords(const FiffInfo& info)
{
    QList<BidsChannel> records;
    QMap<int, QString> kindMap = fiffKindToBidsType();
    QSet<QString> badsSet(info.bads.begin(), info.bads.end());

    for(int i = 0; i < info.chs.size(); ++i) {
        const FiffChInfo& ch = info.chs[i];

        BidsChannel rec;
        rec.name = ch.ch_name;
        rec.type = kindMap.contains(ch.kind) ? kindMap[ch.kind] : QStringLiteral("MISC");
        rec.units = fiffUnitToBidsString(ch.unit, ch.unit_mul);
        rec.samplingFreq = QString::number(static_cast<double>(info.sfreq), 'g', 10);
        rec.lowCutoff  = (info.highpass > 0.0f)
                             ? QString::number(static_cast<double>(info.highpass), 'g', 10)
                             : QStringLiteral("n/a");
        rec.highCutoff = (info.lowpass > 0.0f)
                             ? QString::number(static_cast<double>(info.lowpass), 'g', 10)
                             : QStringLiteral("n/a");
        rec.notch = QStringLiteral("n/a");
        rec.status = badsSet.contains(ch.ch_name) ? QStringLiteral("bad") : QStringLiteral("good");
        rec.description = channelTypeDescription(ch.kind);

        records.append(rec);
    }
    return records;
}

//=========================================================================================================
QList<BidsElectrode> buildElectrodeRecords(const FiffInfo& info)
{
    QList<BidsElectrode> records;

    QMap<int, const FiffDigPoint*> digByIdent;
    for(const auto& dp : info.dig) {
        if(dp.kind == FIFFV_POINT_EEG || dp.kind == FIFFV_POINT_EXTRA)
            digByIdent[dp.ident] = &dp;
    }

    int digIdx = 0;

    for(int i = 0; i < info.chs.size(); ++i) {
        const FiffChInfo& ch = info.chs[i];

        if(ch.kind == FIFFV_STIM_CH)
            continue;
        if(ch.kind != FIFFV_EEG_CH && ch.kind != FIFFV_ECOG_CH &&
           ch.kind != FIFFV_SEEG_CH && ch.kind != FIFFV_DBS_CH)
            continue;

        BidsElectrode rec;
        rec.name = ch.ch_name;

        ++digIdx;
        bool hasPosition = false;

        if(digByIdent.contains(digIdx)) {
            const FiffDigPoint* dp = digByIdent[digIdx];
            if(std::isfinite(dp->r[0]) && std::isfinite(dp->r[1]) && std::isfinite(dp->r[2])) {
                rec.x = QString::number(static_cast<double>(dp->r[0]), 'g', 8);
                rec.y = QString::number(static_cast<double>(dp->r[1]), 'g', 8);
                rec.z = QString::number(static_cast<double>(dp->r[2]), 'g', 8);
                hasPosition = true;
            }
        }

        if(!hasPosition) {
            const Eigen::Vector3f& r0 = ch.chpos.r0;
            if(r0.squaredNorm() > 0.0f && std::isfinite(r0[0])) {
                rec.x = QString::number(static_cast<double>(r0[0]), 'g', 8);
                rec.y = QString::number(static_cast<double>(r0[1]), 'g', 8);
                rec.z = QString::number(static_cast<double>(r0[2]), 'g', 8);
            } else {
                rec.x = QStringLiteral("n/a");
                rec.y = QStringLiteral("n/a");
                rec.z = QStringLiteral("n/a");
            }
        }

        rec.size = QStringLiteral("n/a");
        rec.type = QStringLiteral("n/a");
        rec.material = QStringLiteral("n/a");
        rec.impedance = QStringLiteral("n/a");

        records.append(rec);
    }
    return records;
}

//=========================================================================================================
QJsonObject buildIeegSidecarJson(const BidsRawData& data,
                                 const BIDSPath& bidsPath)
{
    QJsonObject json;
    const FiffInfo& info = data.raw.info;

    // Required
    json[QStringLiteral("TaskName")]            = bidsPath.task();
    json[QStringLiteral("SamplingFrequency")]   = static_cast<double>(info.sfreq);
    json[QStringLiteral("PowerLineFrequency")]  = static_cast<double>(info.linefreq);

    // Reference
    if(!data.ieegReference.isEmpty())
        json[QStringLiteral("iEEGReference")] = data.ieegReference;
    else
        json[QStringLiteral("iEEGReference")] = QStringLiteral("n/a");

    // Channel counts — computed from FiffInfo
    int ecog = 0, seeg = 0, dbs = 0, eeg = 0, eog = 0, ecg = 0, emg = 0, misc = 0, trig = 0;
    for(const auto& ch : info.chs) {
        switch(ch.kind) {
        case FIFFV_ECOG_CH: ++ecog; break;
        case FIFFV_SEEG_CH: ++seeg; break;
        case FIFFV_DBS_CH:  ++dbs;  break;
        case FIFFV_EEG_CH:  ++eeg;  break;
        case FIFFV_EOG_CH:  ++eog;  break;
        case FIFFV_ECG_CH:  ++ecg;  break;
        case FIFFV_EMG_CH:  ++emg;  break;
        case FIFFV_MISC_CH: ++misc; break;
        case FIFFV_STIM_CH: ++trig; break;
        default: break;
        }
    }
    json[QStringLiteral("ECOGChannelCount")] = ecog;
    json[QStringLiteral("SEEGChannelCount")] = seeg;
    if(dbs > 0)  json[QStringLiteral("DBSChannelCount")]     = dbs;
    if(eeg > 0)  json[QStringLiteral("EEGChannelCount")]     = eeg;
    if(eog > 0)  json[QStringLiteral("EOGChannelCount")]     = eog;
    if(ecg > 0)  json[QStringLiteral("ECGChannelCount")]     = ecg;
    if(emg > 0)  json[QStringLiteral("EMGChannelCount")]     = emg;
    if(misc > 0) json[QStringLiteral("MiscChannelCount")]    = misc;
    if(trig > 0) json[QStringLiteral("TriggerChannelCount")] = trig;

    // Recording metadata
    if(!data.recordingType.isEmpty())
        json[QStringLiteral("RecordingType")] = data.recordingType;
    else
        json[QStringLiteral("RecordingType")] = QStringLiteral("continuous");

    if(info.sfreq > 0.0f && data.raw.last_samp >= data.raw.first_samp) {
        double dur = static_cast<double>(data.raw.last_samp - data.raw.first_samp + 1)
                     / static_cast<double>(info.sfreq);
        json[QStringLiteral("RecordingDuration")] = dur;
    }

    // Optional strings from BidsRawData
    if(!data.taskDescription.isEmpty())
        json[QStringLiteral("TaskDescription")] = data.taskDescription;
    if(!data.manufacturer.isEmpty())
        json[QStringLiteral("Manufacturer")] = data.manufacturer;
    if(!data.manufacturerModelName.isEmpty())
        json[QStringLiteral("ManufacturerModelName")] = data.manufacturerModelName;
    if(!data.softwareVersions.isEmpty())
        json[QStringLiteral("SoftwareVersions")] = data.softwareVersions;

    return json;
}

//=========================================================================================================
bool copyFile(const QString& src, const QString& dst, bool overwrite)
{
    if(!QFileInfo::exists(src)) {
        qWarning() << "[BidsRawData::write] Source file does not exist:" << src;
        return false;
    }
    if(QFileInfo::exists(dst)) {
        if(!overwrite) {
            qWarning() << "[BidsRawData::write] Target file already exists:" << dst;
            return false;
        }
        QFile::remove(dst);
    }
    return QFile::copy(src, dst);
}

//=========================================================================================================
bool copyBrainVisionFiles(const QString& srcVhdr, const BIDSPath& bidsPath, bool overwrite)
{
    QFileInfo srcInfo(srcVhdr);
    QString srcDir = srcInfo.absolutePath();

    QFile vhdrFile(srcVhdr);
    if(!vhdrFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BidsRawData::write] Cannot open .vhdr file:" << srcVhdr;
        return false;
    }

    QString dataFileName;
    QString markerFileName;
    QByteArray vhdrContent = vhdrFile.readAll();
    vhdrFile.close();

    for(const auto& line : vhdrContent.split('\n')) {
        QString sLine = QString::fromUtf8(line).trimmed();
        if(sLine.startsWith("DataFile=", Qt::CaseInsensitive))
            dataFileName = sLine.mid(9).trimmed();
        else if(sLine.startsWith("MarkerFile=", Qt::CaseInsensitive))
            markerFileName = sLine.mid(11).trimmed();
    }

    QString dstDir = bidsPath.directory();
    QString dstBase = bidsPath.basename();
    dstBase = dstBase.left(dstBase.lastIndexOf('.'));

    QString dstVhdr = bidsPath.filePath();
    QString newDataFile   = dstBase + QStringLiteral(".eeg");
    QString newMarkerFile = dstBase + QStringLiteral(".vmrk");

    QString vhdrStr = QString::fromUtf8(vhdrContent);
    if(!dataFileName.isEmpty())
        vhdrStr.replace("DataFile=" + dataFileName,
                        "DataFile=" + QFileInfo(newDataFile).fileName());
    if(!markerFileName.isEmpty())
        vhdrStr.replace("MarkerFile=" + markerFileName,
                        "MarkerFile=" + QFileInfo(newMarkerFile).fileName());

    if(QFileInfo::exists(dstVhdr) && !overwrite) {
        qWarning() << "[BidsRawData::write] Target file already exists:" << dstVhdr;
        return false;
    }
    if(QFileInfo::exists(dstVhdr))
        QFile::remove(dstVhdr);

    QFile dstVhdrFile(dstVhdr);
    if(!dstVhdrFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[BidsRawData::write] Cannot write .vhdr file:" << dstVhdr;
        return false;
    }
    dstVhdrFile.write(vhdrStr.toUtf8());
    dstVhdrFile.close();

    if(!dataFileName.isEmpty()) {
        QString srcData = QDir(srcDir).absoluteFilePath(dataFileName);
        QString dstData = dstDir + QFileInfo(newDataFile).fileName();
        if(!copyFile(srcData, dstData, overwrite)) {
            qWarning() << "[BidsRawData::write] Failed to copy data file:" << srcData;
            return false;
        }
    }

    if(!markerFileName.isEmpty()) {
        QString srcMarker = QDir(srcDir).absoluteFilePath(markerFileName);
        QString dstMarker = dstDir + QFileInfo(newMarkerFile).fileName();

        if(QFileInfo::exists(srcMarker)) {
            QFile markerFile(srcMarker);
            if(markerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString markerContent = QString::fromUtf8(markerFile.readAll());
                markerFile.close();

                markerContent.replace("DataFile=" + dataFileName,
                                      "DataFile=" + QFileInfo(newDataFile).fileName());

                if(QFileInfo::exists(dstMarker)) {
                    if(!overwrite) {
                        qWarning() << "[BidsRawData::write] Target marker file already exists:" << dstMarker;
                        return false;
                    }
                    QFile::remove(dstMarker);
                }

                QFile dstMarkerFile(dstMarker);
                if(dstMarkerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    dstMarkerFile.write(markerContent.toUtf8());
                    dstMarkerFile.close();
                }
            }
        }
    }

    return true;
}

//=========================================================================================================
bool copyRawDataFile(const QString& sourcePath, const BIDSPath& bidsPath, bool overwrite)
{
    if(sourcePath.isEmpty())
        return true;

    QString ext = bidsPath.extension().toLower();
    if(ext == ".vhdr" || ext == ".ahdr")
        return copyBrainVisionFiles(sourcePath, bidsPath, overwrite);

    return copyFile(sourcePath, bidsPath.filePath(), overwrite);
}

} // anonymous namespace

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

BidsRawData::BidsRawData(BidsRawData&& other) noexcept
    : raw(other.raw)                         // FiffRawData has only copy ctor
    , events(std::move(other.events))
    , eventIdMap(std::move(other.eventIdMap))
    , electrodes(std::move(other.electrodes))
    , coordinateSystem(std::move(other.coordinateSystem))
    , reader(std::move(other.reader))
    , ieegReference(std::move(other.ieegReference))
    , taskDescription(std::move(other.taskDescription))
    , manufacturer(std::move(other.manufacturer))
    , manufacturerModelName(std::move(other.manufacturerModelName))
    , softwareVersions(std::move(other.softwareVersions))
    , recordingType(std::move(other.recordingType))
    , m_bIsValid(other.m_bIsValid)
{
    other.m_bIsValid = false;
}

//=============================================================================================================

BidsRawData& BidsRawData::operator=(BidsRawData&& other) noexcept
{
    if(this != &other) {
        raw                     = other.raw;   // FiffRawData has only copy ctor
        events                  = std::move(other.events);
        eventIdMap              = std::move(other.eventIdMap);
        electrodes              = std::move(other.electrodes);
        coordinateSystem        = std::move(other.coordinateSystem);
        reader                  = std::move(other.reader);
        ieegReference           = std::move(other.ieegReference);
        taskDescription         = std::move(other.taskDescription);
        manufacturer            = std::move(other.manufacturer);
        manufacturerModelName   = std::move(other.manufacturerModelName);
        softwareVersions        = std::move(other.softwareVersions);
        recordingType           = std::move(other.recordingType);
        m_bIsValid              = other.m_bIsValid;
        other.m_bIsValid        = false;
    }
    return *this;
}

//=============================================================================================================

void BidsRawData::clear()
{
    raw = FiffRawData();
    events.clear();
    eventIdMap.clear();
    electrodes.clear();
    reader.reset();

    ieegReference.clear();
    taskDescription.clear();
    manufacturer.clear();
    manufacturerModelName.clear();
    softwareVersions.clear();
    recordingType.clear();

    coordinateSystem = BidsCoordinateSystem();

    m_bIsValid = false;
}

//=============================================================================================================

AbstractFormatReader::UPtr BidsRawData::createReader(const QString& sExtension)
{
    QString ext = sExtension.toLower();
    if(ext == ".vhdr" || ext == ".ahdr")
        return std::make_unique<BrainVisionReader>();
    if(ext == ".edf" || ext == ".bdf")
        return std::make_unique<EDFReader>();
    return nullptr;
}

//=============================================================================================================
// BidsRawData::read()
//=============================================================================================================

BidsRawData BidsRawData::read(const BIDSPath& bidsPath)
{
    BidsRawData result;

    //=========================================================================================================
    // Step 1 — Validate BIDSPath
    //=========================================================================================================
    if(bidsPath.root().isEmpty()) {
        qWarning() << "[BidsRawData::read] BIDSPath root is not set";
        return result;
    }
    if(bidsPath.subject().isEmpty()) {
        qWarning() << "[BidsRawData::read] BIDSPath subject is not set";
        return result;
    }
    if(bidsPath.extension().isEmpty()) {
        qWarning() << "[BidsRawData::read] BIDSPath extension is not set";
        return result;
    }

    //=========================================================================================================
    // Step 2 — Resolve raw file path
    //=========================================================================================================
    QString rawFilePath = bidsPath.filePath();

    if(!QFileInfo::exists(rawFilePath)) {
        QString ext = bidsPath.extension();
        if(ext.toLower() == ".edf") {
            BIDSPath altPath(bidsPath);
            altPath.setExtension(".EDF");
            if(QFileInfo::exists(altPath.filePath()))
                rawFilePath = altPath.filePath();
        }
    }

    if(!QFileInfo::exists(rawFilePath)) {
        qWarning() << "[BidsRawData::read] Raw data file not found:" << rawFilePath;
        return result;
    }

    //=========================================================================================================
    // Step 3 — Create and open the format reader
    //=========================================================================================================
    result.reader = createReader(bidsPath.extension());
    if(!result.reader) {
        qWarning() << "[BidsRawData::read] Unsupported file extension:" << bidsPath.extension();
        return result;
    }

    if(!result.reader->open(rawFilePath)) {
        qWarning() << "[BidsRawData::read] Failed to open raw file:" << rawFilePath;
        return result;
    }

    //=========================================================================================================
    // Step 4 — Build FiffRawData from the reader
    //=========================================================================================================
    result.raw = result.reader->toFiffRawData();

    //=========================================================================================================
    // Step 5 — Read and apply *_channels.tsv
    //=========================================================================================================
    BIDSPath channelsPath = bidsPath.channelsTsvPath();
    if(QFileInfo::exists(channelsPath.filePath())) {
        QList<BidsChannel> channels = BidsChannel::readTsv(channelsPath.filePath());
        applyChannelsTsv(result.raw.info, channels);
    }

    //=========================================================================================================
    // Step 6 — Read *_coordsystem.json (before electrodes, for scale/frame info)
    //=========================================================================================================
    BIDSPath coordsysPath = bidsPath.coordsystemJsonPath();
    if(QFileInfo::exists(coordsysPath.filePath()))
        result.coordinateSystem = BidsCoordinateSystem::readJson(coordsysPath.filePath());

    //=========================================================================================================
    // Step 7 — Read and apply *_electrodes.tsv
    //=========================================================================================================
    BIDSPath electrodesPath = bidsPath.electrodesTsvPath();
    if(QFileInfo::exists(electrodesPath.filePath())) {
        result.electrodes = BidsElectrode::readTsv(electrodesPath.filePath());
        applyElectrodePositions(result.raw.info, result.electrodes,
                                result.coordinateSystem.system, result.coordinateSystem.units);
    }

    //=========================================================================================================
    // Step 8 — Read *_events.tsv
    //=========================================================================================================
    BIDSPath eventsPath = bidsPath.eventsTsvPath();
    if(QFileInfo::exists(eventsPath.filePath())) {
        result.events = BidsEvent::readTsv(eventsPath.filePath());

        // Compute sample from onset*sfreq if sample column was absent
        float sfreq = result.raw.info.sfreq;
        for(auto& ev : result.events) {
            if(ev.sample == 0 && ev.onset > 0.0f && sfreq > 0)
                ev.sample = static_cast<int>(ev.onset * sfreq);
        }

        // Build eventIdMap from trial_type → value
        for(const auto& ev : result.events) {
            if(!ev.trialType.isEmpty() && ev.trialType != "n/a")
                result.eventIdMap.insert(ev.trialType, ev.value);
        }
    }

    //=========================================================================================================
    // Step 9 — Read sidecar *_{datatype}.json
    //=========================================================================================================
    BIDSPath sidecarPath = bidsPath.sidecarJsonPath();
    if(QFileInfo::exists(sidecarPath.filePath()))
        readSidecarJson(sidecarPath.filePath(), result.raw.info, result);

    //=========================================================================================================
    // Done
    //=========================================================================================================
    result.m_bIsValid = true;
    return result;
}

//=============================================================================================================
// BidsRawData::write()
//=============================================================================================================

BIDSPath BidsRawData::write(const BIDSPath& bidsPath,
                            const QString& sourcePath,
                            const WriteOptions& options) const
{
    BIDSPath result;

    //=========================================================================================================
    // Step 1 — Validate
    //=========================================================================================================
    if(bidsPath.root().isEmpty()) {
        qWarning() << "[BidsRawData::write] BIDSPath root is not set";
        return result;
    }
    if(bidsPath.subject().isEmpty()) {
        qWarning() << "[BidsRawData::write] BIDSPath subject is not set";
        return result;
    }
    if(bidsPath.task().isEmpty()) {
        qWarning() << "[BidsRawData::write] BIDSPath task is not set";
        return result;
    }
    if(bidsPath.datatype().isEmpty()) {
        qWarning() << "[BidsRawData::write] BIDSPath datatype is not set";
        return result;
    }
    if(raw.info.isEmpty()) {
        qWarning() << "[BidsRawData::write] FiffRawData info is empty";
        return result;
    }

    //=========================================================================================================
    // Step 2 — Create directory structure
    //=========================================================================================================
    if(!bidsPath.mkdirs()) {
        qWarning() << "[BidsRawData::write] Failed to create directory:" << bidsPath.directory();
        return result;
    }

    //=========================================================================================================
    // Step 3 — Copy raw data file
    //=========================================================================================================
    if(options.copyData && !sourcePath.isEmpty()) {
        if(!copyRawDataFile(sourcePath, bidsPath, options.overwrite)) {
            qWarning() << "[BidsRawData::write] Failed to copy raw data file";
            return result;
        }
    }

    //=========================================================================================================
    // Step 4 — Write *_channels.tsv
    //=========================================================================================================
    {
        BIDSPath channelsPath = bidsPath.channelsTsvPath();
        if(!options.overwrite && QFileInfo::exists(channelsPath.filePath())) {
            qWarning() << "[BidsRawData::write] channels.tsv already exists:" << channelsPath.filePath();
            return result;
        }

        QList<BidsChannel> channelRecords = buildChannelRecords(raw.info);
        if(!BidsChannel::writeTsv(channelsPath.filePath(), channelRecords)) {
            qWarning() << "[BidsRawData::write] Failed to write channels.tsv";
            return result;
        }
    }

    //=========================================================================================================
    // Step 5 — Write *_electrodes.tsv + *_coordsystem.json
    //=========================================================================================================
    {
        QList<BidsElectrode> electrodeRecords = buildElectrodeRecords(raw.info);

        if(!electrodeRecords.isEmpty()) {
            BIDSPath electrodesPath = bidsPath.electrodesTsvPath();
            if(options.overwrite || !QFileInfo::exists(electrodesPath.filePath())) {
                if(!BidsElectrode::writeTsv(electrodesPath.filePath(), electrodeRecords))
                    qWarning() << "[BidsRawData::write] Failed to write electrodes.tsv";
            }

            BIDSPath coordsysPath = bidsPath.coordsystemJsonPath();
            if(options.overwrite || !QFileInfo::exists(coordsysPath.filePath())) {
                // Build coordinate system, deriving defaults from FiffInfo if needed
                BidsCoordinateSystem cs = coordinateSystem;
                if(cs.system.isEmpty()) {
                    if(raw.info.dig.isEmpty()) {
                        cs.system = QStringLiteral("Other");
                        cs.units = QStringLiteral("n/a");
                    } else {
                        int coordFrame = raw.info.dig.first().coord_frame;
                        QMap<int, QString> frameMap = fiffFrameToBidsCoord();
                        cs.system = frameMap.contains(coordFrame)
                                       ? frameMap[coordFrame]
                                       : QStringLiteral("Other");
                        cs.units = QStringLiteral("m");
                    }
                }
                if(cs.description.isEmpty() && !raw.info.dig.isEmpty())
                    cs.description = QStringLiteral("Coordinate system derived from recording data");

                if(!BidsCoordinateSystem::writeJson(coordsysPath.filePath(), cs))
                    qWarning() << "[BidsRawData::write] Failed to write coordsystem.json";
            }
        }
    }

    //=========================================================================================================
    // Step 6 — Write *_events.tsv
    //=========================================================================================================
    if(!events.isEmpty()) {
        BIDSPath eventsPath = bidsPath.eventsTsvPath();
        if(!options.overwrite && QFileInfo::exists(eventsPath.filePath())) {
            qWarning() << "[BidsRawData::write] events.tsv already exists:" << eventsPath.filePath();
            return result;
        }

        QList<BidsEvent> eventsToWrite = events;

        // Apply trial_type fallback from eventIdMap
        QMap<int, QString> valueToType;
        for(auto it = eventIdMap.constBegin(); it != eventIdMap.constEnd(); ++it)
            valueToType[it.value()] = it.key();
        for(auto& ev : eventsToWrite) {
            if(ev.trialType.isEmpty() || ev.trialType == "n/a")
                ev.trialType = valueToType.value(ev.value, QStringLiteral("n/a"));
        }

        if(!BidsEvent::writeTsv(eventsPath.filePath(), eventsToWrite))
            qWarning() << "[BidsRawData::write] Failed to write events.tsv";
    }

    //=========================================================================================================
    // Step 7 — Write *_{datatype}.json sidecar
    //=========================================================================================================
    {
        BIDSPath sidecarPath = bidsPath.sidecarJsonPath();
        if(!options.overwrite && QFileInfo::exists(sidecarPath.filePath())) {
            qWarning() << "[BidsRawData::write] Sidecar JSON already exists:" << sidecarPath.filePath();
            return result;
        }

        QJsonObject sidecarJson = buildIeegSidecarJson(*this, bidsPath);
        if(!writeJsonFile(sidecarPath.filePath(), sidecarJson)) {
            qWarning() << "[BidsRawData::write] Failed to write sidecar JSON";
            return result;
        }
    }

    //=========================================================================================================
    // Step 8 — Write dataset_description.json (never overwrite)
    //=========================================================================================================
    {
        QString descPath = bidsPath.root() + QDir::separator()
                           + QStringLiteral("dataset_description.json");

        if(!QFileInfo::exists(descPath)) {
            BidsDatasetDescription desc;
            desc.name = options.datasetName.isEmpty()
                            ? QStringLiteral("[Unspecified]")
                            : options.datasetName;
            desc.bidsVersion = QStringLiteral("1.9.0");
            desc.datasetType = QStringLiteral("raw");

            if(!BidsDatasetDescription::write(descPath, desc))
                qWarning() << "[BidsRawData::write] Failed to write dataset_description.json";
        }
    }

    //=========================================================================================================
    // Done
    //=========================================================================================================
    result = bidsPath;
    return result;
}
