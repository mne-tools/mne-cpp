//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_buffer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceBuffer, the stored samples and channel descriptions of a raw-data trace view.
 */

#ifndef SCENE_TRACE_BUFFER_H
#define SCENE_TRACE_BUFFER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scene_global.h"
#include "style.h"
#include "trace_geometry.h"
#include "trace_source.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief The samples of a raw-data trace view and the description of its channels, as a TraceSource.
 *
 * Stores the samples of every channel from an absolute first sample on (replaced, or appended to with an
 * optional capacity beyond which the oldest samples are dropped) and resolves the channels the host
 * describes (ChannelSpec) into what the view draws: the amplitude scale from a scale map keyed by the host's
 * channel kinds, the colour from the signal colour when the host gives none, and the bad flags. Stored rows
 * beyond the described channels show as "CH n" of type "MISC". Traces are decimated with decimate_trace()
 * and detrended on the fly. Channels are addressed by their stored index.
 *
 * All accessors are thread-safe; writers notify the listeners after releasing the lock.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT TraceBuffer : public TraceSource
{
public:
    /** What changed, as reported to the listeners. */
    enum class Change
    {
        Data,    /**< The samples or the detrending. */
        Channels /**< The channel descriptions: scale, colour, bad flags. */
    };

    /** Called after a change, outside the lock. */
    using Listener = std::function<void(Change)>;

    /** A channel as the host describes it. */
    struct ChannelSpec
    {
        std::string name;            /**< Name; empty becomes "Virtual n" for virtual channels. */
        std::string type_label;      /**< Type label; empty becomes "MISC" for virtual channels. */
        std::optional<Color> color;  /**< Trace colour; none uses the signal colour. */
        std::vector<int> scale_keys; /**< Keys of the scale map to look up in order. */
        float default_scale = 1.f;   /**< Amplitude scale if no key is in the scale map; <= 0 becomes 1. */
        bool bad = false;            /**< Whether the channel is marked bad. */
        bool is_virtual = false;     /**< A derived channel of the host without a recorded row. */
    };

    /** Supplies the channel descriptions whenever the buffer resolves its channels. */
    using SpecProvider = std::function<std::vector<ChannelSpec>()>;

    //=========================================================================================================
    /**
     * Registers a listener.
     *
     * @param[in] listener   Called after each change.
     *
     * @return The id to remove it with.
     */
    int add_listener(Listener listener);

    //=========================================================================================================
    /**
     * Removes a listener.
     *
     * @param[in] id   Id from add_listener().
     */
    void remove_listener(int id);

    //=========================================================================================================
    /**
     * Sets where the channel descriptions come from. Nothing is resolved until reload_channels() or a change of
     * the scale map or signal colour.
     *
     * @param[in] provider   Supplies the channel descriptions.
     */
    void set_spec_provider(SpecProvider provider);

    //=========================================================================================================
    /**
     * Reads the channel descriptions again and sizes the samples to them.
     *
     * @param[in] clear_data   Drop all samples and restart at sample 0 (notifies Channels only); otherwise keep
     *                         the samples of the remaining channels (notifies Channels, then Data).
     */
    void reload_channels(bool clear_data);

    //=========================================================================================================
    /**
     * Describes the channels, keeping the samples of the remaining ones (a fixed set_spec_provider() and
     * reload_channels(false)).
     *
     * @param[in] channels   The channel descriptions.
     */
    void set_channels(std::vector<ChannelSpec> channels);

    //=========================================================================================================
    /**
     * Replaces all samples.
     *
     * @param[in] data           Channels x samples; empty data is ignored.
     * @param[in] first_sample   Absolute sample of column 0.
     */
    void set_data(const Eigen::MatrixXd& data, int first_sample = 0);

    //=========================================================================================================
    /**
     * Appends samples, dropping the oldest beyond the capacity.
     *
     * @param[in] data   Channels x new samples; empty data is ignored.
     */
    void append_data(const Eigen::MatrixXd& data);

    /** Drops all samples and restarts at sample 0; the channels stay. */
    void clear_data();

    //=========================================================================================================
    /**
     * Limits the stored samples per channel; appending beyond drops the oldest.
     *
     * @param[in] samples   The capacity, 0 (or less) for no limit.
     */
    void set_capacity(int samples);

    //=========================================================================================================
    /**
     * Sets the amplitude scales.
     *
     * @param[in] scales   Amplitude scale per key (see ChannelSpec::scale_keys).
     */
    void set_scale_map(std::map<int, float> scales);

    /** @return The amplitude scales. */
    std::map<int, float> scale_map() const;

    //=========================================================================================================
    /**
     * Sets the colour of channels without their own colour.
     *
     * @param[in] color   The signal colour.
     */
    void set_signal_color(Color color);

    //=========================================================================================================
    /**
     * Marks a channel bad or good until its channels are resolved again.
     *
     * @param[in] index   Channel index.
     * @param[in] bad     Whether it is bad.
     */
    void set_channel_bad(int index, bool bad);

    //=========================================================================================================
    /**
     * Sets the detrending of the traces.
     *
     * @param[in] detrend   Detrending per trace window.
     */
    void set_detrend(Detrend detrend);

    /** @return The detrending of the traces. */
    Detrend detrend() const;

    //=========================================================================================================
    /**
     * Sets the sampling rate the host reports.
     *
     * @param[in] hz   Samples per second.
     */
    void set_sample_rate(float hz);

    /** @return The sampling rate, 0 if unknown. */
    float sample_rate() const;

    //=========================================================================================================
    /**
     * Tells whether a channel is a virtual channel of the host.
     *
     * @param[in] index   Channel index.
     *
     * @return Whether it is virtual; false for an unknown channel.
     */
    bool is_virtual(int index) const;

    //=========================================================================================================
    /**
     * Returns the RMS of a channel over a window, from at most its last 1000 samples.
     *
     * @param[in] index   Channel index.
     * @param[in] first   Absolute first sample.
     * @param[in] last    Absolute end sample (exclusive).
     *
     * @return The RMS in physical units, 0 without data.
     */
    float rms(int index, int first, int last) const;

    // TraceSource; first_sample() is the absolute sample of the first stored sample, kept when no samples are stored
    int channel_count() const override;
    std::optional<Channel> channel(int index) const override;
    float value_at(int index, int sample) const override;
    int first_sample() const override;
    int sample_count() const override;
    TraceVertices vertices(int index, int first, int last, int pixel_width) const override;

private:
    void reload_scales_and_colors();
    void resolve();
    void notify(Change change) const;
    int stored_count() const;

    mutable std::shared_mutex m_lock;

    SpecProvider m_specProvider;
    std::vector<ChannelSpec> m_specs;
    std::vector<Channel> m_channels; // resolved, one per channel_count()
    std::vector<bool> m_virtual;
    std::vector<std::vector<float>> m_data; // [channel][sample]
    int m_firstSample = 0;
    int m_capacity = 0;
    std::map<int, float> m_scales;
    Color m_signalColor{0, 128, 0, 255};
    Detrend m_detrend = Detrend::None;
    float m_sampleRate = 0.f;

    std::vector<std::pair<int, Listener>> m_listeners;
    int m_nextListenerId = 0;
};

} // namespace mne::scene

#endif // SCENE_TRACE_BUFFER_H
