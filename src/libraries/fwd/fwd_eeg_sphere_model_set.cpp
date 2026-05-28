//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fwd_eeg_sphere_model_set.cpp
 * @since 2022
 * @date  March 2026
 * @brief FwdEegSphereModelSet implementation — @c mne_setup_eeg_sphere_model parameter-file parser plus name-based lookup over the resulting FwdEegSphereModel instances.
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

