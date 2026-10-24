//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     figureexporter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Write the figures of a program as captioned PNG files when MNE_CPP_FIGURE_DIR is set.
 *
 * FigureExporter is the MNE-CPP counterpart of a matplotlib figure scraper: a
 * program calls save() for every figure it wants to show, and nothing happens
 * unless the environment variable @c MNE_CPP_FIGURE_DIR names a directory. When
 * it does, the widget is rendered offscreen at twice its logical size and written
 * as @c NN_name.png, @c NN counting the figures of the process so their order
 * is kept, with the caption stored in the PNG's @c Description text chunk. The
 * examples gallery of the website is produced this way.
 */

#ifndef FIGUREEXPORTER_H
#define FIGUREEXPORTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSize>
#include <QString>

class QImage;
class QWidget;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * Writes captioned figures to @c MNE_CPP_FIGURE_DIR.
 *
 * @snippet ex_disp_plots/main.cpp figure_export
 */
class DISPSHARED_EXPORT FigureExporter
{
public:
    //=========================================================================================================
    /**
     * Returns the figure directory, @c MNE_CPP_FIGURE_DIR, or an empty string when figures are not written.
     */
    static QString directory();

    //=========================================================================================================
    /**
     * Renders @p widget offscreen at @p size and writes it as the next figure.
     *
     * @param[in] widget     The widget to render; it is resized and never shown on screen.
     * @param[in] name       File name stem; characters other than letters, digits, '-' and '_' become '_'.
     * @param[in] caption    Caption shown below the figure.
     * @param[in] size       Logical size of the figure in pixels.
     *
     * @return The path of the written PNG, or an empty string when figures are not written or writing failed.
     */
    static QString save(QWidget& widget,
                        const QString& name,
                        const QString& caption,
                        const QSize& size = QSize(800, 450));

    //=========================================================================================================
    /**
     * Writes @p image as the next figure.
     *
     * @param[in] image      The figure.
     * @param[in] name       File name stem; characters other than letters, digits, '-' and '_' become '_'.
     * @param[in] caption    Caption shown below the figure.
     *
     * @return The path of the written PNG, or an empty string when figures are not written or writing failed.
     */
    static QString save(const QImage& image,
                        const QString& name,
                        const QString& caption);
};

} // namespace DISPLIB

#endif // FIGUREEXPORTER_H
