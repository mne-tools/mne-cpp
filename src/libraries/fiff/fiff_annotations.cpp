//=============================================================================================================
/**
 * @file     fiff_annotations.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Definition of the FiffAnnotations class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_annotations.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnnotations::FiffAnnotations()
{
}

//=============================================================================================================

int FiffAnnotations::size() const
{
    return m_annotations.size();
}

//=============================================================================================================

bool FiffAnnotations::isEmpty() const
{
    return m_annotations.isEmpty();
}

//=============================================================================================================

const FiffAnnotation& FiffAnnotations::operator[](int index) const
{
    return m_annotations[index];
}

//=============================================================================================================

FiffAnnotation& FiffAnnotations::operator[](int index)
{
    return m_annotations[index];
}

//=============================================================================================================

void FiffAnnotations::append(const FiffAnnotation& annotation)
{
    m_annotations.append(annotation);
}

//=============================================================================================================

void FiffAnnotations::append(double onset, double duration, const QString& description,
                             const QStringList& channelNames, const QString& comment)
{
    FiffAnnotation a;
    a.onset = onset;
    a.duration = duration;
    a.description = description;
    a.channelNames = channelNames;
    a.comment = comment;
    m_annotations.append(a);
}

//=============================================================================================================

void FiffAnnotations::remove(int index)
{
    m_annotations.remove(index);
}

//=============================================================================================================

void FiffAnnotations::clear()
{
    m_annotations.clear();
}

//=============================================================================================================

const QVector<FiffAnnotation>& FiffAnnotations::toVector() const
{
    return m_annotations;
}

//=============================================================================================================

int FiffAnnotations::onsetToSample(int index, double sfreq, int firstSample) const
{
    return static_cast<int>(m_annotations[index].onset * sfreq) + firstSample;
}

//=============================================================================================================

int FiffAnnotations::endToSample(int index, double sfreq, int firstSample) const
{
    const FiffAnnotation& a = m_annotations[index];
    return static_cast<int>((a.onset + a.duration) * sfreq) + firstSample;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::select(const QString& descriptionFilter) const
{
    FiffAnnotations result;
    for (const FiffAnnotation& a : m_annotations) {
        if (a.description.startsWith(descriptionFilter)) {
            result.append(a);
        }
    }
    return result;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::selectByChannel(const QString& channelName) const
{
    FiffAnnotations result;
    for (const FiffAnnotation& a : m_annotations) {
        if (a.channelNames.isEmpty() || a.channelNames.contains(channelName)) {
            result.append(a);
        }
    }
    return result;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::crop(double tmin, double tmax) const
{
    FiffAnnotations result;
    for (const FiffAnnotation& a : m_annotations) {
        double aEnd = a.onset + a.duration;

        // Check overlap: annotation interval [onset, aEnd] vs window [tmin, tmax]
        if (a.onset > tmax || aEnd < tmin) {
            continue;
        }

        FiffAnnotation clipped = a;
        if (clipped.onset < tmin) {
            double shift = tmin - clipped.onset;
            clipped.onset = tmin;
            clipped.duration -= shift;
        }
        double clippedEnd = clipped.onset + clipped.duration;
        if (clippedEnd > tmax) {
            clipped.duration = tmax - clipped.onset;
        }
        result.append(clipped);
    }
    return result;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::readJson(const QString& path)
{
    FiffAnnotations annot;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FiffAnnotations::readJson] Cannot open file:" << path;
        return annot;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[FiffAnnotations::readJson] JSON parse error:" << parseError.errorString();
        return annot;
    }

    QJsonObject root = doc.object();
    QJsonArray arr = root.value("annotations").toArray();

    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr[i].toObject();
        FiffAnnotation a;
        a.onset = obj.value("onset").toDouble(0.0);
        a.duration = obj.value("duration").toDouble(0.0);
        a.description = obj.value("description").toString();

        QJsonArray chArr = obj.value("channel_names").toArray();
        for (int j = 0; j < chArr.size(); ++j) {
            a.channelNames.append(chArr[j].toString());
        }

        a.comment = obj.value("comment").toString();
        a.extras = obj.value("extras").toObject().toVariantMap();

        annot.append(a);
    }

    return annot;
}

//=============================================================================================================

bool FiffAnnotations::writeJson(const QString& path, const FiffAnnotations& annot)
{
    QJsonArray arr;
    for (const FiffAnnotation& a : annot.toVector()) {
        QJsonObject obj;
        obj["onset"] = a.onset;
        obj["duration"] = a.duration;
        obj["description"] = a.description;

        QJsonArray chArr;
        for (const QString& ch : a.channelNames) {
            chArr.append(ch);
        }
        obj["channel_names"] = chArr;
        obj["comment"] = a.comment;
        obj["extras"] = QJsonObject::fromVariantMap(a.extras);
        arr.append(obj);
    }

    QJsonObject root;
    root["annotations"] = arr;
    QJsonDocument doc(root);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[FiffAnnotations::writeJson] Cannot open file for writing:" << path;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::readCsv(const QString& path)
{
    FiffAnnotations annot;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FiffAnnotations::readCsv] Cannot open file:" << path;
        return annot;
    }

    QTextStream in(&file);
    // Skip header line
    if (!in.atEnd()) {
        in.readLine();
    }

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        // Parse: onset,duration,description
        // Description may contain commas, so split only the first two commas
        int firstComma = line.indexOf(',');
        if (firstComma < 0) continue;
        int secondComma = line.indexOf(',', firstComma + 1);
        if (secondComma < 0) continue;

        bool onsetOk = false, durationOk = false;
        double onset = line.left(firstComma).toDouble(&onsetOk);
        double duration = line.mid(firstComma + 1, secondComma - firstComma - 1).toDouble(&durationOk);
        QString description = line.mid(secondComma + 1);

        if (!onsetOk || !durationOk) {
            continue;
        }

        annot.append(onset, duration, description);
    }

    file.close();
    return annot;
}

//=============================================================================================================

bool FiffAnnotations::writeCsv(const QString& path, const FiffAnnotations& annot)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[FiffAnnotations::writeCsv] Cannot open file for writing:" << path;
        return false;
    }

    QTextStream out(&file);
    out << "onset,duration,description\n";

    for (const FiffAnnotation& a : annot.toVector()) {
        out << a.onset << "," << a.duration << "," << a.description << "\n";
    }

    file.close();
    return true;
}

//=============================================================================================================

FiffAnnotations FiffAnnotations::read(const QString& path)
{
    QString ext = QFileInfo(path).suffix().toLower();

    if (ext == "json") {
        return readJson(path);
    } else if (ext == "csv") {
        return readCsv(path);
    }

    // Unknown extension: try JSON first, then CSV
    FiffAnnotations annot = readJson(path);
    if (!annot.isEmpty()) {
        return annot;
    }
    return readCsv(path);
}

//=============================================================================================================

bool FiffAnnotations::write(const QString& path, const FiffAnnotations& annot)
{
    QString ext = QFileInfo(path).suffix().toLower();

    if (ext == "csv") {
        return writeCsv(path, annot);
    }
    // Default to JSON
    return writeJson(path, annot);
}
