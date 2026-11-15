//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_ctf_comp_data_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNECTFCompDataSet.
 *
 * Implements set-level FIFF I/O, the search for the currently active
 * compensator in @ref mne::FiffInfo and the build of the
 * compensator that maps the current grade to a requested one.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ctf_comp_data_set.h"
#include <memory>
#include <base/log.h>

#include <vector>

#include <base/string_utils.h>
#include "mne_ctf_comp_data.h"

#include "mne_types.h"

#include <fiff/fiff_coord_trans.h>

#include <fiff/fiff_types.h>

#include <Eigen/Core>

constexpr int FAIL = -1;
constexpr int OK = 0;


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_32(const std::filesystem::path& name,
                                     std::vector<mne::FiffChInfo>& megp, /* MEG channels */
                                     int* nmegp,
                                     std::vector<mne::FiffChInfo>& meg_compp,
                                     int* nmeg_compp,
                                     std::vector<mne::FiffChInfo>& eegp, /* EEG channels */
                                     int* neegp,
                                     FiffCoordTrans* meg_head_t)
/*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
    FiffStream stream(name);

    std::vector<mne::FiffChInfo> chs;
    int nchan = 0;
    std::vector<mne::FiffChInfo> meg;
    int nmeg = 0;
    std::vector<mne::FiffChInfo> meg_comp;
    int nmeg_comp = 0;
    std::vector<mne::FiffChInfo> eeg;
    int neeg = 0;
    std::vector<std::shared_ptr<FiffDirNode>> nodes;
    std::shared_ptr<FiffDirNode> info;
    std::optional<FiffTag> t_pTag;
    mne::FiffChInfo this_ch;
    FiffCoordTrans t;
    fiff_int_t kind, pos;
    int j, k, to_find;

    if (!stream.open()) {
        stream.close();
        return FIFF_FAIL;
    }

    nodes = stream.dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (nodes.size() == 0) {
        nodes = stream.dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
        if (nodes.size() == 0) {
            mne::Log::critical("Could not find the channel information.");
            stream.close();
            return FIFF_FAIL;
        }
    }
    info = nodes[0];
    to_find = 0;
    for (k = 0; k < info->nent(); k++) {
        kind = info->dir[k].kind;
        pos = info->dir[k].pos;
        switch (kind) {
            case FIFF_NCHAN:
                if (t_pTag = stream.read_tag(pos); !t_pTag) {
                    stream.close();
                    return FIFF_FAIL;
                }
                nchan = *t_pTag->to_int();

                for (j = 0; j < nchan; j++) {
                    chs.push_back(FiffChInfo());
                    chs[j].scanNo = -1;
                }
                to_find = nchan;
                break;

            case FIFF_COORD_TRANS:
                if (t_pTag = stream.read_tag(pos); !t_pTag) {
                    stream.close();
                    return FIFF_FAIL;
                }
                t = FiffCoordTrans::from_tag(*t_pTag);
                if (t.from != FIFFV_COORD_DEVICE || t.to != FIFFV_COORD_HEAD)
                    t = FiffCoordTrans();
                break;

            case FIFF_CH_INFO: /* Information about one channel */
                if (t_pTag = stream.read_tag(pos); !t_pTag) {
                    stream.close();
                    return FIFF_FAIL;
                }

                this_ch = t_pTag->to_ch_info();
                if (this_ch.scanNo <= 0 || this_ch.scanNo > nchan) {
                    mne::Log::critical("FIFF_CH_INFO : scan # out of range {} ({})!", this_ch.scanNo, nchan);
                    stream.close();
                    return FIFF_FAIL;
                } else
                    chs[this_ch.scanNo - 1] = this_ch;
                to_find--;
                break;
        }
    }
    if (to_find != 0) {
        mne::Log::critical("Some of the channel information was missing.");
        stream.close();
        return FIFF_FAIL;
    }
    if (t.empty() && meg_head_t != nullptr) {
        /*
     * Try again in a more general fashion
     */
        t = FiffCoordTrans::read(name, FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD).value_or(FiffCoordTrans());
        if (t.empty()) {
            mne::Log::critical("MEG -> head coordinate transformation not found.");
            stream.close();
            return FIFF_FAIL;
        }
    }
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            meg.push_back(chs[k]);
            nmeg++;
        } else if (chs[k].kind == FIFFV_REF_MEG_CH) {
            meg_comp.push_back(chs[k]);
            nmeg_comp++;
        } else if (chs[k].kind == FIFFV_EEG_CH) {
            eeg.push_back(chs[k]);
            neeg++;
        }
    }
    //    fiff_close(in);
    stream.close();

    megp = meg;
    if (nmegp) {
        *nmegp = nmeg;
    }

    meg_compp = meg_comp;
    if (nmeg_compp) {
        *nmeg_compp = nmeg_comp;
    }

    eegp = eeg;
    if (neegp) {
        *neegp = neeg;
    }

    if (meg_head_t == nullptr) {
    } else
        *meg_head_t = t;

    return FIFF_OK;
}

