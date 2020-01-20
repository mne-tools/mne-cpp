//=============================================================================================================
/**
 * @file     analyzesettings.h
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
 * @brief    Contains declaration of AnalyzeSettings class.
 *
 */

#ifndef ANALYZESETTINGS_H
#define ANALYZESETTINGS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


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



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//=========================================================================================================
/**
 * DECLARE CLASS AnalyzeSettings
 *
 * @brief The AnalyzeSettings class is the base settings container.
 */
class ANSHAREDSHARED_EXPORT AnalyzeSettings : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<AnalyzeSettings> SPtr;               /**< Shared pointer type for AnalyzeSettings. */
    typedef QSharedPointer<const AnalyzeSettings> ConstSPtr;    /**< Const shared pointer type for AnalyzeSettings. */

    //=========================================================================================================
    /**
    * Constructs the Analyze Settings.
    */
    AnalyzeSettings(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Analyze Settings.
    */
    virtual ~AnalyzeSettings();

    QString bemName() const;                    /*!< Returns the current BEM file name. @return the BEM file name.*/
    void setBEMName(const QString &bemName);    /*!< Sets the current BEM file name. [in] bemName   the new BEM file name.*/

    QString mriName() const;                    /*!< Returns the current MRI file name. @return the MRI file name.*/
    void setMRIName(const QString &mriName);    /*!< Sets the current MRI file name. [in] mriName   the new MRI file name.*/

    QString surfName() const;                   /*!< Returns the current Inner Skull Surface file name. @return the Inner Skull Surface file name.*/
    void setSurfName(const QString &surfName);  /*!< Sets the current Inner Skull Surface file name. [in] surfName  the new Inner Skull Surface file name.*/

    QString noiseName() const;                  /*!< Returns the current Noise Covariance file name. @return the Noise Covariance file name.*/
    void setNoiseName(const QString &noiseName);/*!< Sets the current Noise Covariance file name. [in] noiseName  the new Noise Covariance file name.*/

    QString measName() const;                   /*!< Returns the current Measurement file name. @return the Measurment file name.*/
    void setMeasName(const QString &measName);  /*!< Sets the current Measurement file name. [in] measName  the new Measurement file name.*/

    QStringList projNames() const;                      /*!< Returns the current Projection file name. @return the Projection file name.*/
    void setProjNames(const QStringList &projNames);    /*!< Sets the current Projection file name. [in] projNames  the new Projection file name.*/

    QString eegSphereModelName() const;                             /*!< Returns the current EEG Sphere Model Specifications file. @return the EEG Model Sphere Specifications file.*/
    void setEEGSphereModelName(const QString &eegSphereModelName);  /*!< Sets the current EEG Sphere Model Specifications file. [in] eegModelFile  the new EEG Sphere Model Specifications file.*/

    QString eegModelName() const;                       /*!< Returns the current EEG Model file name. @return the EEG Model file name.*/
    void setEEGModelName(const QString &eegModelName);  /*!< Sets the current EEG Sphere Model Specifications file. [in] eegModelName  the new EEG Sphere Model Specifications file.*/

signals:
    void bemNameChanged_signal();   /*!< Emmitted when the BEM file name changed.*/

    void mriNameChanged_signal();   /*!< Emmitted when the MRI file name changed.*/

    void surfNameChanged_signal();  /*!< Emmitted when the Inner Skull Surface file name changed.*/

    void noiseNameChanged_signal(); /*!< Emmitted when the Noise Covariance file name changed.*/

    void measNameChanged_signal();  /*!< Emmitted when the Measurement file name changed.*/

    void projNamesChanged_signal(); /*!< Emmitted when the Projection file name changed.*/

    void eegSphereModelNameChanged_signal();    /*!< Emmitted when the EEG Sphere Model Specifications file name changed.*/

    void eegModelNameChanged_signal();  /*!< Emmitted when the EEG Model file name changed.*/

protected:


private:
    QString m_BEMName;              /**< Boundary-element model */

    QString m_MRIName;              /**< Gives the MRI <-> head transform */

    QString m_SurfName;             /**< Load the inner skull surface from this BEM file */

    QString m_NoiseName;            /**< Noise-covariance matrix */

    QString m_MeasName;             /**< Data file */

    QStringList m_ProjNames;        /**< Projection file names */

    QString m_EEGSphereModelName;   /**< File of EEG sphere model specifications */

    QString m_EEGModelName;         /**< Name of the EEG model to use */



};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} //Namespace

#endif //ANALYZESETTINGS_H
