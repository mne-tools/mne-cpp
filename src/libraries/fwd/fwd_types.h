//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    std::function aliases for the generic dipole field / potential / field-gradient callbacks driving the source-space loop.
 *
 * The compute_forward driver iterates over every dipole position in a
 * source space and, at each step, calls a *field function* and an
 * optional *gradient function*. The field function evaluates the scalar
 * magnetic flux (MEG) or electric potential (EEG) produced at a sensor
 * for a given dipole moment Q, while the vector variant emits the full
 * 3 × N_coil lead-field block in one call. The gradient function adds
 * the spatial derivatives ∂B/∂r needed by signal-space-separation and
 * Levenberg-Marquardt dipole fitting.
 *
 * Wrapping all three behind std::function lets the solver swap an
 * infinite-medium analytic kernel (e.g. Sarvas), a BEM kernel, or a
 * compensation-aware wrapper (FwdCompData) without templating the call
 * sites; this matches the function-pointer dispatch used in MNE-C
 * @c compute_forward.c.
 */

#ifndef FWD_TYPES_H
#define FWD_TYPES_H

#include <fiff/fiff_types.h>
#include <mne/mne_types.h>

#include "fwd_coil_set.h"

#include <mne/mne_ctf_comp_data_set.h>

#include <Eigen/Core>

#include <functional>

/*
 * Convenient generic field / potential computation functions
 */
using fwdFieldFunc = std::function<int(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                       FWDLIB::FwdCoilSet& coils, Eigen::Ref<Eigen::VectorXf> res, void *client)>;
using fwdVecFieldFunc = std::function<int(const Eigen::Vector3f& rd,
                                          FWDLIB::FwdCoilSet& coils, Eigen::Ref<Eigen::MatrixXf> res, void *client)>;
using fwdFieldGradFunc = std::function<int(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q,
                                           FWDLIB::FwdCoilSet& coils, Eigen::Ref<Eigen::VectorXf> res,
                                           Eigen::Ref<Eigen::VectorXf> xgrad, Eigen::Ref<Eigen::VectorXf> ygrad, Eigen::Ref<Eigen::VectorXf> zgrad, void *client)>;

#endif // FWD_TYPES_H
