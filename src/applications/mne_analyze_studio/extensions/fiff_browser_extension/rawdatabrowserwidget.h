//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rawdatabrowserwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  2.1.0
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