static struct
{
    int grad_comp;
    int ctf_comp;
} compMap[] = {{MNE_CTFV_NOGRAD, MNE_CTFV_COMP_NONE},
               {MNE_CTFV_GRAD1, MNE_CTFV_COMP_G1BR},
               {MNE_CTFV_GRAD2, MNE_CTFV_COMP_G2BR},
               {MNE_CTFV_GRAD3, MNE_CTFV_COMP_G3BR},
               {MNE_4DV_COMP1, MNE_4DV_COMP1}, /* One-to-one mapping for 4D data */
               {MNE_CTFV_COMP_UNKNOWN, MNE_CTFV_COMP_UNKNOWN}};

int mne_unmap_ctf_comp_kind(int ctf_comp)

{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (ctf_comp == compMap[k].ctf_comp)
            return compMap[k].grad_comp;
    return ctf_comp;
}

std::unique_ptr<FiffSparseMatrix> mne_convert_to_sparse(const Eigen::MatrixXf& dense, /* The dense matrix to be converted */
                                                        float small)                  /* How small elements should be ignored? */
/*
 * Convert a dense matrix to sparse using Eigen's sparseView.
 */
{
    if (small < 0) { /* Automatic scaling */
        float maxval = dense.cwiseAbs().maxCoeff();
        if (maxval > 0)
            small = maxval * std::fabs(small);
        else
            small = std::fabs(small);
    }

    Eigen::SparseMatrix<float> eigenSparse = dense.sparseView(small, 1.0f);
    eigenSparse.makeCompressed();

    if (eigenSparse.nonZeros() <= 0) {
        mne::Log::warning("No nonzero elements found.");
        return nullptr;
    }

    return std::make_unique<FiffSparseMatrix>(std::move(eigenSparse), FIFFTS_MC_RCS);
}

int mne_sparse_vec_mult2_32(FiffSparseMatrix* mat, /* The sparse matrix */
                            float* vector,         /* Vector to be multiplied */
                            float* res)            /* Result of the multiplication */
