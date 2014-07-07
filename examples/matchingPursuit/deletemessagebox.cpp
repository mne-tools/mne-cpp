#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"
#include "QtGui"

QString parentName;

DeleteMessageBox::DeleteMessageBox(QWidget *parent) :    QDialog(parent),    ui(new Ui::DeleteMessageBox)
{
    parentName =  parent->accessibleName();
    ui->setupUi(this);
}

DeleteMessageBox::~DeleteMessageBox()
{
    delete ui;
}

void DeleteMessageBox::on_btt_yes_clicked()
{
    setResult(1);
    hide();
}

void DeleteMessageBox::on_btt_No_clicked()
{
    setResult(0);
    hide();
}

void DeleteMessageBox::on_chb_NoMessageBox_toggled(bool checked)
{
    bool mustEdit = false;
    QString contents;
    QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");


    if(checked)
    {
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            while(!configFile.atEnd())
            {
                contents = configFile.readLine(0).constData();
                if(QString::compare(parentName, "simple") == 0)
                {
                    if(contents.startsWith("ShowDeleteMessageBox=true;"))
                        mustEdit = true;
                }
                else if(QString::compare(parentName, "formel") == 0)
                {
                    if(contents.startsWith("ShowDeleteFormelMessageBox=true;"))
                        mustEdit = true;
                }
            }
        }
        configFile.close();


        if(mustEdit)
        {
            QFile configTempFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox-Temp.config");

            if(!configTempFile.exists())
            {
                if (configTempFile.open(QIODevice::ReadWrite | QIODevice::Text))
                configTempFile.close();
            }


            QTextStream stream( &configTempFile );
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                if (configTempFile.open (QIODevice::WriteOnly| QIODevice::Append))
                {
                    while(!configFile.atEnd())
                    {
                        contents = configFile.readLine(0).constData();
                        if(QString::compare(parentName, "simple") == 0)
                        {
                            if(contents.startsWith("ShowDeleteMessageBox=true;"))
                                stream << QString("ShowDeleteMessageBox=false;");
                            else
                                stream << contents;
                        }
                        else if(QString::compare(parentName, "formel") == 0)
                        {
                            if(contents.startsWith("ShowDeleteFormelMessageBox=true;"))
                                stream << QString("ShowDeleteFormelMessageBox=false;");
                            else
                                stream << contents;
                        }
                    }
                }
            }

            configFile.close();
            configTempFile.close();

            configFile.remove();
            configTempFile.rename("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
        }
    }
    else
    {
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            while(!configFile.atEnd())
            {
                contents = configFile.readLine(0).constData();
                if(QString::compare(parentName, "simple") == 0)
                {
                    if(contents.startsWith("ShowDeleteMessageBox=false;"))
                        mustEdit = true;
                }
                else if(QString::compare(parentName, "formel") == 0)
                {
                    if(contents.startsWith("ShowDeleteFormelMessageBox=false;"))
                        mustEdit = true;
                }
            }
        }
        configFile.close();


        if(mustEdit)
        {
            QFile configTempFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox-Temp.config");

            if(!configTempFile.exists())
            {
                if (configTempFile.open(QIODevice::ReadWrite | QIODevice::Text))
                configTempFile.close();
            }


            QTextStream stream( &configTempFile );
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                if (configTempFile.open (QIODevice::WriteOnly| QIODevice::Append))
                {
                    while(!configFile.atEnd())
                    {
                        contents = configFile.readLine(0).constData();
                        if(QString::compare(parentName, "simple") == 0)
                        {
                            if(contents.startsWith("ShowDeleteMessageBox=false;"))
                                stream << QString("ShowDeleteMessageBox=true;");
                            else
                                stream << contents;
                        }
                        else if(QString::compare(parentName, "formel") == 0)
                        {
                            if(contents.startsWith("ShowDeleteFormelMessageBox=false;"))
                                stream << QString("ShowDeleteFormelMessageBox=true;");
                            else
                                stream << contents;
                        }
                    }
                }
            }

            configFile.close();
            configTempFile.close();

            configFile.remove();
            configTempFile.rename("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
        }
    }
}

