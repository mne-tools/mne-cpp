//=============================================================================================================
/**
* @file     analyzesettings.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the Analyze Settings Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "analyzesettings.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeSettings::AnalyzeSettings(QObject *parent)
: QObject(parent)
{

}


//*************************************************************************************************************

AnalyzeSettings::~AnalyzeSettings()
{

}


//*************************************************************************************************************

QString AnalyzeSettings::bemName() const
{
    return m_BEMName;
}


//*************************************************************************************************************

void AnalyzeSettings::setBEMName(const QString &BEMName)
{
    m_BEMName = BEMName;
    emit bemNameChanged_signal();
}


//*************************************************************************************************************

QString AnalyzeSettings::mriName() const
{
    return m_MRIName;
}


//*************************************************************************************************************

void AnalyzeSettings::setMRIName(const QString &MRIName)
{
    m_MRIName = MRIName;
    emit mriNameChanged_signal();
}


//*************************************************************************************************************

QString AnalyzeSettings::surfName() const
{
    return m_SurfName;
}


//*************************************************************************************************************

void AnalyzeSettings::setSurfName(const QString &SurfName)
{
    m_SurfName = SurfName;
    emit surfNameChanged_signal();
}


//*************************************************************************************************************

QString AnalyzeSettings::noiseName() const
{
    return m_NoiseName;
}


//*************************************************************************************************************

void AnalyzeSettings::setNoiseName(const QString &NoiseName)
{
    m_NoiseName = NoiseName;
    emit noiseNameChanged_signal();
}


//*************************************************************************************************************

QString AnalyzeSettings::measName() const
{
    return m_MeasName;
}


//*************************************************************************************************************

void AnalyzeSettings::setMeasName(const QString &MeasName)
{
    m_MeasName = MeasName;
    emit measNameChanged_signal();
}


//*************************************************************************************************************

QStringList AnalyzeSettings::projNames() const
{
    return m_ProjNames;
}


//*************************************************************************************************************

void AnalyzeSettings::setProjNames(const QStringList &ProjNames)
{
    m_ProjNames = ProjNames;
    emit projNamesChanged_signal();
}


//*************************************************************************************************************

QString AnalyzeSettings::eegSphereModelName() const
{
    return m_EEGSphereModelName;
}


//*************************************************************************************************************

void AnalyzeSettings::setEEGSphereModelName(const QString &EEGModelFile)
{
    m_EEGSphereModelName = EEGModelFile;
    emit eegSphereModelNameChanged_signal();
}


//*************************************************************************************************************

QString AnalyzeSettings::eegModelName() const
{
    return m_EEGModelName;
}


//*************************************************************************************************************

void AnalyzeSettings::setEEGModelName(const QString &EEGModelName)
{
    m_EEGModelName = EEGModelName;
    emit eegModelNameChanged_signal();
}
