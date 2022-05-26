//=============================================================================================================
/**
 * @file     neurofeedback.h
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Contains the declaration of the Neurofeedback class.
 *
 */

#ifndef     NEUROFEEDBACK_H
#define     NEUROFEEDBACK_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedback_global.h"

#include <scShared/Plugins/abstractalgorithm.h>
#include <utils/generics/circularbuffer.h>

#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeNeurofeedbackResult;
}



//=============================================================================================================
// DEFINE NAMESPACE NEUROFEEDBACKPLUGIN
//=============================================================================================================

namespace NEUROFEEDBACKPLUGIN
{

//=============================================================================================================
// NEUROFEEDBACKPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS Neurofeedback
 *
 * @brief The Neurofeedback class provides a Neurofeedback algorithm structure.
 */
class NEUROFEEDBACKSHARED_EXPORT Neurofeedback : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neurofeedback.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

    friend class NeurofeedbackSettingsWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a Neurofeedback.
     */
    Neurofeedback();

    //=========================================================================================================
    /**
     * Destroys the Neurofeedback.
     */
    ~Neurofeedback();

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     */
    virtual void unload();
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Udates the pugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Initialise input and output connectors.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Changes the Type of Output.
     *
     * @param[in] value  Type of Output for Neurofeedback.
     */
     void changeOutput(int value);

     //=========================================================================================================
     /**
      * Changes the Number of Sliders.
      *
      * @param[in] value  Number of Sliders.
      */
     void changeSliders(int value);

     //=========================================================================================================
     /**
      * Changes the Bool for all Channels.
      *
      * @param[in] ballCh  Bool for all Channels selected or not.
      */
     void changeballCh(bool ballCh);

     //=========================================================================================================
     /**
      * Changes the Bool for minimum autoscale.
      *
      * @param[in] bMinAutoScale  Bool for minimum autoscale.
      */
     void changeMinAutoScale(bool bMinAutoScale);

     //=========================================================================================================
     /**
      * Changes the Bool for maximum autoscale.
      *
      * @param[in] bMaxAutoScale  Bool for maximum autoscale.
      */
     void changeMaxAutoScale(bool bMaxAutoScale);

     //=========================================================================================================
     /**
      * Changes the Bool for reset autoscale.
      *
      * @param[in] bResetAutoScale  Bool for reset autoscale.
      */
     void changeResetAutoScale(bool bResetAutoScale);

     //=========================================================================================================
     /**
      * Changes the Number of Classification.
      *
      * @param[in] value  Number of Classification.
      */
     void changeNumbofClass(int value);

     //=========================================================================================================
     /**
      * Changes the Textoutput for Classification 0.
      *
      * @param[in] text  Textouptut for Classification 0.
      */
     void changeClass0(QString text);

     //=========================================================================================================
     /**
      * Changes the Textoutput for Classification 1.
      *
      * @param[in] text  Textouptut for Classification 1.
      */
     void changeClass1(QString text);

     //=========================================================================================================
     /**
      * Changes the Textoutput for Classification 2.
      *
      * @param[in] text  Textouptut for Classification 2.
      */
     void changeClass2(QString text);

     //=========================================================================================================
     /**
      * Changes the Textoutput for Classification 3.
      *
      * @param[in] text  Textouptut for Classification 3.
      */
     void changeClass3(QString text);

     //=========================================================================================================
     /**
      * Changes the Textoutput for Classification 4.
      *
      * @param[in] text  Textouptut for Classification 4.
      */
     void changeClass4(QString text);

     //=========================================================================================================
     /**
      * Changes the Color for Textouput Classification 0.
      *
      * @param[in] text  Color for Textouput Classification 0.
      */
     void changeGBC0(QString text);

     //=========================================================================================================
     /**
      * Changes the Color for Textouput Classification 1.
      *
      * @param[in] text  Color for Textouput Classification 1.
      */
     void changeGBC1(QString text);

     //=========================================================================================================
     /**
      * Changes the Color for Textouput Classification 2.
      *
      * @param[in] text  Color for Textouput Classification 2.
      */
     void changeGBC2(QString text);

     //=========================================================================================================
     /**
      * Changes the Color for Textouput Classification 3.
      *
      * @param[in] text  Color for Textouput Classification 3.
      */
     void changeGBC3(QString text);

     //=========================================================================================================
     /**
      * Changes the Color for Textouput Classification 4.
      *
      * @param[in] text  Color for Textouput Classification 4.
      */
     void changeGBC4(QString text);

     //=========================================================================================================
     /**
      * Changes the Integer for Classification 0.
      *
      * @param[in] value  Integer for Classification 0.
      */
     void changeiClass0(int value);

     //=========================================================================================================
     /**
      * Changes the Integer for Classification 1.
      *
      * @param[in] value  Integer for Classification 1.
      */
     void changeiClass1(int value);

     //=========================================================================================================
     /**
      * Changes the Integer for Classification 2.
      *
      * @param[in] value  Integer for Classification 2.
      */
     void changeiClass2(int value);

