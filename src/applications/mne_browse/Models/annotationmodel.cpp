//=============================================================================================================
/**
 * @file     annotationmodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Definition of the AnnotationModel class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationmodel.h"

#include <QBrush>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

#include <algorithm>

using namespace MNEBROWSE;

//=============================================================================================================

AnnotationModel::AnnotationModel(QObject *parent)
: QAbstractTableModel(parent)
, m_pFiffInfo(new FIFFLIB::FiffInfo)
{
}

//=============================================================================================================

AnnotationModel::~AnnotationModel() = default;

//=============================================================================================================

int AnnotationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_annotations.size();
}

//=============================================================================================================

int AnnotationModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 5;
}

//=============================================================================================================

QVariant AnnotationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
        return QVariant();
    }

    if(role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0:
                return QVariant("Start");
            case 1:
                return QVariant("End");
            case 2:
                return QVariant("Start (s)");
            case 3:
                return QVariant("Duration (s)");
            case 4:
                return QVariant("Label");
            default:
                break;
        }
    } else if(orientation == Qt::Vertical) {
        return QString("Annotation %1").arg(section + 1);
    }

    return QVariant();
}

//=============================================================================================================

QVariant AnnotationModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_annotations.size()) {
        return QVariant();
    }

    const AnnotationEntry& entry = m_annotations.at(index.row());

    if(role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
    }

    if(role == Qt::BackgroundRole) {
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        QColor color = colorForLabel(entry.label);
        color.setAlpha(40);
        brush.setColor(color);
        return QVariant(brush);
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    const int relStart = entry.startSample - m_iFirstSample;
    const int relEnd = entry.endSample - m_iFirstSample;
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0) ? m_pFiffInfo->sfreq : 1.0;

    switch(index.column()) {
        case 0:
            return QVariant(relStart);
        case 1:
            return QVariant(relEnd);
        case 2:
            return QVariant(static_cast<double>(relStart) / sfreq);
        case 3:
            return QVariant(static_cast<double>(entry.endSample - entry.startSample + 1) / sfreq);
        case 4:
            return QVariant(entry.label);
        default:
            return QVariant();
    }
}

//=============================================================================================================

Qt::ItemFlags AnnotationModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

//=============================================================================================================

bool AnnotationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role != Qt::EditRole || !index.isValid() || index.row() < 0 || index.row() >= m_annotations.size()) {
        return false;
    }

    AnnotationEntry& entry = m_annotations[index.row()];
    const double sfreq = (m_pFiffInfo && m_pFiffInfo->sfreq > 0.0) ? m_pFiffInfo->sfreq : 1.0;

    switch(index.column()) {
        case 0:
            entry.startSample = value.toInt() + m_iFirstSample;
            break;
        case 1:
            entry.endSample = value.toInt() + m_iFirstSample;
            break;
        case 2:
            entry.startSample = qRound(value.toDouble() * sfreq) + m_iFirstSample;
            break;
        case 3:
            entry.endSample = entry.startSample + qMax(0, qRound(value.toDouble() * sfreq) - 1);
            break;
        case 4:
            entry.label = value.toString().trimmed();
            break;
        default:
            return false;
    }

    entry = normalizeEntry(entry.startSample, entry.endSample, entry.label);

    beginResetModel();
    sortEntries();
    endResetModel();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if(position < 0 || rows <= 0 || position + rows > m_annotations.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    for(int i = 0; i < rows; ++i) {
        m_annotations.removeAt(position);
    }
    endRemoveRows();

    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

void AnnotationModel::setFiffInfo(const FIFFLIB::FiffInfo::SPtr& pFiffInfo)
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

int AnnotationModel::addAnnotation(int startSample, int endSample, const QString& label)
{
    AnnotationEntry entry = normalizeEntry(startSample, endSample, label);

    beginResetModel();
    m_annotations.append(entry);
    sortEntries();
    endResetModel();

    notifyAnnotationsChanged();

    for(int row = 0; row < m_annotations.size(); ++row) {
        if(m_annotations.at(row).startSample == entry.startSample
           && m_annotations.at(row).endSample == entry.endSample
           && m_annotations.at(row).label == entry.label) {
            return row;
        }
    }

    return -1;
}

//=============================================================================================================

QPair<int, int> AnnotationModel::getSampleRange(int row) const
{
    if(row < 0 || row >= m_annotations.size()) {
        return QPair<int, int>(0, 0);
    }

    const AnnotationEntry& entry = m_annotations.at(row);
    return QPair<int, int>(entry.startSample, entry.endSample);
}

//=============================================================================================================

QVector<AnnotationSpanData> AnnotationModel::getAnnotationSpans() const
{
    QVector<AnnotationSpanData> spans;
    spans.reserve(m_annotations.size());

    for(const AnnotationEntry& entry : m_annotations) {
        AnnotationSpanData span;
        span.startSample = entry.startSample;
        span.endSample = entry.endSample;
        span.color = colorForLabel(entry.label);
        span.label = entry.label;
        spans.append(span);
    }

    return spans;
}

//=============================================================================================================

bool AnnotationModel::loadAnnotationData(QFile& qFile)
{
    if(!qFile.open(QIODevice::ReadOnly)) {
        qWarning() << "AnnotationModel: could not open annotation file" << qFile.fileName();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(qFile.readAll(), &parseError);
    qFile.close();

    if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "AnnotationModel: invalid annotation json" << qFile.fileName() << parseError.errorString();
        return false;
    }

    const QJsonArray annotationArray = document.object().value(QStringLiteral("annotations")).toArray();

    beginResetModel();
    m_annotations.clear();

    for(const QJsonValue& value : annotationArray) {
        const QJsonObject object = value.toObject();
        if(object.isEmpty()) {
            continue;
        }

        const int startSample = object.value(QStringLiteral("start_sample")).toInt();
        const int endSample = object.value(QStringLiteral("end_sample")).toInt(startSample);
        const QString label = object.value(QStringLiteral("label")).toString(QStringLiteral("BAD_manual"));

        m_annotations.append(normalizeEntry(startSample, endSample, label));
    }

    sortEntries();
    endResetModel();

    m_bFileLoaded = true;
    notifyAnnotationsChanged();
    return true;
}

//=============================================================================================================

bool AnnotationModel::saveAnnotationData(QFile& qFile) const
{
    QFileInfo fileInfo(qFile);
    if(fileInfo.dir().exists() == false) {
        qWarning() << "AnnotationModel: annotation directory does not exist" << fileInfo.absolutePath();
        return false;
    }

    if(!qFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "AnnotationModel: could not write annotation file" << qFile.fileName();
        return false;
    }

    QJsonArray annotationArray;
    for(const AnnotationEntry& entry : m_annotations) {
        QJsonObject object;
        object.insert(QStringLiteral("start_sample"), entry.startSample);
        object.insert(QStringLiteral("end_sample"), entry.endSample);
        object.insert(QStringLiteral("label"), entry.label);
        annotationArray.append(object);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("version"), 1);
    rootObject.insert(QStringLiteral("annotations"), annotationArray);

    const QJsonDocument document(rootObject);
    const qint64 bytesWritten = qFile.write(document.toJson(QJsonDocument::Indented));
    qFile.close();

    return bytesWritten >= 0;
}

//=============================================================================================================

void AnnotationModel::clearModel()
{
    beginResetModel();
    m_annotations.clear();
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

AnnotationModel::AnnotationEntry AnnotationModel::normalizeEntry(int startSample,
                                                                 int endSample,
                                                                 const QString& label) const
{
    AnnotationEntry entry;
    entry.startSample = qMin(startSample, endSample);
    entry.endSample = qMax(startSample, endSample);
    entry.label = label.trimmed().isEmpty() ? QStringLiteral("BAD_manual") : label.trimmed();

    if(m_iLastSample > m_iFirstSample) {
        entry.startSample = qBound(m_iFirstSample, entry.startSample, m_iLastSample);
        entry.endSample = qBound(entry.startSample, entry.endSample, m_iLastSample);
    }

    return entry;
}

//=============================================================================================================

void AnnotationModel::sortEntries()
{
    std::sort(m_annotations.begin(),
              m_annotations.end(),
              [](const AnnotationEntry& left, const AnnotationEntry& right) {
                  if(left.startSample != right.startSample) {
                      return left.startSample < right.startSample;
                  }

                  if(left.endSample != right.endSample) {
                      return left.endSample < right.endSample;
                  }

                  return left.label < right.label;
              });
}

//=============================================================================================================

QColor AnnotationModel::colorForLabel(const QString& label) const
{
    const QString normalizedLabel = label.trimmed().toUpper();

    if(normalizedLabel.startsWith(QStringLiteral("BAD"))) {
        return QColor(210, 60, 60, 90);
    }

    if(normalizedLabel.startsWith(QStringLiteral("EDGE"))) {
        return QColor(230, 160, 40, 90);
    }

    return QColor(60, 130, 220, 90);
}

//=============================================================================================================

void AnnotationModel::notifyAnnotationsChanged()
{
    emit annotationsChanged();
}
