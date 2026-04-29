//=============================================================================================================
/**
 * @file     videooverlay.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @brief    VideoOverlay class declaration - generic live RGB video texture overlay.
 */

#ifndef VIDEOOVERLAY_H
#define VIDEOOVERLAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QImage>
#include <QVector3D>

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * @brief Camera-facing textured quad rendered at a focus point in the 3-D scene.
 *
 * The overlay displays any live RGB/video frame "as a small cut-out view"
 * on top of the 3-D scene. It is a billboarded square positioned at
 * @p focusPosition with side length @p sizeMeters, textured with the latest
 * video frame.
 *
 * This class is a passive data holder: BrainRenderer owns the GPU
 * resources (vertex buffer, texture, sampler, pipeline) and reads from
 * here each frame.
 */
class DISP3DSHARED_EXPORT VideoOverlay
{
public:
    VideoOverlay() = default;

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /** @return World-space centre of the overlay quad (metres). */
    QVector3D focusPosition() const { return m_focusPosition; }
    void setFocusPosition(const QVector3D &pos) { m_focusPosition = pos; }

    /** @return Side length of the overlay quad (metres). */
    float sizeMeters() const { return m_size; }
    void setSizeMeters(float size) { m_size = size; }

    /** @return Latest video frame (CPU-side). Empty until a frame arrives. */
    const QImage &frame() const { return m_frame; }
    bool hasFrame() const { return !m_frame.isNull(); }

    /** Push a new video frame; bumps the generation counter so the GPU re-uploads. */
    void setFrame(const QImage &image)
    {
        if (image.isNull())
            return;
        m_frame = image;
        ++m_frameGeneration;
    }

    /** Monotonic counter, incremented on every setFrame() call. */
    quint64 frameGeneration() const { return m_frameGeneration; }

    /** @return Overall opacity of the overlay [0..1]. */
    float opacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = opacity; }

private:
    bool m_enabled = false;
    QVector3D m_focusPosition = QVector3D(0.0f, 0.05f, 0.08f); // arbitrary default near the top of the head
    float m_size = 0.06f;        //!< 6 cm wide by default
    float m_opacity = 1.0f;
    QImage m_frame;
    quint64 m_frameGeneration = 0;
};

} // namespace DISP3DLIB

#endif // VIDEOOVERLAY_H
