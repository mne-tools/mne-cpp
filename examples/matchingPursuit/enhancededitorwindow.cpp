//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"

#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "QtGui"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================
Enhancededitorwindow::Enhancededitorwindow(QWidget *parent) :    QWidget(parent),    ui(new Ui::Enhancededitorwindow)
{
    this->setAccessibleName("formel");
    ui->setupUi(this);
    //this->setFixedWidth(627);
    //this->setFixedHeight(160);
    QString contents;
    QFile formulaFile("Matching-Pursuit-Toolbox/user.fml");
    //if (formulaFile == ) return; //Aufpassen
    if (formulaFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while(!formulaFile.atEnd())
        {
            contents = formulaFile.readLine(0).constData();
            ui->cb_AtomFormula->addItem(QIcon(":/images/icons/formel.png"), contents.trimmed());
        }
    }
    formulaFile.close();
    if(ui->cb_AtomFormula->count() != 0)
        ui->cb_AtomFormula->setCurrentIndex(0);
}

Enhancededitorwindow::~Enhancededitorwindow()
{
    delete ui;
}

// when "all combined" toogelt
void Enhancededitorwindow::on_chb_allCombined_toggled(bool checked)
{
    if(checked)
    {
        ui->sb_Atomcount->setEnabled(false);
        ui->dsb_EndValueA->setEnabled(true);
        ui->dsb_EndValueB->setEnabled(true);
        ui->dsb_EndValueC->setEnabled(true);
        ui->dsb_EndValueD->setEnabled(true);
        ui->dsb_EndValueE->setEnabled(true);
        ui->dsb_EndValueF->setEnabled(true);
        ui->dsb_EndValueG->setEnabled(true);
        ui->dsb_EndValueH->setEnabled(true);
    }
    else
    {
        ui->sb_Atomcount->setEnabled(true);
        ui->dsb_EndValueA->setEnabled(false);
        ui->dsb_EndValueB->setEnabled(false);
        ui->dsb_EndValueC->setEnabled(false);
        ui->dsb_EndValueD->setEnabled(false);
        ui->dsb_EndValueE->setEnabled(false);
        ui->dsb_EndValueF->setEnabled(false);
        ui->dsb_EndValueG->setEnabled(false);
        ui->dsb_EndValueH->setEnabled(false);
    }
}


// when number of atoms was changed
void Enhancededitorwindow::on_sb_Atomcount_editingFinished()
{

}

// when number of atoms is changing
void Enhancededitorwindow::on_sb_Atomcount_valueChanged(int arg1)
{
    if(ui->sb_Atomcount->value() <= 1)
    {
        ui->dsb_StepVauleA->setEnabled(false);
        ui->dsb_StepVauleB->setEnabled(false);
        ui->dsb_StepVauleC->setEnabled(false);
        ui->dsb_StepVauleD->setEnabled(false);
        ui->dsb_StepVauleE->setEnabled(false);
        ui->dsb_StepVauleF->setEnabled(false);
        ui->dsb_StepVauleG->setEnabled(false);
        ui->dsb_StepVauleH->setEnabled(false);
    }
    else
    {
        ui->dsb_StepVauleA->setEnabled(true);
        ui->dsb_StepVauleB->setEnabled(true);
        ui->dsb_StepVauleC->setEnabled(true);
        ui->dsb_StepVauleD->setEnabled(true);
        ui->dsb_StepVauleE->setEnabled(true);
        ui->dsb_StepVauleF->setEnabled(true);
        ui->dsb_StepVauleG->setEnabled(true);
        ui->dsb_StepVauleH->setEnabled(true);
    }
}

