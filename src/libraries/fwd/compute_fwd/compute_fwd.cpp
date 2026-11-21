//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     compute_fwd.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2017
 * @brief    ComputeFwd implementation — orchestrates source-space, BEM/sphere model, coil-set and compensation initialisation, then assembles the lead-field matrix @c G column by column via the parallel dipole loop.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compute_fwd.h"

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <fstream>
#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

constexpr int FAIL = -1;
constexpr int OK = 0;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeFwd::ComputeFwd(std::shared_ptr<ComputeFwdSettings> pSettings)
: m_pSettings(pSettings)
{
    initFwd();
}

//=============================================================================================================

ComputeFwd::~ComputeFwd()
{
}

//=============================================================================================================

//=============================================================================================================

void ComputeFwd::initFwd()
{
    m_bInitialized = false;
    m_spaces.clear();
    m_iNSource = 0;

    m_mri_head_t = FiffCoordTrans();
    m_meg_head_t = FiffCoordTrans();

    m_listMegChs = std::vector<FiffChInfo>();
    m_listEegChs = std::vector<FiffChInfo>();
    m_listCompChs = std::vector<FiffChInfo>();

    int iNMeg = 0;
    int iNEeg = 0;
    int iNComp = 0;

    m_templates.reset();
    m_megcoils.reset();
    m_compcoils.reset();
    m_eegels.reset();
    m_eegModels.reset();
    m_iNChan = 0;

    int k;
    m_mri_id = FiffId();
    m_meas_id.clear();

    std::ofstream filteredFile;

    m_eegModel.reset();
    m_bemModel.reset();

    // Report the setup
    mne::Log::info("Source space                 : {}", m_pSettings->srcname);
    if (!(m_pSettings->transname.empty()) || !(m_pSettings->mriname.empty())) {
        mne::Log::info("MRI -> head transform source : {}", m_pSettings->mriname.empty() ? m_pSettings->transname : m_pSettings->mriname);
    } else {
        mne::Log::info("MRI and head coordinates are assumed to be identical.");
    }
    mne::Log::info("Measurement data             : {}", m_pSettings->measname);
    if (!m_pSettings->bemname.empty()) {
        mne::Log::info("BEM model                    : {}", m_pSettings->bemname);
    } else {
        mne::Log::info("Sphere model                 : origin at ({: 7.2f} {: 7.2f} {: 7.2f}) mm", 1000.0f * m_pSettings->r0[X], 1000.0f * m_pSettings->r0[Y], 1000.0f * m_pSettings->r0[Z]);
        if (m_pSettings->include_eeg) {
            if (m_pSettings->eeg_model_file.empty()) {
                mne::Log::warning("EEG model file not specified; using default.");
            }
            m_eegModels = FwdEegSphereModelSet::fwd_load_eeg_sphere_models(m_pSettings->eeg_model_file, std::move(m_eegModels));
            m_eegModels->fwd_list_eeg_sphere_models();

            if (m_pSettings->eeg_model_name.empty()) {
                m_pSettings->eeg_model_name = "Default";
            }
            m_eegModel = m_eegModels->fwd_select_eeg_sphere_model(m_pSettings->eeg_model_name);
            if (!m_eegModel) {
                return;
            }

            if (!m_eegModel->fwd_setup_eeg_sphere_model(m_pSettings->eeg_sphere_rad, m_pSettings->use_equiv_eeg, 3)) {
                return;
            }

            mne::Log::info("Using EEG sphere model \"{}\" with scalp radius {:7.1f} mm", m_pSettings->eeg_model_name, 1000 * m_pSettings->eeg_sphere_rad);
            mne::Log::info("{} the electrode locations to scalp", m_pSettings->scale_eeg_pos ? "Scale" : "Do not scale");

            m_eegModel->scale_pos = m_pSettings->scale_eeg_pos;
            m_eegModel->r0 = m_pSettings->r0;
        }
    }
    mne::Log::info("{} field computations", m_pSettings->accurate ? "Accurate" : "Standard");
    mne::Log::info("Do computations in {} coordinates.", std::string(FiffCoordTrans::frame_name(m_pSettings->coord_frame)));
    mne::Log::info("{} source orientations", m_pSettings->fixed_ori ? "Fixed" : "Free");
    if (m_pSettings->compute_grad) {
        mne::Log::info("Compute derivatives with respect to source location coordinates");
    }
    mne::Log::info("Destination for the solution : {}", m_pSettings->solname);
    if (m_pSettings->do_all) {
        mne::Log::info("Calculate solution for all source locations.");
    }
    if (m_pSettings->nlabel > 0) {
        mne::Log::info("Source space will be restricted to sources in {} labels", m_pSettings->nlabel);
    }

    // Read the source locations
    mne::Log::info("Reading {}...", m_pSettings->srcname);
    if (MNESourceSpace::read_source_spaces(mne::path_from_utf8(m_pSettings->srcname), m_spaces) != OK) {
        return;
    }
    for (k = 0, m_iNSource = 0; k < static_cast<int>(m_spaces.size()); k++) {
        if (m_pSettings->do_all) {
            m_spaces[k]->enable_all_sources();
        }
        m_iNSource += m_spaces[k]->nuse;
    }
    if (m_iNSource == 0) {
        mne::Log::critical("No sources are active in these source spaces. --all option should be used.");
        return;
    }
    mne::Log::info("Read {} source spaces a total of {} active source locations", static_cast<int>(m_spaces.size()), m_iNSource);
    if (MNESourceSpace::restrict_sources_to_labels(m_spaces, m_pSettings->labels, m_pSettings->nlabel) == FAIL) {
        return;
    }

    // Read the MRI -> head coordinate transformation
    if (!m_pSettings->mriname.empty()) {
        const auto mriHead = FiffCoordTrans::read(mne::path_from_utf8(m_pSettings->mriname), FIFFV_COORD_MRI, FIFFV_COORD_HEAD);
        if (!mriHead) {
            mne::Log::critical("{}", mriHead.error().what());
            return;
        }
        m_mri_head_t = *mriHead;
        auto mriStream = FiffStream::open(mne::path_from_utf8(m_pSettings->mriname));
        m_mri_id = mriStream ? mriStream->id() : FiffId();
        if (m_mri_id.empty()) {
            mne::Log::critical("Couldn't read MRI file id (How come?)");
            return;
        }
    } else if (!m_pSettings->transname.empty()) {
        const auto headMri = FiffCoordTrans::read_ascii(mne::path_from_utf8(m_pSettings->transname), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        if (!headMri) {
            mne::Log::critical("{}", headMri.error().what());
            return;
        }
        m_mri_head_t = headMri->inverted();
    } else {
        m_mri_head_t = FiffCoordTrans::identity(FIFFV_COORD_MRI, FIFFV_COORD_HEAD);
    }
    m_mri_head_t.print();

    // Read the channel information and the MEG device -> head coordinate transformation

    if (!m_pSettings->pFiffInfo) {
        auto pStream = FiffStream::open(mne::path_from_utf8(m_pSettings->measname));
        if (!pStream) {
            mne::Log::critical("{}", pStream.error().what());
            return;
        }
        std::shared_ptr<mne::FiffDirNode> DirNode;
        mne::FiffInfo fiffInfo;
        if (!pStream->read_meas_info(pStream->dirtree(), fiffInfo, DirNode)) {
            mne::Log::critical("Could not find the channel information.");
            return;
        }
        m_pInfoBase = std::make_shared<FiffInfo>(fiffInfo);
    } else {
        m_pInfoBase = m_pSettings->pFiffInfo;
    }
    if (!m_pInfoBase) {
        mne::Log::critical("ComputeFwd::initFwd(): no FiffInfo");
        return;
    }
    m_pInfoBase->mne_read_meg_comp_eeg_ch_info(m_listMegChs,
                                               iNMeg,
                                               m_listCompChs,
                                               iNComp,
                                               m_listEegChs,
                                               iNEeg,
                                               m_meg_head_t,
                                               m_meas_id);
    if (!m_pSettings->meg_head_t.empty()) {
        m_meg_head_t = m_pSettings->meg_head_t;
    }
    if (m_meg_head_t.empty()) {
        mne::Log::critical("MEG -> head coordinate transformation not found.");
        return;
    }

    m_iNChan = iNMeg + iNEeg;

    if (iNMeg > 0) {
        mne::Log::info("Read {:3} MEG channels from {}", iNMeg, m_pSettings->measname);
    }
    if (iNComp > 0) {
        mne::Log::info("Read {:3} MEG compensation channels from {}", iNComp, m_pSettings->measname);
    }
    if (iNEeg > 0) {
        mne::Log::info("Read {:3} EEG channels from {}", iNEeg, m_pSettings->measname);
    }
    if (!m_pSettings->include_meg) {
        mne::Log::info("MEG not requested. MEG channels omitted.");
        m_listMegChs.clear();
        m_listCompChs.clear();
        iNMeg = 0;
        iNComp = 0;
    } else
        m_meg_head_t.print();
    if (!m_pSettings->include_eeg) {
        mne::Log::info("EEG not requested. EEG channels omitted.");
        m_listEegChs.clear();
        iNEeg = 0;
    } else {
        if (!FiffChInfo::checkEegLocations(m_listEegChs, iNEeg)) {
            return;
        }
    }

    // Create coil descriptions with transformation to head or MRI frame

    if (m_pSettings->include_meg) {
        m_qPath = mne::resource_path("general/coilDefinitions/coil_def.dat");

        m_templates = FwdCoilSet::read_coil_defs(m_qPath);
        if (!m_templates) {
            return;
        }

        // Compensation data

        m_compData = MNECTFCompDataSet::read(mne::path_from_utf8(m_pSettings->measname));
        if (!m_compData) {
            return;
        }
        if (m_compData->ncomp > 0) {
            mne::Log::info("{} compensation data sets in {}", m_compData->ncomp, m_pSettings->measname);
        } else {
            m_listCompChs.clear();
            iNComp = 0;

            m_compData.reset();
        }
    }
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTrans head_mri_t = m_mri_head_t.inverted();
        FiffCoordTrans meg_mri_t = FiffCoordTrans::combine(FIFFV_COORD_DEVICE, FIFFV_COORD_MRI, m_meg_head_t, head_mri_t);
        if (meg_mri_t.empty()) {
            return;
        }
        m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                   iNMeg,
                                                   m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                   meg_mri_t);
        if (!m_megcoils) {
            return;
        }
        if (iNComp > 0) {
            m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                        iNComp,
                                                        FWD_COIL_ACCURACY_NORMAL,
                                                        meg_mri_t);
            if (!m_compcoils) {
                return;
            }
        }
        m_eegels = FwdCoilSet::create_eeg_els(m_listEegChs,
                                              iNEeg,
                                              head_mri_t);
        if (!m_eegels) {
            return;
        }

        mne::Log::info("MRI coordinate coil definitions created.");
    } else {
        m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                   iNMeg,
                                                   m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                   m_meg_head_t);
        if (!m_megcoils) {
            return;
        }

        if (iNComp > 0) {
            m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                        iNComp,
                                                        FWD_COIL_ACCURACY_NORMAL, m_meg_head_t);
            if (!m_compcoils) {
                return;
            }
        }
        m_eegels = FwdCoilSet::create_eeg_els(m_listEegChs,
                                              iNEeg);
        if (!m_eegels) {
            return;
        }
        mne::Log::info("Head coordinate coil definitions created.");
    }

    // Transform the source spaces into the appropriate coordinates
    {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame, m_mri_head_t, m_spaces) != OK) {
            return;
        }
    }
    mne::Log::info("Source spaces are now in {} coordinates.", std::string(FiffCoordTrans::frame_name(m_pSettings->coord_frame)));

    // Prepare the BEM model if necessary

    if (!m_pSettings->bemname.empty()) {
        m_pSettings->bemname = mne::utf8_from_path(FwdBemModel::fwd_bem_make_bem_sol_name(mne::path_from_utf8(m_pSettings->bemname)));

        mne::Log::info("Setting up the BEM model using {}...", m_pSettings->bemname);
        mne::Log::info("Loading surfaces...");
        m_bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(m_pSettings->bemname);

        if (m_bemModel) {
            mne::Log::info("Three-layer model surfaces loaded.");
        } else {
            m_bemModel = FwdBemModel::fwd_bem_load_homog_surface(m_pSettings->bemname);
            if (!m_bemModel) {
                return;
            }
            mne::Log::info("Homogeneous model surface loaded.");
        }
        if (iNEeg > 0 && m_bemModel->nsurf == 1) {
            mne::Log::critical("Cannot use a homogeneous model in EEG calculations.");
            return;
        }
        mne::Log::info("Loading the solution matrix...");
        if (m_bemModel->fwd_bem_load_recompute_solution(m_pSettings->bemname.c_str(), FWD_BEM_UNKNOWN, false) == FAIL) {
            return;
        }
        if (m_pSettings->coord_frame == FIFFV_COORD_HEAD) {
            mne::Log::info("Employing the head->MRI coordinate transform with the BEM model.");
            if (m_bemModel->fwd_bem_set_head_mri_t(m_mri_head_t) == FAIL) {
                return;
            }
        }
        mne::Log::info("BEM model {} is now set up", m_bemModel->sol_name);
    } else {
        mne::Log::info("Using the sphere model.");
        m_bemModel = std::make_unique<FwdBemModel>(); // no surfaces selects the sphere branch
    }

    // Try to circumvent numerical problems by excluding points too close or outside the inner skull surface

    if (m_pSettings->filter_spaces) {
        if (!m_pSettings->mindistoutname.empty()) {
            filteredFile.open(mne::path_from_utf8(m_pSettings->mindistoutname), std::ios::binary);
            if (!filteredFile) {
                mne::Log::critical("{}", m_pSettings->mindistoutname);
                return;
            }
            mne::Log::info("Omitted source space points will be output to : {}", m_pSettings->mindistoutname);
        }
        MNESourceSpace::filter_source_spaces(m_pSettings->mindist,
                                             mne::path_from_utf8(m_pSettings->bemname),
                                             m_mri_head_t,
                                             m_spaces,
                                             filteredFile.is_open() ? &filteredFile : nullptr, m_pSettings->use_threads);
    }
    m_bInitialized = true;
}

