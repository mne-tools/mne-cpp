//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     annotationmodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     April, 2026
 * @version  2.2.0
 * @brief    Definition of the AnnotationModel class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "annotationmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBrush>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QTimeZone>
#include <QtMath>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

using namespace MNEBROWSE;
using namespace mne;

namespace
{

enum AnnotationColumn
{
    StartSampleColumn = 0,
    EndSampleColumn,
    OnsetSecondsColumn,
    DurationSecondsColumn,
    DescriptionColumn,
    ChannelsColumn,
    CommentColumn,
    AnnotationColumnCount
};

constexpr int FiffBlockMneAnnotations = 3810;
constexpr int FiffMneBaselineMin = 3568;
constexpr int FiffMneBaselineMax = 3569;
constexpr int FiffMneEpochsDropLog = 3801;

QString qText(const std::string& text)
{
    return QString::fromStdString(text);
}

QStringList qNames(const std::vector<std::string>& names)
{
    QStringList list;
    list.reserve(static_cast<qsizetype>(names.size()));
    for (const std::string& name : names) {
        list.append(QString::fromStdString(name));
    }
    return list;
}

std::vector<std::string> stdNames(const QStringList& names)
{
    std::vector<std::string> list;
    list.reserve(static_cast<std::size_t>(names.size()));
    for (const QString& name : names) {
        list.push_back(name.toStdString());
    }
    return list;
}

QVariantMap qExtras(const FiffAnnotation::Extras& extras)
{
    QVariantMap map;
    for (const auto& [key, value] : extras) {
        map.insert(QString::fromStdString(key), std::visit([](const auto& v) -> QVariant {
                       using T = std::decay_t<decltype(v)>;
                       if constexpr (std::is_same_v<T, std::monostate>) {
                           return QVariant();
                       } else if constexpr (std::is_same_v<T, std::string>) {
                           return QString::fromStdString(v);
                       } else if constexpr (std::is_same_v<T, std::int64_t>) {
                           return QVariant::fromValue(static_cast<qlonglong>(v));
                       } else {
                           return QVariant(v);
                       }
                   },
                                                           value));
    }
    return map;
}

FiffAnnotation::Extras stdExtras(const QVariantMap& map)
{
    FiffAnnotation::Extras extras;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QVariant& value = it.value();
        FiffAnnotation::Extra extra;
        switch (value.typeId()) {
            case QMetaType::Bool:
                extra = value.toBool();
                break;
            case QMetaType::Int:
            case QMetaType::UInt:
            case QMetaType::LongLong:
            case QMetaType::ULongLong:
                extra = static_cast<std::int64_t>(value.toLongLong());
                break;
            case QMetaType::Double:
            case QMetaType::Float:
                extra = value.toDouble();
                break;
            default:
                if (value.isValid() && !value.isNull()) {
                    extra = value.toString().toStdString();
                }
        }
        extras.insert_or_assign(it.key().toStdString(), std::move(extra));
    }
    return extras;
}

QString defaultAnnotationDescription()
{
    return QStringLiteral("BAD_manual");
}

QString displayChannelNames(const QStringList& channelNames)
{
    return channelNames.join(QStringLiteral(", "));
}

QStringList parseDisplayChannelNames(QString text)
{
    text = text.trimmed();
    if (text.isEmpty()) {
        return {};
    }

    text.replace(QLatin1Char(';'), QLatin1Char(','));
    const QChar separator = text.contains(QLatin1Char(',')) ? QLatin1Char(',') : QLatin1Char(':');

    QStringList channelNames;
    const QStringList parts = text.split(separator, Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty() && !channelNames.contains(trimmed)) {
            channelNames.append(trimmed);
        }
    }

    return channelNames;
}

QString encodeMneChannelNames(const QStringList& channelNames)
{
    QStringList encodedNames;
    encodedNames.reserve(channelNames.size());

    for (const QString& channelName : channelNames) {
        QString sanitized = channelName.trimmed();
        if (sanitized.isEmpty()) {
            continue;
        }

        sanitized.replace(QStringLiteral("{COLON}"), QStringLiteral("{COLON}{COLON}"));
        sanitized.replace(QLatin1Char(':'), QStringLiteral("{COLON}"));
        encodedNames.append(sanitized);
    }

    return encodedNames.join(QLatin1Char(':'));
}

QStringList decodeMneChannelNames(const QString& text)
{
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty()) {
        return {};
    }

    QStringList channelNames;
    const QStringList parts = trimmedText.split(QLatin1Char(':'), Qt::KeepEmptyParts);
    for (QString part : parts) {
        part.replace(QStringLiteral("{COLON}{COLON}"), QStringLiteral("{COLON}"));
        part.replace(QStringLiteral("{COLON}"), QStringLiteral(":"));
        part = part.trimmed();
        if (!part.isEmpty()) {
            channelNames.append(part);
        }
    }

    return channelNames;
}

QString variantToText(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QString();
    }

    switch (value.metaType().id()) {
        case QMetaType::Bool:
            return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        case QMetaType::Double:
            return QString::number(value.toDouble(), 'g', 15);
        default:
            return value.toString();
    }
}

QVariant textToVariant(const QString& text)
{
    if (text.isEmpty()) {
        return QString();
    }

    QString trimmed = text.trimmed();
    if (trimmed.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    if (trimmed.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
        return false;
    }

    bool intOk = false;
    const qlonglong intValue = trimmed.toLongLong(&intOk);
    if (intOk) {
        return intValue;
    }

    bool doubleOk = false;
    const double doubleValue = trimmed.toDouble(&doubleOk);
    if (doubleOk) {
        return doubleValue;
    }

    return trimmed;
}

QJsonObject variantMapToJsonObject(const QVariantMap& values)
{
    QJsonObject object;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        object.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }

    return object;
}

QString sanitizeTxtField(QString text)
{
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    text.replace(QLatin1Char('\r'), QLatin1Char(' '));
    text.replace(QLatin1Char(','), QLatin1Char(' '));
    return text.trimmed();
}

QString escapeCsvField(QString text)
{
    text.replace(QStringLiteral("\""), QStringLiteral("\"\""));

    if (text.contains(QLatin1Char(',')) || text.contains(QLatin1Char('"')) || text.contains(QLatin1Char('\n')) || text.contains(QLatin1Char('\r'))) {
        return QStringLiteral("\"%1\"").arg(text);
    }

    return text;
}

