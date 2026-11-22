//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mnebrowserrawdelegate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  2.1.0
 * @brief    Declares the waveform delegate used by the FIFF browser extension widget.
 */

#ifndef MNE_ANALYZE_STUDIO_MNEBROWSERRAWDELEGATE_H
#define MNE_ANALYZE_STUDIO_MNEBROWSERRAWDELEGATE_H

#include "Models/rawmodel.h"
#include "Models/eventmodel.h"
#include "Utils/types.h"

#include <QAbstractItemDelegate>
#include <QColor>
#include <QMap>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Item delegate that renders MNE browse raw traces inside the studio table view.
 */
class MneBrowseRawDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit MneBrowseRawDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    int plotHeight() const;
    double pixelsPerSample() const;
    void setPixelsPerSample(double pixelsPerSample);
    double amplitudeScale() const;
    void setAmplitudeScale(double scale);
    bool removeDC() const;
    void setRemoveDC(bool remove);
    void setEventModel(MNEBROWSE::EventModel* eventModel);

private:
    void createGridPath(QPainterPath& path,
                        const QStyleOptionViewItem& option,
                        const QList<MNEBROWSE::RowVectorPair>& listPairs) const;
    void createPlotPath(const QModelIndex& index,
                        const QStyleOptionViewItem& option,
                        QPainterPath& path,
                        const QList<MNEBROWSE::RowVectorPair>& listPairs) const;
    void drawEvents(const QModelIndex& index,
                    const QStyleOptionViewItem& option,
                    QPainter* painter,
                    const MNEBROWSE::RawModel* rawModel) const;
    double scaleForChannel(const MNEBROWSE::RawModel* model, int row) const;
    QColor colorForChannel(const MNEBROWSE::RawModel* model, int row) const;

    int m_plotHeight;
    int m_gridLineCount;
    double m_dx;
    double m_amplitudeScale;
    bool m_removeDC;
    MNEBROWSE::EventModel* m_eventModel;
    QMap<QString, double> m_scaleMap;
    QMap<qint32, QColor> m_colorMap;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_MNEBROWSERRAWDELEGATE_H
