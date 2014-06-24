#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"
#include "QtGui"

processdurationmessagebox::processdurationmessagebox(QWidget *parent) :    QDialog(parent),    ui(new Ui::processdurationmessagebox)
{
    ui->setupUi(this);
}

processdurationmessagebox::~processdurationmessagebox()
{
    delete ui;
}

void processdurationmessagebox::on_chb_NoMessageBox_toggled(bool checked)
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
                if(QString::compare("ShowProcessDurationMessageBox=true;\n", contents) == 0)
                    mustEdit = true;
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
                        if(QString::compare("ShowProcessDurationMessageBox=true;\n", contents) == 0)
                            stream << QString("ShowProcessDurationMessageBox=false;");
                        else
                            stream << contents;
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
                if(QString::compare("ShowProcessDurationMessageBox=false;\n", contents) == 0)
                    mustEdit = true;
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
                        if(QString::compare("ShowProcessDurationMessageBox=false;\n", contents) == 0)
                            stream << QString("ShowProcessDurationMessageBox=true;");
                        else
                            stream << contents;
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
void processdurationmessagebox::on_pushButton_clicked()
{
    setResult(1);
    hide();
}


