//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     channeldatamodel.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Circular-buffer Qt model exposing a rolling window of the live FIFF stream as a table.
 *
 * ChannelDataModel is the Qt face of @ref mne::scene::TraceBuffer, which stores the
 * samples of the most recent N seconds per channel and resolves the channels' scales,
 * colours and bad flags. The model describes the channels of a FiffInfo to it (names,
 * types, per-kind scales and colours), keeps the FiffInfo's bad list in step and
 * turns the buffer's change notifications into Qt signals.
 */

#ifndef CHANNELDATAMODEL_H
#define CHANNELDATAMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include <mne/scene/trace_buffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QColor>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief Channel display metadata (read-only from the renderer's perspective).
 */
enum class DetrendMode
{
    None = 0,  /**< No detrending. */
    Mean = 1,  /**< Remove DC offset (mean subtraction). */
    Linear = 2 /**< Remove linear trend (least-squares line). */
};

//=============================================================================================================
/**
 * @brief Channel display metadata (read-only from the renderer's perspective).
 */
struct ChannelDisplayInfo
{
    std::string name;              /**< Channel name for the label. */
    std::string typeLabel;         /**< Short type string: "MEG grad", "MEG mag", "EEG", "EOG", "ECG", "EMG", "STIM", "MISC". */
    QColor color;                  /**< Line colour to use in the GPU renderer. */
    float amplitudeMax;            /**< Amplitude value (physical units) that maps to full row height. */
    bool bad;                      /**< Whether the channel is currently marked bad. */
    bool isVirtualChannel = false; /**< Browser-level derived channel without a direct FIFF row. */
};

//=============================================================================================================
/**
 * @brief Circular-buffer Qt model exposing a rolling window of the live FIFF stream as a table.
 *
 * Wraps a Qt-free @ref mne::scene::TraceBuffer (see buffer()) that stores the
 * samples and resolves the channel descriptions taken from the FiffInfo; emits
 * dataChanged() and metaChanged() when the buffer reports a change.
 */
class DISPSHARED_EXPORT ChannelDataModel : public QObject
{
    Q_OBJECT

public:
    explicit ChannelDataModel(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Initialise with channel metadata from a FiffInfo.
     *
     * @param[in] pFiffInfo  The FiffInfo describing the channels.
     */
    void init(const std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Replace all stored data with the provided matrix (channels × samples).
     * The absolute index of the first column is @p firstSample.
     *
     * @param[in] data         Channels × samples matrix (double precision).
     * @param[in] firstSample  Absolute sample index of column 0.
     */
    void setData(const Eigen::MatrixXd& data, int firstSample = 0);

    //=========================================================================================================
    /**
     * Append new samples to the ring buffer.  Oldest samples are dropped when the
     * internal capacity (maxStoredSamples) is exceeded.
     *
     * @param[in] data  Channels × newSamples matrix.
     */
    void appendData(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * Override the per-channel-type amplitude scale.
     * Key = FIFF channel kind constant (e.g. FIFFV_MEG_CH), value = physical amplitude.
     *
     * @param[in] scaleMap  Map from FIFF kind to amplitude max.
     */
    void setScaleMap(const std::map<int, float>& scaleMap);

    //=============================================================================================================
    /**
     * Override the per-channel-type amplitude scale using channel-type names.
     *
     * @param[in] scaleMap  Map from channel-type name (e.g. "MEG_grad") to amplitude max.
     */
    void setScaleMapFromStrings(const std::map<std::string, double>& scaleMap);

    //=============================================================================================================
    /**
     * Clear all stored sample data without affecting channel metadata.
     * After this call totalSamples() == 0 and firstSample() == 0.
     * Emits dataChanged().
     */
    void clearData();

    //=========================================================================================================
    /**
     * Set extra browser-level virtual channels appended after the FIFF channels.
     *
     * @param[in] virtualChannels  Display metadata for each appended virtual channel.
     */
    void setVirtualChannels(const std::vector<ChannelDisplayInfo>& virtualChannels);

    //=========================================================================================================
    /**
     * Set the signal line colour for all channels.
     * Individual channel colours are derived from this base colour by type.
     *
     * @param[in] color  The base signal colour.
     */
    void setSignalColor(const QColor& color);

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
    bool removeDC() const
    {
        return detrendMode() != DetrendMode::None;
    }

    void setDetrendMode(DetrendMode mode);
    DetrendMode detrendMode() const;

    // ── Accessors (all thread-safe read) ──────────────────────────────

    int channelCount() const;
    int firstSample() const;
    int totalSamples() const;
    /**
     * @return Sampling frequency in Hz; 0 if no FiffInfo attached.
     */
    float sfreq() const;

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
    std::vector<float> decimatedVertices(int channelIdx,
                                         int firstSample,
                                         int lastSample,
                                         int pixelWidth,
                                         int& vboFirstSample) const;

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

    //=========================================================================================================
    /**
     * The Qt-free store behind this model: samples, channel scales, colours and bad flags as a trace source
     * indexed by channel.
     *
     * @return The trace buffer.
     *
     * @since 3.0.0
     */
    const scene::TraceBuffer& buffer() const
    {
        return m_buffer;
    }

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
    std::vector<scene::TraceBuffer::ChannelSpec> channelSpecs() const;

    scene::TraceBuffer m_buffer; // samples, channel scales, colours and bad flags

    mutable std::mutex m_infoLock; // guards m_pFiffInfo and m_virtualDisplayInfo
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo;
    std::vector<ChannelDisplayInfo> m_virtualDisplayInfo;
};

} // namespace mne::disp

#endif // CHANNELDATAMODEL_H
