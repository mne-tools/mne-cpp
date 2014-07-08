#ifndef FORMULAEDITOR_H
#define FORMULAEDITOR_H
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QtGui>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

#define ANZFUNKTKONST 10

class AtomPaintWindow;

namespace Ui
{
    class Formulaeditor;
}

class Formulaeditor : public QWidget
{

    Q_OBJECT
    
public:

    explicit Formulaeditor(QWidget *parent = 0);
    ~Formulaeditor();
    QString GetFormula();
    void StripFormula(QString& strFormula);
    void set_formula(QString Formula);
    void SetFunctConst(int index, double val);
    double Calculation(QString strFormula, qreal xValue, bool strip  =true);
    
private slots:

    void on_tb_A_textChanged(const QString &arg1);
    void on_tb_B_textChanged(const QString &arg1);
    void on_tb_C_textChanged(const QString &arg1);
    void on_tb_D_textChanged(const QString &arg1);
    void on_tb_E_textChanged(const QString &arg1);
    void on_tb_F_textChanged(const QString &arg1);
    void on_tb_G_textChanged(const QString &arg1);
    void on_tb_H_textChanged(const QString &arg1);
    void on_tb_Formula_textChanged(const QString &arg1);
    void on_btt_Test_clicked();
    void on_btt_Save_clicked();
    void on_dsb_StartValue_editingFinished();
    void on_dsb_StepWidth_editingFinished();

private:
    Ui::Formulaeditor *ui;
    AtomPaintWindow *callAtomPaintWindow;

    QString m_strFormula;
    QString m_strFunction;
    QString m_strErrortext;

    static QString g_strF;
    double m_dFktValue;
    double m_dFunctionConstant[ANZFUNKTKONST];
    QStringList m_strStandardFunction;

    qreal SignFactor(qint32 &nPosition, QString& strCharacter);
    double Expression(int& nPosition, QString& strCharacter);
    double SimpleExpression(int& nPosition, QString& strCharacter);
    double Term(int& nPosition, QString& strCharacter);
    double Factor(qint32 &nPosition, QString& strCharacter);
    double Char_n(int& nPosition, QString& strCharacter);
    QString strChar_(QString DecimalZahl);

    QString GetNextToken(QString& strSrc, const QString strDelim);
    double SINQ(double Winkel_grad);
    double COSQ(double Winkel_grad);
    double DEG(double x /* rad */) ;
    double RAD(double x /* grad */);
    double cot(double x);
    long double signl(long double x);
    double ArSinh(double x);
    double ArCosh(double x);
    double ArTanh(double x);
    double ArCoth(double x);
    double sqr(double x);

};

class AtomPaintWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
public:
   void PaintSignal(QList<qreal> valueList, QSize windowSize);

};

#endif // FORMULAEDITOR_H

