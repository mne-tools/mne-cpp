//=============================================================================================================
/**
 * @file     fwd_eeg_sphere_model_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FwdEegSphereModelSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_model_set.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <Eigen/Core>

using namespace Eigen;

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModelSet::FwdEegSphereModelSet()
{
}

//=============================================================================================================

FwdEegSphereModelSet::~FwdEegSphereModelSet()
{
}

//=============================================================================================================

FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(FwdEegSphereModelSet* s, FwdEegSphereModel::UPtr m)
{
    if (!s)
        s = new FwdEegSphereModelSet;

    s->models.push_back(std::move(m));
    return s;
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(FwdEegSphereModelSet* s)
{
    static const int   def_nlayer        = 4;
    VectorXf def_unit_rads(def_nlayer);
    def_unit_rads << 0.90f,0.92f,0.97f,1.0f;
    VectorXf def_sigmas(def_nlayer);
    def_sigmas << 0.33f,1.0f,0.4e-2f,0.33f;

    return FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(s,FwdEegSphereModel::fwd_create_eeg_sphere_model("Default",
                                                                         def_nlayer,def_unit_rads,def_sigmas));
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_load_eeg_sphere_models(const QString& filename, FwdEegSphereModelSet* now)
{
    if (!now)
        now = fwd_add_default_eeg_sphere_model(now);

    if (filename.isEmpty())
        return now;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return now;

    QTextStream in(&file);
    QString name;
    VectorXf rads;
    VectorXf sigmas;
    int nlayer = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("[:\\n\\r]"), Qt::SkipEmptyParts);
        if (parts.isEmpty())
            continue;

        name = parts[0].trimmed();
        nlayer = 0;

        for (int i = 1; i + 1 < parts.size(); i += 2) {
            bool okRad = false, okSig = false;
            float r = parts[i].trimmed().toFloat(&okRad);
            float s = parts[i + 1].trimmed().toFloat(&okSig);
            if (!okRad || !okSig)
                break;
            rads.conservativeResize(nlayer + 1);
            sigmas.conservativeResize(nlayer + 1);
            rads[nlayer] = r;
            sigmas[nlayer] = s;
            nlayer++;
        }
        if (nlayer > 0)
            now = fwd_add_to_eeg_sphere_model_set(now, FwdEegSphereModel::fwd_create_eeg_sphere_model(name, nlayer, rads, sigmas));
    }
    return now;
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
FwdEegSphereModel* FwdEegSphereModelSet::fwd_select_eeg_sphere_model(const QString& p_sName)
{
    QString name("Default");

    if (!p_sName.isEmpty())
        name = p_sName;

    if (this->nmodel() == 0) {
        qWarning("No EEG sphere model definitions available");
        return nullptr;
    }

    for (int k = 0; k < this->nmodel(); k++) {
        if (this->models[k]->name.compare(name) == 0) {
            qInfo("Selected model: %s",this->models[k]->name.toUtf8().constData());
            return new FwdEegSphereModel(*(this->models[k]));
        }
    }
    qWarning("EEG sphere model %s not found.",name.toUtf8().constData());
    return nullptr;
}

//=============================================================================================================
//dipole_fit_setup.c
void FwdEegSphereModelSet::fwd_list_eeg_sphere_models()
{
    if (this->nmodel() <= 0)
        return;
    qInfo("Available EEG sphere models:");
    for (int k = 0; k < this->nmodel(); k++) {
        FwdEegSphereModel* this_model = this->models[k].get();
        QString line = QString("\t%1 : %2").arg(this_model->name).arg(this_model->nlayer());
        for (int p = 0; p < this_model->nlayer(); p++)
            line += QString(" : %1 : %2").arg(this_model->layers[p].rel_rad, 7, 'f', 3).arg(this_model->layers[p].sigma, 7, 'f', 3);
        qInfo("%s", line.toUtf8().constData());
    }
}

