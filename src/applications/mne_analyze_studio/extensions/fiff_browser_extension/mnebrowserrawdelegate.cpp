//=============================================================================================================
/**
 * @file     mnebrowserrawdelegate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @brief    Implements the waveform delegate for the FIFF browser extension widget.
 */

#include "mnebrowserrawdelegate.h"

#include <fiff/fiff_constants.h>

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

using namespace MNEANALYZESTUDIO;

MneBrowseRawDelegate::MneBrowseRawDelegate(QObject* parent)
: QAbstractItemDelegate(parent)
, m_plotHeight(40)
, m_gridLineCount(6)
, m_dx(1.0)
, m_amplitudeScale(1.0)
, m_removeDC(false)
, m_eventModel(nullptr)
{
    m_scaleMap["MEG_grad"] = 400 * 1e-15 * 100;
    m_scaleMap["MEG_mag"] = 1.2 * 1e-12;
    m_scaleMap["MEG_EEG"] = 30 * 1e-06;
    m_scaleMap["MEG_EOG"] = 150 * 1e-06;
    m_scaleMap["MEG_EMG"] = 1 * 1e-03;
    m_scaleMap["MEG_ECG"] = 1 * 1e-03;
    m_scaleMap["MEG_MISC"] = 1.0;
    m_scaleMap["MEG_STIM"] = 5.0;

    m_colorMap[FIFFV_MEG_CH]     = QColor(0x27, 0x6F, 0xBF); // steel blue — MEG (mag resolved per-unit below)
    m_colorMap[FIFFV_REF_MEG_CH] = QColor(0x5B, 0xA4, 0xCF); // lighter blue — reference MEG
    m_colorMap[FIFFV_EEG_CH]     = QColor(0x2A, 0x9D, 0x8F); // teal — EEG
    m_colorMap[FIFFV_STIM_CH]    = QColor(0xE7, 0x6F, 0x51); // orange-red — stimulus
    m_colorMap[FIFFV_EOG_CH]     = QColor(0xE9, 0xC4, 0x6A); // amber — EOG
    m_colorMap[FIFFV_ECG_CH]     = QColor(0xE6, 0x3B, 0x5A); // crimson — ECG
    m_colorMap[FIFFV_EMG_CH]     = QColor(0x8E, 0x44, 0xAD); // violet — EMG
    m_colorMap[FIFFV_MISC_CH]    = QColor(0x95, 0xA5, 0xA6); // gray — misc
}

void MneBrowseRawDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    painter->save();

    if(index.column() == 0) {
        const auto* rawModelLabel = qobject_cast<const MNEBROWSE::RawModel*>(index.model());
        const bool isBad = rawModelLabel && rawModelLabel->fiffInfo()
                           && rawModelLabel->fiffInfo()->bads.contains(
                               rawModelLabel->channelInfoList().at(index.row()).ch_name);
        QColor labelColor = isBad ? QColor(0xCC, 0x44, 0x44) : colorForChannel(rawModelLabel, index.row());
        if(isBad) {
            labelColor.setAlphaF(0.7);
        }
        QPalette labelPalette = option.palette;
        labelPalette.setColor(QPalette::Text, labelColor);
        QFont labelFont = option.font;
        if(isBad) {
            labelFont.setItalic(true);
        }
        painter->setFont(labelFont);
        QApplication::style()->drawItemText(painter,
                                            option.rect.adjusted(6, 0, -6, 0),
                                            Qt::AlignVCenter | Qt::AlignLeft,
                                            labelPalette,
                                            true,
                                            index.data(Qt::DisplayRole).toString(),
                                            QPalette::Text);
        painter->restore();
        return;
    }

    if(index.column() != 1) {
        painter->restore();
        return;
    }

    QVariant backgroundVariant = index.model()->data(index, Qt::BackgroundRole);
    if(backgroundVariant.canConvert<QBrush>()) {
        painter->fillRect(option.rect, qvariant_cast<QBrush>(backgroundVariant));
    } else {
        painter->fillRect(option.rect, option.palette.base());
    }

    QList<MNEBROWSE::RowVectorPair> listPairs = index.model()->data(index, Qt::DisplayRole).value<QList<MNEBROWSE::RowVectorPair>>();
    if(listPairs.isEmpty()) {
        painter->restore();
        return;
    }

    QPainterPath gridPath(QPointF(option.rect.left(), option.rect.top()));
    createGridPath(gridPath, option, listPairs);
    QPen gridPen(option.palette.mid().color());
    gridPen.setStyle(Qt::DotLine);
    gridPen.setWidthF(0.5);
    painter->setPen(gridPen);
    painter->drawPath(gridPath);

    QPainterPath plotPath(QPointF(option.rect.left(), option.rect.center().y()));
    createPlotPath(index, option, plotPath, listPairs);
    const auto* rawModel = qobject_cast<const MNEBROWSE::RawModel*>(index.model());
    const bool traceIsBad = rawModel && rawModel->fiffInfo()
                            && rawModel->fiffInfo()->bads.contains(
                                rawModel->channelInfoList().at(index.row()).ch_name);
    QColor traceColor = traceIsBad ? QColor(0xCC, 0x44, 0x44) : colorForChannel(rawModel, index.row());
    if(traceIsBad) {
        traceColor.setAlphaF(0.45);
    }
    QPen dataPen(traceColor);
    dataPen.setWidthF(1.0);
    painter->setPen(dataPen);
    painter->drawPath(plotPath);

    if(m_eventModel && m_eventModel->rowCount() > 0) {
        drawEvents(index, option, painter, rawModel);
    }

    painter->restore();
}

