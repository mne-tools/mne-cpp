//=============================================================================================================
/**
* @file     modalityselectionview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the ModalitySelectionView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "modalityselectionview.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ModalitySelectionView::ModalitySelectionView(QWidget *parent,
                                             Qt::WindowFlags f)
: QWidget(parent, f)
{
    this->setWindowTitle("Modality Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void ModalitySelectionView::init(const QList<DISPLIB::Modality>& modalityList)
{
    m_qListModalities.clear();
    bool hasMAG = false;
    bool hasGRAD = false;
    bool hasEEG = false;
    bool hasEOG = false;
    bool hasMISC = false;
    for(qint32 i = 0; i < modalityList.size(); ++i)
    {
        if(modalityList.at(i).m_sName.contains("MAG"))
            hasMAG = true;
        if(modalityList.at(i).m_sName.contains("GRAD"))
            hasGRAD = true;
        else if(modalityList.at(i).m_sName.contains("EEG"))
            hasEEG = true;
        else if(modalityList.at(i).m_sName.contains("EOG"))
            hasEOG = true;
        else if(modalityList.at(i).m_sName.contains("MISC"))
            hasMISC = true;
    }

    bool sel = true;
    float val = 1e-11f;

    if(hasMAG)
        m_qListModalities.append(Modality("MAG",sel,val));
    if(hasGRAD)
        m_qListModalities.append(Modality("GRAD",sel,val));
    if(hasEEG)
        m_qListModalities.append(Modality("EEG",false,val));
    if(hasEOG)
        m_qListModalities.append(Modality("EOG",false,val));
    if(hasMISC)
        m_qListModalities.append(Modality("MISC",false,val));

    QGridLayout* t_pGridLayout = new QGridLayout;

    for(qint32 i = 0; i < m_qListModalities.size(); ++i)
    {
        QString mod = m_qListModalities[i].m_sName;

        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText(mod);
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QCheckBox* t_pCheckBoxModality = new QCheckBox;
        t_pCheckBoxModality->setChecked(m_qListModalities[i].m_bActive);
        m_qListModalityCheckBox << t_pCheckBoxModality;
        connect(t_pCheckBoxModality,&QCheckBox::stateChanged,
                this,&ModalitySelectionView::onUpdateModalityCheckbox);
        t_pGridLayout->addWidget(t_pCheckBoxModality,i,1,1,1);
    }

    //Find Modalities tab and add current layout
    this->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void ModalitySelectionView::init(const FiffInfo::SPtr pFiffInfo)
{
    if(pFiffInfo) {
        m_pFiffInfo = pFiffInfo;

        m_qListModalities.clear();
        bool hasMag = false;
        bool hasGrad = false;
        bool hasEEG = false;
        bool hasEOG = false;
        bool hasMISC = false;
        for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
        {
            if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
            {
                if(!hasMag && m_pFiffInfo->chs[i].unit == FIFF_UNIT_T)
                    hasMag = true;
                else if(!hasGrad &&  m_pFiffInfo->chs[i].unit == FIFF_UNIT_T_M)
                    hasGrad = true;
            }
            else if(!hasEEG && m_pFiffInfo->chs[i].kind == FIFFV_EEG_CH)
                hasEEG = true;
            else if(!hasEOG && m_pFiffInfo->chs[i].kind == FIFFV_EOG_CH)
                hasEOG = true;
            else if(!hasMISC && m_pFiffInfo->chs[i].kind == FIFFV_MISC_CH)
                hasMISC = true;
        }

        bool sel = true;
        float val = 1e-11f;

        if(hasMag)
            m_qListModalities.append(Modality("MAG",sel,val));
        if(hasGrad)
            m_qListModalities.append(Modality("GRAD",sel,val));
        if(hasEEG)
            m_qListModalities.append(Modality("EEG",false,val));
        if(hasEOG)
            m_qListModalities.append(Modality("EOG",false,val));
        if(hasMISC)
            m_qListModalities.append(Modality("MISC",false,val));

        QGridLayout* t_pGridLayout = new QGridLayout;

        for(qint32 i = 0; i < m_qListModalities.size(); ++i)
        {
            QString mod = m_qListModalities[i].m_sName;

            QCheckBox* t_pCheckBoxModality = new QCheckBox();
            t_pCheckBoxModality->setChecked(m_qListModalities[i].m_bActive);
            t_pCheckBoxModality->setText(mod);
            m_qListModalityCheckBox << t_pCheckBoxModality;
            connect(t_pCheckBoxModality,&QCheckBox::stateChanged,
                    this,&ModalitySelectionView::onUpdateModalityCheckbox);
            t_pGridLayout->addWidget(t_pCheckBoxModality,i,1,1,1);
        }

        //Find Modalities tab and add current layout
        this->setLayout(t_pGridLayout);
    }
}


//*************************************************************************************************************

void ModalitySelectionView::setModalities(const QList<Modality> &lModalities)
{
    for(int i = 0; i < m_qListModalityCheckBox.size(); i++) {
        for(int j = 0; j < lModalities.size(); j++) {
            if(m_qListModalityCheckBox.at(i)->text().contains(lModalities.at(j).m_sName)) {
                //qDebug()<<"ModalitySelectionView::setModalities - Set "<<lModalities.at(j).m_sName<<"to"<<lModalities.at(j).m_bActive;
                m_qListModalityCheckBox.at(i)->setChecked(lModalities.at(j).m_bActive);
            }
        }
    }
}


//*************************************************************************************************************

void ModalitySelectionView::onUpdateModalityCheckbox(qint32 state)
{
    Q_UNUSED(state)

    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i)
    {
        if(m_qListModalityCheckBox[i]->isChecked())
            m_qListModalities[i].m_bActive = true;
        else
            m_qListModalities[i].m_bActive = false;
    }

    emit modalitiesChanged(m_qListModalities);
}

