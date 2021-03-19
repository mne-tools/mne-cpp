//=============================================================================================================
/**
 * @file     spectrumsettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the SpectrumSettingsView Class.
 *
 */

#ifndef SPECTRUMSETTINGSVIEW_H
#define SPECTRUMSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSlider;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS SpectrumSettingsView
 *
 * @brief The SpectrumSettingsView class provides settings for the spectrum estimation
 */
class DISPSHARED_EXPORT SpectrumSettingsView : public AbstractView
{
    Q_OBJECT

public:    
    typedef QSharedPointer<SpectrumSettingsView> SPtr;              /**< Shared pointer type for SpectrumSettingsView. */
    typedef QSharedPointer<const SpectrumSettingsView> ConstSPtr;   /**< Const shared pointer type for SpectrumSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a SpectrumSettingsView which is a child of parent.
     *
     * @param[in] parent    parent of widget.
     */
    SpectrumSettingsView(const QString& sSettingsPath = "",
                         QWidget *parent = Q_NULLPTR,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the SpectrumSettingsView.
     */
    ~SpectrumSettingsView();

    //=========================================================================================================
    /**
     * Update slider value
     *
     * @param[in] value    slider value.
     */
    void updateValue(qint32 value);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Sets the sampling frequency as well as boundaries and configure the GUI elements accordingly
     *
     * @param[in] fSFreq           The sampling frequency.
     * @param[in] fLowerBound      The lower bound of the spectrum.
     * @param[in] fUpperBound      The upper bound of the spectrum.
     */
    void setBoundaries(float fSFreq,
                       float fLowerBound,
                       float fUpperBound);

    //=========================================================================================================
    /**
     * Returns the lower bound
     *
     * @return Returns the lower bound as a float.
     */
    float getLowerBound();

    //=========================================================================================================
    /**
     * Returns the upper bound
     *
     * @return Returns the upper bound as a float.
     */
    float getUpperBound();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    QPointer<QSlider>   m_pSliderLowerBound;    /**< Lower bound frequency. */
    QPointer<QSlider>   m_pSliderUpperBound;    /**< Upper bound frequency. */

signals:
    //=========================================================================================================
    /**
     * Emitted whenever the settings changed and are ready to be retreived.
     */
    void settingsChanged();
};
} // NAMESPACE

#endif // SPECTRUMSETTINGSVIEW_H