     //=========================================================================================================
     /**
      * Changes the Integer for Classification 3.
      *
      * @param[in] value  Integer for Classification 3.
      */
     void changeiClass3(int value);

     //=========================================================================================================
     /**
      * Changes the Integer for Classification 4.
      *
      * @param[in] value  Integer for Classification 4.
      */
     void changeiClass4(int value);

     //=========================================================================================================
     /**
      * Changes the Directory for the Imageoutput Classification 0.
      *
      * @param[im] text  Directory for Imageoutput Classification 0.
      */
     void changeDirClass0(QString text);

     //=========================================================================================================
     /**
      * Changes the Directory for the Imageoutput Classification 1.
      *
      * @param[im] text  Directory for Imageoutput Classification 1.
      */
     void changeDirClass1(QString text);

     //=========================================================================================================
     /**
      * Changes the Directory for the Imageoutput Classification 2.
      *
      * @param[im] text  Directory for Imageoutput Classification 2.
      */
     void changeDirClass2(QString text);

     //=========================================================================================================
     /**
      * Changes the Directory for the Imageoutput Classification 3.
      *
      * @param[im] text  Directory for Imageoutput Classification 3.
      */
     void changeDirClass3(QString text);

     //=========================================================================================================
     /**
      * Changes the Directory for the Imageoutput Classification 4.
      *
      * @param[im] text  Directory for Imageoutput Classification 4.
      */
     void changeDirClass4(QString text);

     //=========================================================================================================
     /**
      * Changes the Imageoutput for Classification 0.
      *
      * @param[im] image  Imageoutput for Classification 0.
      */
     void changeImgClass0(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Imageoutput for Classification 1.
      *
      * @param[im] image  Imageoutput for Classification 1.
      */
     void changeImgClass1(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Imageoutput for Classification 2.
      *
      * @param[im] image  Imageoutput for Classification 2.
      */
     void changeImgClass2(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Imageoutput for Classification 3.
      *
      * @param[im] image  Imageoutput for Classification 3.
      */
     void changeImgClass3(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Imageoutput for Classification 4.
      *
      * @param[im] image  Imageoutput for Classification 4.
      */
     void changeImgClass4(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Backgroundimage.
      *
      * @param[im] image  Backgroundimage.
      */
     void changeImgBackground(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Directory for the Objectimage.
      *
      * @param[im] image  Objectimage.
      */
     void changeImgObject(QPixmap image);

     //=========================================================================================================
     /**
      * Changes the Miniumumfrequency for Sliders.
      *
      * @param[im] value  Minimumfrequency Slider.
      */
     void changeFMin(int value);

     //=========================================================================================================
     /**
      * Changes the Maximumfrequency for Sliders.
      *
      * @param[im] value  Maximumfrequency Slider.
      */
     void changeFMax(int value);

     //=========================================================================================================
     /**
      * Changes the Minimumfrequency for the Balloon.
      *
      * @param[im] value  Minimumfrequency Balloon.
      */
     void changeBMin(int value);

     //=========================================================================================================
     /**
      * Changes the Maximumfrequency for the Balloon.
      *
      * @param[im] value  Maximumfrequency Balloon.
      */
     void changeBMax(int value);

     //=========================================================================================================
     /**
      * Changes the Channel for the Balloon.
      *
      * @param[im] value  Channel for Balloon.
      */
     void changeChannel(int value);
     //=========================================================================================================



protected:
    virtual void run();

private:

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr                 m_pNeurofeedbackInput;
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeNeurofeedbackResult>::SPtr             m_pNeurofeedbackOutput;

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                                  m_pCircularBuffer;          /**< Holds incoming raw data. */

    QMutex                                          m_qMutex;                           /**< Provides access serialization between threads. */

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                        /**< Fiff measurement info.*/

    QStringList                                     m_lResponsibleTriggerTypes;         /**< List of all trigger types which lead to the recent emit of a new evoked set. */

    QMap<QString,int>                               m_mapStimChsIndexNames;             /**< The currently available stim channels and their corresponding index in the data. */

    int m_iSliders;
    int m_iNumbofclass;
    int m_iClass0;
    int m_iClass1;
    int m_iClass2;
    int m_iClass3;
    int m_iClass4;

    QString m_sClass0;
    QString m_sClass1;
    QString m_sClass2;
    QString m_sClass3;
    QString m_sClass4;
    QString m_sNeurooutput;

    bool m_ballCh;



signals:
    void stimChannelsChanged(const QMap<QString,int>& mapStimChsIndexNames);
    void fiffChInfoChanged(const QList<FIFFLIB::FiffChInfo>& fiffChInfoList);
    void evokedSetChanged(const FIFFLIB::FiffEvokedSet& evokedSet);
    void updateMin(int iMin);
    void updateMax(int iMax);
};
} // NAMESPACE

#endif //  NEUROFEEDBACK_H