/*
      * Multiply a vector by a sparse matrix using Eigen.
      */
{
    Eigen::Map<const Eigen::VectorXf> vecIn(vector, mat->cols());
    Eigen::Map<Eigen::VectorXf> vecOut(res, mat->rows());
    vecOut = mat->eigen() * vecIn;
    return 0;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNECTFCompDataSet::MNECTFCompDataSet()
: ncomp(0)
, nch(0)
, undo(nullptr)
, current(nullptr)
{
}

//=============================================================================================================

MNECTFCompDataSet::MNECTFCompDataSet(const MNECTFCompDataSet& set)
: ncomp(0)
, nch(set.nch)
, undo(nullptr)
, current(nullptr)
{
    if (set.ncomp > 0) {
        for (int k = 0; k < set.ncomp; k++)
            if (set.comps[k])
                this->comps.push_back(std::make_unique<MNECTFCompData>(*set.comps[k]));
        this->ncomp = static_cast<int>(this->comps.size());
    }

    this->chs = set.chs;

    if (set.undo)
        this->undo = std::make_unique<MNECTFCompData>(*set.undo);

    if (set.current)
        this->current = std::make_unique<MNECTFCompData>(*set.current);
}

//=============================================================================================================

MNECTFCompDataSet::~MNECTFCompDataSet()
{
}

//=============================================================================================================

std::unique_ptr<MNECTFCompDataSet> MNECTFCompDataSet::read(const std::filesystem::path& name)
/*
     * Read all CTF compensation data from a given file
     */
{
    FiffStream stream(name);

    std::unique_ptr<MNECTFCompDataSet> set;
    std::vector<std::shared_ptr<FiffDirNode>> nodes;
    std::vector<std::shared_ptr<FiffDirNode>> comps;
    int ncomp;
    int kind, k;
    std::optional<FiffTag> t_pTag;
    std::vector<FiffChInfo> chs;
    int nch = 0;
    int calibrated;
    /*
        * Read the channel information
        */
    {
        std::vector<FiffChInfo> comp_chs, temp;
        int ncompch = 0;

        if (mne_read_meg_comp_eeg_ch_info_32(name,
                                             chs,
                                             &nch,
                                             comp_chs,
                                             &ncompch,
                                             temp,
                                             nullptr,
                                             nullptr) == FAIL)
            return nullptr;
        if (ncompch > 0) {
            for (k = 0; k < ncompch; k++)
                chs.push_back(comp_chs[k]);
            nch = nch + ncompch;
        }
    }
    /*
        * Read the rest of the stuff
        */
    if (!stream.open()) {
        stream.close();
        return nullptr;
    }
    set = std::make_unique<MNECTFCompDataSet>();
    /*
        * Locate the compensation data sets
        */
    nodes = stream.dirtree()->dir_tree_find(FIFFB_MNE_CTF_COMP);
    if (nodes.size() == 0) {
        stream.close();
        return set;
    }
    comps = nodes[0]->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);
    if (comps.size() == 0) {
        stream.close();
        return set;
    }
    ncomp = static_cast<int>(comps.size());
    /*
        * Set the channel info
        */
    set->chs = chs;
    set->nch = nch;
    /*
        * Read each data set
        */
    for (k = 0; k < ncomp; k++) {
        auto mat = MNENamedMatrix::read(stream, comps[k], FIFF_MNE_CTF_COMP_DATA);
        if (!mat) {
            stream.close();
            return nullptr;
        }
        t_pTag = comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_KIND);
        if (t_pTag) {
            kind = *t_pTag->to_int();
        } else {
            stream.close();
            return nullptr;
        }
        t_pTag = comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_CALIBRATED);
        if (t_pTag) {
            calibrated = *t_pTag->to_int();
        } else
            calibrated = 0;
        /*
            * Add these data to the set
            */
        auto one = std::make_unique<MNECTFCompData>();
        one->data = std::move(mat);
        one->kind = kind;
        one->mne_kind = mne_unmap_ctf_comp_kind(one->kind);
        one->calibrated = calibrated;

        if (one->calibrate(set->chs, set->nch, true) == FAIL) {
            mne::Log::warning("Warning: Compensation data for '{}' omitted\n", explain_comp(one->kind));
        } else {
            set->comps.push_back(std::move(one));
            set->ncomp++;
        }
    }
#ifdef DEBUG
    mne::Log::info("{} CTF compensation data sets read from {}\n", set->ncomp, name);
#endif
    stream.close();
    return set;
}

//=============================================================================================================

int MNECTFCompDataSet::make_comp(const std::vector<FiffChInfo>& chList,
                                 int nChan,
                                 std::vector<FiffChInfo> compchs,
                                 int nCompChan) /* How many of these */
