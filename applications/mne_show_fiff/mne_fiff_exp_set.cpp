//=============================================================================================================
/**
* @file     mne_fiff_exp_set.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the MneFiffExpSet class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_fiff_exp_set.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneFiffExpSet::MneFiffExpSet()
{
}


//*************************************************************************************************************

MneFiffExpSet::MneFiffExpSet(const MneFiffExpSet &p_MneFiffExpSet)
: m_qListExp(p_MneFiffExpSet.m_qListExp)
{

}


//*************************************************************************************************************

MneFiffExpSet::~MneFiffExpSet()
{

}


//*************************************************************************************************************

const MneFiffExp& MneFiffExpSet::operator[] (int idx) const
{
    if (idx >= m_qListExp.length())
    {
        qWarning("Warning: Required MneFiffExp doesn't exist! Returning MneFiffExp '0'.");
        idx=0;
    }
    return m_qListExp[idx];
}


//*************************************************************************************************************

MneFiffExp& MneFiffExpSet::operator[] (int idx)
{
    if (idx >= m_qListExp.length())
    {
        qWarning("Warning: Required MneFiffExp doesn't exist! Returning MneFiffExp '0'.");
        idx = 0;
    }
    return m_qListExp[idx];
}


//*************************************************************************************************************

MneFiffExpSet &MneFiffExpSet::operator<<(const MneFiffExp &p_MneFiffExp)
{
    this->m_qListExp.append(p_MneFiffExp);
    return *this;
}


//*************************************************************************************************************

MneFiffExpSet MneFiffExpSet::read_fiff_explanations(const QString &name)
{
    QFile file(name);

    MneFiffExpSet res;

    int         exclass,kind;
    QString     text;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << name;
        return res;
    }

    QTextStream in(&file);
    bool ok = false;
    while (!in.atEnd()) {
        QString line = in.readLine();

        QStringList list = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        if(list.size() < 3)
            continue;

        exclass = list[0].toInt(&ok);
        if(!ok)
            continue;

        kind = list[1].toInt(&ok);
        if(!ok)
            break;

        text.clear();

        text = list[2];
        for(int i = 3; i < list.size(); ++i)
            text += " " + list[i];

        text.remove("\"");

        MneFiffExp exp;
        exp.exclass = exclass;
        exp.kind = kind;
        exp.text = text;

        res << exp;
    }


    if (res.size() == 0) {
        qCritical("No explanations in %s",name.toUtf8().constData());
        return res;
    }

    res.sort_fiff_explanations();

    return res;
}


//*************************************************************************************************************

void MneFiffExpSet::list_fiff_explanations(FILE *out)
{
    for (int k = 0; k < this->size(); k++)
        fprintf(out,"%d %d \"%s\"\n",this->m_qListExp[k].exclass,this->m_qListExp[k].kind,this->m_qListExp[k].text.toUtf8().constData());
}


//*************************************************************************************************************

void MneFiffExpSet::sort_fiff_explanations()
{
    if (this->size() == 0)
        return;

    qSort(this->m_qListExp.begin(), this->m_qListExp.end(), MneFiffExp::comp_exp);

    return;
}
