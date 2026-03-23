//=============================================================================================================
/**
 * @file     mnebrowserrawdelegate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the waveform delegate for the embedded raw browser.
 */

#include "mnebrowserrawdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

using namespace MNEANALYZESTUDIO;

MneBrowseRawDelegate::MneBrowseRawDelegate(QObject* parent)
: QAbstractItemDelegate(parent)
, m_plotHeight(40)
, m_gridLineCount(6)
, m_dx(1.0)
{
    m_scaleMap["MEG_grad"] = 400 * 1e-15 * 100;
    m_scaleMap["MEG_mag"] = 1.2 * 1e-12;
    m_scaleMap["MEG_EEG"] = 30 * 1e-06;
    m_scaleMap["MEG_EOG"] = 150 * 1e-06;
    m_scaleMap["MEG_EMG"] = 1 * 1e-03;
    m_scaleMap["MEG_ECG"] = 1 * 1e-03;
    m_scaleMap["MEG_MISC"] = 1.0;
    m_scaleMap["MEG_STIM"] = 5.0;
}

void MneBrowseRawDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    painter->save();

    if(index.column() == 0) {
        QApplication::style()->drawItemText(painter,
                                            option.rect.adjusted(6, 0, -6, 0),
                                            Qt::AlignVCenter | Qt::AlignLeft,
                                            option.palette,
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
    QPen dataPen(option.palette.highlight().color().darker(120));
    dataPen.setWidthF(1.0);
    painter->setPen(dataPen);
    painter->drawPath(plotPath);

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
    const double scaleY = option.rect.height() / (2.0 * maxValue);
    const double yBase = option.rect.center().y();

    bool hasMoved = false;
    double currentX = option.rect.left();

    for(const MNEBROWSE::RowVectorPair& pair : listPairs) {
        for(qint32 sample = 0; sample < pair.second; ++sample) {
            const double value = *(pair.first + sample);
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
    if(!model || row < 0 || row >= model->m_chInfolist.size()) {
        return 1e-9;
    }

    const qint32 kind = model->m_chInfolist.at(row).kind;
    switch(kind) {
        case FIFFV_MEG_CH: {
            const qint32 unit = model->m_pfiffIO->m_qlistRaw[0]->info.chs[row].unit;
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