//=============================================================================================================

void ComputeFwd::populateMetadata(MNEForwardSolution& fwd)
{
    fwd.coord_frame = m_pSettings->coord_frame;
    fwd.source_ori = m_pSettings->fixed_ori ? FIFFV_MNE_FIXED_ORI : FIFFV_MNE_FREE_ORI;
    fwd.surf_ori = false;
    fwd.mri_filename = m_pSettings->mriname;

    int nmeg = m_megcoils ? m_megcoils->ncoil() : 0;
    int neeg = m_eegels ? m_eegels->ncoil() : 0;
    fwd.nchan = nmeg + neeg;

    fwd.mri_head_t = m_mri_head_t;
    fwd.mri_id = m_mri_id;

    // Source spaces: store in MRI frame (FIFF convention), keep m_spaces in computation frame
    {
        if (MNESourceSpace::transform_source_spaces_to(FIFFV_COORD_MRI, m_mri_head_t, m_spaces) != OK) {
            return;
        }
        fwd.src.clear();
        fwd.nsource = 0;
        for (int i = 0; i < static_cast<int>(m_spaces.size()); ++i) {
            fwd.src.append(*m_spaces[i]);
            fwd.nsource += m_spaces[i]->nuse;
        }
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame, m_mri_head_t, m_spaces) != OK) {
            return;
        }
    }

    // Source locations and orientations in the computation frame, as MNEForwardSolution::read sets them.
    const int nOri = m_pSettings->fixed_ori ? 1 : 3;
    fwd.source_rr = MatrixX3f(fwd.nsource, 3);
    fwd.source_nn = MatrixX3f(nOri * fwd.nsource, 3);
    for (int i = 0, p = 0; i < static_cast<int>(m_spaces.size()); ++i) {
        const MNESourceSpace& s = *m_spaces[i];
        for (int k = 0; k < s.nuse; ++k, ++p) {
            fwd.source_rr.row(p) = s.rr.row(s.vertno(k));
            if (nOri == 1) {
                fwd.source_nn.row(p) = s.nn.row(s.vertno(k));
            } else {
                fwd.source_nn.middleRows(3 * p, 3) = Matrix3f::Identity();
            }
        }
    }

    // Measurement provenance
    fwd.info.filename = m_pSettings->measname;
    fwd.info.meas_id = m_meas_id;
    fwd.info.dev_head_t = m_meg_head_t;
    fwd.info.nchan = fwd.nchan;

    fwd.info.chs.clear();
    fwd.info.ch_names.clear();
    for (int i = 0; i < nmeg; ++i) {
        fwd.info.chs.push_back(m_listMegChs[i]);
        fwd.info.ch_names.push_back(m_listMegChs[i].ch_name);
    }
    for (int i = 0; i < neeg; ++i) {
        fwd.info.chs.push_back(m_listEegChs[i]);
        fwd.info.ch_names.push_back(m_listEegChs[i].ch_name);
    }

    // Bad channels
    if (!m_pSettings->measname.empty()) {
        if (auto badStream = FiffStream::open(mne::path_from_utf8(m_pSettings->measname)))
            fwd.info.bads = badStream->read_bad_channels(badStream->dirtree());
    }
}

