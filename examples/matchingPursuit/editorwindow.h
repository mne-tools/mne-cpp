#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QModelIndex>

//=============================================================================================================
// USED NAMESPACES



//=============================================================================================================


namespace Ui {
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

    void readDicts();

    void calcScaleValue();
    void calcModuValue();
    void calcPhaseValue();
    void calcChirpValue();

    void calcAtomCountAllCombined();

    QList<qreal> calcLinPosParameters(qreal startValue, qreal linStepValue);
    QList<qreal> calcLinNegParameters(qreal startValue, qreal expStepValue);
    QList<qreal> calcExpPosParameters(qreal startValue, qreal linStepValue);
    QList<qreal> calcExpNegParameters(qreal startValue, qreal linStepValue);

    QList<qreal> calcAllCombParameterValuesScale(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calcAllCombParameterValuesModu(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calcAllCombParameterValuesPhase(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calcAllCombParameterValuesChirp(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue);

    QList<qreal> calcParameterValuesScale(qreal startValue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calcParameterValuesModu(qreal startValue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calcParameterValuesPhase(qreal startValue, qreal linStepValue, qreal expStepValue);
    QList<qreal> calcParameterValuesChirp(qreal startValue, qreal linStepValue, qreal expStepValue);


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

private:
    Ui::EditorWindow *ui;
};

#endif // EDITORWINDOW_H
