
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
 * @brief    Definition of the ComputeFwd class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compute_fwd.h"

#include <fiff/fiff.h>
#include <fiff/fiff_coord_trans.h>
#include "../fwd_coil_set.h"
#include "../fwd_comp_data.h"
#include <mne/mne_ctf_comp_data_set.h>
#include "../fwd_eeg_sphere_model_set.h"
#include "../fwd_bem_model.h"

#include <mne/mne_nearest.h>
#include <mne/mne_source_space.h>
#include <mne/mne_forward_solution.h>

#include <fiff/fiff_sparse_matrix.h>

#include <fiff/fiff_types.h>

//=============================================================================================================
// SYSTEM INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

constexpr int FAIL = -1;
constexpr int OK   =  0;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;



//=============================================================================================================
// LOCAL HELPER FUNCTIONS
//=============================================================================================================

//=============================================================================================================

/**
 * Read channel information from FiffInfoBase and split into MEG, MEG compensation, and EEG channel lists.
 *
 * @param[in]     pFiffInfoBase  The FiffInfoBase containing channel information.
 * @param[in,out] listMegCh      Populated with MEG channels.
 * @param[in,out] iNMeg          Set to the number of MEG channels found.
 * @param[in,out] listMegComp    Populated with MEG compensation channels.
 * @param[in,out] iNMegCmp       Set to the number of compensation channels found.
 * @param[in,out] listEegCh      Populated with EEG channels.
 * @param[in,out] iNEeg          Set to the number of EEG channels found.
 * @param[in,out] transDevHead   Set to the device-to-head transformation.
 * @param[in,out] id             Set to the measurement ID.
 * @return OK on success, FAIL on error.
 */