//=============================================================================================================

std::unique_ptr<MNEForwardSolution> ComputeFwd::calculateFwd()
{
    if (!m_bInitialized) {
        mne::Log::critical("ComputeFwd::calculateFwd - the forward computation could not be set up.");
        return nullptr;
    }
    auto fwdSolution = std::make_unique<MNEForwardSolution>();
    populateMetadata(*fwdSolution);
    int iNMeg = 0;
    int iNEeg = 0;

    if (m_megcoils) {
        iNMeg = m_megcoils->ncoil();
    }
    if (m_eegels) {
        iNEeg = m_eegels->ncoil();
    }
    if (!m_bemModel || m_bemModel->nsurf == 0) {
        m_pSettings->use_threads = false;
    }

    // check if source spaces are still in computation frame
    if (m_spaces[0]->coord_frame != m_pSettings->coord_frame) {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame, m_mri_head_t, m_spaces) != OK) {
            return nullptr;
        }
    }

    // Do the actual computation
    if (iNMeg > 0) {
        if ((m_bemModel->compute_forward_meg(m_spaces,
                                             m_megcoils.get(),
                                             m_compcoils.get(),
                                             m_compData.get(),
                                             m_pSettings->fixed_ori,
                                             m_pSettings->r0,
                                             m_pSettings->use_threads,
                                             m_meg_forward,
                                             m_meg_forward_grad,
                                             m_pSettings->compute_grad)) == FAIL) {
            return nullptr;
        }
    }
    if (iNEeg > 0) {
        if ((m_bemModel->compute_forward_eeg(m_spaces,
                                             m_eegels.get(),
                                             m_pSettings->fixed_ori,
                                             m_eegModel.get(),
                                             m_pSettings->use_threads,
                                             m_eeg_forward,
                                             m_eeg_forward_grad,
                                             m_pSettings->compute_grad)) == FAIL) {
            return nullptr;
        }
    }

    // Assemble combined sol
    if (iNMeg > 0 && iNEeg > 0) {
        if (m_meg_forward.data.cols() != m_eeg_forward.data.cols()) {
            mne::Log::warning("The MEG and EEG forward solutions do not match");
            return nullptr;
        }
        fwdSolution->sol.clear();
        fwdSolution->sol.nrow = m_meg_forward.nrow + m_eeg_forward.nrow;
        fwdSolution->sol.ncol = m_meg_forward.ncol;
        fwdSolution->sol.data = MatrixXd(fwdSolution->sol.nrow, fwdSolution->sol.ncol);
        fwdSolution->sol.data.block(0, 0, m_meg_forward.nrow, m_meg_forward.ncol) = m_meg_forward.data;
        fwdSolution->sol.data.block(m_meg_forward.nrow, 0, m_eeg_forward.nrow, m_eeg_forward.ncol) = m_eeg_forward.data;
        fwdSolution->sol.row_names = m_meg_forward.row_names;
        fwdSolution->sol.row_names.insert(fwdSolution->sol.row_names.end(), m_eeg_forward.row_names.begin(), m_eeg_forward.row_names.end());
        fwdSolution->sol.col_names = m_meg_forward.col_names;
    } else if (iNMeg > 0) {
        fwdSolution->sol = m_meg_forward;
    } else {
        fwdSolution->sol = m_eeg_forward;
    }

    if (m_pSettings->compute_grad) {
        if (iNMeg > 0 && iNEeg > 0) {
            if (m_meg_forward_grad.data.cols() != m_eeg_forward_grad.data.cols()) {
                mne::Log::warning("The MEG and EEG forward solutions do not match");
                return nullptr;
            }
            fwdSolution->sol_grad.clear();
            fwdSolution->sol_grad.nrow = m_meg_forward_grad.nrow + m_eeg_forward_grad.nrow;
            fwdSolution->sol_grad.ncol = m_meg_forward_grad.ncol;
            fwdSolution->sol_grad.data = MatrixXd(fwdSolution->sol_grad.nrow, fwdSolution->sol_grad.ncol);
            fwdSolution->sol_grad.data.block(0, 0, m_meg_forward_grad.nrow, m_meg_forward_grad.ncol) = m_meg_forward_grad.data;
            fwdSolution->sol_grad.data.block(m_meg_forward_grad.nrow, 0, m_eeg_forward_grad.nrow, m_eeg_forward_grad.ncol) = m_eeg_forward_grad.data;
            fwdSolution->sol_grad.row_names = m_meg_forward_grad.row_names;
            fwdSolution->sol_grad.row_names.insert(fwdSolution->sol_grad.row_names.end(), m_eeg_forward_grad.row_names.begin(), m_eeg_forward_grad.row_names.end());
            fwdSolution->sol_grad.col_names = m_meg_forward_grad.col_names;
        } else if (iNMeg > 0) {
            fwdSolution->sol_grad = m_meg_forward_grad;
        } else {
            fwdSolution->sol_grad = m_eeg_forward_grad;
        }
    }

    return fwdSolution;
}

