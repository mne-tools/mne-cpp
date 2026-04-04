//=============================================================================================================
/**
 * @file     channeldatamodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
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
 * @brief    Declaration of the ChannelDataModel class.
 *
 */

#ifndef CHANNELDATAMODEL_H
#define CHANNELDATAMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QColor>
#include <QMap>
#include <QVector>
#include <QReadWriteLock>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief Channel display metadata (read-only from the renderer's perspective).
 */
enum class DetrendMode {
    None   = 0,  /**< No detrending. */
    Mean   = 1,  /**< Remove DC offset (mean subtraction). */
    Linear = 2   /**< Remove linear trend (least-squares line). */
};

//=============================================================================================================
/**
 * @brief Channel display metadata (read-only from the renderer's perspective).
 */
struct ChannelDisplayInfo {
    QString name;           /**< Channel name for the label. */
    QString typeLabel;      /**< Short type string: "MEG grad", "MEG mag", "EEG", "EOG", "ECG", "EMG", "STIM", "MISC". */
    QColor  color;          /**< Line colour to use in the GPU renderer. */
    float   amplitudeMax;   /**< Amplitude value (physical units) that maps to full row height. */
    bool    bad;            /**< Whether the channel is currently marked bad. */
    bool    isVirtualChannel = false; /**< Browser-level derived channel without a direct FIFF row. */
};

//=============================================================================================================
/**
 * @brief ChannelDataModel – lightweight data container for ChannelDataView / ChannelRhiView.
 *
 * Stores per-channel float amplitude data and derives per-channel display metadata
 * (colour, scale, name) from an optional FiffInfo.  The renderer calls
 * decimatedVertices() to obtain a ready-to-upload flat float array (x_offset, y_amp)
 * pairs for the current view window.
 */
