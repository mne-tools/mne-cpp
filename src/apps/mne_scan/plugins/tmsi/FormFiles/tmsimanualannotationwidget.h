//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsimanualannotationwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2014
 * @brief    Contains the declaration of the TMSIManualAnnotation class.
 */

#ifndef TMSIMANUALANNOTATIONWIDGET_H
#define TMSIMANUALANNOTATIONWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

namespace Ui
{
class TMSIManualAnnotationWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSI;

//=============================================================================================================
/**
 * DECLARE CLASS TMSIManualAnnotationWidget
 *
 * @brief The TMSIManualAnnotationWidget class provides a widget/window for manually annotating the trigger, which the subject pressed during the session.
 */
class TMSIManualAnnotationWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a TMSIManualAnnotationWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new TMSIManualAnnotationWidget becomes a window. If parent is another widget, TMSIManualAnnotationWidget becomes a child window inside parent. TMSIManualAnnotationWidget is deleted when its parent is deleted.
     * @param[in] pTMSI a pointer to the corresponding ECGSimulator.
     */
    TMSIManualAnnotationWidget(TMSI* pTMSI, QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the TMSIManualAnnotationWidget.
     * All TMSIManualAnnotationWidget's children are deleted first. The application exits if TMSIManualAnnotationWidget is the main widget.
     */
    ~TMSIManualAnnotationWidget() override;

    //=========================================================================================================
    /**
     * Initializes the Connector's GUI properties.
     *
     */
    void initGui();

private:
    TMSI* m_pTMSI; /**< a pointer to corresponding TMSI.*/

    std::unique_ptr<Ui::TMSIManualAnnotationWidget> ui; /**< the user interface for the TMSIManualAnnotationWidget.*/

    void keyPressEvent(QKeyEvent* event) override;
};
} // NAMESPACE

#endif // TMSIMANUALANNOTATIONWIDGET_H
