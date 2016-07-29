
#ifndef _fwd_h
#define _fwd_h

#include <stdio.h>
#include "fwd_types.h"

#if defined(__cplusplus) 
extern "C" {
#endif






/* fwd_spheredot.c */
extern double fwd_eeg_sphere_dot_r0(double r, float *rel1, float *rel2, float *r0);
extern void fwd_lead_dot_stats(FILE *f);
extern double fwd_sphere_dot_r0(double r, float *rmag1, float *rmag2, float *cosmag1, float *cosmag2, float *r0, int volume_integral);
/* fwd_spherefield.c */
extern int fwd_sphere_field(float *rd, float Q[], fwdCoilSet coils, float Bval[], void *client);
extern int fwd_sphere_field_vec(float *rd, fwdCoilSet coils, float **Bval, void *client);
extern int fwd_sphere_field_grad(float *rd, float Q[], fwdCoilSet coils, float Bval[], float xgrad[], float ygrad[], float zgrad[], void *client);
/* fwd_coil_def.c */
extern fwdCoilSet fwd_new_coil_set(void);
extern void fwd_free_coil_set_user_data(fwdCoilSet set);
extern void fwd_free_coil_set(fwdCoilSet set);
extern int fwd_is_axial_coil(fwdCoil coil);
extern int fwd_is_magnetometer_coil(fwdCoil coil);
extern int fwd_is_planar_coil(fwdCoil coil);
extern int fwd_is_eeg_electrode(fwdCoil coil);
extern int fwd_is_planar_coil_type(int type, fwdCoilSet set);
extern int fwd_is_axial_coil_type(int type, fwdCoilSet set);
extern int fwd_is_magnetometer_coil_type(int type, fwdCoilSet set);
extern int fwd_is_eeg_electrode_type(int type, fwdCoilSet set);
extern fwdCoilSet fwd_read_coil_defs(char *name);
extern fwdCoil fwd_create_meg_coil(fwdCoilSet set, fiffChInfo ch, int acc, fiffCoordTrans t);
extern fwdCoilSet fwd_create_meg_coils(fwdCoilSet set, fiffChInfo chs, int nch, int acc, fiffCoordTrans t);
extern fwdCoil fwd_create_eeg_el(fiffChInfo ch, fiffCoordTrans t);
extern fwdCoilSet fwd_create_eeg_els(fiffChInfo chs, int nch, fiffCoordTrans t);
extern fwdCoilSet fwd_create_eeg_els_at(float **rr, int np, fiffCoordTrans t);
extern void fwd_print_coil(FILE *out, fwdCoil coil);
extern fwdCoil fwd_dup_coil(fwdCoil c);
extern fwdCoilSet fwd_dup_coil_set(fwdCoilSet s, fiffCoordTrans t);
/* fwd_eeg_sphere_models.c */
extern void fwd_free_eeg_sphere_model(fwdEegSphereModel m);
extern fwdEegSphereModel fwd_dup_eeg_sphere_model(fwdEegSphereModel m);
extern void fwd_free_eeg_sphere_model_set(fwdEegSphereModelSet s);
extern void fwd_list_eeg_sphere_models(FILE *f, fwdEegSphereModelSet s);
extern fwdEegSphereModel fwd_select_eeg_sphere_model(char *name, fwdEegSphereModelSet s);
extern fwdEegSphereModelSet fwd_load_eeg_sphere_models(char *filename, fwdEegSphereModelSet now);
extern int fwd_setup_eeg_sphere_model(fwdEegSphereModel m, float rad, int fit_berg_scherg, int nfit);
/* fwd_multi_spherepot.c */
extern void fwd_eeg_multi_spherepot_stats(void);
extern double fwd_eeg_get_multi_sphere_model_coeff(fwdEegSphereModel m, int n);
extern int fwd_eeg_multi_spherepot_coil1(float *rd, float *Q, fwdCoilSet els, float *Vval, void *client);
extern int fwd_eeg_spherepot_vec(float *rd, float **el, int neeg, float **Vval_vec, void *client);
extern int fwd_eeg_spherepot(float *rd, float *Q, float **el, int neeg, float *Vval, void *client);
extern int fwd_eeg_spherepot_coil(float *rd, float *Q, fwdCoilSet els, float *Vval, void *client);
extern int fwd_eeg_spherepot_grad_coil(float *rd, float Q[], fwdCoilSet coils, float Vval[], float xgrad[], float ygrad[], float zgrad[], void *client);
extern int fwd_eeg_spherepot_coil_vec(float *rd, fwdCoilSet els, float **Vval_vec, void *client);
/* fwd_fit_berg_scherg.c */
extern int fwd_eeg_fit_berg_scherg(fwdEegSphereModel m, int nterms, int nfit, float *rv);
/* fwd_bem_model.c */
extern fwdBemModel fwd_bem_new_model(void);
extern void fwd_bem_free_solution(fwdBemModel m);
extern void fwd_bem_free_coil_solution(void *user);
extern fwdBemSolution fwd_bem_new_coil_solution(void);
extern void fwd_bem_free_model(fwdBemModel m);
extern char *fwd_bem_explain_surface(int kind);
extern char *fwd_bem_explain_method(int method);

//extern mneSurface fwd_bem_find_surface(fwdBemModel model, int kind);

mneSurface fwd_bem_find_surface(fwdBemModel model, int kind)
/*
 * Return a pointer to a specific surface in a BEM
 */
{
    int k;
    if (!model) {
        printf("No model specified for fwd_bem_find_surface");
        return NULL;
    }
    for (k = 0; k < model->nsurf; k++)
        if (model->surfs[k]->id == kind)
            return model->surfs[k];
    printf("Desired surface (%d = %s) not found.", kind,fwd_bem_explain_surface(kind));
    return NULL;
}



extern fwdBemModel fwd_bem_load_surfaces(char *name, int *kinds, int nkind);
extern fwdBemModel fwd_bem_load_homog_surface(char *name);
extern fwdBemModel fwd_bem_load_three_layer_surfaces(char *name);
extern int fwd_bem_save_model(char *name, fwdBemModel m);
extern int fwd_bem_load_solution(char *name, int bem_method, fwdBemModel m);
extern int fwd_bem_compute_solution(fwdBemModel m, int bem_method);
extern int fwd_bem_load_recompute_solution(char *name, int bem_method, int force_recompute, fwdBemModel m);
extern int fwd_bem_set_head_mri_t(fwdBemModel m, fiffCoordTrans t);
extern char *fwd_bem_make_bem_name(char *name);
extern char *fwd_bem_make_bem_sol_name(char *name);
/* fwd_bem_solution.c */
extern float **fwd_bem_multi_solution(float **solids, float **gamma, int nsurf, int *ntri);
extern float **fwd_bem_homog_solution(float **solids, int ntri);
extern void fwd_bem_ip_modify_solution(float **solution, float **ip_solution, float ip_mult, int nsurf, int *ntri);
/* fwd_bem_linear_collocation.c */
extern int fwd_bem_linear_collocation_solution(fwdBemModel m);
/* fwd_bem_constant_collocation.c */
extern int fwd_bem_constant_collocation_solution(fwdBemModel m);
/* fwd_bem_pot.c */
extern float fwd_bem_inf_pot(float *rd, float *Q, float *rp);
extern float fwd_bem_inf_pot_der(float *rd, float *Q, float *rp, float *comp);
extern int fwd_bem_pot(float *rd, float *Q, fwdBemModel m, int all_surfs, float *pot);
extern int fwd_bem_pot_els(float *rd, float *Q, fwdCoilSet els, float *pot, void *client);
extern int fwd_bem_pot_grad_els(float *rd, float *Q, fwdCoilSet els, float *pot, float *xgrad, float *ygrad, float *zgrad, void *client);
extern int fwd_bem_specify_els(fwdBemModel m, fwdCoilSet els);
/* fwd_bem_field.c */
extern float **fwd_bem_field_coeff(fwdBemModel m, fwdCoilSet coils);
extern void fwd_bem_one_lin_field_coeff_uran(float *dest, float *dir, mneTriangle tri, double *res);
extern void fwd_bem_one_lin_field_coeff_ferg(float *dest, float *dir, mneTriangle tri, double *res);
extern void fwd_bem_one_lin_field_coeff_simple(float *dest, float *normal, mneTriangle source, double *res);
extern float **fwd_bem_lin_field_coeff(fwdBemModel m, fwdCoilSet coils, int method);
extern int fwd_bem_specify_coils(fwdBemModel m, fwdCoilSet coils);
extern int fwd_bem_field(float *rd, float *Q, fwdCoilSet coils, float *B, void *client);
extern int fwd_bem_field_grad(float *rd, float Q[], fwdCoilSet coils, float Bval[], float xgrad[], float ygrad[], float zgrad[], void *client);
/* fwd_bem_radon.c */
extern void fwd_free_int_points(fwdIntPoints p);
extern fwdIntPoints fwd_radon_points(void);
extern double fwd_tri_int_approx(fwdIntApproxEvalFunc func, fwdIntPoints points, float *r1, float *r2, float *r3, float eps, void *user);
/* fwd_comp.c */
extern void fwd_free_comp_data(void *d);
extern fwdCompData fwd_new_comp_data(void);
extern fwdCompData fwd_make_comp_data(mneCTFcompDataSet set, fwdCoilSet coils, fwdCoilSet comp_coils, fwdFieldFunc field, fwdVecFieldFunc vec_field, fwdFieldGradFunc field_grad, void *client, fwdUserFreeFunc client_free);
extern int fwd_comp_field(float *rd, float *Q, fwdCoilSet coils, float *res, void *client);
extern int fwd_comp_field_vec(float *rd, fwdCoilSet coils, float **res, void *client);
extern int fwd_comp_field_grad(float *rd, float *Q, fwdCoilSet coils, float *res, float *xgrad, float *ygrad, float *zgrad, void *client);

#if defined(__cplusplus)
}
#endif
#endif