int ComputeFwd::mne_read_meg_comp_eeg_ch_info_41(FIFFLIB::FiffInfoBase::SPtr pFiffInfoBase,
                                                 QList<FiffChInfo>& listMegCh,
                                                 int& iNMeg,
                                                 QList<FiffChInfo>& listMegComp,
                                                 int& iNMegCmp,
                                                 QList<FiffChInfo>& listEegCh,
                                                 int &iNEeg,
                                                 FiffCoordTrans& transDevHead,
                                                 FiffId& id)
{
    int iNumCh = pFiffInfoBase->nchan;
    for (int k = 0; k < iNumCh; k++) {
        if (pFiffInfoBase->chs[k].kind == FIFFV_MEG_CH) {
            listMegCh.append(pFiffInfoBase->chs[k]);
            iNMeg++;
        } else if (pFiffInfoBase->chs[k].kind == FIFFV_REF_MEG_CH) {
            listMegComp.append(pFiffInfoBase->chs[k]);
            iNMegCmp++;
        } else if (pFiffInfoBase->chs[k].kind == FIFFV_EEG_CH) {
            listEegCh.append(pFiffInfoBase->chs[k]);
            iNEeg++;
        }
    }

    if(m_pSettings->meg_head_t.isEmpty()) {
        transDevHead = pFiffInfoBase->dev_head_t;
    } else {
        transDevHead = m_pSettings->meg_head_t;
    }
    if(m_meg_head_t.isEmpty()) {
        qCritical("MEG -> head coordinate transformation not found.");
        return FIFF_FAIL;
    }
    id = pFiffInfoBase->meas_id;
    return OK;
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

/**
 * Constructs a ComputeFwd object and initializes the forward computation.
 *
 * @param[in] pSettings   Shared pointer to the forward computation settings.
 */
ComputeFwd::ComputeFwd(ComputeFwdSettings::SPtr pSettings)
    : sol(new FiffNamedMatrix)
    , sol_grad(new FiffNamedMatrix)
    , m_meg_forward(new FiffNamedMatrix)
    , m_meg_forward_grad(new FiffNamedMatrix)
    , m_eeg_forward(new FiffNamedMatrix)
    , m_eeg_forward_grad(new FiffNamedMatrix)
    , m_pSettings(pSettings)
{
    initFwd();
}

//=============================================================================================================

/**
 * Destroys the ComputeFwd object.
 */
ComputeFwd::~ComputeFwd()
{
}

//=============================================================================================================

/**
 * Initialize the forward computation.
 *
 * Reads source spaces, coordinate transforms, channel information, coil definitions,
 * compensation data, BEM model (or sphere model parameters), and prepares all data
 * structures required for forward calculation.
 */
void ComputeFwd::initFwd()
{
    // TODO: This only temporary until we have the fwd dlibrary refactored. This is only done in order to provide easy testing in test_forward_solution.
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

    QFile filteredFile;                       /**< Output filtered points here. */
    QTextStream *filteredStream = Q_NULLPTR;   /**< Text stream for filtered output. */

    m_eegModel.reset();
    m_bemModel.reset();

    // Report the setup
    printf("\n");
    printf("Source space                 : %s\n",m_pSettings->srcname.toUtf8().constData());
    if (!(m_pSettings->transname.isEmpty()) || !(m_pSettings->mriname.isEmpty())) {
        printf("MRI -> head transform source : %s\n",!(m_pSettings->mriname.isEmpty()) ? m_pSettings->mriname.toUtf8().constData() : m_pSettings->transname.toUtf8().constData());
    } else {
        printf("MRI and head coordinates are assumed to be identical.\n");
    }
    printf("Measurement data             : %s\n",m_pSettings->measname.toUtf8().constData());
    if (!m_pSettings->bemname.isEmpty()) {
        printf("BEM model                    : %s\n",m_pSettings->bemname.toUtf8().constData());
    } else {
        printf("Sphere model                 : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
               1000.0f*m_pSettings->r0[X],1000.0f*m_pSettings->r0[Y],1000.0f*m_pSettings->r0[Z]);
        if (m_pSettings->include_eeg) {
            printf("\n");

            if (m_pSettings->eeg_model_file.isEmpty()) {
                qCritical("!!!!!!!!!!TODO: default_eeg_model_file();");
                //                m_pSettings->eeg_model_file = default_eeg_model_file();
            }
            m_eegModels.reset(FwdEegSphereModelSet::fwd_load_eeg_sphere_models(m_pSettings->eeg_model_file,m_eegModels.release()));
            m_eegModels->fwd_list_eeg_sphere_models(stderr);

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

            printf("Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
                   m_pSettings->eeg_model_name.toUtf8().constData(),1000*m_pSettings->eeg_sphere_rad);
            printf("%s the electrode locations to scalp\n",m_pSettings->scale_eeg_pos ? "Scale" : "Do not scale");

            m_eegModel->scale_pos = m_pSettings->scale_eeg_pos;
            m_eegModel->r0 = m_pSettings->r0;

            printf("\n");
        }
    }
    printf("%s field computations\n",m_pSettings->accurate ? "Accurate" : "Standard");
    printf("Do computations in %s coordinates.\n",FiffCoordTrans::frame_name(m_pSettings->coord_frame).toUtf8().constData());
    printf("%s source orientations\n",m_pSettings->fixed_ori ? "Fixed" : "Free");
    if (m_pSettings->compute_grad) {
        printf("Compute derivatives with respect to source location coordinates\n");
    }
    printf("Destination for the solution : %s\n",m_pSettings->solname.toUtf8().constData());
    if (m_pSettings->do_all) {
        printf("Calculate solution for all source locations.\n");
    }
    if (m_pSettings->nlabel > 0) {
        printf("Source space will be restricted to sources in %d labels\n",m_pSettings->nlabel);
    }

    // Read the source locations

    printf("\n");
    printf("Reading %s...\n",m_pSettings->srcname.toUtf8().constData());
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
    printf("Read %d source spaces a total of %d active source locations\n", static_cast<int>(m_spaces.size()),m_iNSource);
    if (MNESourceSpace::restrict_sources_to_labels(m_spaces,m_pSettings->labels,m_pSettings->nlabel) == FAIL) {
        return;
    }

    // Read the MRI -> head coordinate transformation
    printf("\n");
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
    // replace mne_read_meg_comp_eeg_ch_info_41()

    if(!m_pSettings->pFiffInfo) {

        QFile measname(m_pSettings->measname);
        FIFFLIB::FiffDirNode::SPtr DirNode;
        FiffStream::SPtr pStream(new FiffStream(&measname));
        FIFFLIB::FiffInfo fiffInfo;
        if(!pStream->open()) {
            qCritical() << "Could not open Stream.";
            return;
        }

        //Get Fiff info
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
    if (mne_read_meg_comp_eeg_ch_info_41(m_pInfoBase,
                                         m_listMegChs,
                                         iNMeg,
                                         m_listCompChs,
                                         iNComp,
                                         m_listEegChs,
                                         iNEeg,
                                         m_meg_head_t,
                                         m_meas_id) != OK) {
        return;
    }

    m_iNChan = iNMeg + iNEeg;

    printf("\n");
    if (iNMeg > 0) {
        printf("Read %3d MEG channels from %s\n",iNMeg,m_pSettings->measname.toUtf8().constData());
    }
    if (iNComp > 0) {
        printf("Read %3d MEG compensation channels from %s\n",iNComp,m_pSettings->measname.toUtf8().constData());
    }
    if (iNEeg > 0) {
        printf("Read %3d EEG channels from %s\n",iNEeg,m_pSettings->measname.toUtf8().constData());
    }
    if (!m_pSettings->include_meg) {
        printf("MEG not requested. MEG channels omitted.\n");
        m_listMegChs.clear();
        m_listCompChs.clear();
        iNMeg = 0;
        iNComp = 0;
    }
    else
        m_meg_head_t.print();
    if (!m_pSettings->include_eeg) {
        printf("EEG not requested. EEG channels omitted.\n");
        m_listEegChs.clear();
        iNEeg = 0;
    } else {
        if (!FiffChInfo::checkEegLocations(m_listEegChs, iNEeg)) {
            return;
        }
    }

    // Create coil descriptions with transformation to head or MRI frame

    if (m_pSettings->include_meg) {
        qPath = QString(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
        file.setFileName(qPath);
        if ( !QCoreApplication::startingUp() ) {
            qPath = QCoreApplication::applicationDirPath() + QString("/../resources/general/coilDefinitions/coil_def.dat");
        } else if (!file.exists()) {
            qPath = "../resources/general/coilDefinitions/coil_def.dat";
        }

        m_templates.reset(FwdCoilSet::read_coil_defs(qPath));
        if (!m_templates) {
            return;
        }

        // Compensation data

        m_compData = MNECTFCompDataSet::read(m_pSettings->measname);
        if (!m_compData) {
            return;
        }
        // Compensation channel information may be needed
        if (m_compData->ncomp > 0) {
            printf("%d compensation data sets in %s\n",m_compData->ncomp,m_pSettings->measname.toUtf8().constData());
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
        m_megcoils.reset(m_templates->create_meg_coils(m_listMegChs,
                                                      iNMeg,
                                                      m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                      meg_mri_t));
        if (!m_megcoils) {
            return;
        }
        if (iNComp > 0) {
            m_compcoils.reset(m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,
                                                             meg_mri_t));
            if (!m_compcoils) {
                return;
            }
        }
        m_eegels.reset(FwdCoilSet::create_eeg_els(m_listEegChs,
                                                   iNEeg,
                                                   head_mri_t));
        if (!m_eegels) {
            return;
        }

        printf("MRI coordinate coil definitions created.\n");
    } else {
        m_megcoils.reset(m_templates->create_meg_coils(m_listMegChs,
                                                        iNMeg,
                                                        m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                        m_meg_head_t));
        if (!m_megcoils) {
            return;
        }

        if (iNComp > 0) {
            m_compcoils.reset(m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,m_meg_head_t));
            if (!m_compcoils) {
                return;
            }
        }
        m_eegels.reset(FwdCoilSet::create_eeg_els(m_listEegChs,
                                                   iNEeg));
        if (!m_eegels) {
            return;
        }
        printf("Head coordinate coil definitions created.\n");
    }

    // Transform the source spaces into the appropriate coordinates
    {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces) != OK) {
            return;
        }
    }
    printf("Source spaces are now in %s coordinates.\n",FiffCoordTrans::frame_name(m_pSettings->coord_frame).toUtf8().constData());

    // Prepare the BEM model if necessary

    if (!m_pSettings->bemname.isEmpty()) {
        QString bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(m_pSettings->bemname);
        m_pSettings->bemname = bemsolname;

        printf("\nSetting up the BEM model using %s...\n",m_pSettings->bemname.toUtf8().constData());
        printf("\nLoading surfaces...\n");
        m_bemModel.reset(FwdBemModel::fwd_bem_load_three_layer_surfaces(m_pSettings->bemname));

        if (m_bemModel) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            m_bemModel.reset(FwdBemModel::fwd_bem_load_homog_surface(m_pSettings->bemname));
            if (!m_bemModel) {
                return;
            }
            printf("Homogeneous model surface loaded.\n");
        }
        if (iNEeg > 0 && m_bemModel->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            return;
        }
        printf("\nLoading the solution matrix...\n");
        if (m_bemModel->fwd_bem_load_recompute_solution(m_pSettings->bemname.toUtf8().data(),FWD_BEM_UNKNOWN,false) == FAIL) {
            return;
        }
        if (m_pSettings->coord_frame == FIFFV_COORD_HEAD) {
            printf("Employing the head->MRI coordinate transform with the BEM model.\n");
            if (m_bemModel->fwd_bem_set_head_mri_t(m_mri_head_t) == FAIL) {
                return;
            }
        }
        printf("BEM model %s is now set up\n",m_bemModel->sol_name.toUtf8().constData());
    } else {
        printf("Using the sphere model.\n");
    }
    printf ("\n");

    // Try to circumvent numerical problems by excluding points too close our ouside the inner skull surface

    if (m_pSettings->filter_spaces) {
        if (!m_pSettings->mindistoutname.isEmpty()) {
            filteredFile.setFileName(m_pSettings->mindistoutname);
            if (!filteredFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qCritical() << m_pSettings->mindistoutname;
                return;
            }
            filteredStream = new QTextStream(&filteredFile);
            printf("Omitted source space points will be output to : %s\n",m_pSettings->mindistoutname.toUtf8().constData());
        }
        MNESourceSpace::filter_source_spaces(m_pSettings->mindist,
                                                 m_pSettings->bemname,
                                                 m_mri_head_t,
                                                 m_spaces,
                                                 filteredStream,m_pSettings->use_threads);
        delete filteredStream;
        filteredStream = Q_NULLPTR;
    }
}