/*
     * Make compensation data to apply to a set of channels to yield (or uncompensated) compensated data
     */
{
    Eigen::VectorXi compGrades;
    int need_comp;
    int first_comp;
    MNECTFCompData* this_comp;
    Eigen::VectorXi comp_sel;
    StringList names;
    int j, k, p;

    std::unique_ptr<FiffSparseMatrix> presel;
    std::unique_ptr<FiffSparseMatrix> postsel;
    std::unique_ptr<MNENamedMatrix> data;


    if (compchs.empty()) {
        compchs = chList;
        nCompChan = nChan;
    }
    mne::Log::info("Setting up compensation data...\n");
    if (nChan == 0)
        return OK;
    current.reset();
    compGrades.resize(nChan);
    for (k = 0, need_comp = 0, first_comp = MNE_CTFV_COMP_NONE; k < nChan; k++) {
        if (chList[k].kind == FIFFV_MEG_CH) {
            compGrades[k] = chList[k].chpos.coil_type >> 16;
            if (compGrades[k] != MNE_CTFV_COMP_NONE) {
                if (first_comp == MNE_CTFV_COMP_NONE)
                    first_comp = compGrades[k];
                else {
                    if (compGrades[k] != first_comp) {
                        mne::Log::critical("We do not support nonuniform compensation yet.");
                        return FAIL;
                    }
                }
                need_comp++;
            }
        } else
            compGrades[k] = MNE_CTFV_COMP_NONE;
    }
    if (need_comp == 0) {
        mne::Log::info("\tNo compensation set. Nothing more to do.\n");
        return OK;
    }
    mne::Log::info("\t{} out of {} channels have the compensation set.\n", need_comp, nChan);
    /*
        * Find the desired compensation data matrix
        */
    for (k = 0, this_comp = nullptr; k < this->ncomp; k++) {
        if (this->comps[k]->mne_kind == first_comp) {
            this_comp = this->comps[k].get();
            break;
        }
    }
    if (!this_comp) {
        mne::Log::critical("Did not find the desired compensation data : {}", explain_comp(map_comp_kind(first_comp)));
        return FAIL;
    }
    mne::Log::info("\tDesired compensation data ({}) found.\n", explain_comp(map_comp_kind(first_comp)));
    /*
        * Find the compensation channels
        */
    comp_sel.resize(this_comp->data->ncol());
    for (k = 0; k < this_comp->data->ncol(); k++) {
        comp_sel[k] = -1;
        const std::string& name = this_comp->data->collist[k];
        for (p = 0; p < nCompChan; p++)
            if ((name == compchs[p].ch_name)) {
                comp_sel[k] = p;
                break;
            }
        if (comp_sel[k] < 0) {
            mne::Log::critical("Compensation channel {} not found", name);
            return FAIL;
        }
    }
    mne::Log::info("\tAll compensation channels found.\n");
    /*
        * Create the preselector
        */
    {
        Eigen::MatrixXf sel = Eigen::MatrixXf::Zero(this_comp->data->ncol(), nCompChan);
        for (j = 0; j < this_comp->data->ncol(); j++)
            sel(j, comp_sel[j]) = 1.0f;
        presel = mne_convert_to_sparse(sel, 1e-30f);
        if (!presel)
            return FAIL;
        mne::Log::info("\tPreselector created.\n");
    }
    /*
     * Pick the desired channels
     */
    for (k = 0; k < nChan; k++) {
        if (compGrades[k] != MNE_CTFV_COMP_NONE)
            names.push_back(chList[k].ch_name);
    }

    {
        auto d = this_comp->data->pick(names, {});
        if (!d)
            return FAIL;
        data = std::move(d);
    }
    mne::Log::info("\tCompensation data matrix created.\n");
    /*
        * Create the postselector
        */
    {
        Eigen::MatrixXf sel = Eigen::MatrixXf::Zero(nChan, data->nrow());
        for (j = 0, p = 0; j < nChan; j++) {
            if (compGrades[j] != MNE_CTFV_COMP_NONE)
                sel(j, p++) = 1.0f;
        }
        postsel = mne_convert_to_sparse(sel, 1e-30f);
        if (!postsel)
            return FAIL;
        mne::Log::info("\tPostselector created.\n");
    }
    current = std::make_unique<MNECTFCompData>();
    current->kind = this_comp->kind;
    current->mne_kind = this_comp->mne_kind;
    current->data = std::move(data);
    current->presel = std::move(presel);
    current->postsel = std::move(postsel);

    mne::Log::info("\tCompensation set up.\n");
    return OK;
}

//=============================================================================================================

int MNECTFCompDataSet::set_comp(std::vector<mne::FiffChInfo>& chs,
                                int nch,
                                int comp)
/*
     * Set the compensation bits to the desired value
     */
{
    int k;
    int nset;
    for (k = 0, nset = 0; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            chs[k].chpos.coil_type = (chs[k].chpos.coil_type & 0xFFFF) | (comp << 16);
            nset++;
        }
    }
    mne::Log::info("A new compensation value ({}) was assigned to {} MEG channels.\n", explain_comp(map_comp_kind(comp)), nset);
    return nset;
}

//=============================================================================================================

int MNECTFCompDataSet::apply(bool do_it, Eigen::Ref<Eigen::VectorXf> data)
{
    return apply(do_it, data, data);
}

//=============================================================================================================