QStringList parseCsvRow(const QString& line)
{
    QStringList fields;
    QString currentField;
    bool insideQuotes = false;

    for (int index = 0; index < line.size(); ++index) {
        const QChar character = line.at(index);

        if (character == QLatin1Char('"')) {
            if (insideQuotes && index + 1 < line.size() && line.at(index + 1) == QLatin1Char('"')) {
                currentField.append(QLatin1Char('"'));
                ++index;
            } else {
                insideQuotes = !insideQuotes;
            }
            continue;
        }

        if (character == QLatin1Char(',') && !insideQuotes) {
            fields.append(currentField);
            currentField.clear();
            continue;
        }

        currentField.append(character);
    }

    fields.append(currentField);

    if (!fields.isEmpty() && !fields.first().isEmpty() && fields.first().at(0) == QChar(0xFEFF)) {
        fields[0].remove(0, 1);
    }

    return fields;
}

bool parseTimestampToUsecs(const QString& text, qint64& usecsSinceEpoch)
{
    static const QRegularExpression timestampExpression(
        QStringLiteral("^(\\d{4})-(\\d{2})-(\\d{2})[ T](\\d{2}):(\\d{2}):(\\d{2})(?:\\.(\\d{1,6}))?$"));

    const QRegularExpressionMatch match = timestampExpression.match(text.trimmed());
    if (!match.hasMatch()) {
        return false;
    }

    const QDate date(match.captured(1).toInt(),
                     match.captured(2).toInt(),
                     match.captured(3).toInt());
    const QTime time(match.captured(4).toInt(),
                     match.captured(5).toInt(),
                     match.captured(6).toInt());

    if (!date.isValid() || !time.isValid()) {
        return false;
    }

    const QDateTime dateTime(date, time, QTimeZone::UTC);
    QString fractionalPart = match.captured(7);
    if (!fractionalPart.isEmpty()) {
        fractionalPart = fractionalPart.leftJustified(6, QLatin1Char('0'), true).left(6);
    } else {
        fractionalPart = QStringLiteral("000000");
    }

    usecsSinceEpoch = dateTime.toSecsSinceEpoch() * 1000000LL + fractionalPart.toLongLong();
    return true;
}

QString formatTimestampFromUsecs(qint64 usecsSinceEpoch)
{
    qint64 secondsSinceEpoch = usecsSinceEpoch / 1000000LL;
    qint64 microseconds = usecsSinceEpoch % 1000000LL;

    if (microseconds < 0) {
        microseconds += 1000000LL;
        --secondsSinceEpoch;
    }

    const QDateTime dateTime = QDateTime::fromSecsSinceEpoch(secondsSinceEpoch, QTimeZone::UTC);
    return QStringLiteral("%1.%2")
        .arg(dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
        .arg(microseconds, 6, 10, QLatin1Char('0'));
}

} // namespace

//=============================================================================================================

AnnotationModel::AnnotationModel(QObject* parent)
: QAbstractTableModel(parent)
, m_pFiffInfo(new mne::FiffInfo)
{
}

//=============================================================================================================

AnnotationModel::~AnnotationModel() = default;

//=============================================================================================================

int AnnotationModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return std::ssize(m_fiffAnnotations);
}

//=============================================================================================================

int AnnotationModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return AnnotationColumnCount;
}

//=============================================================================================================

QVariant AnnotationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case StartSampleColumn:
                return QVariant(QStringLiteral("Start"));
            case EndSampleColumn:
                return QVariant(QStringLiteral("End"));
            case OnsetSecondsColumn:
                return QVariant(QStringLiteral("Onset (s)"));
            case DurationSecondsColumn:
                return QVariant(QStringLiteral("Duration (s)"));
            case DescriptionColumn:
                return QVariant(QStringLiteral("Description"));
            case ChannelsColumn:
                return QVariant(QStringLiteral("Channels"));
            case CommentColumn:
                return QVariant(QStringLiteral("Comment"));
            default:
                break;
        }
    } else if (orientation == Qt::Vertical) {
        return QStringLiteral("Annotation %1").arg(section + 1);
    }

    return QVariant();
}

//=============================================================================================================

QVariant AnnotationModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= std::ssize(m_fiffAnnotations)) {
        return QVariant();
    }

    const FiffAnnotation& annot = m_fiffAnnotations[index.row()];

    if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
    }

    if (role == Qt::BackgroundRole) {
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        QColor color = colorForLabel(qText(annot.description));
        color.setAlpha(40);
        brush.setColor(color);
        return QVariant(brush);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;
    const int startSample = m_iFirstSample + static_cast<int>(qRound(annot.onset * sfreq));
    const int endSample = startSample + static_cast<int>(qRound(annot.duration * sfreq));
    const int relativeStart = startSample - m_iFirstSample;
    const int relativeEnd = endSample - m_iFirstSample;

    switch (index.column()) {
        case StartSampleColumn:
            return QVariant(relativeStart);
        case EndSampleColumn:
            return QVariant(relativeEnd);
        case OnsetSecondsColumn:
            return QVariant(annot.onset);
        case DurationSecondsColumn:
            return QVariant(annot.duration);
        case DescriptionColumn:
            return QVariant(qText(annot.description));
        case ChannelsColumn:
            return QVariant(displayChannelNames(qNames(annot.ch_names)));
        case CommentColumn:
            return QVariant(qText(annot.comment));
        default:
            return QVariant();
    }
}

//=============================================================================================================

Qt::ItemFlags AnnotationModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

//=============================================================================================================

bool AnnotationModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role != Qt::EditRole || !index.isValid() || index.row() < 0 || index.row() >= std::ssize(m_fiffAnnotations)) {
        return false;
    }

    FiffAnnotation& annot = m_fiffAnnotations[index.row()];
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;

    switch (index.column()) {
        case StartSampleColumn:
            annot.onset = static_cast<double>(value.toInt()) / sfreq;
            break;
        case EndSampleColumn: {
            double startSec = annot.onset;
            double endSec = static_cast<double>(value.toInt()) / sfreq;
            annot.duration = qMax(0.0, endSec - startSec);
            break;
        }
        case OnsetSecondsColumn:
            annot.onset = value.toDouble();
            break;
        case DurationSecondsColumn:
            annot.duration = qMax(0.0, value.toDouble());
            break;
        case DescriptionColumn:
            annot.description = value.toString().trimmed().toStdString();
            break;
        case ChannelsColumn:
            annot.ch_names = stdNames(parseDisplayChannelNames(value.toString()));
            break;
        case CommentColumn:
            annot.comment = value.toString().trimmed().toStdString();
            break;
        default:
            return false;
    }

    // Ensure non-negative onset and duration
    if (annot.onset < 0.0)
        annot.onset = 0.0;
    if (annot.duration < 0.0)
        annot.duration = 0.0;

    beginResetModel();
    sortEntries();
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent)

    if (position < 0 || rows <= 0 || position + rows > std::ssize(m_fiffAnnotations)) {
        return false;
    }

    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    m_fiffAnnotations.erase(m_fiffAnnotations.begin() + position, m_fiffAnnotations.begin() + position + rows);
    endRemoveRows();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