QSize MneBrowseRawDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if(index.column() == 0) {
        return QSize(180, m_plotHeight);
    }

    if(index.column() == 1) {
        const auto* rawModel = qobject_cast<const MNEBROWSE::RawModel*>(index.model());
        if(rawModel) {
            const qint32 sampleCount = rawModel->lastSample() - rawModel->firstSample();
            return QSize(static_cast<int>(sampleCount * m_dx), m_plotHeight);
        }
    }

    Q_UNUSED(option)
    return QSize(120, m_plotHeight);
}

int MneBrowseRawDelegate::plotHeight() const
{
    return m_plotHeight;
}

double MneBrowseRawDelegate::pixelsPerSample() const
{
    return m_dx;
}

void MneBrowseRawDelegate::setPixelsPerSample(double pixelsPerSample)
{
    m_dx = qMax(0.1, pixelsPerSample);
}

double MneBrowseRawDelegate::amplitudeScale() const
{
    return m_amplitudeScale;
}

void MneBrowseRawDelegate::setAmplitudeScale(double scale)
{
    m_amplitudeScale = qBound(0.01, scale, 1000.0);
}

bool MneBrowseRawDelegate::removeDC() const
{
    return m_removeDC;
}

void MneBrowseRawDelegate::setRemoveDC(bool remove)
{
    m_removeDC = remove;
}

void MneBrowseRawDelegate::setEventModel(MNEBROWSE::EventModel* eventModel)
{
    m_eventModel = eventModel;
}

void MneBrowseRawDelegate::createGridPath(QPainterPath& path,
                                          const QStyleOptionViewItem& option,
                                          const QList<MNEBROWSE::RowVectorPair>& listPairs) const
{
    const double distance = static_cast<double>(option.rect.height()) / m_gridLineCount;
    QPointF lineStart(option.rect.left(), option.rect.top());
    QPointF lineEnd(option.rect.left() + listPairs.first().second * listPairs.size() * m_dx, option.rect.top());

    for(int i = 1; i < m_gridLineCount; ++i) {
        lineStart.setY(option.rect.top() + i * distance);
        lineEnd.setY(lineStart.y());
        path.moveTo(lineStart);
        path.lineTo(lineEnd);
    }
}