int MNECTFCompDataSet::apply(bool do_it, Eigen::Ref<Eigen::VectorXf> data, Eigen::Ref<const Eigen::VectorXf> compdata)
/*
     * Apply compensation or revert to uncompensated data
     */
{
    MNECTFCompData* this_comp;
    int ndata = static_cast<int>(data.size());
    int ncompdata = static_cast<int>(compdata.size());

    if (!current)
        return OK;
    this_comp = current.get();
    /*
       * Dimension checks
       */
    if (this_comp->presel) {
        if (this_comp->presel->cols() != ncompdata) {
            mne::Log::critical("Compensation data dimension mismatch. Expected {}, got {} channels.", this_comp->presel->cols(), ncompdata);
            return FAIL;
        }
    } else if (this_comp->data->ncol() != ncompdata) {
        mne::Log::critical("Compensation data dimension mismatch. Expected {}, got {} channels.", this_comp->data->ncol(), ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->rows() != ndata) {
            mne::Log::critical("Data dimension mismatch. Expected {}, got {} channels.", this_comp->postsel->rows(), ndata);
            return FAIL;
        }
    } else if (this_comp->data->nrow() != ndata) {
        mne::Log::critical("Data dimension mismatch. Expected {}, got {} channels.", this_comp->data->nrow(), ndata);
        return FAIL;
    }
    /*
        * Preselection is optional
        */
    const float* presel;
    if (this_comp->presel) {
        if (this_comp->presel_data.size() == 0)
            this_comp->presel_data.resize(this_comp->presel->rows());
        if (mne_sparse_vec_mult2_32(this_comp->presel.get(), const_cast<float*>(compdata.data()), this_comp->presel_data.data()) != OK)
            return FAIL;
        presel = this_comp->presel_data.data();
    } else
        presel = compdata.data();
    /*
        * This always happens
        */
    if (this_comp->comp_data.size() == 0)
        this_comp->comp_data.resize(this_comp->data->nrow());
    {
        Eigen::Map<const Eigen::VectorXf> preselVec(presel, this_comp->data->ncol());
        Eigen::Map<Eigen::VectorXf> compVec(this_comp->comp_data.data(), this_comp->data->nrow());
        compVec = this_comp->data->data * preselVec;
    }
    /*
        * Optional postselection
        */
    const float* comp;
    if (!this_comp->postsel)
        comp = this_comp->comp_data.data();
    else {
        if (this_comp->postsel_data.size() == 0)
            this_comp->postsel_data.resize(this_comp->postsel->rows());
        if (mne_sparse_vec_mult2_32(this_comp->postsel.get(), this_comp->comp_data.data(), this_comp->postsel_data.data()) != OK)
            return FAIL;
        comp = this_comp->postsel_data.data();
    }
    /*
        * Compensate or revert compensation?
        */
    Eigen::Map<const Eigen::VectorXf> compVec(comp, ndata);
    if (do_it)
        data -= compVec;
    else
        data += compVec;
    return OK;
}

//=============================================================================================================

int MNECTFCompDataSet::apply_transpose(bool do_it, Eigen::MatrixXf& data)
/*
     * Apply compensation or revert to uncompensated data
     */
{
    MNECTFCompData* this_comp;
    int ndata = static_cast<int>(data.rows());
    int ncompdata = ndata;

    if (!current)
        return OK;
    this_comp = current.get();
    /*
        * Dimension checks
        */
    if (this_comp->presel) {
        if (this_comp->presel->cols() != ncompdata) {
            mne::Log::critical("Compensation data dimension mismatch. Expected {}, got {} channels.", this_comp->presel->cols(), ncompdata);
            return FAIL;
        }
    } else if (this_comp->data->ncol() != ncompdata) {
        mne::Log::critical("Compensation data dimension mismatch. Expected {}, got {} channels.", this_comp->data->ncol(), ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->rows() != ndata) {
            mne::Log::critical("Data dimension mismatch. Expected {}, got {} channels.", this_comp->postsel->rows(), ndata);
            return FAIL;
        }
    } else if (this_comp->data->nrow() != ndata) {
        mne::Log::critical("Data dimension mismatch. Expected {}, got {} channels.", this_comp->data->nrow(), ndata);
        return FAIL;
    }
    /*
        * Preselection is optional — sparse matrix * data
        */
    Eigen::MatrixXf preselMat;
    if (this_comp->presel) {
        preselMat = this_comp->presel->eigen() * data;
    } else {
        preselMat = data;
    }
    /*
        * Compensation: comp = data_matrix * preselMat
        * data_matrix is float** (contiguous row-major via mne_cmatrix)
        */
    Eigen::MatrixXf comp = this_comp->data->data * preselMat;
    /*
        * Optional postselection — sparse matrix * comp
        */
    if (this_comp->postsel) {
        comp = this_comp->postsel->eigen() * comp;
    }
    /*
        * Compensate or revert compensation?
        */
    if (do_it)
        data -= comp;
    else
        data += comp;
    return OK;
}

