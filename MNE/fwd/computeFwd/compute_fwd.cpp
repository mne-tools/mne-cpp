

#include "compute_fwd.h"

#include <inverse/dipoleFit/mne_source_space_old.h>
#include <inverse/dipoleFit/fiff_coord_trans_old.h>
#include <inverse/dipoleFit/fwd_coil_set.h>
#include <inverse/dipoleFit/mne_ctf_comp_data_set.h>
#include <inverse/dipoleFit/fwd_eeg_sphere_model_set.h>
#include <inverse/dipoleFit/fwd_bem_model.h>
#include <inverse/dipoleFit/mne_named_matrix.h>

#include <fiff/fiff_types.h>

#include <QCoreApplication>
#include <QFile>


using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace FWDLIB;


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif



#define X_41 0
#define Y_41 1
#define Z_41 2


#define MALLOC_41(x,t) (t *)malloc((x)*sizeof(t))

#define FREE_41(x) if ((char *)(x) != NULL) free((char *)(x))


#define VEC_COPY_41(to,from) {\
    (to)[X_41] = (from)[X_41];\
    (to)[Y_41] = (from)[Y_41];\
    (to)[Z_41] = (from)[Z_41];\
    }



#define VEC_DOT_41(x,y) ((x)[X_41]*(y)[X_41] + (x)[Y_41]*(y)[Y_41] + (x)[Z_41]*(y)[Z_41])

#define VEC_LEN_41(x) sqrt(VEC_DOT_41(x,x))



//fiffId get_file_id(char *name)
//{
//    fiffFile in = fiff_open(name);
//    fiffId   id;
//    if (in == NULL)
//        return NULL;
//    else {
//        id = MALLOC(1,fiffIdRec);
//        *id = *(in->id);
//        fiff_close(in);
//        return id;
//    }
//}


//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_41(const QString& name,
                                     fiffChInfo     *megp,	 /* MEG channels */
                                     int            *nmegp,
                                     fiffChInfo     *meg_compp,
                                     int            *nmeg_compp,
                                     fiffChInfo     *eegp,	 /* EEG channels */
                                     int            *neegp,
                                     FiffCoordTransOld* *meg_head_t,
                                     fiffId         *idp)	 /* The measurement ID */