//=========================================================================================================

/**
 * Perform the forward computation for MEG and/or EEG.
 *
 * Computes the forward solutions using the BEM or sphere model and assembles
 * the combined MEG/EEG solution matrices. Results are stored in the sol and
 * sol_grad member variables.
 */
void ComputeFwd::calculateFwd()
{
    int iNMeg = 0;
    int iNEeg = 0;

    if(m_megcoils) {
        iNMeg = m_megcoils->ncoil;
    }
    if(m_eegels) {
        iNEeg = m_eegels->ncoil;
    }
    if (!m_bemModel) {
        m_pSettings->use_threads = false;
    }

    // check if source spaces are still in head space
    if(m_spaces[0]->coord_frame != FIFFV_COORD_HEAD) {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces) != OK) {
            return;
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
            return;
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
            return;
        }
    }
    if(iNMeg > 0 && iNEeg > 0) {
        if(m_meg_forward->data.cols() != m_eeg_forward->data.cols()) {
            qWarning() << "The MEG and EEG forward solutions do not match";
            return;
        }
        sol->clear();
        sol->data = MatrixXd(m_meg_forward->nrow + m_eeg_forward->nrow, m_meg_forward->ncol);
        sol->data.block(0,0,m_meg_forward->nrow,m_meg_forward->ncol) = m_meg_forward->data;
        sol->data.block(m_meg_forward->nrow,0,m_eeg_forward->nrow,m_eeg_forward->ncol) = m_eeg_forward->data;
        sol->nrow = m_meg_forward->nrow + m_eeg_forward->nrow;
        sol->row_names.append(m_meg_forward->row_names);
    } else if (iNMeg > 0) {
        sol = m_meg_forward;
    } else {
        sol = m_eeg_forward;
    }

    if(m_pSettings->compute_grad) {
        if(iNMeg > 0 && iNEeg > 0) {
            if(m_meg_forward_grad->data.cols() != m_eeg_forward_grad->data.cols()) {
                qWarning() << "The MEG and EEG forward solutions do not match";
                return;
            }
            sol_grad->clear();
            sol_grad->data = MatrixXd(m_meg_forward_grad->nrow + m_eeg_forward_grad->nrow, m_meg_forward_grad->ncol);
            sol_grad->data.block(0,0,m_meg_forward_grad->nrow,m_meg_forward_grad->ncol) = m_meg_forward_grad->data;
            sol_grad->data.block(m_meg_forward_grad->nrow,0,m_eeg_forward_grad->nrow,m_eeg_forward_grad->ncol) = m_eeg_forward_grad->data;
            sol_grad->nrow = m_meg_forward_grad->nrow + m_eeg_forward_grad->nrow;
            sol_grad->row_names.append(m_meg_forward_grad->row_names);
        } else if (iNMeg > 0) {
            sol_grad = m_meg_forward_grad;
        } else {
            sol_grad = m_eeg_forward_grad;
        }
    }
}

