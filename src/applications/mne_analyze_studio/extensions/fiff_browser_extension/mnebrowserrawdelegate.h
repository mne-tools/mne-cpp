//=============================================================================================================
/**
 * @file     mnebrowserrawdelegate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
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
