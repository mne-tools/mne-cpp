//=============================================================================================================
/**
 * @file     compute_fwd.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Ruben Dörfel <ruben.deorfel@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    ComputeFwd class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compute_fwd.h"
#include <mne/mne_forward_solution.h>

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

constexpr int FAIL = -1;
constexpr int OK   =  0;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeFwd::ComputeFwd(std::shared_ptr<ComputeFwdSettings> pSettings)
    : m_meg_forward(new FiffNamedMatrix)
    , m_meg_forward_grad(new FiffNamedMatrix)
    , m_eeg_forward(new FiffNamedMatrix)
    , m_eeg_forward_grad(new FiffNamedMatrix)
    , m_pSettings(pSettings)
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
    m_spaces.clear();
    m_iNSource              = 0;

    m_mri_head_t            = FiffCoordTrans();
    m_meg_head_t            = FiffCoordTrans();

    m_listMegChs = QList<FiffChInfo>();
    m_listEegChs = QList<FiffChInfo>();
    m_listCompChs = QList<FiffChInfo>();

    int iNMeg               = 0;
    int iNEeg               = 0;
    int iNComp              = 0;

    m_templates.reset();
    m_megcoils.reset();
    m_compcoils.reset();
    m_eegels.reset();
    m_eegModels.reset();
    m_iNChan                = 0;

    int k;
    m_mri_id                = FiffId();
    m_meas_id.clear();

    QFile filteredFile;
    QTextStream *filteredStream = nullptr;

    m_eegModel.reset();
    m_bemModel.reset();

    // Report the setup
    qInfo("Source space                 : %s",m_pSettings->srcname.toUtf8().constData());
    if (!(m_pSettings->transname.isEmpty()) || !(m_pSettings->mriname.isEmpty())) {
        qInfo("MRI -> head transform source : %s",!(m_pSettings->mriname.isEmpty()) ? m_pSettings->mriname.toUtf8().constData() : m_pSettings->transname.toUtf8().constData());
    } else {
        qInfo("MRI and head coordinates are assumed to be identical.");
    }
    qInfo("Measurement data             : %s",m_pSettings->measname.toUtf8().constData());
    if (!m_pSettings->bemname.isEmpty()) {
        qInfo("BEM model                    : %s",m_pSettings->bemname.toUtf8().constData());
    } else {
        qInfo("Sphere model                 : origin at (% 7.2f % 7.2f % 7.2f) mm",
               1000.0f*m_pSettings->r0[X],1000.0f*m_pSettings->r0[Y],1000.0f*m_pSettings->r0[Z]);
        if (m_pSettings->include_eeg) {

            if (m_pSettings->eeg_model_file.isEmpty()) {
                qWarning("EEG model file not specified; using default.");
            }
            m_eegModels.reset(FwdEegSphereModelSet::fwd_load_eeg_sphere_models(m_pSettings->eeg_model_file,m_eegModels.release()));
            m_eegModels->fwd_list_eeg_sphere_models();

            if (m_pSettings->eeg_model_name.isEmpty()) {
                m_pSettings->eeg_model_name = QString("Default");
            }
            m_eegModel.reset(m_eegModels->fwd_select_eeg_sphere_model(m_pSettings->eeg_model_name));
            if (!m_eegModel) {
                return;
            }

            if (!m_eegModel->fwd_setup_eeg_sphere_model(m_pSettings->eeg_sphere_rad,m_pSettings->use_equiv_eeg,3)) {
                return;
            }

            qInfo("Using EEG sphere model \"%s\" with scalp radius %7.1f mm",
                   m_pSettings->eeg_model_name.toUtf8().constData(),1000*m_pSettings->eeg_sphere_rad);
            qInfo("%s the electrode locations to scalp",m_pSettings->scale_eeg_pos ? "Scale" : "Do not scale");

            m_eegModel->scale_pos = m_pSettings->scale_eeg_pos;
            m_eegModel->r0 = m_pSettings->r0;
        }
    }
    qInfo("%s field computations",m_pSettings->accurate ? "Accurate" : "Standard");
    qInfo("Do computations in %s coordinates.",FiffCoordTrans::frame_name(m_pSettings->coord_frame).toUtf8().constData());
    qInfo("%s source orientations",m_pSettings->fixed_ori ? "Fixed" : "Free");
    if (m_pSettings->compute_grad) {
        qInfo("Compute derivatives with respect to source location coordinates");
    }
    qInfo("Destination for the solution : %s",m_pSettings->solname.toUtf8().constData());
    if (m_pSettings->do_all) {
        qInfo("Calculate solution for all source locations.");
    }
    if (m_pSettings->nlabel > 0) {
        qInfo("Source space will be restricted to sources in %d labels",m_pSettings->nlabel);
    }

    // Read the source locations
    qInfo("Reading %s...",m_pSettings->srcname.toUtf8().constData());
    if (MNESourceSpace::read_source_spaces(m_pSettings->srcname,m_spaces) != OK) {
        return;
    }
    for (k = 0, m_iNSource = 0; k < static_cast<int>(m_spaces.size()); k++) {
        if (m_pSettings->do_all) {
            m_spaces[k]->enable_all_sources();
        }
        m_iNSource += m_spaces[k]->nuse;
    }
    if (m_iNSource == 0) {
        qCritical("No sources are active in these source spaces. --all option should be used.");
        return;
    }
    qInfo("Read %d source spaces a total of %d active source locations", static_cast<int>(m_spaces.size()),m_iNSource);
    if (MNESourceSpace::restrict_sources_to_labels(m_spaces,m_pSettings->labels,m_pSettings->nlabel) == FAIL) {
        return;
    }

    // Read the MRI -> head coordinate transformation
    if (!m_pSettings->mriname.isEmpty()) {
        m_mri_head_t = FiffCoordTrans::readMriTransform(m_pSettings->mriname);
        if (m_mri_head_t.isEmpty()) {
            return;
        }
        {
            QFile mriFile(m_pSettings->mriname);
            FiffStream::SPtr mriStream(new FiffStream(&mriFile));
            if (mriStream->open()) {
                m_mri_id.version   = mriStream->id().version;
                m_mri_id.machid[0] = mriStream->id().machid[0];
                m_mri_id.machid[1] = mriStream->id().machid[1];
                m_mri_id.time      = mriStream->id().time;
                mriStream->close();
            } else {
                mriStream->close();
                m_mri_id = FiffId();
            }
        }
        if (m_mri_id.isEmpty()) {
            qCritical("Couldn't read MRI file id (How come?)");
            return;
        }
    }
    else if (!m_pSettings->transname.isEmpty()) {
        FiffCoordTrans t = FiffCoordTrans::readFShead2mriTransform(m_pSettings->transname.toUtf8().data());
        if (t.isEmpty()) {
            return;
        }
        m_mri_head_t = t.inverted();
    } else {
        m_mri_head_t = FiffCoordTrans::identity(FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
    }
    m_mri_head_t.print();

    // Read the channel information and the MEG device -> head coordinate transformation

    if(!m_pSettings->pFiffInfo) {

        QFile measname(m_pSettings->measname);
        FIFFLIB::FiffDirNode::SPtr DirNode;
        FiffStream::SPtr pStream(new FiffStream(&measname));
        FIFFLIB::FiffInfo fiffInfo;
        if(!pStream->open()) {
            qCritical() << "Could not open Stream.";
            return;
        }

        if(!pStream->read_meas_info(pStream->dirtree(), fiffInfo, DirNode)){
            qCritical() << "Could not find the channel information.";
            return;
        }
        pStream->close();
        m_pInfoBase = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(fiffInfo));
    } else {
        m_pInfoBase = m_pSettings->pFiffInfo;
    }
    if(!m_pInfoBase) {
        qCritical ("ComputeFwd::initFwd(): no FiffInfo");
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
    if (!m_pSettings->meg_head_t.isEmpty()) {
        m_meg_head_t = m_pSettings->meg_head_t;
    }
    if (m_meg_head_t.isEmpty()) {
        qCritical("MEG -> head coordinate transformation not found.");
        return;
    }

    m_iNChan = iNMeg + iNEeg;

    if (iNMeg > 0) {
        qInfo("Read %3d MEG channels from %s",iNMeg,m_pSettings->measname.toUtf8().constData());
    }
    if (iNComp > 0) {
        qInfo("Read %3d MEG compensation channels from %s",iNComp,m_pSettings->measname.toUtf8().constData());
    }
    if (iNEeg > 0) {
        qInfo("Read %3d EEG channels from %s",iNEeg,m_pSettings->measname.toUtf8().constData());
    }
    if (!m_pSettings->include_meg) {
        qInfo("MEG not requested. MEG channels omitted.");
        m_listMegChs.clear();
        m_listCompChs.clear();
        iNMeg = 0;
        iNComp = 0;
    }
    else
        m_meg_head_t.print();
    if (!m_pSettings->include_eeg) {
        qInfo("EEG not requested. EEG channels omitted.");
        m_listEegChs.clear();
        iNEeg = 0;
    } else {
        if (!FiffChInfo::checkEegLocations(m_listEegChs, iNEeg)) {
            return;
        }
    }

    // Create coil descriptions with transformation to head or MRI frame

    if (m_pSettings->include_meg) {
        m_qPath = QString(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
        if ( !QCoreApplication::startingUp() ) {
            m_qPath = QCoreApplication::applicationDirPath() + QString("/../resources/general/coilDefinitions/coil_def.dat");
        } else if (!QFile::exists(m_qPath)) {
            m_qPath = "../resources/general/coilDefinitions/coil_def.dat";
        }

        m_templates = FwdCoilSet::read_coil_defs(m_qPath);
        if (!m_templates) {
            return;
        }

        // Compensation data

        m_compData = MNECTFCompDataSet::read(m_pSettings->measname);
        if (!m_compData) {
            return;
        }
        if (m_compData->ncomp > 0) {
            qInfo("%d compensation data sets in %s",m_compData->ncomp,m_pSettings->measname.toUtf8().constData());
        } else {
            m_listCompChs.clear();
            iNComp = 0;

            m_compData.reset();
        }
    }
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTrans head_mri_t = m_mri_head_t.inverted();
        FiffCoordTrans meg_mri_t = FiffCoordTrans::combine(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,m_meg_head_t,head_mri_t);
        if (meg_mri_t.isEmpty()) {
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

        qInfo("MRI coordinate coil definitions created.");
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
                                                             FWD_COIL_ACCURACY_NORMAL,m_meg_head_t);
            if (!m_compcoils) {
                return;
            }
        }
        m_eegels = FwdCoilSet::create_eeg_els(m_listEegChs,
                                                   iNEeg);
        if (!m_eegels) {
            return;
        }
        qInfo("Head coordinate coil definitions created.");
    }

    // Transform the source spaces into the appropriate coordinates
    {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces) != OK) {
            return;
        }
    }
    qInfo("Source spaces are now in %s coordinates.",FiffCoordTrans::frame_name(m_pSettings->coord_frame).toUtf8().constData());

    // Prepare the BEM model if necessary

    if (!m_pSettings->bemname.isEmpty()) {
        QString bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(m_pSettings->bemname);
        m_pSettings->bemname = bemsolname;

        qInfo("Setting up the BEM model using %s...",m_pSettings->bemname.toUtf8().constData());
        qInfo("Loading surfaces...");
        m_bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(m_pSettings->bemname);

        if (m_bemModel) {
            qInfo("Three-layer model surfaces loaded.");
        }
        else {
            m_bemModel = FwdBemModel::fwd_bem_load_homog_surface(m_pSettings->bemname);
            if (!m_bemModel) {
                return;
            }
            qInfo("Homogeneous model surface loaded.");
        }
        if (iNEeg > 0 && m_bemModel->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            return;
        }
        qInfo("Loading the solution matrix...");
        if (m_bemModel->fwd_bem_load_recompute_solution(m_pSettings->bemname.toUtf8().data(),FWD_BEM_UNKNOWN,false) == FAIL) {
            return;
        }
        if (m_pSettings->coord_frame == FIFFV_COORD_HEAD) {
            qInfo("Employing the head->MRI coordinate transform with the BEM model.");
            if (m_bemModel->fwd_bem_set_head_mri_t(m_mri_head_t) == FAIL) {
                return;
            }
        }
        qInfo("BEM model %s is now set up",m_bemModel->sol_name.toUtf8().constData());
    } else {
        qInfo("Using the sphere model.");
    }

    // Try to circumvent numerical problems by excluding points too close or outside the inner skull surface

    if (m_pSettings->filter_spaces) {
        if (!m_pSettings->mindistoutname.isEmpty()) {
            filteredFile.setFileName(m_pSettings->mindistoutname);
            if (!filteredFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qCritical() << m_pSettings->mindistoutname;
                return;
            }
            filteredStream = new QTextStream(&filteredFile);
            qInfo("Omitted source space points will be output to : %s",m_pSettings->mindistoutname.toUtf8().constData());
        }
        MNESourceSpace::filter_source_spaces(m_pSettings->mindist,
                                                 m_pSettings->bemname,
                                                 m_mri_head_t,
                                                 m_spaces,
                                                 filteredStream,m_pSettings->use_threads);
        delete filteredStream;
        filteredStream = nullptr;
    }
}

//=============================================================================================================

void ComputeFwd::populateMetadata(MNEForwardSolution& fwd)
{
    fwd.coord_frame  = m_pSettings->coord_frame;
    fwd.source_ori   = m_pSettings->fixed_ori ? FIFFV_MNE_FIXED_ORI : FIFFV_MNE_FREE_ORI;
    fwd.surf_ori     = false;
    fwd.mri_filename = m_pSettings->mriname;

    int nmeg = m_megcoils ? m_megcoils->ncoil() : 0;
    int neeg = m_eegels  ? m_eegels->ncoil()   : 0;
    fwd.nchan = nmeg + neeg;

    fwd.mri_head_t = m_mri_head_t;
    fwd.mri_id     = m_mri_id;

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

    // Measurement provenance
    fwd.info.filename   = m_pSettings->measname;
    fwd.info.meas_id    = m_meas_id;
    fwd.info.dev_head_t = m_meg_head_t;
    fwd.info.nchan      = fwd.nchan;

    fwd.info.chs.clear();
    fwd.info.ch_names.clear();
    for (int i = 0; i < nmeg; ++i) {
        fwd.info.chs.append(m_listMegChs[i]);
        fwd.info.ch_names.append(m_listMegChs[i].ch_name);
    }
    for (int i = 0; i < neeg; ++i) {
        fwd.info.chs.append(m_listEegChs[i]);
        fwd.info.ch_names.append(m_listEegChs[i].ch_name);
    }

    // Bad channels
    if (!m_pSettings->measname.isEmpty()) {
        QFile fileBad(m_pSettings->measname);
        FiffStream::SPtr t_pStreamBads(new FiffStream(&fileBad));
        if (t_pStreamBads->open()) {
            fwd.info.bads = t_pStreamBads->read_bad_channels(t_pStreamBads->dirtree());
            t_pStreamBads->close();
        }
    }
}

//=============================================================================================================

std::unique_ptr<MNEForwardSolution> ComputeFwd::calculateFwd()
{
    auto fwdSolution = std::make_unique<MNEForwardSolution>();
    populateMetadata(*fwdSolution);
    int iNMeg = 0;
    int iNEeg = 0;

    if(m_megcoils) {
        iNMeg = m_megcoils->ncoil();
    }
    if(m_eegels) {
        iNEeg = m_eegels->ncoil();
    }
    if (!m_bemModel) {
        m_pSettings->use_threads = false;
    }

    // check if source spaces are still in computation frame
    if(m_spaces[0]->coord_frame != m_pSettings->coord_frame) {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces) != OK) {
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
                                              *m_meg_forward.data(),
                                              *m_meg_forward_grad.data(),
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
                                              *m_eeg_forward.data(),
                                              *m_eeg_forward_grad.data(),
                                              m_pSettings->compute_grad))== FAIL) {
            return nullptr;
        }
    }

    // Assemble combined sol
    if(iNMeg > 0 && iNEeg > 0) {
        if(m_meg_forward->data.cols() != m_eeg_forward->data.cols()) {
            qWarning() << "The MEG and EEG forward solutions do not match";
            return nullptr;
        }
        fwdSolution->sol->clear();
        fwdSolution->sol->nrow = m_meg_forward->nrow + m_eeg_forward->nrow;
        fwdSolution->sol->ncol = m_meg_forward->ncol;
        fwdSolution->sol->data = MatrixXd(fwdSolution->sol->nrow, fwdSolution->sol->ncol);
        fwdSolution->sol->data.block(0,0,m_meg_forward->nrow,m_meg_forward->ncol) = m_meg_forward->data;
        fwdSolution->sol->data.block(m_meg_forward->nrow,0,m_eeg_forward->nrow,m_eeg_forward->ncol) = m_eeg_forward->data;
        fwdSolution->sol->row_names = m_meg_forward->row_names;
        fwdSolution->sol->row_names.append(m_eeg_forward->row_names);
        fwdSolution->sol->col_names = m_meg_forward->col_names;
    } else if (iNMeg > 0) {
        fwdSolution->sol = m_meg_forward;
    } else {
        fwdSolution->sol = m_eeg_forward;
    }

    if(m_pSettings->compute_grad) {
        if(iNMeg > 0 && iNEeg > 0) {
            if(m_meg_forward_grad->data.cols() != m_eeg_forward_grad->data.cols()) {
                qWarning() << "The MEG and EEG forward solutions do not match";
                return nullptr;
            }
            fwdSolution->sol_grad->clear();
            fwdSolution->sol_grad->nrow = m_meg_forward_grad->nrow + m_eeg_forward_grad->nrow;
            fwdSolution->sol_grad->ncol = m_meg_forward_grad->ncol;
            fwdSolution->sol_grad->data = MatrixXd(fwdSolution->sol_grad->nrow, fwdSolution->sol_grad->ncol);
            fwdSolution->sol_grad->data.block(0,0,m_meg_forward_grad->nrow,m_meg_forward_grad->ncol) = m_meg_forward_grad->data;
            fwdSolution->sol_grad->data.block(m_meg_forward_grad->nrow,0,m_eeg_forward_grad->nrow,m_eeg_forward_grad->ncol) = m_eeg_forward_grad->data;
            fwdSolution->sol_grad->row_names = m_meg_forward_grad->row_names;
            fwdSolution->sol_grad->row_names.append(m_eeg_forward_grad->row_names);
            fwdSolution->sol_grad->col_names = m_meg_forward_grad->col_names;
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
    int iNMeg = 0;
    if(m_megcoils) {
        iNMeg = m_megcoils->ncoil();
    }

    int iNComp = 0;
    if(m_compcoils) {
        iNComp = m_compcoils->ncoil();
    }

    // create new coilset with updated head position
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTrans head_mri_t = m_mri_head_t.inverted();
        FiffCoordTrans meg_mri_t = FiffCoordTrans::combine(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,transDevHead,head_mri_t);
        if (meg_mri_t.isEmpty()) {
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
                                                             FWD_COIL_ACCURACY_NORMAL,transDevHead);
            if (!m_compcoils) {
                return false;
            }
        }
    }

    // check if source spaces are still in computation frame
    if(m_spaces[0]->coord_frame != m_pSettings->coord_frame) {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces) != OK) {
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
                                          *m_meg_forward.data(),
                                          *m_meg_forward_grad.data(),
                                          m_pSettings->compute_grad)) == FAIL) {
        return false;
    }

    // Update transformation matrix and info
    m_meg_head_t = transDevHead;
    fwd.info.dev_head_t = transDevHead;

    // update solution
    fwd.sol->data.block(0,0,m_meg_forward->nrow,m_meg_forward->ncol) = m_meg_forward->data;
    if(m_pSettings->compute_grad) {
        fwd.sol_grad->data.block(0,0,m_meg_forward_grad->nrow,m_meg_forward_grad->ncol) = m_meg_forward_grad->data;
    }

    return true;
}
