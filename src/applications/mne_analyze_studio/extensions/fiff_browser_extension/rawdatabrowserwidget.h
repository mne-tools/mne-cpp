//=============================================================================================================
/**
 * @file     rawdatabrowserwidget.h
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
 * @brief    Declares the embedded raw FIFF browser widget contributed by the FIFF extension.
 */

#ifndef MNE_ANALYZE_STUDIO_RAWDATABROWSERWIDGET_H
#define MNE_ANALYZE_STUDIO_RAWDATABROWSERWIDGET_H

#include <irawdataview.h>

#include "Models/rawmodel.h"
#include "Models/eventmodel.h"

#include <QWidget>

class QEvent;
class QLabel;
class QFile;
class QPushButton;
class QFrame;
class QTableView;
class QScrollBar;
class QPaintEvent;
class QWheelEvent;

namespace MNEANALYZESTUDIO
{

class MneBrowseRawDelegate;
class TimeRulerWidget;

/**
 * @brief Extension-owned widget that hosts the existing MNE browse raw signal view.
 */
class RawDataBrowserWidget : public QWidget, public IRawDataView
{
    Q_OBJECT

public:
    explicit RawDataBrowserWidget(QWidget* parent = nullptr);

    bool loadFile(const QString& filePath);
    QString filePath() const override;
    QString summaryText() const override;
    QString stateText() const override;
    bool gotoSample(int sample) override;
    bool setZoomPixelsPerSample(double pixelsPerSample) override;
    double pixelsPerSample() const override;
    int cursorSample() const override;

signals:
    void outputMessage(const QString& message);
    void statusMessage(const QString& message);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateScrollConfiguration();
    void updateTimeRuler();
    QString formatSeconds(int sample) const;
    void setMarkerSample(int sample);
    void updateMarkerOverlay();
    void adjustZoom(double factor);
    void adjustAmplitude(double factor);
    QString currentVisibleRangeText() const;
    QString currentCursorText() const;
    void publishBrowserState(bool appendToOutput);

    QString m_filePath;
    QFile* m_file;
    QPushButton* m_zoomOutButton;
    QPushButton* m_zoomResetButton;
    QPushButton* m_zoomInButton;
    QPushButton* m_dcButton;
    QTableView* m_tableView;
    QFrame* m_markerLine;
    TimeRulerWidget* m_timeRuler;
    MNEBROWSE::RawModel* m_rawModel;
    MNEBROWSE::EventModel* m_eventModel;
    MneBrowseRawDelegate* m_delegate;
    int m_markerSample;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_RAWDATABROWSERWIDGET_H