void MneBrowseRawDelegate::createPlotPath(const QModelIndex& index,
                                          const QStyleOptionViewItem& option,
                                          QPainterPath& path,
                                          const QList<MNEBROWSE::RowVectorPair>& listPairs) const
{
    const auto* rawModel = qobject_cast<const MNEBROWSE::RawModel*>(index.model());
    if(!rawModel) {
        return;
    }

    const double maxValue = scaleForChannel(rawModel, index.row());
    const double scaleY = option.rect.height() / (2.0 * maxValue) * m_amplitudeScale;
    const double yBase = option.rect.center().y();

    double dcOffset = 0.0;
    if(m_removeDC && rawModel) {
        const QModelIndex meanIndex = rawModel->index(index.row(), 1);
        const QVariant meanVariant = rawModel->data(meanIndex, MNEBROWSE::RawModelRoles::GetChannelMean);
        if(meanVariant.isValid()) {
            dcOffset = meanVariant.toDouble();
        }
    }

    bool hasMoved = false;
    double currentX = option.rect.left();

    for(const MNEBROWSE::RowVectorPair& pair : listPairs) {
        for(qint32 sample = 0; sample < pair.second; ++sample) {
            const double value = *(pair.first + sample) - dcOffset;
            const QPointF samplePoint(currentX, yBase - value * scaleY);
            if(!hasMoved) {
                path.moveTo(samplePoint);
                hasMoved = true;
            } else {
                path.lineTo(samplePoint);
            }
            currentX += m_dx;
        }
    }
}

double MneBrowseRawDelegate::scaleForChannel(const MNEBROWSE::RawModel* model, int row) const
{
    if(!model || row < 0 || row >= model->channelInfoList().size()) {
        return 1e-9;
    }

    const qint32 kind = model->channelInfoList().at(row).kind;
    switch(kind) {
        case FIFFV_MEG_CH: {
            const qint32 unit = model->channelUnit(row);
            return unit == FIFF_UNIT_T_M ? m_scaleMap["MEG_grad"] : m_scaleMap["MEG_mag"];
        }
        case FIFFV_EEG_CH:
            return m_scaleMap["MEG_EEG"];
        case FIFFV_EOG_CH:
            return m_scaleMap["MEG_EOG"];
        case FIFFV_STIM_CH:
            return m_scaleMap["MEG_STIM"];
        case FIFFV_EMG_CH:
            return m_scaleMap["MEG_EMG"];
        case FIFFV_MISC_CH:
            return m_scaleMap["MEG_MISC"];
        default:
            return 1e-9;
    }
}

void MneBrowseRawDelegate::drawEvents(const QModelIndex& index,
                                      const QStyleOptionViewItem& option,
                                      QPainter* painter,
                                      const MNEBROWSE::RawModel* rawModel) const
{
    if(!rawModel) {
        return;
    }

    // EventModel::data(col 0) returns sample offset relative to firstSample
    const qint32 totalSamples = rawModel->lastSample() - rawModel->firstSample();

    const QMap<int, QColor> eventColors = m_eventModel->getEventTypeColors();

    QPen eventPen;
    eventPen.setWidth(2);

    for(int i = 0; i < m_eventModel->rowCount(); ++i) {
        // relativeSample is already (absoluteSample - firstSample)
        const int relativeSample = m_eventModel->data(m_eventModel->index(i, 0)).toInt();
        if(relativeSample < 0 || relativeSample > totalSamples) {
            continue;
        }

        const int type = m_eventModel->data(m_eventModel->index(i, 2)).toInt();
        QColor color = eventColors.value(type, QColor(0x27, 0xAE, 0x60)); // default green
        color.setAlpha(160);
        eventPen.setColor(color);
        painter->setPen(eventPen);

        const int xPos = option.rect.left()
                         + static_cast<int>(relativeSample * m_dx);
        painter->drawLine(xPos, option.rect.top(), xPos, option.rect.bottom());
    }
}

QColor MneBrowseRawDelegate::colorForChannel(const MNEBROWSE::RawModel* model, int row) const
{
    if(!model || row < 0 || row >= model->channelInfoList().size()) {
        return m_colorMap.value(FIFFV_MISC_CH, Qt::darkGray);
    }

    const qint32 kind = model->channelInfoList().at(row).kind;

    // MEG magnetometers and gradiometers share FIFFV_MEG_CH but get distinct shades
    if(kind == FIFFV_MEG_CH) {
        const qint32 unit = model->channelUnit(row);
        if(unit == FIFF_UNIT_T_M) {
            return QColor(0x14, 0x99, 0xC6); // cyan-blue — gradiometer
        }
    }

    return m_colorMap.value(kind, Qt::darkGray);
}
