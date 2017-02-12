

#include "compute_fwd.h"

#include <inverse/dipoleFit/mne_surface_or_volume.h>
#include <inverse/dipoleFit/fiff_coord_trans_old.h>
#include <inverse/dipoleFit/fwd_coil_set.h>
#include <inverse/dipoleFit/mne_ctf_comp_data_set.h>
#include <inverse/dipoleFit/fwd_eeg_sphere_model_set.h>
#include <inverse/dipoleFit/fwd_bem_model.h>
#include <inverse/dipoleFit/mne_named_matrix.h>

#include <fiff/fiff_types.h>



using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace FWDLIB;



#define X_41 0
#define Y_41 1
#define Z_41 2


#define FREE_41(x) if ((char *)(x) != NULL) free((char *)(x))


#define VEC_COPY_41(to,from) {\
    (to)[X_41] = (from)[X_41];\
    (to)[Y_41] = (from)[Y_41];\
    (to)[Z_41] = (from)[Z_41];\
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
    QList<MneCSourceSpace*> spaces;  /* The source spaces */
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
//    FILE           *out = NULL;		  /* Output filtered points here */

    FwdCoilSet*       templates = NULL;
    FwdEegSphereModel* eeg_model = NULL;
    FwdBemModel*       bem_model = NULL;

    /*
    * Report the setup
    */
//    fprintf(stderr,"\n");
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
//    fprintf(stderr,"%s field computations\n",accurate ? "Accurate" : "Standard");
//    fprintf(stderr,"Do computations in %s coordinates.\n",mne_coord_frame_name(coord_frame));
//    fprintf(stderr,"%s source orientations\n",fixed_ori ? "Fixed" : "Free");
//    if (compute_grad)
//        fprintf(stderr,"Compute derivatives with respect to source location coordinates\n");
//    fprintf(stderr,"Destination for the solution : %s\n",solname);
//    if (do_all)
//        fprintf(stderr,"Calculate solution for all source locations.\n");
//    if (nlabel > 0)
//        fprintf(stderr,"Source space will be restricted to sources in %d labels\n",nlabel);
//    /*
//     * Read the source locations
//     */
//    fprintf(stderr,"\n");
//    fprintf(stderr,"Reading %s...\n",srcname);
//    if (mne_read_source_spaces(srcname,&spaces,&nspace) != OK)
//        goto out;
//    for (k = 0, nsource = 0; k < nspace; k++) {
//        if (do_all)
//            enable_all_sources(spaces[k]);
//        nsource += spaces[k]->nuse;
//    }
//    if (nsource == 0) {
//        err_set_error("No sources are active in these source spaces. --all option should be used.");
//        goto out;
//    }
//    fprintf(stderr,"Read %d source spaces a total of %d active source locations\n",
//            nspace,nsource);
//    if (restrict_sources_to_labels(spaces,nspace,labels,nlabel) == FAIL)
//        goto out;
//    /*
//     * Read the MRI -> head coordinate transformation
//     */
//    fprintf(stderr,"\n");
//    if (mriname) {
//        if ((mri_head_t = mne_read_mri_transform(mriname)) == NULL)
//            goto out;
//        if ((mri_id = get_file_id(mriname)) == NULL) {
//            err_set_error("Couln't read MRI file id (How come?)");
//            goto out;
//        }
//    }
//    else if (transname) {
//        fiffCoordTrans t;
//        if ((t = mne_read_FShead2mri_transform(transname)) == NULL)
//            goto out;
//        mri_head_t = fiff_invert_transform(t);
//        FREE(t);
//    }
//    else
//        mri_head_t = mne_identity_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
//    mne_print_coord_transform(stderr,mri_head_t);
//    /*
//     * Read the channel information
//     * and the MEG device -> head coordinate transformation
//     */
//    fprintf(stderr,"\n");
//    if (mne_read_meg_comp_eeg_ch_info(measname,&megchs,&nmeg,&compchs,&ncomp,&eegchs,&neeg,&meg_head_t,&meas_id) != OK)
//        goto out;
//    if (nmeg > 0)
//        fprintf(stderr,"Read %3d MEG channels from %s\n",nmeg,measname);
//    if (ncomp > 0)
//        fprintf(stderr,"Read %3d MEG compensation channels from %s\n",ncomp,measname);
//    if (neeg > 0)
//        fprintf(stderr,"Read %3d EEG channels from %s\n",neeg,measname);
//    if (!include_meg) {
//        fprintf(stderr,"MEG not requested. MEG channels omitted.\n");
//        FREE(megchs); megchs = NULL;
//        FREE(compchs); compchs = NULL;
//        nmeg = 0;
//        ncomp = 0;
//    }
//    else
//        mne_print_coord_transform(stderr,meg_head_t);
//    if (!include_eeg) {
//        fprintf(stderr,"EEG not requested. EEG channels omitted.\n");
//        FREE(eegchs); eegchs = NULL;
//        neeg = 0;
//    }
//    else {
//        if (mne_check_chinfo(eegchs,neeg) != OK)
//            goto out;
//    }
//    /*
//     * Create coil descriptions with transformation to head or MRI frame
//     */
//    if (include_meg) {
//#ifdef USE_SHARE_PATH
//        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
//#else
//        char *coilfile = mne_compose_mne_name("setup/mne","coil_def.dat");
//#endif
//        if (!coilfile)
//            goto out;
//        templates = fwd_read_coil_defs(coilfile);
//        if (!templates)
//            goto out;
//        FREE(coilfile);
//        /*
//       * Compensation data
//       */
//        if ((comp_data = mne_read_ctf_comp_data(measname)) == NULL)
//            goto out;
//        if (comp_data->ncomp > 0) 	/* Compensation channel information may be needed */
//            fprintf(stderr,"%d compensation data sets in %s\n",comp_data->ncomp,measname);
//        else {
//            FREE(compchs); compchs = NULL;
//            ncomp = 0;