// when formula is changed
void Enhancededitorwindow::on_cb_AtomFormula_currentIndexChanged(const QString &arg1)
{
    QList<QChar> foundChar;
    foundChar.clear();
    for(qint32 i = 0; i < arg1.length(); i++)
    {
        bool beforeFound = false;
        bool nextfound = false;
        QChar upperChar = arg1.at(i).toUpper();
        if((upperChar >= 'A' && upperChar <= 'H') || upperChar == 'X')
        {
            if(i != 0)
            {
                QChar beforeUpperChar = arg1.at(i - 1).toUpper();
                if(beforeUpperChar < 'A' || (beforeUpperChar > 'Z' && beforeUpperChar < 126))   beforeFound = true;
            }
            else    beforeFound = true;

            if(i < arg1.length() - 1)
            {
                QChar nextUpperChar = arg1.at(i+1).toUpper();
                if(nextUpperChar < 'A' || (nextUpperChar > 'Z' && nextUpperChar < 126))     nextfound = true;
            }
            else    nextfound = true;


            if(beforeFound && nextfound)
            {
                if(upperChar == 'A')        foundChar.append(upperChar);
                else if(upperChar == 'B')   foundChar.append(upperChar);
                else if(upperChar == 'C')   foundChar.append(upperChar);
                else if(upperChar == 'D')   foundChar.append(upperChar);
                else if(upperChar == 'E')   foundChar.append(upperChar);
                else if(upperChar == 'F')   foundChar.append(upperChar);
                else if(upperChar == 'G')   foundChar.append(upperChar);
                else if(upperChar == 'H')   foundChar.append(upperChar);
                else if(upperChar == 'X')   foundChar.append(upperChar);
            }
        }
    }

    ui->fr_A->setHidden(true);
    ui->fr_B->setHidden(true);
    ui->fr_C->setHidden(true);
    ui->fr_D->setHidden(true);
    ui->fr_E->setHidden(true);
    ui->fr_F->setHidden(true);
    ui->fr_G->setHidden(true);
    ui->fr_H->setHidden(true);

    for(qint32 j = 0; j < foundChar.length(); j++)
    {
        if(foundChar.at(j) =='A') ui->fr_A->setHidden(false);
        else if(foundChar.at(j) =='B') ui->fr_B->setHidden(false);
        else if(foundChar.at(j) =='C') ui->fr_C->setHidden(false);
        else if(foundChar.at(j) =='D') ui->fr_D->setHidden(false);
        else if(foundChar.at(j) =='E') ui->fr_E->setHidden(false);
        else if(foundChar.at(j) =='F') ui->fr_F->setHidden(false);
        else if(foundChar.at(j) =='G') ui->fr_G->setHidden(false);
        else if(foundChar.at(j) =='H') ui->fr_H->setHidden(false);
    }

    resize(minimumSize());
    setFixedHeight(sizeHint().height()); // no user access
}


void Enhancededitorwindow::on_btt_DeleteFormula_clicked()
{
    QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
    bool showMsgBox = false;
    QString contents;
    if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        while(!configFile.atEnd())
        {
            contents = configFile.readLine(0).constData();
            if(contents.startsWith("ShowDeleteFormelMessageBox=true;"))
                showMsgBox = true;
        }
    }
    configFile.close();

    if(showMsgBox)
    {
        DeleteMessageBox* msgBox = new DeleteMessageBox(this);
        msgBox->setModal(true);
        qint32 result = msgBox->exec();

        if(result == 0)
        {
            msgBox->close();
            return;
        }
        msgBox->close();
    }

    QFile formelFile("Matching-Pursuit-Toolbox/user.fml");
    QFile formelTempFile("Matching-Pursuit-Toolbox/user.temp");

    if(!formelTempFile.exists())
    {
        if (formelTempFile.open(QIODevice::ReadWrite | QIODevice::Text))
        formelTempFile.close();
    }

    QTextStream stream( &formelTempFile );
    if (formelFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        if (formelTempFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            while(!formelFile.atEnd())
            {
                contents = formelFile.readLine(0).constData();
                if(!QString::compare(ui->cb_AtomFormula->currentText() + "\n", contents) == 0)
                    stream << contents;
            }
        }
    }

    formelFile.close();
    formelTempFile.close();

    formelFile.remove();
    formelTempFile.rename("Matching-Pursuit-Toolbox/user.fml");

    ui->cb_AtomFormula->removeItem(ui->cb_AtomFormula->currentIndex());
}
