//=============================================================================================================
/**
* @file     editorwindow.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the EditorWindow class.
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

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

TreebasedDictWindow::TreebasedDictWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TreebasedDictWindow)
{
    ui->setupUi(this);
}

TreebasedDictWindow::~TreebasedDictWindow()
{
    delete ui;
}

void TreebasedDictWindow::on_btt_calc_treebased_clicked()
{
    GaborAtom *temp_atom = new GaborAtom;
    QList<GaborAtom> atoms_to_dict;
    qint32 sample_count = ui->spb_AtomLength->value();
    qint32 scale = 1;
    qint32 count = 0;

    while(scale < sample_count)
    {
        qreal modulation = 0;
        for(quint32 translation = 0; translation < sample_count; translation++)
        {
            qreal phase = 0;
            while(modulation < floor(sample_count/2))
            {
                while(phase < 2 * PI)
                {
                    temp_atom->sample_count = sample_count;
                    temp_atom->scale = scale;
                    temp_atom->modulation = modulation;
                    temp_atom->phase_list.append(phase);
                    count++;
                    std::cout << count << "\n";

                    //atoms_to_dict.append(*temp_atom);

                    phase += (2*PI)/ sample_count;
                }
                modulation += 0.5;
            }
        }
        scale++;
    }
    //FixDictMp::create_tree_dict(atoms_to_dict);
    delete temp_atom;
}