//=============================================================================================================

bool ComputeFwd::updateHeadPos(const FiffCoordTrans& transDevHead, MNEForwardSolution& fwd)
{
    if (!m_bInitialized)
        return false;
    int iNMeg = 0;
    if (m_megcoils) {
        iNMeg = m_megcoils->ncoil();
    }

    int iNComp = 0;
    if (m_compcoils) {
        iNComp = m_compcoils->ncoil();
    }

    // create new coilset with updated head position
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTrans head_mri_t = m_mri_head_t.inverted();
        FiffCoordTrans meg_mri_t = FiffCoordTrans::combine(FIFFV_COORD_DEVICE, FIFFV_COORD_MRI, transDevHead, head_mri_t);
        if (meg_mri_t.empty()) {
            return false;
        }
        m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                   iNMeg,
                                                   m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                   meg_mri_t);
        if (!m_megcoils) {
            return false;
        }
        if (iNComp > 0) {
            m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                        iNComp,
                                                        FWD_COIL_ACCURACY_NORMAL,
                                                        meg_mri_t);
            if (!m_compcoils) {
                return false;
            }
        }
    } else {
        m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                   iNMeg,
                                                   m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                   transDevHead);
        if (!m_megcoils) {
            return false;
        }

        if (iNComp > 0) {
            m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                        iNComp,
                                                        FWD_COIL_ACCURACY_NORMAL, transDevHead);
            if (!m_compcoils) {
                return false;
            }
        }
    }

    // check if source spaces are still in computation frame
    if (m_spaces[0]->coord_frame != m_pSettings->coord_frame) {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame, m_mri_head_t, m_spaces) != OK) {
            return false;
        }
    }

    // recompute meg forward
    if ((m_bemModel->compute_forward_meg(m_spaces,
                                         m_megcoils.get(),
                                         m_compcoils.get(),
                                         m_compData.get(),
                                         m_pSettings->fixed_ori,
                                         m_pSettings->r0,
                                         m_pSettings->use_threads,
                                         m_meg_forward,
                                         m_meg_forward_grad,
                                         m_pSettings->compute_grad)) == FAIL) {
        return false;
    }

    // Update transformation matrix and info
    m_meg_head_t = transDevHead;
    fwd.info.dev_head_t = transDevHead;

    // update solution
    fwd.sol.data.block(0, 0, m_meg_forward.nrow, m_meg_forward.ncol) = m_meg_forward.data;
    if (m_pSettings->compute_grad) {
        fwd.sol_grad.data.block(0, 0, m_meg_forward_grad.nrow, m_meg_forward_grad.ncol) = m_meg_forward_grad.data;
    }

    return true;
}