class DISPSHARED_EXPORT ChannelDataModel : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<ChannelDataModel>       SPtr;
    typedef QSharedPointer<const ChannelDataModel> ConstSPtr;

    explicit ChannelDataModel(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Initialise with channel metadata from a FiffInfo.
     *
     * @param[in] pFiffInfo  The FiffInfo describing the channels.
     */
    void init(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Replace all stored data with the provided matrix (channels × samples).
     * The absolute index of the first column is @p firstSample.
     *
     * @param[in] data         Channels × samples matrix (double precision).
     * @param[in] firstSample  Absolute sample index of column 0.
     */
    void setData(const Eigen::MatrixXd &data, int firstSample = 0);

    //=========================================================================================================
    /**
     * Append new samples to the ring buffer.  Oldest samples are dropped when the
     * internal capacity (maxStoredSamples) is exceeded.
     *
     * @param[in] data  Channels × newSamples matrix.
     */
    void appendData(const Eigen::MatrixXd &data);

    //=========================================================================================================
    /**
     * Override the per-channel-type amplitude scale.
     * Key = FIFF channel kind constant (e.g. FIFFV_MEG_CH), value = physical amplitude.
     *
     * @param[in] scaleMap  Map from FIFF kind to amplitude max.
     */
    //=========================================================================================================
    /**
     * Clear all stored sample data without affecting channel metadata.
     * After this call totalSamples() == 0 and firstSample() == 0.
     * Emits dataChanged().
     */
    void clearData();

    void setScaleMap(const QMap<qint32, float> &scaleMap);
    void setScaleMapFromStrings(const QMap<QString, double> &scaleMap);

    //=========================================================================================================
    /**
     * Set extra browser-level virtual channels appended after the FIFF channels.
     *
     * @param[in] virtualChannels  Display metadata for each appended virtual channel.
     */
    void setVirtualChannels(const QVector<ChannelDisplayInfo> &virtualChannels);

    //=========================================================================================================
    /**
     * Set the signal line colour for all channels.
     * Individual channel colours are derived from this base colour by type.
     *
     * @param[in] color  The base signal colour.
     */
    void setSignalColor(const QColor &color);

    //=========================================================================================================
    /**
     * Set the maximum number of samples to keep in the ring buffer.
     * When samples are appended beyond this limit, the oldest are dropped.
     * Set to 0 (default) for unlimited storage.
     *
     * @param[in] n  Maximum sample count per channel, or 0 for unlimited.
     */
    void setMaxStoredSamples(int n);

    //=========================================================================================================
    /**
     * Mark/unmark a channel as bad.
     *
     * @param[in] channelIdx  Zero-based channel index.
     * @param[in] bad         True to mark bad.
     */
    void setChannelBad(int channelIdx, bool bad);

    //=========================================================================================================
    /**
     * Enable or disable DC (mean) removal applied on-the-fly during rendering.
     * When enabled each channel's mean amplitude over the requested window is subtracted.
     *
     * @param[in] remove  true = subtract mean, false = raw data.
     */
    void setRemoveDC(bool remove);
    bool removeDC() const { return m_detrendMode != DetrendMode::None; }

    void setDetrendMode(DetrendMode mode);
    DetrendMode detrendMode() const { return m_detrendMode; }

    // ── Accessors (all thread-safe read) ──────────────────────────────

    int     channelCount()  const;
    int     firstSample()   const;
    int     totalSamples()  const;
    float   sfreq()         const; /**< Sampling frequency in Hz; 0 if no FiffInfo attached. */

    //=========================================================================================================
    /**
     * Display metadata for the given channel.
     *
     * @param[in] channelIdx  Zero-based channel index.
     * @return ChannelDisplayInfo struct with name, colour, amplitudeMax, bad flag.
     */
    ChannelDisplayInfo channelInfo(int channelIdx) const;

    //=========================================================================================================
    /**
     * Compute the RMS amplitude of a channel over a sample window.
     * Samples outside the buffer are silently ignored.
     * Capped internally at 1000 samples for rendering-thread safety.
     *
     * @param[in] channelIdx   Zero-based channel index.
     * @param[in] firstSample  Absolute first sample (inclusive).
     * @param[in] lastSample   Absolute last  sample (exclusive).
     * @return RMS in raw (physical) units, or 0 if no data is available.
     */
    float channelRms(int channelIdx, int firstSample, int lastSample) const;

    //=========================================================================================================
    /**
     * Return a flat float array ready for VBO upload.
     * The array contains interleaved (x_offset, y_amplitude) pairs.
     *
     * When samplesPerPixel <= 1 (zoomed in), raw samples are returned.
     * When samplesPerPixel  > 1 (zoomed out), min/max decimation is applied so that
     * at most 2 * pixelWidth vertices are returned.
     *
     * @param[in] channelIdx      Zero-based channel index.
     * @param[in] firstSample     Absolute sample index of the first desired sample.
     * @param[in] lastSample      Absolute sample index (exclusive) of the last desired sample.
     * @param[in] pixelWidth      Destination viewport width in pixels.
     * @param[out] vboFirstSample The absolute sample index stored at VBO vertex 0
     *                            (= firstSample, returned for the UBO uniform).
     * @return Flat float array: [x0, y0,  x1, y1, ...] where x is offset from vboFirstSample.
     */
    QVector<float> decimatedVertices(int   channelIdx,
                                     int   firstSample,
                                     int   lastSample,
                                     int   pixelWidth,
                                     int  &vboFirstSample) const;

    //=========================================================================================================
    /**
     * Return the raw sample value at a specific absolute sample index for the given channel.
     * Returns 0 if the sample is outside the buffer or channel index is out of range.
     *
     * @param[in] channelIdx  Zero-based channel index.
     * @param[in] sample      Absolute sample index.
     * @return Sample value in physical units, or 0 if unavailable.
     */
    float sampleValueAt(int channelIdx, int sample) const;

signals:
    //=========================================================================================================
    /**
     * Emitted whenever stored data changes (new data appended or replaced).
     */
    void dataChanged();

    //=========================================================================================================
    /**
     * Emitted when channel metadata (scale, colour, bad-flag) changes.
     */
    void metaChanged();

private:
    void    rebuildDisplayInfo();
    float   amplitudeMaxForChannel(int ch) const;
    QColor  colorForChannel(int ch) const;
    QString typeLabelForChannel(int ch) const;

    mutable QReadWriteLock                    m_lock;

    QSharedPointer<FIFFLIB::FiffInfo>         m_pFiffInfo;
    QVector<QVector<float>>                   m_channelData;   // [ch][sample]
    int                                       m_firstSample = 0;
    int                                       m_maxStoredSamples = 0; // 0 = unlimited

    QMap<qint32, float>                       m_scaleMap;
    QColor                                    m_signalColor { Qt::darkGreen };

    QVector<ChannelDisplayInfo>               m_virtualDisplayInfo;
    QVector<ChannelDisplayInfo>               m_displayInfo;  // pre-computed, rebuild on meta change
    DetrendMode                                m_detrendMode = DetrendMode::None;
};

} // namespace DISPLIB

#endif // CHANNELDATAMODEL_H
