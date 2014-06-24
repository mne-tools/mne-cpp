#ifndef ENHANCEDEDITORWINDOW_H
#define ENHANCEDEDITORWINDOW_H
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// USED NAMESPACES



//=============================================================================================================

namespace Ui
{
    class Enhancededitorwindow;
}

class Enhancededitorwindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit Enhancededitorwindow(QWidget *parent = 0);
    ~Enhancededitorwindow();
    
private slots:
    void on_chb_allCombined_toggled(bool checked);

    void on_sb_Atomcount_editingFinished();

    void on_cb_AtomFormula_currentIndexChanged(const QString &arg1);

    void on_sb_Atomcount_valueChanged(int arg1);

    void on_btt_DeleteFormula_clicked();

private:
    Ui::Enhancededitorwindow *ui;
};

#endif // ENHANCEDEDITORWINDOW_H