//=============================================================================================================

int MNECTFCompDataSet::get_comp(const std::vector<mne::FiffChInfo>& chs, int nch)
{
    int res = MNE_CTFV_NOGRAD;
    int first_comp, comp;
    int k;

    for (k = 0, first_comp = -1; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comp = chs[k].chpos.coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (first_comp != comp) {
                mne::Log::critical("Non uniform compensation not supported.");
                return FAIL;
            }
        }
    }
    if (first_comp >= 0)
        res = first_comp;
    return res;
}

//=============================================================================================================

int MNECTFCompDataSet::map_comp_kind(int grad)
/*
     * Simple mapping
     */
{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (grad == compMap[k].grad_comp)
            return compMap[k].ctf_comp;
    return grad;
}

//=============================================================================================================

std::string MNECTFCompDataSet::explain_comp(int kind)
{
    static const struct
    {
        int kind;
        const char* expl;
    } explain[] = {{MNE_CTFV_COMP_NONE, "uncompensated"},
                   {MNE_CTFV_COMP_G1BR, "first order gradiometer"},
                   {MNE_CTFV_COMP_G2BR, "second order gradiometer"},
                   {MNE_CTFV_COMP_G3BR, "third order gradiometer"},
                   {MNE_4DV_COMP1, "4D comp 1"},
                   {MNE_CTFV_COMP_UNKNOWN, "unknown"}};
    int k;

    for (k = 0; explain[k].kind != MNE_CTFV_COMP_UNKNOWN; k++)
        if (explain[k].kind == kind)
            return explain[k].expl;
    return explain[k].expl;
}

//=============================================================================================================

int MNECTFCompDataSet::set_compensation(int compensate_to,
                                        std::vector<FiffChInfo>& chList,
                                        int nchan,
                                        std::vector<FiffChInfo> comp_chs,
                                        int ncomp_chan) /* How many */
/*
     * Make data which has the third-order gradient compensation applied
     */
{
    int k;
    int have_comp_chs;
    int comp_was = MNE_CTFV_COMP_UNKNOWN;

    if (comp_chs.empty()) {
        comp_chs = chList;
        ncomp_chan = nchan;
    }
    undo.reset();
    current.reset();
    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
            have_comp_chs++;
    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
        mne::Log::warning("No compensation channels in these data.");
        return FAIL;
    }
    /*
        * Update the 'current' field to reflect the compensation possibly present in the data now
        */
    if (make_comp(chList, nchan, comp_chs, ncomp_chan) == FAIL)
        return FAIL;
    /*
        * Are we there already?
        */
    if (current && current->mne_kind == compensate_to) {
        mne::Log::info("No further compensation necessary (comp = {})\n", explain_comp(current->kind));
        current.reset();
        return OK;
    }
    undo = std::move(current);
    if (compensate_to == MNE_CTFV_NOGRAD) {
        mne::Log::info("No compensation was requested.\n");
        set_comp(chList, nchan, compensate_to);
        return OK;
    }
    if (set_comp(chList, nchan, compensate_to) > 0) {
        if (undo)
            comp_was = undo->mne_kind;
        else
            comp_was = MNE_CTFV_NOGRAD;
        if (make_comp(chList, nchan, comp_chs, ncomp_chan) == FAIL) {
            if (comp_was != MNE_CTFV_COMP_UNKNOWN)
                set_comp(chList, nchan, comp_was);
            return FAIL;
        }
        mne::Log::info("Compensation set up as requested ({} -> {}).\n", explain_comp(map_comp_kind(comp_was)), explain_comp(current->kind));
    }
    return OK;
}
