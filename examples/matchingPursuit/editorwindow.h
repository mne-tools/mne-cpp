//=============================================================================================================
/**
* @file editorwindow.h
* @author Martin Henfling <martin.henfling@tu-ilmenau.de>;
* Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
* Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version 1.0
* @date July, 2014
*
* @section LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
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
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QModelIndex>
#include <QtXml/QtXml>
#include <QDomDocument>

//=============================================================================================================
// USED NAMESPACES

namespace Ui
{

//=============================================================================================================

class EditorWindow;
}

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EditorWindow(QWidget *parent = 0);
    ~EditorWindow();

    enum AtomType
    {
             Gauss,
             Chirp
    };

private:    

    void read_dicts();
    void deleteDicts();
    void calc_scale_value();
    void calc_modu_value();
    void calc_phase_value();
    void calc_chirp_value();
    void calc_atom_count_all_combined();

    QList<qreal> calc_lin_pos_parameters(qreal startValue, qreal linStepValue);
    QList<qreal> calc_lin_neg_parameters(qreal startValue, qreal expStepValue);
    QList<qreal> calc_exp_pos_parameters(qreal startValue, qreal linStepValue);
    QList<qreal> calc_exp_neg_parameters(qreal startValue, qreal linStepValue);

    QList<qreal> calc_all_comb_parameter_values_scale(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calc_all_comb_parameter_values_modu(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calc_all_comb_parameter_values_phase(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calc_all_comb_parameter_values_chirp(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);

    QList<qreal> calc_parameter_values_scale(qreal startValue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calc_parameter_values_modu(qreal startValue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calc_parameter_values_phase(qreal startValue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calc_parameter_values_chirp(qreal startValue, qreal linStepValue, qreal expStepValue);

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

signals:

    void dict_saved();


private:    
    Ui::EditorWindow *ui;

protected:
    //void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
};

#endif // EDITORWINDOW_H
