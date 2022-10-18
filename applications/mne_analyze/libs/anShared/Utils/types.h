//=============================================================================================================
/**
 * @file     types.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains general application specific types
 *
 */
#ifndef ANSHARED_TYPES_H
#define ANSHARED_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/dipoleFit/ecd.h>
#include <Eigen/Core>

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QColor>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace ANSHAREDLIB
{
    //=========================================================================================================
    /**
     * The following directory paths are only imaginary.
     * They should be used for models that are not stored to the file system yet.
     *
     * Convention: Imaginary paths start with '*', end with '/' and all characters are upper case.
     */
    #define ECD_SET_MODEL_DEFAULT_DIR_PATH  QStringLiteral("*ECDSETMODEL/")

    //=========================================================================================================
    /**
     * The MODEL_TYPE enum lists all available model types.
     * Naming convention: NAMESPACE_CLASSNAME_MODEL
     */
    enum MODEL_TYPE
    {
        ANSHAREDLIB_SURFACE_MODEL,
        ANSHAREDLIB_QENTITYLIST_MODEL,
        ANSHAREDLIB_ECDSET_MODEL,
        ANSHAREDLIB_FIFFRAW_MODEL,
        ANSHAREDLIB_EVENT_MODEL,
        ANSHAREDLIB_AVERAGING_MODEL,
        ANSHAREDLIB_BEMDATA_MODEL,
        ANSHAREDLIB_NOISE_MODEL,
        ANSHAREDLIB_MRICOORD_MODEL,
        ANSHAREDLIB_DIPOLEFIT_MODEL,
        ANSHAREDLIB_FORWARDSOLUTION_MODEL
    };

    //=========================================================================================================
    /**
     * Public enum for all available Event types.
     */
    enum EVENT_TYPE
    {
        PING,                       ///< [NO DATA] Dummy event for testing and debuggin purposes
        PLUGIN_INIT_FINISHED,       ///< [NO DATA] Send when all plugins finished initializing
        STATUS_BAR_MSG,             ///< [QString] Send a message to the status bar (part of gui)
        SELECTED_MODEL_CHANGED,     ///< [QSharedPointer<ANSHAREDLIB::AbstractModel>>] Send whenever the selection changes in the datamanager plugin
        NEW_EVENT_ADDED,            ///< [int] event send whenever the user adds a new event in the rawdataviewer plugin
        EVENTS_UPDATED,             ///< [NO DATA] send when events or events-groups have changed
        TRIGGER_REDRAW,             ///< [NO DATA] send when viewer needs to be updated
        TRIGGER_ACTIVE_CHANGED,     ///< [int] send when the trigger active state was toggled
        TRIGGER_VIEWER_MOVE,        ///< [NO DATA] send when scroll position of viewer needs to be moved
        FILTER_CHANNEL_TYPE_CHANGED,///< [QString] send when the channel type to be filtered changed
        FILTER_ACTIVE_CHANGED,      ///< [bool] send when the filter active state was toggled
        FILTER_DESIGN_CHANGED,      ///< [FilterKernel] send when the designed filter changed
        CHANNEL_SELECTION_ITEMS,    ///< [send when channel selection changes with channel info about graphic items to draw
        SCALING_MAP_CHANGED,        ///< [DISPLIB::SelectionItem*] send when view scaling controls are updated
        VIEW_SETTINGS_CHANGED,      ///< [ANSHAREDLIB::ViewParameters] send to trigger view settings update
        LOADING_START,              ///< [QString] send to start status bar loaidng bar and message
        LOADING_END,                ///< [QString] send to end status bar loading bar and message
        SELECTED_BEM_CHANGED,       ///< [QSharedPointer<ANSHAREDLIB::BemDataModel>] event send whenever the Bem file within the coregistration changed
        NEW_DIGITIZER_ADDED,        ///< [FIFFLIB::FiffDigPointSet] event send whenever new digitizers are loaded
        NEW_FIDUCIALS_ADDED,        ///< [FIFFLIB::FiffDigPointSet] event send whenever new fiducials are loaded
        NEW_TRANS_AVAILABE,         ///< [FIFFLIB::FiffCoordTrans] event send whenever a new head-mri transformation is available
        FID_PICKING_STATUS,         ///< [bool] event send whenever status of fiducial picking has changed
        NEW_FIDUCIAL_PICKED,        ///< [QVector3D] event send whenever a new fiducial was picked
        FIDUCIAL_CHANGED,           ///< [int] event send when fiducial was changed
        SET_DATA3D_TREE_MODEL,      ///< [QSharedPointer<DISP3DLIB::Data3DTreeModel>] send when a new 3D Model is set
        VIEW3D_SETTINGS_CHANGED,    ///< [ANSHAREDLIB::View3DParameters] send to trigger view 3D settings update
        MODEL_REMOVED               ///< [QSharedPointer<ANSHAREDLIB::AbstractModel>>] send to alert plugins when model is being removed
    };

    //=========================================================================================================
    /**
     * Public struct for sending scaling parameters through the event manager
     */
    struct ScalingParameters{
        QList<QString>      m_sViewsToApply;        /**< Which views should apply changes. */
        QMap<qint32, float> m_mScalingMap;          /**< Scaling map. */
    };

    //=========================================================================================================
    /**
     * Public struct for sending 2D view parameters though the event manager
     */
    struct ViewParameters{
        enum ViewSetting{
            signal,
            background,
            zoom,
            window,
            spacer,
            screenshot,
            all
        };

        QList<QString>      m_sViewsToApply;
        ViewSetting         m_sSettingsToApply;
        QColor              m_colorSignal;
        QColor              m_colorBackground;
        double              m_dZoomValue;
        int                 m_iTimeWindow;
        int                 m_iTimeSpacers;
        QString             m_sImageType;

    };

    //=========================================================================================================
    /**
     * Public struct for sending 3D view parameters throug hthe event manager
     */
    struct View3DParameters{
        enum View3DSetting{
            sceneColor,
            rotation,
            coordAxis,
            fullscreen,
            lightColor,
            lightIntensity,
            screenshot
        };

        View3DSetting       m_settingsToApply;
        QColor              m_sceneColor;
        bool                m_bToggleRotation;
        bool                m_bToogleCoordAxis;
        bool                m_bToggleFullscreen;
        QColor              m_lightColor;
        double              m_dLightIntensity;
    };

    //=========================================================================================================
    /**
     * Public struct for sending channel selection parameters through the event manager
     */
//    struct SelectionParameters{
//        QList<QString>     m_sViewsToApply;         /**< Which views should apply changes. */
//        QList<QString>     m_sChannelName;          /**< The channel's name.*/
//        QList<int>         m_iChannelNumber;        /**< The channel number.*/
//        QList<int>         m_iChannelKind;          /**< The channel kind.*/
//        QList<int>         m_iChannelUnit;          /**< The channel unit.*/
//        QList<QPointF>     m_qpChannelPosition;     /**< The channel's 2D position in the scene.*/
//    };
} //NAMESPACE

//Declare structs to be used in QVariant
#ifndef metatype_ANSHAREDLIB_scalingparam
#define metatype_ANSHAREDLIB_scalingparam
Q_DECLARE_METATYPE(ANSHAREDLIB::ScalingParameters);
Q_DECLARE_METATYPE(ANSHAREDLIB::ScalingParameters*);
#endif

#ifndef metatype_ANSHAREDLIB_viewparam
#define metatype_ANSHAREDLIB_viewparam
Q_DECLARE_METATYPE(ANSHAREDLIB::ViewParameters);
Q_DECLARE_METATYPE(ANSHAREDLIB::ViewParameters*);
#endif

#ifndef metatype_ANSHAREDLIB_view3Dparam
#define metatype_ANSHAREDLIB_view3Dparam
Q_DECLARE_METATYPE(ANSHAREDLIB::View3DParameters);
Q_DECLARE_METATYPE(ANSHAREDLIB::View3DParameters*);
#endif

//Q_DECLARE_METATYPE(ANSHAREDLIB::SelectionParameters);
//Q_DECLARE_METATYPE(ANSHAREDLIB::SelectionParameters*);

#endif // TYPES_H
