//=============================================================================================================
/**
 * @file     analyzedata.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh. All rights reserved.
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
 * @brief    Contains declaration of AnalyzeData Container class.
 *
 */

#ifndef ANALYZEDATA_H
#define ANALYZEDATA_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"

#include <mne/mne_sourceestimate.h>

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/ecd_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>
#include <QList>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================


//=========================================================================================================
/**
 * DECLARE CLASS AnalyzeData
 *
 * @brief The AnalyzeData class is the base data container.
 */
class ANSHAREDSHARED_EXPORT AnalyzeData : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<AnalyzeData> SPtr;               /**< Shared pointer type for AnalyzeData. */
    typedef QSharedPointer<const AnalyzeData> ConstSPtr;    /**< Const shared pointer type for AnalyzeData. */

    //=========================================================================================================
    /**
    * Constructs the Analyze Data.
    */
    AnalyzeData(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Analyze Data.
    */
    virtual ~AnalyzeData();

signals:
    void stcChanged_signal();                   /**< Emmited when the current STC changed.*/
    void stcSampleChanged_signal(int sample);   /**< Emmited when the current STC sample point changed. @param [in] sample  The sample time point;*/
    void ecdSetChanged_signal();                /**< Emmited when the current ECD Set changed.*/

public:
//STC
    const MNELIB::MNESourceEstimate& currentSTC() const;    /*!< Returns the current STC. @return The current STC.*/
    void addSTC( const MNELIB::MNESourceEstimate &stc );    /*!< Adds a new STC. @param [in] stc    the new STC;*/
    void setCurrentSTCSample(int sample);                   /*!< Sets the current stc sample;*/
    int currentSTCSample();                                 /*!< Returns the current STC sample. @return The current STC sample.*/

//ECD
    const INVERSELIB::ECDSet& currentECDSet() const;                                            /*!< Returns the current ECD Set. @return The current ECD Set.*/
    void addECDSet( INVERSELIB::DipoleFitSettings &ecdSettings,  INVERSELIB::ECDSet &ecdSet );  /*!< Sets the current ECD Set. @param [in] ecdSettings  Sets the settings corresponding to the current ECD Set; @param [in] ecdSet  Sets the current ECD Set;*/
    const QList< QPair< INVERSELIB::DipoleFitSettings, INVERSELIB::ECDSet > >& ecdSets() const; /*!< Returns a list of all past ECD Sets. @return All past ECD Sets.*/

// Database -> Consider using abstract item models or other datamanagement architecture
private:
// STCs
    QList<MNELIB::MNESourceEstimate>    m_qListEstimates;       /**< List of all Source Estimates.*/
    int                                 m_iCurrentEstimate;     /**< Current Estimate */
    int                                 m_iCurrentSample;       /**< Current sample point of the current estimate */

// ECDs
    QList< QPair< INVERSELIB::DipoleFitSettings, INVERSELIB::ECDSet > > m_qListECDSets;     /**< List of all past ECD Sets.*/
    int                                                                 m_iCurrentECDSet;   /**< Current ECD Set */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} //Namespace

#endif //ANALYZEDATA_H
