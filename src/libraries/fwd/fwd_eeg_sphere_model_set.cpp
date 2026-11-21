//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     fwd_eeg_sphere_model_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     December 2016
 * @brief    FwdEegSphereModelSet implementation — @c mne_setup_eeg_sphere_model parameter-file parser plus name-based lookup over the resulting FwdEegSphereModel instances.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_model_set.h"

#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

using namespace Eigen;

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <fstream>
#include <string>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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

std::unique_ptr<FwdEegSphereModelSet> FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(std::unique_ptr<FwdEegSphereModelSet> s, std::unique_ptr<FwdEegSphereModel> m)
{
    if (!s)
        s = std::make_unique<FwdEegSphereModelSet>();

    s->models.push_back(std::move(m));
    return s;
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
std::unique_ptr<FwdEegSphereModelSet> FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(std::unique_ptr<FwdEegSphereModelSet> s)
{
    static const int def_nlayer = 4;
    VectorXf def_unit_rads(def_nlayer);
    def_unit_rads << 0.90f, 0.92f, 0.97f, 1.0f;
    VectorXf def_sigmas(def_nlayer);
    def_sigmas << 0.33f, 1.0f, 0.4e-2f, 0.33f;

    return FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(std::move(s), FwdEegSphereModel::fwd_create_eeg_sphere_model("Default", def_nlayer, def_unit_rads, def_sigmas));
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
std::unique_ptr<FwdEegSphereModelSet> FwdEegSphereModelSet::fwd_load_eeg_sphere_models(const std::filesystem::path& filename, std::unique_ptr<FwdEegSphereModelSet> now)
{
    if (!now)
        now = fwd_add_default_eeg_sphere_model(nullptr);

    if (filename.empty())
        return now;

    std::ifstream file(filename);
    if (!file)
        return now;

    // One model per line: name : rad1 : sigma1 : rad2 : sigma2 ...
    VectorXf rads;
    VectorXf sigmas;
    for (std::string raw; std::getline(file, raw);) {
        const std::string_view line = mne::trimmed(raw);
        if (line.empty() || line.starts_with('#'))
            continue;

        const mne::StringList parts = mne::split(line, ':', true);
        if (parts.empty())
            continue;

        const std::string name(mne::trimmed(parts[0]));
        int nlayer = 0;
        for (std::size_t i = 1; i + 1 < parts.size(); i += 2) {
            const std::optional<double> r = mne::to_double(parts[i]);
            const std::optional<double> sg = mne::to_double(parts[i + 1]);
            if (!r || !sg) {
                nlayer = 0;
                break;
            }
            rads.conservativeResize(nlayer + 1);
            sigmas.conservativeResize(nlayer + 1);
            rads[nlayer] = static_cast<float>(*r);
            sigmas[nlayer] = static_cast<float>(*sg);
            nlayer++;
        }
        if (nlayer > 0)
            now = fwd_add_to_eeg_sphere_model_set(std::move(now), FwdEegSphereModel::fwd_create_eeg_sphere_model(name, nlayer, rads, sigmas));
    }
    return now;
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
std::unique_ptr<FwdEegSphereModel> FwdEegSphereModelSet::fwd_select_eeg_sphere_model(std::string_view p_sName) const
{
    const std::string_view name = p_sName.empty() ? std::string_view("Default") : p_sName;

    if (this->nmodel() == 0) {
        mne::Log::warning("No EEG sphere model definitions available");
        return nullptr;
    }

    for (int k = 0; k < this->nmodel(); k++) {
        if (this->models[k]->name == name) {
            mne::Log::info("Selected model: {}", this->models[k]->name);
            return std::make_unique<FwdEegSphereModel>(*(this->models[k]));
        }
    }
    mne::Log::warning("EEG sphere model {} not found.", std::string(name));
    return nullptr;
}

//=============================================================================================================
//dipole_fit_setup.c
void FwdEegSphereModelSet::fwd_list_eeg_sphere_models()
{
    if (this->nmodel() <= 0)
        return;
    mne::Log::info("Available EEG sphere models:");
    for (int k = 0; k < this->nmodel(); k++) {
        FwdEegSphereModel* this_model = this->models[k].get();
        std::string line = std::format("\t{} : {}", this_model->name, this_model->nlayer());
        for (int p = 0; p < this_model->nlayer(); p++)
            std::format_to(std::back_inserter(line), " : {:7.3f} : {:7.3f}", this_model->layers[p].rel_rad, this_model->layers[p].sigma);
        mne::Log::info("{}", line);
    }
}