/*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));


    fiffChInfo chs   = NULL;
    int        nchan = 0;
    fiffChInfo meg   = NULL;
    int        nmeg  = 0;
    fiffChInfo meg_comp = NULL;
    int        nmeg_comp = 0;
    fiffChInfo eeg   = NULL;
    int        neeg  = 0;
    fiffId     id    = NULL;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr info;
    FiffTag::SPtr t_pTag;
    fiffChInfo   this_ch = NULL;
    FiffCoordTransOld* t = NULL;
    fiff_int_t kind, pos;
    int j,k,to_find;

    if(!stream->open())
        goto bad;

    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (nodes.size() == 0) {
        nodes = stream->tree()->dir_tree_find(FIFFB_MEAS_INFO);
        if (nodes.size() == 0) {
            qCritical ("Could not find the channel information.");
            goto bad;
        }
    }
    info = nodes[0];
    to_find = 0;
    for (k = 0; k < info->nent; k++) {
        kind = info->dir[k]->kind;
        pos  = info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();
            chs = MALLOC_41(nchan,fiffChInfoRec);
            for (j = 0; j < nchan; j++)
                chs[j].scanNo = -1;
            to_find = nchan;
            break;

        case FIFF_PARENT_BLOCK_ID :
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
            //            id = t_pTag->toFiffID();
            *id = *(fiffId)t_pTag->data();
            break;

        case FIFF_COORD_TRANS :
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
            //            t = t_pTag->toCoordTrans();
            t = FiffCoordTransOld::read_helper( t_pTag );
            if (t->from != FIFFV_COORD_DEVICE || t->to   != FIFFV_COORD_HEAD)
                t = NULL;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
            //            this_ch = t_pTag->toChInfo();
            this_ch = (fiffChInfo)malloc(sizeof(fiffChInfoRec));
            *this_ch = *(fiffChInfo)(t_pTag->data());
            if (this_ch->scanNo <= 0 || this_ch->scanNo > nchan) {
                printf ("FIFF_CH_INFO : scan # out of range %d (%d)!",this_ch->scanNo,nchan);
                goto bad;
            }
            else
                chs[this_ch->scanNo-1] = *this_ch;
            to_find--;
            break;
        }
    }
    if (to_find != 0) {
        qCritical("Some of the channel information was missing.");
        goto bad;
    }
    if (t == NULL && meg_head_t != NULL) {
        /*
     * Try again in a more general fashion
     */
        if ((t = FiffCoordTransOld::mne_read_meas_transform(name)) == NULL) {
            qCritical("MEG -> head coordinate transformation not found.");
            goto bad;
        }
    }
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            nmeg++;
        else if (chs[k].kind == FIFFV_REF_MEG_CH)
            nmeg_comp++;
        else if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;
    if (nmeg > 0)
        meg = MALLOC_41(nmeg,fiffChInfoRec);
    if (neeg > 0)
        eeg = MALLOC_41(neeg,fiffChInfoRec);
    if (nmeg_comp > 0)
        meg_comp = MALLOC_41(nmeg_comp,fiffChInfoRec);
    neeg = nmeg = nmeg_comp = 0;

    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            meg[nmeg++] = chs[k];
        else if (chs[k].kind == FIFFV_REF_MEG_CH)
            meg_comp[nmeg_comp++] = chs[k];
        else if (chs[k].kind == FIFFV_EEG_CH)
            eeg[neeg++] = chs[k];
    //    fiff_close(in);
    stream->close();
    FREE_41(chs);
    if (megp) {
        *megp  = meg;
        *nmegp = nmeg;
    }
    else
        FREE_41(meg);
    if (meg_compp) {
        *meg_compp = meg_comp;
        *nmeg_compp = nmeg_comp;
    }
    else
        FREE_41(meg_comp);
    if (eegp) {
        *eegp  = eeg;
        *neegp = neeg;
    }
    else
        FREE_41(eeg);
    if (idp == NULL) {
        FREE_41(id);
    }
    else
        *idp   = id;
    if (meg_head_t == NULL) {
        FREE_41(t);
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
        //        fiff_close(in);
        stream->close();
        FREE_41(chs);
        FREE_41(meg);
        FREE_41(eeg);
        FREE_41(id);
        //        FREE_41(tag.data);
        FREE_41(t);
        return FIFF_FAIL;
    }
}



int mne_check_chinfo(fiffChInfo chs,
                     int        nch)