//=========================================================================================================

/**
 * Update the head position and recompute the MEG forward solution.
 *
 * Recreates MEG and compensation coil definitions using the new device-to-head
 * transform, then recomputes only the MEG portion of the forward solution.
 *
 * @param[in] transDevHead   The updated device-to-head coordinate transform.
 */
void ComputeFwd::updateHeadPos(const FiffCoordTrans& transDevHead)
{

    int iNMeg = 0;
    if(m_megcoils) {
        iNMeg = m_megcoils->ncoil;
    }

    int iNComp = 0;
    if(m_compcoils) {
        iNComp = m_compcoils->ncoil;
    }

    // create new coilset with updated head position
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTrans head_mri_t = m_mri_head_t.inverted();
        FiffCoordTrans meg_mri_t = FiffCoordTrans::combine(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,transDevHead,head_mri_t);
        if (meg_mri_t.isEmpty()) {
            return;
        }
        m_megcoils.reset(m_templates->create_meg_coils(m_listMegChs,
                                                        iNMeg,
                                                        m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                        meg_mri_t));
        if (!m_megcoils) {
            return;
        }
        if (iNComp > 0) {
            m_compcoils.reset(m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,
                                                             meg_mri_t));
            if (!m_compcoils) {
                return;
            }
        }
    } else {
        m_megcoils.reset(m_templates->create_meg_coils(m_listMegChs,
                                                        iNMeg,
                                                        m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                        transDevHead));
        if (!m_megcoils) {
            return;
        }

        if (iNComp > 0) {
            m_compcoils.reset(m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,transDevHead));
            if (!m_compcoils) {
                return;
            }
        }
    }

    // check if source spaces are still in head space
    if(m_spaces[0]->coord_frame != FIFFV_COORD_HEAD) {
        if (MNESourceSpace::transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces) != OK) {
            return;
        }
    }

    // recompute meg forward
    if ((m_bemModel->compute_forward_meg(m_spaces,
                                          m_megcoils.get(),
                                          m_compcoils.get(),
                                          m_compData.get(),                   // we might have to update this too
                                          m_pSettings->fixed_ori,
                                          m_pSettings->r0,
                                          m_pSettings->use_threads,
                                          *m_meg_forward.data(),
                                          *m_meg_forward_grad.data(),
                                          m_pSettings->compute_grad)) == FAIL) {
        return;
    }

    // Update new Transformation Matrix
    m_meg_head_t = transDevHead;
    // update solution
    sol->data.block(0,0,m_meg_forward->nrow,m_meg_forward->ncol) = m_meg_forward->data;
    if(m_pSettings->compute_grad) {
        sol_grad->data.block(0,0,m_meg_forward_grad->nrow,m_meg_forward_grad->ncol) = m_meg_forward_grad->data;
    }
}

