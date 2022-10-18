//=============================================================================================================
/**
 * @file     sourcelocalization.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    SourceLocalization class declaration.
 *
 */

#ifndef MNEANALYZE_SOURCELOCALIZATION_H
#define MNEANALYZE_SOURCELOCALIZATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelocalization_global.h"

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
    class FiffRawViewModel;
    class AveragingDataModel;
    class ForwardSolutionModel;
}

//=============================================================================================================
// DEFINE NAMESPACE SOURCELOCALIZATIONPLUGIN
//=============================================================================================================

namespace SOURCELOCALIZATIONPLUGIN
{

//=============================================================================================================
// SOURCELOCALIZATIONPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * SourceLocalization Plugin
 *
 * @brief The sourcelocalization class provides a plugin for computing averages.
 */
class SOURCELOCALIZATIONSHARED_EXPORT SourceLocalization : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "sourcelocalization.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs an SourceLocalization object.
     */
    SourceLocalization();

    //=========================================================================================================
    /**
     * Destroys the SourceLocalization object.
     */
    ~SourceLocalization() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildInfo() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:

    enum Mode{
        SOURCE_LOC_FROM_AVG,
        SOURCE_LOC_FROM_SINGLE_TRIAL
    };

    void sourceLocalizationFromAverage();

    void sourceLocalizationFromSingleTrial();

    //=========================================================================================================
    /**
     * Loads new Fiff model whan current loaded model is changed
     *
     * @param[in, out] pNewModel    pointer to currently loaded FiffRawView Model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;                   /**< To broadcst signals. */

    QSharedPointer<ANSHAREDLIB::ForwardSolutionModel>       m_pFwdSolutionModel;
    QSharedPointer<ANSHAREDLIB::AveragingDataModel>         m_pAverageDataModel;
    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>           m_pRawDataModel;

    int                                                     m_iSelectedSample;

    Mode m_sourceLocalizationMode;
};

} // NAMESPACE

#endif // MNEANALYZE_SOURCELOCALIZATION_H