//            mne_free_ctf_comp_data_set(comp_data);
//            comp_data = NULL;
//        }
//    }
//    if (coord_frame == FIFFV_COORD_MRI) {
//        fiffCoordTrans head_mri_t = fiff_invert_transform(mri_head_t);
//        fiffCoordTrans meg_mri_t = fiff_combine_transforms(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,meg_head_t,head_mri_t);
//        if (meg_mri_t == NULL)
//            goto out;
//        if ((megcoils = fwd_create_meg_coils(templates,megchs,nmeg,
//                                             accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
//                                             meg_mri_t)) == NULL)
//            goto out;
//        if (ncomp > 0) {
//            if ((compcoils = fwd_create_meg_coils(templates,compchs,ncomp,
//                                                  FWD_COIL_ACCURACY_NORMAL,meg_mri_t)) == NULL)
//                goto out;
//        }
//        if ((eegels = fwd_create_eeg_els(eegchs,neeg,head_mri_t)) == NULL)
//            goto out;
//        FREE(head_mri_t);
//        fprintf(stderr,"MRI coordinate coil definitions created.\n");
//    }
//    else {
//        if ((megcoils = fwd_create_meg_coils(templates,megchs,nmeg,
//                                             accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
//                                             meg_head_t)) == NULL)
//            goto out;
//        if (ncomp > 0) {
//            if ((compcoils = fwd_create_meg_coils(templates,compchs,ncomp,
//                                                  FWD_COIL_ACCURACY_NORMAL,meg_head_t)) == NULL)
//                goto out;
//        }
//        if ((eegels = fwd_create_eeg_els(eegchs,neeg,NULL)) == NULL)
//            goto out;
//        fprintf(stderr,"Head coordinate coil definitions created.\n");
//    }
//    /*
//     * Transform the source spaces into the appropriate coordinates
//     */
//    if (mne_transform_source_spaces_to(coord_frame,mri_head_t,spaces,nspace) != OK)
//        goto out;
//    fprintf(stderr,"Source spaces are now in %s coordinates.\n",mne_coord_frame_name(coord_frame));
//    /*
//     * Prepare the BEM model if necessary
//     */
//    if (bemname) {
//        char *bemsolname = fwd_bem_make_bem_sol_name(bemname);
//        FREE(bemname); bemname = bemsolname;

//        fprintf(stderr,"\nSetting up the BEM model using %s...\n",bemname);
//        fprintf(stderr,"\nLoading surfaces...\n");
//        bem_model = fwd_bem_load_three_layer_surfaces(bemname);
//        if (bem_model) {
//            fprintf(stderr,"Three-layer model surfaces loaded.\n");
//        }
//        else {
//            bem_model = fwd_bem_load_homog_surface(bemname);
//            if (!bem_model)
//                goto out;
//            fprintf(stderr,"Homogeneous model surface loaded.\n");
//        }
//        if (neeg > 0 && bem_model->nsurf == 1) {
//            err_set_error("Cannot use a homogeneous model in EEG calculations.");
//            goto out;
//        }
//        fprintf(stderr,"\nLoading the solution matrix...\n");
//        if (fwd_bem_load_recompute_solution(bemname,FWD_BEM_UNKNOWN,FALSE,bem_model) == FAIL)
//            goto out;
//        if (coord_frame == FIFFV_COORD_HEAD) {
//            fprintf(stderr,"Employing the head->MRI coordinate transform with the BEM model.\n");
//            if (fwd_bem_set_head_mri_t(bem_model,mri_head_t) == FAIL)
//                goto out;
//        }
//        fprintf(stderr,"BEM model %s is now set up\n",bem_model->sol_name);
//    }
//    else
//        fprintf(stderr,"Using the sphere model.\n");
//    fprintf (stderr,"\n");
//    /*
//     * Try to circumvent numerical problems by excluding points too close our ouside the inner skull surface
//     */
//    if (filter_spaces) {
//        if (mindistoutname != NULL) {
//            out = fopen(mindistoutname,"w");
//            if (out == NULL) {
//                err_set_sys_error(mindistoutname);
//                goto out;
//            }
//            fprintf(stderr,"Omitted source space points will be output to : %s\n",mindistoutname);
//        }
//        if (filter_source_spaces(mindist,bemname,mri_head_t,spaces,nspace,out,use_threads) == FAIL)
//            goto out;
//        if (out) {
//            fclose(out);
//            out = NULL;
//        }
//    }
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
//    fprintf(stderr,"done\n");
//    res = OK;
//    fprintf(stderr,"\nFinished.\n");

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