//=========================================================================================================

/**
 * Write the forward solution to a FIFF file.
 *
 * Transforms source spaces back into MRI coordinates, writes the solution file,
 * and attaches environment metadata.
 *
 * @param[in] sSolName   Output file path ("default" uses the path from settings).
 */
void ComputeFwd::storeFwd(const QString& sSolName)
{
    // We are ready to spill it out
    // Transform the source spaces back into MRI coordinates
    {
        if (MNESourceSpace::transform_source_spaces_to(FIFFV_COORD_MRI,m_mri_head_t,m_spaces) != OK) {
            return;
        }
    }

    QString sName;

    if(sSolName == "default") {
        sName = m_pSettings->solname;
    } else {
        sName = sSolName;
    }

    printf("\nwriting %s...",sSolName.toUtf8().constData());

    //
    //  Populate an MNEForwardSolution from internal data
    //
    MNEForwardSolution fwd;

    int nmeg = m_megcoils->ncoil;
    int neeg = m_eegels->ncoil;

    fwd.coord_frame = m_pSettings->coord_frame;
    fwd.source_ori  = m_pSettings->fixed_ori ? FIFFV_MNE_FIXED_ORI : FIFFV_MNE_FREE_ORI;
    fwd.surf_ori    = false;
    fwd.mri_head_t  = m_mri_head_t;
    fwd.mri_filename = m_pSettings->mriname;
    fwd.mri_id      = m_mri_id;
    fwd.nchan       = nmeg + neeg;

    //  Source spaces
    fwd.src.clear();
    fwd.nsource = 0;
    for (int k = 0; k < static_cast<int>(m_spaces.size()); ++k) {
        fwd.src.append(*m_spaces[k]);
        fwd.nsource += m_spaces[k]->nuse;
    }

    //  Measurement provenance
    fwd.info.filename  = m_pSettings->measname;
    fwd.info.meas_id   = m_meas_id;
    fwd.info.dev_head_t = m_meg_head_t;
    fwd.info.nchan     = fwd.nchan;

    //  Channel info: MEG first, then EEG
    fwd.info.chs.clear();
    fwd.info.ch_names.clear();
    for (int k = 0; k < nmeg; ++k) {
        fwd.info.chs.append(m_listMegChs[k]);
        fwd.info.ch_names.append(m_listMegChs[k].ch_name);
    }
    for (int k = 0; k < neeg; ++k) {
        fwd.info.chs.append(m_listEegChs[k]);
        fwd.info.ch_names.append(m_listEegChs[k].ch_name);
    }

    //  Bad channels from measurement file
    {
        QFile fileBad(m_pSettings->measname);
        FiffStream::SPtr t_pStreamBads(new FiffStream(&fileBad));
        if (t_pStreamBads->open()) {
            fwd.info.bads = t_pStreamBads->read_bad_channels(t_pStreamBads->dirtree());
            t_pStreamBads->close();
        }
    }

    //  Combined forward solution matrix: MEG rows on top, EEG below
    {
        int ncols = m_meg_forward ? m_meg_forward->ncol : (m_eeg_forward ? m_eeg_forward->ncol : 0);
        fwd.sol = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
        fwd.sol->nrow = fwd.nchan;
        fwd.sol->ncol = ncols;
        fwd.sol->data = MatrixXd::Zero(fwd.nchan, ncols);

        if (nmeg > 0 && m_meg_forward) {
            fwd.sol->data.block(0, 0, nmeg, ncols) = m_meg_forward->data;
            fwd.sol->row_names = m_meg_forward->row_names;
            fwd.sol->col_names = m_meg_forward->col_names;
        }
        if (neeg > 0 && m_eeg_forward) {
            fwd.sol->data.block(nmeg, 0, neeg, ncols) = m_eeg_forward->data;
            fwd.sol->row_names.append(m_eeg_forward->row_names);
            if (fwd.sol->col_names.isEmpty())
                fwd.sol->col_names = m_eeg_forward->col_names;
        }
    }

    //  Combined grad matrix (if computed)
    if (m_pSettings->compute_grad) {
        int ncols_grad = m_meg_forward_grad ? m_meg_forward_grad->ncol : (m_eeg_forward_grad ? m_eeg_forward_grad->ncol : 0);
        fwd.sol_grad = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
        fwd.sol_grad->nrow = fwd.nchan;
        fwd.sol_grad->ncol = ncols_grad;
        fwd.sol_grad->data = MatrixXd::Zero(fwd.nchan, ncols_grad);

        if (nmeg > 0 && m_meg_forward_grad) {
            fwd.sol_grad->data.block(0, 0, nmeg, ncols_grad) = m_meg_forward_grad->data;
            fwd.sol_grad->row_names = m_meg_forward_grad->row_names;
            fwd.sol_grad->col_names = m_meg_forward_grad->col_names;
        }
        if (neeg > 0 && m_eeg_forward_grad) {
            fwd.sol_grad->data.block(nmeg, 0, neeg, ncols_grad) = m_eeg_forward_grad->data;
            fwd.sol_grad->row_names.append(m_eeg_forward_grad->row_names);
            if (fwd.sol_grad->col_names.isEmpty())
                fwd.sol_grad->col_names = m_eeg_forward_grad->col_names;
        }
    }

    //
    //  Write the solution
    //
    QFile file(sName);
    if (!fwd.write(file)) {
        return;
    }

    {
        QFile fileEnv(sName);
        if (!fileEnv.exists()) {
            qCritical("File %s does not exist. Cannot attach env info.", sName.toUtf8().constData());
            return;
        }
        FiffStream::SPtr t_pStreamEnv = FiffStream::open_update(fileEnv);
        if (!t_pStreamEnv) {
            return;
        }
        if (!t_pStreamEnv->attach_env(QDir::currentPath(), m_pSettings->command)) {
            return;
        }
    }
    printf("done\n");
    printf("\nFinished.\n");
}