void AnnotationModel::setFiffInfo(const std::shared_ptr<mne::FiffInfo>& pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}

//=============================================================================================================

void AnnotationModel::setFirstLastSample(int firstSample, int lastSample)
{
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
}

//=============================================================================================================

int AnnotationModel::addAnnotation(int startSample,
                                   int endSample,
                                   const QString& description,
                                   const QStringList& channelNames,
                                   const QString& comment)
{
    AnnotationEntry entry = normalizeEntry(startSample, endSample, description, channelNames, comment);

    // Convert to time-based FiffAnnotation
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;
    // Auto-merge: absorb any existing same-description annotations that overlap or are adjacent
    QVector<int> mergeIndices;
    for (int i = 0; i < std::ssize(m_fiffAnnotations); ++i) {
        const FiffAnnotation& existing = m_fiffAnnotations[i];
        if (qText(existing.description) != entry.description)
            continue;
        // Convert existing to samples for overlap check
        int existStart = m_iFirstSample + static_cast<int>(qRound(existing.onset * sfreq));
        int existEnd = existStart + static_cast<int>(qRound(existing.duration * sfreq));
        // Overlapping or adjacent (within 1 sample)?
        if (existStart <= entry.endSample + 1 && existEnd >= entry.startSample - 1) {
            entry.startSample = qMin(entry.startSample, existStart);
            entry.endSample = qMax(entry.endSample, existEnd);
            mergeIndices.append(i);
        }
    }

    // Recompute onset/duration after merge
    const double onset = static_cast<double>(entry.startSample - m_iFirstSample) / sfreq;
    const double duration = static_cast<double>(entry.endSample - entry.startSample) / sfreq;

    beginResetModel();
    // Remove merged entries in reverse order
    std::sort(mergeIndices.begin(), mergeIndices.end(), std::greater<int>());
    for (int idx : mergeIndices)
        m_fiffAnnotations.erase(m_fiffAnnotations.begin() + idx);

    m_fiffAnnotations.push_back({onset, duration, entry.description.toStdString(), stdNames(entry.channelNames), entry.comment.toStdString(), {}});
    sortEntries();
    endResetModel();

    notifyAnnotationsChanged();

    // Find the row of the newly inserted annotation
    for (int row = 0; row < std::ssize(m_fiffAnnotations); ++row) {
        const FiffAnnotation& candidate = m_fiffAnnotations[row];
        if (qAbs(candidate.onset - onset) < 1e-9 && qAbs(candidate.duration - duration) < 1e-9 && qText(candidate.description) == entry.description) {
            return row;
        }
    }

    return -1;
}

//=============================================================================================================

QPair<int, int> AnnotationModel::getSampleRange(int row) const
{
    if (row < 0 || row >= std::ssize(m_fiffAnnotations)) {
        return QPair<int, int>(0, 0);
    }

    const FiffAnnotation& annot = m_fiffAnnotations[row];
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;
    int startSample = m_iFirstSample + static_cast<int>(qRound(annot.onset * sfreq));
    int endSample = startSample + static_cast<int>(qRound(annot.duration * sfreq));
    return QPair<int, int>(startSample, endSample);
}

//=============================================================================================================

