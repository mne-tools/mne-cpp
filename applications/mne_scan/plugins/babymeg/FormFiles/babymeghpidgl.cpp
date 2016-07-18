//=============================================================================================================
/**
* @file     babymeghpidgl.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Limin Sun and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
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
* @brief    Contains the implementation of the BabyMEGSQUIDControlDgl class.
*
*/

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>


#include "babymeghpidgl.h"
#include "ui_babymeghpidgl.h"


using namespace BabyMEGPlugin;

babymeghpidgl::babymeghpidgl(BabyMEG* p_pBabyMEG,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::babymeghpidgl),
    m_pBabyMEG(p_pBabyMEG)
{
    ui->setupUi(this);

    connect(ui->bn_PolhemusFile, SIGNAL(released()), this, SLOT(bnLoadPolhemusFile()));
    connect(this,&babymeghpidgl::SendHPIFiffInfo,m_pBabyMEG,&BabyMEG::RecvHPIFiffInfo);
    connect(ui->buttonBox,SIGNAL(clicked(QAbstractButton *)),this,SLOT(OKProc(QAbstractButton *)));
//    connect(ui->buttonBox,SIGNAL(rejected()),this,SLOT(CancelProc));
}

babymeghpidgl::~babymeghpidgl()
{
    delete ui;
}

void babymeghpidgl::CancelProc()
{

}

void babymeghpidgl::OKProc(QAbstractButton *b)
{
    qDebug()<<"Clicked group button";

    QString s(b->text());
    qDebug()<<"Clicked group button:"<<s;

    if (s == "OK") {
        FileName_HPI = ui->ed_PolFileName->text();
        /* Load Polhemus file*/
        if (FileName_HPI.isEmpty())
        {
            // we do not find a file
            qDebug()<<"Polhemus File Name is empty. Please input the file name.";
        }
        else
        {
            FileName_HPI = FileName_HPI.trimmed();
            QFileInfo checkFile(FileName_HPI);

            if (checkFile.exists() && checkFile.isFile()) {
                ReadPolhemusDig(FileName_HPI);
                qDebug()<<"Load file Finish!";
                m_pBabyMEG->m_pFiffInfo->dig = info.dig;
                //emit SendHPIFiffInfo(info);

            } else {

                qDebug()<<"Polhemus File is not existed. Please check if it is the full path.";
            }
        }
    }

}


void babymeghpidgl::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
}

void babymeghpidgl::bnLoadPolhemusFile()
{
    qDebug()<<" Start to load Polhemus File";
   // FileName_HPI = QFileDialog::getOpenFileName(this,
   //      tr("Open Polhemus File"), "C:/Users/babyMEG/Desktop", tr("Fiff file (*.fif)"));
    FileName_HPI = QFileDialog::getOpenFileName(this,
            tr("Open Polhemus File"), "", tr("Fiff file (*.fif)"));
    //display the text on the text control
    ui->ed_PolFileName->setText(FileName_HPI);
}

void babymeghpidgl::ReadPolhemusDig(QString fileName)
{
    //start to load Polhemus file
    QFile t_headerFiffFile(fileName);

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&t_headerFiffFile));
    QString t_sFileName = t_pStream->streamName();

    printf("Opening header data %s...\n",t_sFileName.toUtf8().constData());

    FiffDirTree t_Tree;
    QList<FiffDirEntry> t_Dir;

    if(!t_pStream->open(t_Tree, t_Dir))
    {
        qDebug()<<"Can not open the Polhemus File";
        return;
    }
    //
    //   Read the measurement info
    //
    //read_hpi_info(t_pStream,t_Tree, info);
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;
    FiffTag::SPtr t_pTag;

    //
    //   Locate the Polhemus data
    //
    QList<FiffDirTree> isotrak = t_Tree.dir_tree_find(FIFFB_ISOTRAK);

    QList<FiffDigPoint> dig;
    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    qint32 k = 0;

    if (isotrak.size() == 1)
    {
        for (k = 0; k < isotrak[0].nent; ++k)
        {
            kind = isotrak[0].dir[k].kind;
            pos  = isotrak[0].dir[k].pos;
            if (kind == FIFF_DIG_POINT)
            {
                FiffTag::read_tag(t_pStream.data(), t_pTag, pos);
                dig.append(t_pTag->toDigPoint());
            }
            else
            {
                if (kind == FIFF_MNE_COORD_FRAME)
                {
                    FiffTag::read_tag(t_pStream.data(), t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                }
                else if (kind == FIFF_COORD_TRANS)
                {
                    FiffTag::read_tag(t_pStream.data(), t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for(k = 0; k < dig.size(); ++k)
        dig[k].coord_frame = coord_frame;

    //
    //   All kinds of auxliary stuff
    //
    info.dig   = dig;
    //garbage collecting
    t_pStream->device()->close();

}


