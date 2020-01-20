//=============================================================================================================
/**
 * @file     treebaseddictwindow.cpp
 * @author   Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Daniel Knobl, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the EditorWindow class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/mp/atom.h>
#include <utils/mp/fixdictmp.h>
#include "treebaseddictwindow.h"
#include "ui_treebaseddictwindow.h"

#include "mainwindow.h"
#include "editorwindow.h"
#include "stdio.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QApplication>
#include <QModelIndex>
#include <QMessageBox>
#include <QtXml/QtXml>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

TreebasedDictWindow::TreebasedDictWindow(QWidget *parent)
: QWidget(parent)
, ui(new Ui::TreebasedDictWindow)
, _treebased_dict_name("")
{
    ui->setupUi(this);
}

TreebasedDictWindow::~TreebasedDictWindow()
{
    delete ui;
}

void TreebasedDictWindow::on_btt_calc_treebased_clicked()
{
    /*
    qint32 sample_count = ui->spb_AtomLength->value();

    qint32 count = 0;
    qreal phase = 0;
    qreal modulation = 0;

    _treebased_dict_name = ui->tb_treebased_dict_name->text();
    QString save_path = QString("Matching-Pursuit-Toolbox/%1.tbd").arg(_treebased_dict_name);

    QFile file(save_path);
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("builtAtomsTreebasedMP");
    xmlWriter.writeAttribute("sample_count", QString::number(sample_count));

    //ToDo: find good stepwidths
    for(qint32 scale = 1; scale <= sample_count; scale +=3)
    {
        for(qint32 translation = 0; translation <sample_count; translation +=2)
        {
            modulation = 0;
            while(modulation < floor(sample_count/2))
            {
                phase = 0;
                //while(phase < 2 * PI)
                {                    
                    xmlWriter.writeStartElement("Atom");
                    xmlWriter.writeAttribute("ID", QString::number(count));
                    xmlWriter.writeAttribute("scale", QString::number(scale));
                    xmlWriter.writeAttribute("translation", QString::number(translation));
                    xmlWriter.writeAttribute("modulation", QString::number(modulation));
                    //xmlWriter.writeAttribute("phase", QString::number(phase));
                    xmlWriter.writeEndElement();

                    count++;
                //    phase += 2*PI / sample_count;//(4*PI)/360;
                }
                modulation +=2;//(sample_count/2) / 32;//floor(5/100*sample_count);
            }
        }
    }

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    file.close();
    std::cout << "number of atoms built: " << count << "\n";
    //FixDictMp::create_tree_dict(save_path);
     */

}

void TreebasedDictWindow::on_tb_treebased_dict_name_editingFinished()
{
    //_treebased_dict_name = ui->tb_treebased_dict_name->text();
}

void TreebasedDictWindow::on_btt_call_tree_creator_clicked()
{
    //QString save_path = QString("Matching-Pursuit-Toolbox/%1.tbd").arg(_treebased_dict_name);

    //FixDictMp::create_tree_dict(save_path);
}