bool AnnotationModel::updateAnnotationBoundary(int row, bool isStart, int absoluteSample)
{
    if (row < 0 || row >= std::ssize(m_fiffAnnotations)) {
        return false;
    }

    FiffAnnotation& annot = m_fiffAnnotations[row];
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;

    int startSample = m_iFirstSample + static_cast<int>(qRound(annot.onset * sfreq));
    int endSample = startSample + static_cast<int>(qRound(annot.duration * sfreq));

    if (isStart) {
        startSample = absoluteSample;
    } else {
        endSample = absoluteSample;
    }

    // Ensure start <= end
    if (startSample > endSample) {
        std::swap(startSample, endSample);
    }

    annot.onset = static_cast<double>(startSample - m_iFirstSample) / sfreq;
    annot.duration = static_cast<double>(endSample - startSample) / sfreq;
    if (annot.onset < 0.0)
        annot.onset = 0.0;
    if (annot.duration < 0.0)
        annot.duration = 0.0;

    beginResetModel();
    sortEntries();
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

QVector<AnnotationSpanData> AnnotationModel::getAnnotationSpans() const
{
    QVector<AnnotationSpanData> spans;
    spans.reserve(std::ssize(m_fiffAnnotations));

    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;

    for (int i = 0; i < std::ssize(m_fiffAnnotations); ++i) {
        const FiffAnnotation& annot = m_fiffAnnotations[i];
        AnnotationSpanData span;
        span.startSample = m_iFirstSample + static_cast<int>(qRound(annot.onset * sfreq));
        span.endSample = span.startSample + static_cast<int>(qRound(annot.duration * sfreq));
        span.label = qText(annot.description);
        span.color = colorForLabel(span.label);
        span.comment = qText(annot.comment);
        span.channelNames = qNames(annot.ch_names);
        spans.append(span);
    }

    return spans;
}

//=============================================================================================================

bool AnnotationModel::loadAnnotationData(QFile& qFile)
{
    const QString suffix = QFileInfo(qFile.fileName()).suffix().trimmed().toLower();
    if (suffix == QStringLiteral("fif")) {
        return loadAnnotationFif(qFile);
    }

    if (suffix == QStringLiteral("csv")) {
        return loadAnnotationCsv(qFile);
    }

    if (suffix == QStringLiteral("txt")) {
        return loadAnnotationTxt(qFile);
    }

    return loadAnnotationJson(qFile);
}

//=============================================================================================================

bool AnnotationModel::saveAnnotationData(QFile& qFile) const
{
    const QFileInfo fileInfo(qFile);
    if (fileInfo.dir().exists() == false) {
        Log::warning("AnnotationModel: annotation directory does not exist {}", fileInfo.absolutePath().toStdString());
        return false;
    }

    const QString suffix = fileInfo.suffix().trimmed().toLower();
    if (suffix == QStringLiteral("fif")) {
        return saveAnnotationFif(qFile);
    }

    if (suffix == QStringLiteral("csv")) {
        return saveAnnotationCsv(qFile);
    }

    if (suffix == QStringLiteral("txt")) {
        return saveAnnotationTxt(qFile);
    }

    return saveAnnotationJson(qFile);
}

//=============================================================================================================

bool AnnotationModel::loadAnnotationJson(QFile& qFile)
{
    if (!qFile.open(QIODevice::ReadOnly)) {
        Log::warning("AnnotationModel: could not open annotation file {}", qFile.fileName().toStdString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(qFile.readAll(), &parseError);
    qFile.close();

    if (parseError.error != QJsonParseError::NoError || (!document.isObject() && !document.isArray())) {
        Log::warning("AnnotationModel: invalid annotation json {} {}", qFile.fileName().toStdString(), parseError.errorString().toStdString());
        return false;
    }

    const QJsonArray annotationArray = document.isArray()
        ? document.array()
        : document.object().value(QStringLiteral("annotations")).toArray();

    QVector<AnnotationEntry> parsedEntries;
    parsedEntries.reserve(annotationArray.size());

    for (const QJsonValue& value : annotationArray) {
        const QJsonObject object = value.toObject();
        if (object.isEmpty()) {
            continue;
        }

        const QString description = object.value(QStringLiteral("description")).toString(object.value(QStringLiteral("label")).toString(defaultAnnotationDescription()));
        const QString comment = object.value(QStringLiteral("comment")).toString();

        QStringList channelNames;
        const QJsonValue channelValue = object.value(QStringLiteral("ch_names"));
        if (channelValue.isArray()) {
            const QJsonArray channelArray = channelValue.toArray();
            for (const QJsonValue& channelNameValue : channelArray) {
                const QString channelName = channelNameValue.toString().trimmed();
                if (!channelName.isEmpty() && !channelNames.contains(channelName)) {
                    channelNames.append(channelName);
                }
            }
        } else if (channelValue.isString()) {
            channelNames = decodeMneChannelNames(channelValue.toString());
        }

        int startSample = object.contains(QStringLiteral("start_sample"))
            ? object.value(QStringLiteral("start_sample")).toInt()
            : onsetSecondsToSample(object.value(QStringLiteral("onset")).toDouble());
        int endSample = object.contains(QStringLiteral("end_sample"))
            ? object.value(QStringLiteral("end_sample")).toInt(startSample)
            : startSample + durationSecondsToSamples(object.value(QStringLiteral("duration")).toDouble());

        QVariantMap extras = object.value(QStringLiteral("extras")).toObject().toVariantMap();
        parsedEntries.append(normalizeEntry(startSample, endSample, description, channelNames, comment, extras));
    }

    beginResetModel();
    importEntries(parsedEntries);
    sortEntries();
    m_bFileLoaded = true;
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::loadAnnotationFif(QFile& qFile)
{
    mne::FiffStream stream(qFile.fileName().toStdU16String());
    if (!stream.open()) {
        Log::warning("AnnotationModel: could not open annotation fif {}", qFile.fileName().toStdString());
        return false;
    }

    const std::vector<std::shared_ptr<mne::FiffDirNode>> annotationBlocks =
        stream.dirtree()->dir_tree_find(FiffBlockMneAnnotations);

    if (annotationBlocks.empty()) {
        qFile.close();
        Log::warning("AnnotationModel: could not find annotation block in {}", qFile.fileName().toStdString());
        return false;
    }

    QVector<double> onsetSeconds;
    QVector<double> endSeconds;
    QStringList descriptions;
    QVector<QStringList> channelNames;
    QVector<QVariantMap> extras;
    bool haveOrigTime = false;

    for (int entryIndex = 0; entryIndex < annotationBlocks.front()->nent(); ++entryIndex) {
        const auto& directoryEntry = annotationBlocks.front()->dir.at(entryIndex);
        std::optional<mne::FiffTag> tag;
        if (tag = stream.read_tag(directoryEntry.pos); !tag) {
            continue;
        }

        if (directoryEntry.kind == FiffMneBaselineMin) {
            if (const float* values = tag->to_float()) {
                const int count = static_cast<int>(tag->size() / sizeof(float));
                onsetSeconds.resize(count);
                for (int index = 0; index < count; ++index) {
                    onsetSeconds[index] = values[index];
                }
            }
        } else if (directoryEntry.kind == FiffMneBaselineMax) {
            if (const float* values = tag->to_float()) {
                const int count = static_cast<int>(tag->size() / sizeof(float));
                endSeconds.resize(count);
                for (int index = 0; index < count; ++index) {
                    endSeconds[index] = values[index];
                }
            }
        } else if (directoryEntry.kind == FIFF_MEAS_DATE) {
            haveOrigTime = true;
        } else if (directoryEntry.kind == FIFF_COMMENT) {
            descriptions = decodeMneChannelNames(QString::fromStdString(tag->to_string()));
        } else if (directoryEntry.kind == FiffMneEpochsDropLog) {
            const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(tag->to_string()));
            const QJsonArray rows = document.array();
            channelNames.resize(rows.size());
            for (int row = 0; row < rows.size(); ++row) {
                const QJsonArray channelRow = rows.at(row).toArray();
                for (const QJsonValue& channelValue : channelRow) {
                    const QString channelName = channelValue.toString().trimmed();
                    if (!channelName.isEmpty()) {
                        channelNames[row].append(channelName);
                    }
                }
            }
        } else if (directoryEntry.kind == FIFF_FREE_LIST) {
            const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(tag->to_string()));
            const QJsonArray rows = document.array();
            extras.resize(rows.size());
            for (int row = 0; row < rows.size(); ++row) {
                extras[row] = rows.at(row).toObject().toVariantMap();
            }
        }
    }

    qFile.close();

    const int count = onsetSeconds.size();
    if (count == 0 || endSeconds.size() != count || descriptions.size() != count) {
        Log::warning("AnnotationModel: incomplete annotation fif content in {}", qFile.fileName().toStdString());
        return false;
    }

    if (channelNames.size() < count) {
        channelNames.resize(count);
    }
    if (extras.size() < count) {
        extras.resize(count);
    }

    QVector<AnnotationEntry> parsedEntries;
    parsedEntries.reserve(count);

    for (int index = 0; index < count; ++index) {
        QVariantMap extraValues = extras.at(index);
        const QString comment = extraValues.value(QStringLiteral("comment")).toString().trimmed();
        extraValues.remove(QStringLiteral("comment"));

        const int startSample = onsetSecondsToSample(onsetSeconds.at(index) + (haveOrigTime ? sampleToOnsetSeconds(0) : 0.0));
        const int endSample = startSample + durationSecondsToSamples(endSeconds.at(index) - onsetSeconds.at(index));
        parsedEntries.append(normalizeEntry(startSample,
                                            endSample,
                                            descriptions.at(index),
                                            channelNames.at(index),
                                            comment,
                                            extraValues));
    }

    beginResetModel();
    importEntries(parsedEntries);
    sortEntries();
    m_bFileLoaded = true;
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::loadAnnotationCsv(QFile& qFile)
{
    if (!qFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::warning("AnnotationModel: could not open annotation csv {}", qFile.fileName().toStdString());
        return false;
    }

    QTextStream stream(&qFile);
    if (stream.atEnd()) {
        qFile.close();
        Log::warning("AnnotationModel: empty annotation csv {}", qFile.fileName().toStdString());
        return false;
    }

    const QStringList columns = parseCsvRow(stream.readLine());
    QMap<QString, int> columnMap;
    for (int index = 0; index < columns.size(); ++index) {
        columnMap.insert(columns.at(index).trimmed(), index);
    }

    if (!columnMap.contains(QStringLiteral("onset")) || !columnMap.contains(QStringLiteral("duration")) || !columnMap.contains(QStringLiteral("description"))) {
        qFile.close();
        Log::warning("AnnotationModel: invalid annotation csv header {}", qFile.fileName().toStdString());
        return false;
    }

    bool haveMeasurementStart = false;
    const qint64 measurementStartUsecs = measurementStartUsecsSinceEpoch(&haveMeasurementStart);

    bool haveFirstTimestamp = false;
    qint64 firstTimestampUsecs = 0;
    bool preferMeasurementStart = false;
    bool haveFirstNumericOnset = false;
    double firstNumericOnsetMilliseconds = 0.0;

    QVector<AnnotationEntry> parsedEntries;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.trimmed().isEmpty()) {
            continue;
        }

        QStringList values = parseCsvRow(line);
        while (values.size() < columns.size()) {
            values.append(QString());
        }

        const QString onsetText = values.value(columnMap.value(QStringLiteral("onset"))).trimmed();
        const QString durationText = values.value(columnMap.value(QStringLiteral("duration"))).trimmed();
        const QString description = values.value(columnMap.value(QStringLiteral("description"))).trimmed();

        double onsetSeconds = 0.0;
        qint64 onsetTimestampUsecs = 0;
        if (parseTimestampToUsecs(onsetText, onsetTimestampUsecs)) {
            if (!haveFirstTimestamp) {
                haveFirstTimestamp = true;
                firstTimestampUsecs = onsetTimestampUsecs;
                preferMeasurementStart = haveMeasurementStart && qAbs(firstTimestampUsecs - measurementStartUsecs) <= 86400LL * 1000000LL;
            }

            const qint64 baseUsecs = preferMeasurementStart ? measurementStartUsecs : firstTimestampUsecs;
            onsetSeconds = static_cast<double>(onsetTimestampUsecs - baseUsecs) / 1000000.0;
        } else {
            bool onsetOk = false;
            const double onsetMilliseconds = onsetText.toDouble(&onsetOk);
            if (!onsetOk) {
                continue;
            }
            if (!haveFirstNumericOnset) {
                haveFirstNumericOnset = true;
                firstNumericOnsetMilliseconds = onsetMilliseconds;
            }
            onsetSeconds = (onsetMilliseconds - firstNumericOnsetMilliseconds) / 1000.0;
        }

        bool durationOk = false;
        const double durationSeconds = durationText.toDouble(&durationOk);
        if (!durationOk) {
            continue;
        }

        QStringList channelNames;
        if (columnMap.contains(QStringLiteral("ch_names"))) {
            channelNames = decodeMneChannelNames(values.value(columnMap.value(QStringLiteral("ch_names"))));
        }

        const QString comment = columnMap.contains(QStringLiteral("comment"))
            ? values.value(columnMap.value(QStringLiteral("comment"))).trimmed()
            : QString();

        QVariantMap extras;
        for (auto it = columnMap.constBegin(); it != columnMap.constEnd(); ++it) {
            if (it.key() == QStringLiteral("onset") || it.key() == QStringLiteral("duration") || it.key() == QStringLiteral("description") || it.key() == QStringLiteral("ch_names") || it.key() == QStringLiteral("comment")) {
                continue;
            }

            extras.insert(it.key(), textToVariant(values.value(it.value()).trimmed()));
        }

        const int startSample = onsetSecondsToSample(onsetSeconds);
        const int endSample = startSample + durationSecondsToSamples(durationSeconds);
        parsedEntries.append(normalizeEntry(startSample, endSample, description, channelNames, comment, extras));
    }

    qFile.close();

    beginResetModel();
    importEntries(parsedEntries);
    sortEntries();
    m_bFileLoaded = true;
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::loadAnnotationTxt(QFile& qFile)
{
    if (!qFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::warning("AnnotationModel: could not open annotation txt {}", qFile.fileName().toStdString());
        return false;
    }

    QTextStream stream(&qFile);
    QStringList columns;
    QVector<AnnotationEntry> parsedEntries;

    while (!stream.atEnd()) {
        const QString rawLine = stream.readLine();
        const QString line = rawLine.trimmed();

        if (line.isEmpty()) {
            continue;
        }

        if (line.startsWith(QLatin1Char('#'))) {
            if (line.startsWith(QStringLiteral("# onset"))) {
                columns = parseCsvRow(line.mid(2));
                for (QString& column : columns) {
                    column = column.trimmed();
                }
            }
            continue;
        }

        QStringList values = parseCsvRow(rawLine);

        QStringList activeColumns = columns;
        if (activeColumns.isEmpty()) {
            activeColumns = {QStringLiteral("onset"),
                             QStringLiteral("duration"),
                             QStringLiteral("description")};
            if (values.size() == 4) {
                activeColumns.append(QStringLiteral("ch_names"));
            }
        }

        while (values.size() < activeColumns.size()) {
            values.append(QString());
        }

        QMap<QString, int> columnMap;
        for (int index = 0; index < activeColumns.size(); ++index) {
            columnMap.insert(activeColumns.at(index), index);
        }

        if (!columnMap.contains(QStringLiteral("onset")) || !columnMap.contains(QStringLiteral("duration")) || !columnMap.contains(QStringLiteral("description"))) {
            continue;
        }

        bool onsetOk = false;
        const double onsetSeconds = values.value(columnMap.value(QStringLiteral("onset"))).trimmed().toDouble(&onsetOk);
        bool durationOk = false;
        const double durationSeconds = values.value(columnMap.value(QStringLiteral("duration"))).trimmed().toDouble(&durationOk);
        if (!onsetOk || !durationOk) {
            continue;
        }

        const QString description = values.value(columnMap.value(QStringLiteral("description"))).trimmed();

        QStringList channelNames;
        if (columnMap.contains(QStringLiteral("ch_names"))) {
            channelNames = decodeMneChannelNames(values.value(columnMap.value(QStringLiteral("ch_names"))));
        }

        const QString comment = columnMap.contains(QStringLiteral("comment"))
            ? values.value(columnMap.value(QStringLiteral("comment"))).trimmed()
            : QString();

        QVariantMap extras;
        for (auto it = columnMap.constBegin(); it != columnMap.constEnd(); ++it) {
            if (it.key() == QStringLiteral("onset") || it.key() == QStringLiteral("duration") || it.key() == QStringLiteral("description") || it.key() == QStringLiteral("ch_names") || it.key() == QStringLiteral("comment")) {
                continue;
            }

            extras.insert(it.key(), textToVariant(values.value(it.value()).trimmed()));
        }

        const int startSample = onsetSecondsToSample(onsetSeconds);
        const int endSample = startSample + durationSecondsToSamples(durationSeconds);
        parsedEntries.append(normalizeEntry(startSample, endSample, description, channelNames, comment, extras));
    }

    qFile.close();

    beginResetModel();
    importEntries(parsedEntries);
    sortEntries();
    m_bFileLoaded = true;
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::saveAnnotationJson(QFile& qFile) const
{
    if (!qFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Log::warning("AnnotationModel: could not write annotation file {}", qFile.fileName().toStdString());
        return false;
    }

    QJsonArray annotationArray;
    const QVector<AnnotationEntry> entries = exportEntries();
    for (const AnnotationEntry& entry : entries) {
        QJsonObject object;
        object.insert(QStringLiteral("start_sample"), entry.startSample);
        object.insert(QStringLiteral("end_sample"), entry.endSample);
        object.insert(QStringLiteral("onset"), sampleToOnsetSeconds(entry.startSample));
        object.insert(QStringLiteral("duration"),
                      static_cast<double>(entry.endSample - entry.startSample) / ((m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0));
        object.insert(QStringLiteral("description"), entry.description);
        object.insert(QStringLiteral("label"), entry.description);
        object.insert(QStringLiteral("comment"), entry.comment);

        QJsonArray channelArray;
        for (const QString& channelName : entry.channelNames) {
            channelArray.append(channelName);
        }
        object.insert(QStringLiteral("ch_names"), channelArray);

        if (!entry.extras.isEmpty()) {
            object.insert(QStringLiteral("extras"), variantMapToJsonObject(entry.extras));
        }

        annotationArray.append(object);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("version"), 2);
    rootObject.insert(QStringLiteral("first_sample"), m_iFirstSample);
    rootObject.insert(QStringLiteral("sample_freq"),
                      (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 0.0);

    bool haveMeasurementStart = false;
    const qint64 measurementStartUsecs = measurementStartUsecsSinceEpoch(&haveMeasurementStart);
    if (haveMeasurementStart) {
        rootObject.insert(QStringLiteral("orig_time"), formatTimestampFromUsecs(measurementStartUsecs));
    }

    if (m_pFiffInfo && !m_pFiffInfo->utc_offset.empty()) {
        rootObject.insert(QStringLiteral("utc_offset"), QString::fromStdString(m_pFiffInfo->utc_offset));
    }

    rootObject.insert(QStringLiteral("annotations"), annotationArray);

    const QJsonDocument document(rootObject);
    const qint64 bytesWritten = qFile.write(document.toJson(QJsonDocument::Indented));
    qFile.close();

    return bytesWritten >= 0;
}

//=============================================================================================================

bool AnnotationModel::saveAnnotationFif(QFile& qFile) const
{
    auto stream = mne::FiffStream::start_file(std::filesystem::path(qFile.fileName().toStdU16String()));
    if (!stream) {
        Log::warning("AnnotationModel: could not write annotation fif {}", qFile.fileName().toStdString());
        return false;
    }

    QVector<float> onsetSeconds;
    QVector<float> endSeconds;
    onsetSeconds.reserve(std::ssize(m_fiffAnnotations));
    endSeconds.reserve(std::ssize(m_fiffAnnotations));

    QStringList descriptions;
    descriptions.reserve(std::ssize(m_fiffAnnotations));

    QJsonArray channelNameRows;
    bool haveChannelNames = false;

    QJsonArray extrasRows;
    bool haveExtras = false;

    bool haveMeasurementStart = false;
    const qint64 measurementStartUsecs = measurementStartUsecsSinceEpoch(&haveMeasurementStart);
    // With an orig_time, MNE counts onsets from the measurement start rather than the first sample
    const double origTimeOffset = haveMeasurementStart ? sampleToOnsetSeconds(0) * -1.0 : 0.0;

    const QVector<AnnotationEntry> entries = exportEntries();
    for (const AnnotationEntry& entry : entries) {
        const float onset = static_cast<float>(sampleToOnsetSeconds(entry.startSample) + origTimeOffset);
        const float duration = static_cast<float>((entry.endSample - entry.startSample) / ((m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0));

        onsetSeconds.append(onset);
        endSeconds.append(onset + duration);
        descriptions.append(entry.description);

        QJsonArray channelRow;
        for (const QString& channelName : entry.channelNames) {
            channelRow.append(channelName);
        }
        haveChannelNames = haveChannelNames || !entry.channelNames.isEmpty();
        channelNameRows.append(channelRow);

        QVariantMap extraValues = entry.extras;
        if (!entry.comment.isEmpty()) {
            extraValues.insert(QStringLiteral("comment"), entry.comment);
        }

        QJsonObject extraObject;
        for (auto it = extraValues.constBegin(); it != extraValues.constEnd(); ++it) {
            extraObject.insert(it.key(), QJsonValue::fromVariant(it.value()));
        }

        haveExtras = haveExtras || !extraObject.isEmpty();
        extrasRows.append(extraObject);
    }

    stream->start_block(FiffBlockMneAnnotations);
    if (!onsetSeconds.isEmpty()) {
        stream->write_float(FiffMneBaselineMin, onsetSeconds.data(), onsetSeconds.size());
        stream->write_float(FiffMneBaselineMax, endSeconds.data(), endSeconds.size());

        const QString encodedDescriptions = encodeMneChannelNames(descriptions);
        stream->write_string(FIFF_COMMENT, encodedDescriptions.toStdString());
    }

    if (haveMeasurementStart) {
        const double measurementStamp[2] = {
            static_cast<double>(measurementStartUsecs / 1000000LL),
            static_cast<double>(measurementStartUsecs % 1000000LL)};
        stream->write_double(FIFF_MEAS_DATE, measurementStamp, 2);
    }

    if (haveChannelNames) {
        const QString json = QString::fromUtf8(QJsonDocument(channelNameRows).toJson(QJsonDocument::Compact));
        stream->write_string(FiffMneEpochsDropLog, json.toStdString());
    }

    if (haveExtras) {
        const QString json = QString::fromUtf8(QJsonDocument(extrasRows).toJson(QJsonDocument::Compact));
        stream->write_string(FIFF_FREE_LIST, json.toStdString());
    }

    stream->end_block(FiffBlockMneAnnotations);
    stream->end_file();
    qFile.close();
    return true;
}

//=============================================================================================================

bool AnnotationModel::saveAnnotationCsv(QFile& qFile) const
{
    if (!qFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        Log::warning("AnnotationModel: could not write annotation csv {}", qFile.fileName().toStdString());
        return false;
    }

    bool haveChannels = false;
    bool haveComments = false;
    QSet<QString> extraKeys;
    const QVector<AnnotationEntry> entries = exportEntries();
    for (const AnnotationEntry& entry : entries) {
        haveChannels = haveChannels || !entry.channelNames.isEmpty();
        haveComments = haveComments || !entry.comment.isEmpty();
        for (auto it = entry.extras.constBegin(); it != entry.extras.constEnd(); ++it) {
            extraKeys.insert(it.key());
        }
    }

    QStringList orderedExtraKeys = extraKeys.values();
    std::sort(orderedExtraKeys.begin(), orderedExtraKeys.end());

    QStringList columns = {QStringLiteral("onset"),
                           QStringLiteral("duration"),
                           QStringLiteral("description")};
    if (haveChannels) {
        columns.append(QStringLiteral("ch_names"));
    }
    if (haveComments) {
        columns.append(QStringLiteral("comment"));
    }
    columns.append(orderedExtraKeys);

    QTextStream stream(&qFile);
    QStringList escapedHeader;
    escapedHeader.reserve(columns.size());
    for (const QString& column : columns) {
        escapedHeader.append(escapeCsvField(column));
    }
    stream << escapedHeader.join(QLatin1Char(',')) << '\n';

    bool haveMeasurementStart = false;
    const qint64 measurementStartUsecs = measurementStartUsecsSinceEpoch(&haveMeasurementStart);
    const qint64 csvBaseUsecs = haveMeasurementStart ? measurementStartUsecs : 0;

    for (const AnnotationEntry& entry : entries) {
        const qint64 onsetUsecs = csvBaseUsecs + qRound64(sampleToOnsetSeconds(entry.startSample) * 1000000.0);
        QStringList values;
        values.reserve(columns.size());
        values.append(formatTimestampFromUsecs(onsetUsecs));
        values.append(QString::number(static_cast<double>(entry.endSample - entry.startSample) / ((m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0),
                                      'g',
                                      15));
        values.append(entry.description);

        if (haveChannels) {
            values.append(encodeMneChannelNames(entry.channelNames));
        }

        if (haveComments) {
            values.append(entry.comment);
        }

        for (const QString& extraKey : orderedExtraKeys) {
            values.append(variantToText(entry.extras.value(extraKey)));
        }

        QStringList escapedValues;
        escapedValues.reserve(values.size());
        for (const QString& value : values) {
            escapedValues.append(escapeCsvField(value));
        }

        stream << escapedValues.join(QLatin1Char(',')) << '\n';
    }

    qFile.close();
    return true;
}

//=============================================================================================================

bool AnnotationModel::saveAnnotationTxt(QFile& qFile) const
{
    if (!qFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        Log::warning("AnnotationModel: could not write annotation txt {}", qFile.fileName().toStdString());
        return false;
    }

    bool haveChannels = false;
    bool haveComments = false;
    QSet<QString> extraKeys;
    const QVector<AnnotationEntry> entries = exportEntries();
    for (const AnnotationEntry& entry : entries) {
        haveChannels = haveChannels || !entry.channelNames.isEmpty();
        haveComments = haveComments || !entry.comment.isEmpty();
        for (auto it = entry.extras.constBegin(); it != entry.extras.constEnd(); ++it) {
            extraKeys.insert(it.key());
        }
    }

    QStringList orderedExtraKeys = extraKeys.values();
    std::sort(orderedExtraKeys.begin(), orderedExtraKeys.end());

    QTextStream stream(&qFile);
    stream << "# MNE-Annotations\n";

    bool haveMeasurementStart = false;
    const qint64 measurementStartUsecs = measurementStartUsecsSinceEpoch(&haveMeasurementStart);
    if (haveMeasurementStart) {
        stream << "# orig_time : " << formatTimestampFromUsecs(measurementStartUsecs) << '\n';
    }

    QStringList columns = {QStringLiteral("onset"),
                           QStringLiteral("duration"),
                           QStringLiteral("description")};
    if (haveChannels) {
        columns.append(QStringLiteral("ch_names"));
    }
    if (haveComments) {
        columns.append(QStringLiteral("comment"));
    }
    columns.append(orderedExtraKeys);

    stream << "# " << columns.join(QStringLiteral(", ")) << '\n';

    for (const AnnotationEntry& entry : entries) {
        QStringList values;
        values.reserve(columns.size());
        values.append(QString::number(sampleToOnsetSeconds(entry.startSample), 'g', 15));
        values.append(QString::number(static_cast<double>(entry.endSample - entry.startSample) / ((m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0),
                                      'g',
                                      15));
        values.append(sanitizeTxtField(entry.description));

        if (haveChannels) {
            values.append(encodeMneChannelNames(entry.channelNames));
        }

        if (haveComments) {
            values.append(sanitizeTxtField(entry.comment));
        }

        for (const QString& extraKey : orderedExtraKeys) {
            values.append(sanitizeTxtField(variantToText(entry.extras.value(extraKey))));
        }

        stream << values.join(QLatin1Char(',')) << '\n';
    }

    qFile.close();
    return true;
}

//=============================================================================================================

void AnnotationModel::clearModel()
{
    beginResetModel();
    m_fiffAnnotations.clear();
    m_bFileLoaded = false;
    endResetModel();

    notifyAnnotationsChanged();
}

//=============================================================================================================

bool AnnotationModel::isFileLoaded() const
{
    return m_bFileLoaded;
}

//=============================================================================================================

const FiffAnnotations& AnnotationModel::fiffAnnotations() const
{
    return m_fiffAnnotations;
}

//=============================================================================================================

void AnnotationModel::setFiffAnnotations(const FiffAnnotations& annotations)
{
    beginResetModel();
    m_fiffAnnotations = annotations;
    m_bFileLoaded = false;
    endResetModel();

    notifyAnnotationsChanged();
}

//=============================================================================================================

AnnotationModel::AnnotationEntry AnnotationModel::normalizeEntry(int startSample,
                                                                 int endSample,
                                                                 const QString& description,
                                                                 const QStringList& channelNames,
                                                                 const QString& comment,
                                                                 const QVariantMap& extras) const
{
    AnnotationEntry entry;
    entry.startSample = qMin(startSample, endSample);
    entry.endSample = qMax(startSample, endSample);
    entry.description = description.trimmed().isEmpty() ? defaultAnnotationDescription() : description.trimmed();
    entry.comment = comment.trimmed();

    for (const QString& channelName : channelNames) {
        const QString trimmed = channelName.trimmed();
        if (!trimmed.isEmpty() && !entry.channelNames.contains(trimmed)) {
            entry.channelNames.append(trimmed);
        }
    }

    static const QSet<QString> reservedExtraKeys = {
        QStringLiteral("start_sample"),
        QStringLiteral("end_sample"),
        QStringLiteral("onset"),
        QStringLiteral("duration"),
        QStringLiteral("description"),
        QStringLiteral("label"),
        QStringLiteral("comment"),
        QStringLiteral("ch_names")};

    for (auto it = extras.constBegin(); it != extras.constEnd(); ++it) {
        if (it.key().trimmed().isEmpty() || reservedExtraKeys.contains(it.key())) {
            continue;
        }
        entry.extras.insert(it.key(), it.value());
    }

    if (m_iLastSample > m_iFirstSample) {
        entry.startSample = qBound(m_iFirstSample, entry.startSample, m_iLastSample);
        entry.endSample = qBound(entry.startSample, entry.endSample, m_iLastSample);
    }

    return entry;
}

//=============================================================================================================

double AnnotationModel::sampleToOnsetSeconds(int sample) const
{
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;
    return static_cast<double>(sample - m_iFirstSample) / sfreq;
}

//=============================================================================================================

int AnnotationModel::onsetSecondsToSample(double onsetSeconds) const
{
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;
    return qRound(onsetSeconds * sfreq) + m_iFirstSample;
}

//=============================================================================================================

int AnnotationModel::durationSecondsToSamples(double durationSeconds) const
{
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;
    return qMax(0, qRound(durationSeconds * sfreq));
}

//=============================================================================================================

qint64 AnnotationModel::measurementStartUsecsSinceEpoch(bool* ok) const
{
    const bool valid = m_pFiffInfo && m_pFiffInfo->meas_date[0] >= 0 && m_pFiffInfo->meas_date[1] >= 0;

    if (ok) {
        *ok = valid;
    }

    if (!valid) {
        return 0;
    }

    return static_cast<qint64>(m_pFiffInfo->meas_date[0]) * 1000000LL + static_cast<qint64>(m_pFiffInfo->meas_date[1]);
}

//=============================================================================================================

void AnnotationModel::importEntries(const QVector<AnnotationEntry>& entries)
{
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;

    m_fiffAnnotations.clear();
    for (const AnnotationEntry& e : entries) {
        double onset = static_cast<double>(e.startSample - m_iFirstSample) / sfreq;
        double duration = static_cast<double>(e.endSample - e.startSample) / sfreq;
        if (onset < 0.0)
            onset = 0.0;
        if (duration < 0.0)
            duration = 0.0;

        m_fiffAnnotations.push_back({onset, duration, e.description.toStdString(), stdNames(e.channelNames), e.comment.toStdString(), stdExtras(e.extras)});
    }
}

//=============================================================================================================

QVector<AnnotationModel::AnnotationEntry> AnnotationModel::exportEntries() const
{
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0f) ? m_pFiffInfo->sfreq : 1.0;

    QVector<AnnotationEntry> entries;
    entries.reserve(std::ssize(m_fiffAnnotations));

    for (int i = 0; i < std::ssize(m_fiffAnnotations); ++i) {
        const FiffAnnotation& annot = m_fiffAnnotations[i];
        AnnotationEntry e;
        e.startSample = m_iFirstSample + static_cast<int>(qRound(annot.onset * sfreq));
        e.endSample = e.startSample + static_cast<int>(qRound(annot.duration * sfreq));
        e.description = qText(annot.description);
        e.channelNames = qNames(annot.ch_names);
        e.comment = qText(annot.comment);
        e.extras = qExtras(annot.extras);
        entries.append(e);
    }

    return entries;
}

//=============================================================================================================

void AnnotationModel::sortEntries()
{
    std::sort(m_fiffAnnotations.begin(), m_fiffAnnotations.end(),
              [](const FiffAnnotation& left, const FiffAnnotation& right) {
                  if (qAbs(left.onset - right.onset) > 1e-12) {
                      return left.onset < right.onset;
                  }
                  if (qAbs(left.duration - right.duration) > 1e-12) {
                      return left.duration < right.duration;
                  }
                  return left.description < right.description;
              });
}

//=============================================================================================================

QColor AnnotationModel::colorForLabel(const QString& description) const
{
    const QString normalizedDescription = description.trimmed().toUpper();

    if (normalizedDescription.startsWith(QStringLiteral("BAD"))) {
        return QColor(210, 60, 60, 90);
    }

    if (normalizedDescription.startsWith(QStringLiteral("EDGE"))) {
        return QColor(230, 160, 40, 90);
    }

    return QColor(60, 130, 220, 90);
}

//=============================================================================================================

void AnnotationModel::notifyAnnotationsChanged()
{
    emit annotationsChanged();
}
