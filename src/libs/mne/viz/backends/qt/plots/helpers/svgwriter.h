//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     svgwriter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    A QPaintDevice that writes what is painted on it as an SVG 1.1 document.
 *
 * SvgWriter replaces QSvgGenerator so that the vector export of widgets and
 * graphics scenes needs Qt Gui only. Paths, lines, rectangles, ellipses,
 * polylines, polygons, text and images are written as SVG elements; pens map to
 * stroke attributes, solid brushes to fill attributes, the painter's world
 * transform to a transform attribute and clipping to clip paths. Gradient and
 * pattern brushes are rasterised by QPainter and embedded as PNG images. The
 * document is complete once QPainter::end() returns.
 */

#ifndef SVGWRITER_H
#define SVGWRITER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPaintDevice>

class QIODevice;

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

class SvgPaintEngine;

//=============================================================================================================
/**
 * Paint device that writes an SVG 1.1 document.
 *
 * Paint on it with a single QPainter; render widgets with QWidget::render(QPainter*) so that all children
 * share that painter and end up in one document.
 *
 * @snippet ex_disp_plots/main.cpp svg_export
 */
class VIZQTSHARED_EXPORT SvgWriter : public QPaintDevice
{
public:
    //=========================================================================================================
    /**
     * Constructs a writer with neither a file name nor an output device.
     */
    SvgWriter();

    //=========================================================================================================
    /**
     * Destroys the writer.
     */
    ~SvgWriter() override;

    SvgWriter(const SvgWriter&) = delete;
    SvgWriter& operator=(const SvgWriter&) = delete;

    //=========================================================================================================
    /**
     * Writes the document to the file @p fileName; clears a previously set output device.
     *
     * @param[in] fileName   Path of the SVG file, UTF-8.
     */
    void setFileName(const std::string& fileName);

    //=========================================================================================================
    /**
     * Returns the file the document is written to, or an empty string when an output device is used.
     *
     * @return The file name.
     */
    std::string fileName() const;

    //=========================================================================================================
    /**
     * Writes the document to @p device, which stays owned by the caller; clears a previously set file name.
     * The device is opened for writing when it is not open yet.
     *
     * @param[in] device     The output device.
     */
    void setOutputDevice(QIODevice* device);

    //=========================================================================================================
    /**
     * Returns the output device, or nullptr when a file name is used.
     *
     * @return The output device.
     */
    QIODevice* outputDevice() const;

    //=========================================================================================================
    /**
     * Sets the size of the document in pixels; it is the size of the paint device.
     *
     * @param[in] size       The size.
     */
    void setSize(const QSize& size);

    //=========================================================================================================
    /**
     * Returns the size of the document; the size of the view box when no size is set.
     *
     * @return The size.
     */
    QSize size() const;

    //=========================================================================================================
    /**
     * Sets the part of the painting coordinate system that is shown; (0, 0, size) by default.
     *
     * @param[in] viewBox    The view box.
     */
    void setViewBox(const QRectF& viewBox);

    //=========================================================================================================
    /**
     * Sets the part of the painting coordinate system that is shown; (0, 0, size) by default.
     *
     * @param[in] viewBox    The view box.
     */
    void setViewBox(const QRect& viewBox);

    //=========================================================================================================
    /**
     * Returns the view box.
     *
     * @return The view box set, or (0, 0, size) when none is set.
     */
    QRectF viewBox() const;

    //=========================================================================================================
    /**
     * Sets the text of the document's title element.
     *
     * @param[in] title      The title; no title element is written when it is empty.
     */
    void setTitle(const std::string& title);

    //=========================================================================================================
    /**
     * Returns the title.
     *
     * @return The title.
     */
    std::string title() const;

    //=========================================================================================================
    /**
     * Sets the text of the document's desc element.
     *
     * @param[in] description    The description; no desc element is written when it is empty.
     */
    void setDescription(const std::string& description);

    //=========================================================================================================
    /**
     * Returns the description.
     *
     * @return The description.
     */
    std::string description() const;

    //=========================================================================================================
    /**
     * Sets the resolution the device reports, which converts font point sizes to pixels; 72 dpi by default.
     *
     * @param[in] dpi        Dots per inch.
     */
    void setResolution(int dpi);

    //=========================================================================================================
    /**
     * Returns the resolution.
     *
     * @return Dots per inch.
     */
    int resolution() const;

    //=========================================================================================================
    /**
     * Returns the paint engine that writes the SVG elements.
     *
     * @return The paint engine, owned by the writer.
     */
    QPaintEngine* paintEngine() const override;

protected:
    //=========================================================================================================
    /**
     * Returns the device metric @p metric.
     *
     * @param[in] metric     The metric.
     *
     * @return Its value.
     */
    int metric(PaintDeviceMetric metric) const override;

private:
    friend class SvgPaintEngine;

    std::string m_fileName;                    /**< File the document is written to, UTF-8. */
    QIODevice* m_pDevice = nullptr;            /**< Output device, owned by the caller. */
    QSize m_size;                              /**< Document size in pixels. */
    QRectF m_viewBox;                          /**< Shown part of the painting coordinate system. */
    std::string m_title;                       /**< Text of the title element. */
    std::string m_description;                 /**< Text of the desc element. */
    int m_resolution = 72;                     /**< Reported dots per inch. */
    std::unique_ptr<SvgPaintEngine> m_pEngine; /**< The engine writing the document. */
};

} // namespace mne::viz::qt

#endif // SVGWRITER_H