/*
 * Check that all EEG channels have reasonable locations
 */
{
    int k;
    fiffChInfo ch;
    float close = 0.02;

    for (k = 0, ch = chs; k < nch; k++, ch++) {
        if (ch->kind == FIFFV_EEG_CH) {
            if (VEC_LEN_41(ch->chpos.r0) < close) {
                qCritical("Some EEG channels do not have locations assigned.");
                return FAIL;
            }
        }
    }
    return OK;
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeFwd::ComputeFwd(ComputeFwdSettings* p_settings)
    : settings(p_settings)
{
}


//*************************************************************************************************************

ComputeFwd::~ComputeFwd()
{
    //ToDo Garbage collection
}


//*************************************************************************************************************

void ComputeFwd::calculateFwd() const
{
    bool                res = false;
    MneSourceSpaceOld*   *spaces = NULL;  /* The source spaces */
    int                 nspace  = 0;
    int                 nsource = 0;     /* Number of source space points */

    FiffCoordTransOld* mri_head_t = NULL;   /* MRI <-> head coordinate transformation */
    FiffCoordTransOld* meg_head_t = NULL;   /* MEG <-> head coordinate transformation */

    fiffChInfo     megchs   = NULL; /* The MEG channel information */
    int            nmeg     = 0;
    fiffChInfo     eegchs   = NULL; /* The EEG channel information */
    int            neeg     = 0;
    fiffChInfo     compchs = NULL;
    int            ncomp    = 0;

    FwdCoilSet*             megcoils = NULL;     /* The coil descriptions */
    FwdCoilSet*             compcoils = NULL;    /* MEG compensation coils */
    MneCTFCompDataSet*      comp_data  = NULL;
    FwdCoilSet*             eegels = NULL;
    FwdEegSphereModelSet*   eeg_models = NULL;

    MneNamedMatrix* meg_forward      = NULL;    /* Result of the MEG forward calculation */
    MneNamedMatrix* eeg_forward      = NULL;    /* Result of the EEG forward calculation */
    MneNamedMatrix* meg_forward_grad = NULL;    /* Result of the MEG forward gradient calculation */
    MneNamedMatrix* eeg_forward_grad = NULL;    /* Result of the EEG forward gradient calculation */
    int            k;
    fiffId         mri_id  = NULL;
    fiffId         meas_id = NULL;
    FILE           *out = NULL;     /* Output filtered points here */

    FwdCoilSet*       templates = NULL;
    FwdEegSphereModel* eeg_model = NULL;
    FwdBemModel*       bem_model = NULL;

    QString qPath;
    QFile file;


    /*
    * Report the setup
    */
    //    printf("\n");
    //    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("\n");
    printf("Source space                 : %s\n",settings->srcname.toLatin1().constData());
    if (!(settings->transname.isEmpty()) || !(settings->mriname.isEmpty()))
        printf("MRI -> head transform source : %s\n",!(settings->mriname.isEmpty()) ? settings->mriname.toLatin1().constData() : settings->transname.toLatin1().constData());
    else
        printf("MRI and head coordinates are assumed to be identical.\n");
    printf("Measurement data             : %s\n",settings->measname.toLatin1().constData());
    if (!settings->bemname.isEmpty())
        printf("BEM model                    : %s\n",settings->bemname.toLatin1().constData());
    else {
        printf("Sphere model                 : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
               1000*settings->r0[X_41],1000*settings->r0[Y_41],1000*settings->r0[Z_41]);
        if (settings->include_eeg) {
            printf("\n");

            if (settings->eeg_model_file.isEmpty()) {
                qCritical("!!!!!!!!!!TODO: default_eeg_model_file();");
                //                settings->eeg_model_file = default_eeg_model_file();
            }
            eeg_models = FwdEegSphereModelSet::fwd_load_eeg_sphere_models(settings->eeg_model_file,eeg_models);
            eeg_models->fwd_list_eeg_sphere_models(stderr);

            if (settings->eeg_model_name.isEmpty())
                settings->eeg_model_name = QString("Default");
            if ((eeg_model = eeg_models->fwd_select_eeg_sphere_model(settings->eeg_model_name)) == NULL)
                goto out;

            if (!eeg_model->fwd_setup_eeg_sphere_model(settings->eeg_sphere_rad,settings->use_equiv_eeg,3))
                goto out;

            printf("Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
                   settings->eeg_model_name.toLatin1().constData(),1000*settings->eeg_sphere_rad);
            printf("%s the electrode locations to scalp\n",settings->scale_eeg_pos ? "Scale" : "Do not scale");

            eeg_model->scale_pos = settings->scale_eeg_pos;
            VEC_COPY_41(eeg_model->r0,settings->r0);

            printf("\n");
        }
    }
    printf("%s field computations\n",settings->accurate ? "Accurate" : "Standard");
    printf("Do computations in %s coordinates.\n",FiffCoordTransOld::mne_coord_frame_name(settings->coord_frame));
    printf("%s source orientations\n",settings->fixed_ori ? "Fixed" : "Free");
    if (settings->compute_grad)
        printf("Compute derivatives with respect to source location coordinates\n");
    printf("Destination for the solution : %s\n",settings->solname.toLatin1().constData());
    if (settings->do_all)
        printf("Calculate solution for all source locations.\n");
    if (settings->nlabel > 0)
        printf("Source space will be restricted to sources in %d labels\n",settings->nlabel);
    /*
     * Read the source locations
     */
    printf("\n");
    printf("Reading %s...\n",settings->srcname.toLatin1().constData());
    if (MneSurfaceOrVolume::mne_read_source_spaces(settings->srcname,&spaces,&nspace) != OK)
        goto out;
    for (k = 0, nsource = 0; k < nspace; k++) {
        if (settings->do_all)
            MneSurfaceOrVolume::enable_all_sources(spaces[k]);
        nsource += spaces[k]->nuse;
    }
    if (nsource == 0) {
        qCritical("No sources are active in these source spaces. --all option should be used.");
        goto out;
    }
    printf("Read %d source spaces a total of %d active source locations\n", nspace,nsource);
    if (MneSurfaceOrVolume::restrict_sources_to_labels(spaces,nspace,settings->labels,settings->nlabel) == FAIL)
        goto out;
    /*
     * Read the MRI -> head coordinate transformation
     */
    printf("\n");
    if (!settings->mriname.isEmpty()) {
        if ((mri_head_t = FiffCoordTransOld::mne_read_mri_transform(settings->mriname)) == NULL)
            goto out;
//        if ((mri_id = get_file_id(settings->mriname)) == NULL) {
//            qCritical("Couln't read MRI file id (How come?)");
//            goto out;
//        }
    }
    else if (!settings->transname.isEmpty()) {
        FiffCoordTransOld* t;
        if ((t = FiffCoordTransOld::mne_read_FShead2mri_transform(settings->transname.toLatin1().data())) == NULL)
            goto out;
        mri_head_t = t->fiff_invert_transform();
        if(t)
            delete t;
    }
    else
        mri_head_t = FiffCoordTransOld::mne_identity_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
    FiffCoordTransOld::mne_print_coord_transform(stderr,mri_head_t);
    /*
    * Read the channel information
    * and the MEG device -> head coordinate transformation
    */
    printf("\n");
    if (mne_read_meg_comp_eeg_ch_info_41(settings->measname,&megchs,&nmeg,&compchs,&ncomp,&eegchs,&neeg,&meg_head_t,&meas_id) != OK)
        goto out;
    if (nmeg > 0)
        printf("Read %3d MEG channels from %s\n",nmeg,settings->measname.toLatin1().constData());
    if (ncomp > 0)
        printf("Read %3d MEG compensation channels from %s\n",ncomp,settings->measname.toLatin1().constData());
    if (neeg > 0)
        printf("Read %3d EEG channels from %s\n",neeg,settings->measname.toLatin1().constData());
    if (!settings->include_meg) {
        printf("MEG not requested. MEG channels omitted.\n");
        FREE_41(megchs); megchs = NULL;
        FREE_41(compchs); compchs = NULL;
        nmeg = 0;
        ncomp = 0;
    }
    else
        FiffCoordTransOld::mne_print_coord_transform(stderr,meg_head_t);
    if (!settings->include_eeg) {
        printf("EEG not requested. EEG channels omitted.\n");
        FREE_41(eegchs); eegchs = NULL;
        neeg = 0;
    }
    else {
        if (mne_check_chinfo(eegchs,neeg) != OK)
            goto out;
    }
    /*
    * Create coil descriptions with transformation to head or MRI frame
    */
    if (settings->include_meg) {
        //#ifdef USE_SHARE_PATH
        //        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
        //#else
        //        char *coilfile = mne_compose_mne_name("setup/mne","coil_def.dat");

        qPath = QString("./resources/coilDefinitions/coil_def.dat");
        file.setFileName(qPath);
        if ( !QCoreApplication::startingUp() )
            qPath = QCoreApplication::applicationDirPath() + QString("/resources/coilDefinitions/coil_def.dat");
        else if (!file.exists())
            qPath = "./bin/resources/coilDefinitions/coil_def.dat";

        char *coilfile = MALLOC_41(strlen(qPath.toLatin1().data())+1,char);
        strcpy(coilfile,qPath.toLatin1().data());

        //#endif

        if (!coilfile)
            goto out;
        templates = FwdCoilSet::read_coil_defs(coilfile);
        if (!templates)
            goto out;
        FREE_41(coilfile);
        /*
        * Compensation data
        */
        if ((comp_data = MneCTFCompDataSet::mne_read_ctf_comp_data(settings->measname)) == NULL)
            goto out;
        if (comp_data->ncomp > 0) /* Compensation channel information may be needed */
            printf("%d compensation data sets in %s\n",comp_data->ncomp,settings->measname.toLatin1().constData());
        else {
            FREE_41(compchs); compchs = NULL;
            ncomp = 0;

            if(comp_data)
                delete comp_data;
            comp_data = NULL;
        }
    }
    if (settings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTransOld* head_mri_t = mri_head_t->fiff_invert_transform();
        FiffCoordTransOld* meg_mri_t = FiffCoordTransOld::fiff_combine_transforms(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,meg_head_t,head_mri_t);
        if (meg_mri_t == NULL)
            goto out;
        if ((megcoils = templates->create_meg_coils(megchs,nmeg,
                                                    settings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                    meg_mri_t)) == NULL)
            goto out;
        if (ncomp > 0) {
            if ((compcoils = templates->create_meg_coils(   compchs,ncomp,
                                                            FWD_COIL_ACCURACY_NORMAL,meg_mri_t)) == NULL)
                goto out;
        }
        if ((eegels = FwdCoilSet::create_eeg_els(eegchs,neeg,head_mri_t)) == NULL)
            goto out;
        FREE_41(head_mri_t);
        printf("MRI coordinate coil definitions created.\n");
    }
    else {
        if ((megcoils = templates->create_meg_coils(megchs,nmeg,
                                             settings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                             meg_head_t)) == NULL)
            goto out;
        if (ncomp > 0) {
            if ((compcoils = templates->create_meg_coils(compchs,ncomp,
                                                  FWD_COIL_ACCURACY_NORMAL,meg_head_t)) == NULL)
                goto out;
        }
        if ((eegels = FwdCoilSet::create_eeg_els(eegchs,neeg,NULL)) == NULL)
            goto out;
        printf("Head coordinate coil definitions created.\n");
    }
    /*
    * Transform the source spaces into the appropriate coordinates
    */
    if (MneSurfaceOrVolume::mne_transform_source_spaces_to(settings->coord_frame,mri_head_t,spaces,nspace) != OK)
        goto out;
    printf("Source spaces are now in %s coordinates.\n",FiffCoordTransOld::mne_coord_frame_name(settings->coord_frame));
    /*
    * Prepare the BEM model if necessary
    */
    if (!settings->bemname.isEmpty()) {
        char *bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(settings->bemname.toLatin1().data());
//        FREE(bemname);
        settings->bemname = QString(bemsolname);

        printf("\nSetting up the BEM model using %s...\n",settings->bemname.toLatin1().constData());
        printf("\nLoading surfaces...\n");
        bem_model = FwdBemModel::fwd_bem_load_three_layer_surfaces(settings->bemname);
        if (bem_model) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            bem_model = FwdBemModel::fwd_bem_load_homog_surface(settings->bemname);
            if (!bem_model)
                goto out;
            printf("Homogeneous model surface loaded.\n");
        }
        if (neeg > 0 && bem_model->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            goto out;
        }
        printf("\nLoading the solution matrix...\n");
        if (FwdBemModel::fwd_bem_load_recompute_solution(settings->bemname.toLatin1().data(),FWD_BEM_UNKNOWN,FALSE,bem_model) == FAIL)
            goto out;
        if (settings->coord_frame == FIFFV_COORD_HEAD) {
            printf("Employing the head->MRI coordinate transform with the BEM model.\n");
            if (FwdBemModel::fwd_bem_set_head_mri_t(bem_model,mri_head_t) == FAIL)
                goto out;
        }
        printf("BEM model %s is now set up\n",bem_model->sol_name.toLatin1().constData());
    }
    else
        printf("Using the sphere model.\n");
    printf ("\n");
    /*
    * Try to circumvent numerical problems by excluding points too close our ouside the inner skull surface
    */
    if (settings->filter_spaces) {
        if (!settings->mindistoutname.isEmpty()) {
            out = fopen(settings->mindistoutname.toLatin1().constData(),"w");
            if (out == NULL) {
                qCritical() << settings->mindistoutname;
                goto out;
            }
            printf("Omitted source space points will be output to : %s\n",settings->mindistoutname.toLatin1().constData());
        }
        if (MneSurfaceOrVolume::filter_source_spaces(settings->mindist,
                                                     settings->bemname.toLatin1().data(),
                                                     mri_head_t,
                                                     spaces,
                                                     nspace,out,settings->use_threads) == FAIL)
            goto out;
        if (out) {
            fclose(out);
            out = NULL;
        }
    }
    //    /*
    //     * Do the actual computation
    //     */
    //    if (!bem_model)
    //        use_threads = FALSE;
    //    if (nmeg > 0)
    //        if ((compute_forward_meg(spaces,nspace,megcoils,compcoils,comp_data,
    //                                 fixed_ori,bem_model,r0,use_threads,&meg_forward,
    //                                 compute_grad ? &meg_forward_grad : NULL)) == FAIL)
    //            goto out;
    //    if (neeg > 0)
    //        if ((compute_forward_eeg(spaces,nspace,eegels,
    //                                 fixed_ori,bem_model,eeg_model,use_threads,&eeg_forward,
    //                                 compute_grad ? &eeg_forward_grad : NULL)) == FAIL)
    //            goto out;
    //    /*
    //     * Transform the source spaces back into MRI coordinates
    //     */
    //    if (mne_transform_source_spaces_to(FIFFV_COORD_MRI,mri_head_t,spaces,nspace) != OK)
    //        goto out;
    //    /*
    //     * We are ready to spill it out
    //     */
    //    fprintf (stderr,"\nwriting %s...",solname);
    //    if (write_solution(solname,	                /* Destination file */
    //                       spaces,			/* The source spaces */
    //                       nspace,
    //                       mriname,mri_id,		/* MRI file and data obtained from there */
    //                       mri_head_t,
    //                       measname,meas_id,		/* MEG file and data obtained from there */
    //                       meg_head_t,
    //                       megchs, nmeg,
    //                       eegchs, neeg,
    //                       fixed_ori,			/* Fixed orientation dipoles? */
    //                       coord_frame,               /* Coordinate frame */
    //                       meg_forward, eeg_forward,
    //                       meg_forward_grad, eeg_forward_grad) == FIFF_FAIL)

    //        goto out;
    //    if (mne_attach_env(solname,command) == FIFF_FAIL)
    //        goto out;
    //    printf("done\n");
    //    res = OK;
    //    printf("\nFinished.\n");

out : {
        //        if (out)
        //            fclose(out);
        for (k = 0; k < nspace; k++)
            if(spaces[k])
                delete spaces[k];
        if(mri_head_t)
            delete mri_head_t;
        if(meg_head_t)
            delete meg_head_t;
        FREE_41(megchs);
        FREE_41(eegchs);
        if(megcoils)
            delete megcoils;
        if(eegels)
            delete eegels;

        if(meg_forward)
            delete meg_forward;
        if(eeg_forward)
            delete eeg_forward;
        if(meg_forward_grad)
            delete meg_forward_grad;
        if(eeg_forward_grad)
            delete eeg_forward_grad;

        if (!res)
            qCritical("err_print_error();");//err_print_error();
        if (res)
            exit(0);
        else
            exit(1);
    }
}

