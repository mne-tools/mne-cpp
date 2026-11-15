//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_dipole_fit.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of the sequential ECD-fit driver.
 *
 * Implements the time-bin loop that calls into the Nelder-Mead /
 * guess-grid optimiser, reads averaged or raw data through @c mne_mne
 * and assembles the per-time-point @ref InvEcd records into the output
 * @ref InvEcdSet. Refactored from @c fit_dipoles.c / @c fit_dipoles_raw
 * in the MNE-C reference implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_dipole_fit.h"
#include <base/log.h>
#include <base/string_utils.h>
#include <mne/mne_meas_data_set.h>
#include <mne/mne_types.h>
#include "inv_guess_data.h"

#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

static constexpr float SEG_LEN = 10.0f;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvDipoleFit::InvDipoleFit(InvDipoleFitSettings* p_settings)
: settings(p_settings)
{
}

//=============================================================================================================
InvEcdSet InvDipoleFit::calculateFit() const
{
    InvEcdSet set;

    mne::Log::info("---- Setting up...\n");
    std::unique_ptr<FwdEegSphereModel> eeg_model;
    if (settings->include_eeg) {
        eeg_model = FwdEegSphereModel::setup_eeg_sphere_model(mne::path_from_utf8(settings->eeg_model_file), settings->eeg_model_name, settings->eeg_sphere_rad);
        if (!eeg_model)
            return set;
    }

    std::unique_ptr<InvDipoleFitData> fit_data(InvDipoleFitData::setup_dipole_fit_data(
        settings->mriname,
        settings->measname,
        settings->bemname,
        &settings->r0,
        eeg_model.get(),
        settings->accurate,
        settings->badname,
        settings->noisename,
        settings->grad_std,
        settings->mag_std,
        settings->eeg_std,
        settings->mag_reg,
        settings->grad_reg,
        settings->eeg_reg,
        settings->diagnoise,
        settings->projnames,
        settings->include_meg,
        settings->include_eeg));
    if (!fit_data)
        return set;

    eeg_model.release(); // ownership transferred to fit_data->eeg_model

    fit_data->fit_mag_dipoles = settings->fit_mag_dipoles;

    std::unique_ptr<MNERawData> raw;
    std::unique_ptr<MNEMeasData> data;
    std::unique_ptr<MNEChSelection> sel;

    if (settings->is_raw) {
        mne::Log::info("\n---- Opening a raw data file...\n");
        raw.reset(MNERawData::open_file(mne::path_from_utf8(settings->measname), true, false, settings->filter));
        if (!raw)
            return set;
        /*
     * Set up a channel selection to pick data from the raw file
     */
        sel = std::make_unique<MNEChSelection>(MNEChSelection::these(
            "fit", std::span(fit_data->ch_names).first(static_cast<std::size_t>(fit_data->nmeg) + static_cast<std::size_t>(fit_data->neeg))));
        sel->assign_channels(raw->info->chInfo, raw->deriv_matched.get());
        for (int c = 0; c < sel->nchan(); c++)
            if (sel->pick[c] < 0) {
                mne::Log::critical("All desired channels were not available");
                return set;
            }
        mne::Log::info("\tChannel selection created.");
        /*
         * Let's be a little generous here
         */
        float t1 = raw->first_samp / raw->info->sfreq;
        float t2 = (raw->first_samp + raw->nsamp - 1) / raw->info->sfreq;
        if (settings->tmin < t1 + settings->integ)
            settings->tmin = t1 + settings->integ;
        if (settings->tmax > t2 - settings->integ)
            settings->tmax = t2 - settings->integ;
        if (settings->tstep < 0)
            settings->tstep = 1.0f / raw->info->sfreq;

        mne::Log::info("\tOpened raw data file {} : {} MEG and {} EEG", settings->measname, fit_data->nmeg, fit_data->neeg);
    } else {
        mne::Log::info("\n---- Reading data...\n");
        data = MNEMeasData::mne_read_meas_data(mne::path_from_utf8(settings->measname),
                                               settings->setno,
                                               nullptr,
                                               nullptr,
                                               fit_data->ch_names,
                                               fit_data->nmeg + fit_data->neeg);
        if (!data)
            return set;
        if (settings->do_baseline)
            data->adjust_baselines(settings->bmin, settings->bmax);
        else
            mne::Log::info("\tNo baseline setting in effect.");
        if (settings->tmin < data->current->tmin + settings->integ / 2.0f)
            settings->tmin = data->current->tmin + settings->integ / 2.0f;
        if (settings->tmax > data->current->tmin + (data->current->np - 1) * data->current->tstep - settings->integ / 2.0f)
            settings->tmax = data->current->tmin + (data->current->np - 1) * data->current->tstep - settings->integ / 2.0f;
        if (settings->tstep < 0)
            settings->tstep = data->current->tstep;

        mne::Log::info("\tRead data set {} from {} : {} MEG and {} EEG", settings->setno, settings->measname, fit_data->nmeg, fit_data->neeg);
        if (!settings->noisename.empty()) {
            mne::Log::info("Scaling the noise covariance...");
            if (InvDipoleFitData::scale_noise_cov(fit_data.get(), data->current->nave) < 0)
                return set;
        }
    }

    /*
     * Proceed to computing the fits
     */
    mne::Log::info("\n---- Computing the forward solution for the guesses...\n");
    auto guess = std::make_unique<InvGuessData>(mne::path_from_utf8(settings->guessname),
                                                mne::path_from_utf8(settings->guess_surfname),
                                                settings->guess_mindist, settings->guess_exclude, settings->guess_grid, fit_data.get(), settings->guess_rad);
    if (guess->nguess == 0) {
        mne::Log::critical("Could not create the initial guesses.");
        return set;
    }

    mne::Log::info("\n---- Fitting : {:7.1f} ... {:7.1f} ms (step: {:6.1f} ms integ: {:6.1f} ms)\n", 1000 * settings->tmin, 1000 * settings->tmax, 1000 * settings->tstep, 1000 * settings->integ);

    if (raw) {
        if (!fit_dipoles_raw(settings->measname, raw.get(), sel.get(), fit_data.get(), guess.get(), settings->tmin, settings->tmax, settings->tstep, settings->integ, settings->verbose, set))
            return set;
    } else {
        if (!fit_dipoles(settings->measname, data.get(), fit_data.get(), guess.get(), settings->tmin, settings->tmax, settings->tstep, settings->integ, settings->verbose, set))
            return set;
    }
    mne::Log::info("{} dipoles fitted", set.size());

    return set;
}

