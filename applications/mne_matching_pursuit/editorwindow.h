//=============================================================================================================
/**
 * @file editorwindow.h
 * @author   Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>
 * @version 1.0
 * @date July, 2014
 *
 * @section LICENSE
 *
 * Copyright (C) 2014, Daniel Knobl. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 * following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided with the distribution.
 * * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written permission.
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
 * @brief Editorwindow class declaration which enables the generation of individual dictionaries. Gaussian
 * atoms (with parameters scale, modulation and phase) or chirp atoms could be created and saved as
 * part dictionaries. For using the atoms for decompostion it's necessary to build an entire
 * dictionary from serveral (minimum one) part dictionaries.
 *
 */

#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne.h>
#include <utils/mp/fixdictmp.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QModelIndex>
#include <QtXml/QtXml>
#include <QDomDocument>

//=============================================================================================================
// USED NAMESPACES

using namespace MNELIB;

namespace Ui
{
    class EditorWindow;
}
//=============================================================================================================

class AtomWindow;
class XAxisAtomWindow;

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:

    //*********************************************************************************************************
    //constructor
    explicit EditorWindow(QWidget *parent = 0);
    //*********************************************************************************************************

    ~EditorWindow();
    enum AtomType
    {
             Gauss,
             Chirp
    };

private:    

    //========================================================================================================
    /**
     * EditorWindow_read_dicts
     *
     * ### MP toolbox EditorWindow function ###
     *
     * reads dictionaries
     *
     * @return void
     */
    void read_dicts();

    //========================================================================================================
    /**
     * EditorWindow_delete_dicts
     *
     * ### MP toolbox EditorWindow function ###
     *
     * delete dictionaries
     *
     * @return void
     */
    void deleteDicts();

    //========================================================================================================
    /**
     * EditorWindow_calc_scale_value
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates scale value
     *
     * @return void
     */
    void calc_scale_value();

    //========================================================================================================
    /**
     * EditorWindow_calc_modu_value
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates modulation value
     *
     * @return void
     */
    void calc_modu_value();

    //========================================================================================================
    /**
     * EditorWindow_calc_phase_value
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates phase value
     *
     * @return void
     */
    void calc_phase_value();

    //========================================================================================================
    /**
     * EditorWindow_calc_chirp_value
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates chirp value
     *
     * @return void
     */
    void calc_chirp_value();

    //========================================================================================================
    /**
     * EditorWindow_calc_atom_count_all_combined
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates number of atoms for all combined
     *
     * @return void
     */
    void calc_atom_count_all_combined();

    //========================================================================================================
    /**
     * EditorWindow_calc_lin_pos_parameters
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates parameters for linear step width in positiv direction
     *
     * @param    startValue      start value
     * @param    linStepValue    linear step width
     *
     * @return   QList<real>     parameters for linear step width in positiv direction
     */
    QList<qreal> calc_lin_pos_parameters(qreal startValue, qreal linStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_lin_neg_parameters
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates parameters for linear step width in negativ direction
     *
     * @param    startValue      start value
     * @param    linStepValue    linear step width
     *
     * @return   QList<real>     parameters for linear step width in negativ direction
     */
    QList<qreal> calc_lin_neg_parameters(qreal startValue, qreal linStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_exp_pos_parameters
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates parameters for exponential step width in positiv direction
     *
     * @param    startValue      start value
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     parameters for exponential step width in positiv direction
     */
    QList<qreal> calc_exp_pos_parameters(qreal startValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_exp_neg_parameters
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates parameters for exponential step width negativ direction
     *
     * @param    startValue      start value
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     parameters for exponential step width in negativ direction
     */
    QList<qreal> calc_exp_neg_parameters(qreal startValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_all_comb_parameter_values_scale
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates scale parameters for "all combined"
     *
     * @param    startValue      start value
     * @param    endValue        end value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     scale parameters for "all combined"
     */
    QList<qreal> calc_all_comb_parameter_values_scale(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_all_comb_parameter_values_modu
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates modulation parameters for "all combined"
     *
     * @param    startValue      start value
     * @param    endValue        end value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     modulation parameters for "all combined"
     */
    QList<qreal> calc_all_comb_parameter_values_modu(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_all_comb_parameter_values_phase
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates phase parameters for "all combined"
     *
     * @param    startValue      start value
     * @param    endValue        end value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     phase parameters for "all combined"
     */
    QList<qreal> calc_all_comb_parameter_values_phase(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_all_comb_parameter_values_chirp
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates chirp parameters for "all combined"
     *
     * @param    startValue      start value
     * @param    endValue        end value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     chirp parameters for "all combined"
     */
    QList<qreal> calc_all_comb_parameter_values_chirp(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_parameter_values_scale
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates scale parameters
     *
     * @param    startValue      start value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     scale parameters
     */
    QList<qreal> calc_parameter_values_scale(qreal startValue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_parameter_values_modu
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates modulation parameters
     *
     * @param    startValue      start value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     modulation parameters
     */
    QList<qreal> calc_parameter_values_modu(qreal startValue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_parameter_values_phase
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates phase parameters
     *
     * @param    startValue      start value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     phase parameters
     */
    QList<qreal> calc_parameter_values_phase(qreal startValue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================
    /**
     * EditorWindow_calc_parameter_values_chirp
     *
     * ### MP toolbox EditorWindow function ###
     *
     * calculates chirp parameters
     *
     * @param    startValue      start value
     * @param    linStepValue    linear step width
     * @param    expStepValue    exponential step width
     *
     * @return   QList<real>     chirp parameters
     */
    QList<qreal> calc_parameter_values_chirp(qreal startValue, qreal linStepValue, qreal expStepValue);

    //========================================================================================================

private slots:    
    void on_btt_CalcAtoms_clicked();
    void on_spb_AtomCount_valueChanged(int arg1);
    void on_dspb_StartValueScale_editingFinished();
    void on_rb_PosCountScale_toggled();
    void on_rb_NoStepScale_toggled(bool checked);
    void on_rb_LinStepScale_toggled(bool checked);
    void on_rb_ExpStepScale_toggled(bool checked);
    void on_dspb_ExpStepScale_editingFinished();
    void on_rb_NegCountScale_toggled();
    void on_dspb_LinStepScale_editingFinished();
    void on_rb_NoStepModu_toggled(bool checked);
    void on_rb_NoStepPhase_toggled(bool checked);
    void on_rb_NoStepChirp_toggled(bool checked);
    void on_dspb_LinStepPhase_editingFinished();
    void on_dspb_LinStepModu_editingFinished();
    void on_dspb_ExpStepModu_editingFinished();
    void on_dspb_StartValueModu_editingFinished();
    void on_rb_PosCountModu_toggled();
    void on_rb_NegCountModu_toggled();
    void on_dspb_ExpStepPhase_editingFinished();
    void on_dspb_StartValuePhase_editingFinished();
    void on_rb_PosCountPhase_toggled();
    void on_rb_NegCountPhase_toggled();
    void on_dspb_LinStepChirp_editingFinished();
    void on_dspb_ExpStepChirp_editingFinished();
    void on_dspb_StartValueChirp_editingFinished();
    void on_rb_PosCountChirp_toggled();
    void on_rb_NegCountChirp_toggled();
    void on_rb_LinStepModu_toggled(bool checked);
    void on_rb_ExpStepModu_toggled(bool checked);
    void on_rb_LinStepPhase_toggled(bool checked);
    void on_rb_ExpStepPhase_toggled(bool checked);
    void on_rb_LinStepChirp_toggled(bool checked);
    void on_rb_ExpStepChirp_toggled(bool checked);
    void on_rb_GaussAtomType_toggled(bool checked);
    void on_rb_ChirpAtomType_toggled(bool checked);
    void on_spb_AtomLength_editingFinished();
    void on_tb_PartDictName_editingFinished();
    void on_chb_CombAllPara_toggled(bool checked);
    void on_dspb_EndValueScale_editingFinished();
    void on_dspb_EndValueModu_editingFinished();
    void on_dspb_EndValuePhase_editingFinished();
    void on_dspb_EndValueChirp_editingFinished();
    void on_btt_ToNewDict_clicked();
    void on_list_AllDict_doubleClicked();
    void on_btt_ToAlldict_clicked();
    void on_list_NewDict_doubleClicked();
    void on_btt_DeleteDict_clicked();
    void on_list_AllDict_itemSelectionChanged();
    void on_list_NewDict_itemSelectionChanged();
    void on_btt_SaveDicts_clicked();
    void on_save_dicts();
    void animation_finished();
    void on_btt_extended_clicked();
    void on_li_all_dicts_itemSelectionChanged();
    void on_btt_next_dict_clicked();
    void on_btt_prev_dict_clicked();
    void atom_changed(qint32 atom_number);

    void on_spb_atom_number_editingFinished();

    void on_gb_atom_editor_toggled(bool arg1);

    void on_gb_dict_editor_toggled(bool arg1);

    void on_gb_atom_viewer_toggled(bool arg1);

signals:
    void dict_saved();


private:    
    qint32 current_atom_number;
    Ui::EditorWindow *ui;
    AtomWindow *callAtomWindow;
    XAxisAtomWindow *callXAxisAtomWindow;
    Dictionary current_dict;
    void closeEvent(QCloseEvent * event);
    void resizeEvent(QResizeEvent *event);

protected:
    void keyReleaseEvent(QKeyEvent *event);
};

//*************************************************************************************************************
// Widget to paint inputsignal
class AtomWindow : public QWidget
{
    Q_OBJECT
//private:
    //fiff_int_t press_pos;

protected:
   void paintEvent(QPaintEvent *event);
   //void mouseMoveEvent(QMouseEvent *event);
   //void mousePressEvent(QMouseEvent *event);
   //void mouseReleaseEvent(QMouseEvent *event);
   //void wheelEvent (QWheelEvent *event);

public:
   //==========================================================================================================
   /**
   * AtomWindow_paint_signal
   *
   * ### MP toolbox GUI function ###
   *
   * painting input signal of chosen channels in butterfly plot
   *
   * @param[in] atom_matrix    matrix of input signal
   * @param[in] window_size      size (height,width) of window
   *
   * @return void
   */
   void paint_signal(MatrixXd atom_matrix, QSize window_size);

//signals:
  //  void read_new();

   //==========================================================================================================
};

// Widget to paint x-axis
class XAxisAtomWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);

public:
   //==========================================================================================================
   /**
   * XAxisAtomWindow_paint_signal
   *
   * ### MP toolbox GUI function ###
   *
   * painting x-axis of chosen channels in butterfly plot
   *
   * @param[in] atom_matrix    matrix of input signal
   * @param[in] window_size      size (height,width) of window
   *
   * @return void
   */
   void paint_axis(MatrixXd atom_matrix, QSize window_size);

   //==========================================================================================================
};


#endif // EDITORWINDOW_H