//=============================================================================================================

bool InvDipoleFit::fit_dipoles(const std::string& dataname, MNEMeasData* data, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set)
{
    Eigen::VectorXf one(data->nchan);
    InvEcdSet set;
    InvEcd dip;
    constexpr int report_interval = 10;

    set.dataname = dataname;

    if (verbose)
        mne::Log::info("Fitting...");
    for (int s = 0; tmin + s * tstep < tmax; s++) {
        float time = tmin + s * tstep;
        if (data->current->getValuesAtTime(time, integ, false, one) < 0) {
            mne::Log::warning("Cannot pick time: {:7.1f} ms", 1000.0f * time);
            continue;
        }

        if (!InvDipoleFitData::fit_one(fit, guess, time, one, verbose, dip))
            mne::Log::warning("t = {:7.1f} ms : fit error", 1000.0f * time);
        else {
            set.push_back(dip);
            if (verbose)
                dip.print();
            else {
                if (set.size() % report_interval == 0)
                    mne::Log::info("{}..", set.size());
            }
        }
    }
    if (!verbose)
        mne::Log::info("[done]");
    p_set = set;
    return true;
}

//=============================================================================================================

bool InvDipoleFit::fit_dipoles_raw(const std::string& dataname, MNERawData* raw, MNEChSelection* sel, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set)
{
    const int nchan = sel->nchan();
    const float sfreq = raw->info->sfreq;
    const float myinteg = integ > 0.0f ? 2 * integ : 0.1f;
    const int overlap = static_cast<int>(std::ceil(myinteg * sfreq));
    const int length = static_cast<int>(SEG_LEN * sfreq);
    const int step = length - overlap;
    const int stepo = step + overlap / 2;
    int start = raw->first_samp;
    constexpr int report_interval = 10;

    Eigen::VectorXf one(nchan);

    MNERawData::RowMajorMatrixXf segment(nchan, length);

    InvEcd dip;
    InvEcdSet set;
    set.dataname = dataname;

    /*
     * Load the initial data segment
     */
    float stime = start / sfreq;
    if (raw->pick_data_filt(sel, start, segment) < 0)
        return false;
    if (verbose)
        mne::Log::info("Fitting...");
    for (int s = 0; tmin + s * tstep < tmax; s++) {
        float time = tmin + s * tstep;
        int picks = time * sfreq - start;
        if (picks > stepo) {
            start = start + step;
            if (raw->pick_data_filt(sel, start, segment) < 0)
                return false;
            picks = time * sfreq - start;
            stime = start / sfreq;
        }
        if (MNEMeasDataSet::getValuesFromChannelData(time, integ, segment, stime, sfreq, false, one) < 0) {
            mne::Log::warning("Cannot pick time: {:8.3f} s", time);
            continue;
        }
        if (!InvDipoleFitData::fit_one(fit, guess, time, one, verbose, dip))
            mne::Log::warning("t = {:8.3f} s : fit error", time);
        else {
            set.push_back(dip);
            if (verbose)
                dip.print();
            else {
                if (set.size() % report_interval == 0)
                    mne::Log::info("{}..", set.size());
            }
        }
    }
    if (!verbose)
        mne::Log::info("[done]");
    p_set = set;
    return true;
}

//=============================================================================================================

bool InvDipoleFit::fit_dipoles_raw(const std::string& dataname, MNERawData* raw, MNEChSelection* sel, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose)
{
    InvEcdSet set;
    return fit_dipoles_raw(dataname, raw, sel, fit, guess, tmin, tmax, tstep, integ, verbose, set);
}
