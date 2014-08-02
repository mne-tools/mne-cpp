//=============================================================================================================
/**
* @file     fixdictmp.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
*
* @version  1.0
* @date     July, 2014

* @section  LICENSE
*
* Copyright (C) 2014, Sebastian Krause,Daniel Knobl and Martin Henfling All rights reserved.
*
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
* @brief    Implemetation of the Matching Pursuit Algorithm using static atom dictionaries to find best matching
*           aproximation of signals.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fixdictmp.h"
#include <vector>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FixDictMp::FixDictMp()
{

}

//*************************************************************************************************************

qint32 FixDictMp::test()
{

   return 34;

}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
QList<GaborAtom> FixDictMp::matching_pursuit(QFile &currentDict, VectorXd signalSamples, qint32 iterationsCount)
{
    GaborAtom* gabor_Atom = new GaborAtom;
    QList<GaborAtom> result_list;
    bool isDouble = false;

    qint32 atomCount = 0;
    qint32 bestCorrStartIndex;
    qreal sample;
    qreal bestCorrValue = 0;
    //qreal residuumEnergie = 0;

    QString contents;
    QString atomName;
    QString bestCorrName;

    QStringList bestAtom;
    QList<qreal> atomSamples;
    QList<QStringList> correlationList;
    VectorXd originalSignalSamples;
    VectorXd residuum;
    VectorXd bestCorrAtomSamples;
    VectorXd normBestCorrAtomSamples;

    residuum(0);
    bestCorrAtomSamples(0);
    normBestCorrAtomSamples(0);
    originalSignalSamples = signalSamples;

    // reading dictionary and give samples and name to scalar function
    if (currentDict.open (QIODevice::ReadOnly))
    {
        while(!currentDict.atEnd())
        {
            contents = currentDict.readLine();
            if(contents.startsWith("atomcount"))
            {
                atomCount = contents.mid(12).toInt();
                break;
            }
        }
        //qint32 i = 0;
        while(!currentDict.atEnd())
        {
            while(!currentDict.atEnd())
            {
                if(contents.contains("_ATOM_"))
                {
                    atomName = contents;
                    break;
                }
                contents = currentDict.readLine();
            }

            contents = "";
            while(!contents.contains("_ATOM_"))
            {
                contents = currentDict.readLine();
                sample = contents.toDouble(&isDouble);
                if(isDouble)
                    atomSamples.append(sample);
                if(currentDict.atEnd())
                    break;
            }
            correlationList.append(correlation(originalSignalSamples, atomSamples, atomName));

            atomSamples.clear();
        }
        currentDict.close();

        // find best matching atom in correlation list
        for(qint32 i = 0; i < correlationList.length(); i++)
        {
            if(fabs(correlationList.at(i).at(2).toDouble()) > fabs(bestCorrValue))
            {
                bestCorrName =  correlationList.at(i).at(0);
                bestCorrStartIndex = correlationList.at(i).at(1).toInt();
                bestCorrValue = correlationList.at(i).at(2).toDouble();
            }
        }

        // find the best matching in dictionary and save content (samples) to list
        if (currentDict.open (QIODevice::ReadOnly))
        {
            bool hasFound = false;
            qint32 j = 0;
            while(!currentDict.atEnd() )
            {
                contents = currentDict.readLine();
                if(QString::compare(contents, bestCorrName) == 0)
                {
                    contents = "";
                    while(!contents.contains("_ATOM_"))
                    {
                        contents = currentDict.readLine();
                        sample = contents.toDouble(&isDouble);
                        if(isDouble)
                        {
                            bestCorrAtomSamples[j] = sample;
                            j++;
                        }
                        if(currentDict.atEnd())
                            break;
                    }
                    hasFound = true;
                }
                if(hasFound) break;
            }
        }
        currentDict.close();

        /*// Quadratische Normierung des Atoms auf den Betrag 1 und Multiplikation mit dem Skalarproduktkoeffizenten
        //**************************** Im Moment weil Testwoerterbuecher nicht nomiert ***********************************

        qreal normFacktorAtom = 0;
        for(qint32 i = 0; i < bestCorrAtomSamples.rows(); i++)
            normFacktorAtom += bestCorrAtomSamples[i]* bestCorrAtomSamples[i];
        normFacktorAtom = sqrt(normFacktorAtom);

        for(qint32 i = 0; i < bestCorrAtomSamples.rows(); i++)
            normBestCorrAtomSamples[i] = (bestCorrAtomSamples[i] / normFacktorAtom) * bestCorrValue;

        //**************************************************************************************************************
        */

        // Subtraktion des Atoms vom Signal
        for(qint32 m = 0; m < normBestCorrAtomSamples.rows(); m++)
        {
            // TODO:
            //signalSamples.append(0);
            //signalSamples.prepend(0);
        }

        residuum = signalSamples;
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
        {
            residuum[normBestCorrAtomSamples.rows() + i + bestCorrStartIndex] = signalSamples[normBestCorrAtomSamples.rows() + i + bestCorrStartIndex] - normBestCorrAtomSamples[i];
            //residuum.removeAt(normBestCorrAtomSamples.rows() + i + bestCorrStartIndex + 1);
        }

        // Loescht die Nullen wieder
        for(qint32 j = 0; j < normBestCorrAtomSamples.rows(); j++)
        {
            // TODO:
            //residuum.removeAt(0);
            //residuum.removeAt(residuum.rows() - 1);
            //signalSamples.removeAt(0);
            //signalSamples.removeAt(signalSamples.rows() - 1);
        }

        iterationsCount++;

        // Traegt das gefunden Atom in eine Liste ein
        bestAtom.append(bestCorrName);
        bestAtom.append(QString("%1").arg(bestCorrStartIndex));
        bestAtom.append(QString("%1").arg(bestCorrValue));
        QString newSignalString = "";
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
            newSignalString.append(QString("%1/n").arg(normBestCorrAtomSamples[i]));

        gabor_Atom->scale              = 0;//scale
        gabor_Atom->translation        = 0;//translation
        gabor_Atom->modulation         = 0;//phase
        gabor_Atom->phase              = 0;
        gabor_Atom->max_scalar_product = 0;

        bestAtom.append(newSignalString);
        result_list.append(*gabor_Atom);

        /*ToDo
        //***************** DEBUGGOUT **********************************************************************************
        QFile newSignal("Matching-Pursuit-Toolbox/newSignal.txt");
        if(!newSignal.exists())
        {
            if (newSignal.open(QIODevice::ReadWrite | QIODevice::Text))
            newSignal.close();
        }
        else    newSignal.remove();

        if(!newSignal.exists())
        {
            if (newSignal.open(QIODevice::ReadWrite | QIODevice::Text))
            newSignal.close();
        }

        if (newSignal.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &newSignal );
            for(qint32 i = 0; i < residuum.rows(); i++)
            {
                QString temp = QString("%1").arg(residuum[i]);
                stream << temp << "\n";
            }
        }
        newSignal.close();

        //**************************************************************************************************************


        // Eintragen und Zeichnen der Ergebnisss in die Liste der UI
        ui->tw_Results->setRowCount(iterationsCount);
        QTableWidgetItem* atomNameItem = new QTableWidgetItem(bestCorrName);

        // Berechnet die Energie des Atoms mit dem NormFaktor des Signals
        qreal normAtomEnergie = 0;
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
            normAtomEnergie += normBestCorrAtomSamples[i] * normBestCorrAtomSamples[i];

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString("%1").arg(normAtomEnergie / signalEnergie * 100));

        //AtomWindow *atomWidget = new AtomWindow();
        //atomWidget->update();
        //ui->tw_Results->setItem(iterationsCount - 1, 1, atomWidget);
        //atomWidget->update();
        //QTableWidgetItem* atomItem = new QTableWidgetItem();
        //atomItem->

        ui->tw_Results->setItem(iterationsCount - 1, 0, atomNameItem);
        ui->tw_Results->setItem(iterationsCount - 1, 2, atomEnergieItem);


        ui->lb_IterationsProgressValue->setText(QString("%1").arg(iterationsCount));
        for(qint32 i = 0; i < residuum.rows(); i++)
            residuumEnergie += residuum[i] * residuum[i];
        if(residuumEnergie == 0)    ui->lb_RestEnergieResiduumValue->setText("0%");
        else    ui->lb_RestEnergieResiduumValue->setText(QString("%1%").arg(residuumEnergie / signalEnergie * 100));

        // Ueberprueft die Abbruchkriterien
        if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        {
            ui->progressBarCalc->setMaximum((1-sollEnergie)*100);
            processValue = (1 - residuumEnergie / signalEnergie + sollEnergie)*100;
            ui->progressBarCalc->setValue(processValue);
            if(ui->sb_Iterations->value() <= iterationsCount)
                ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

            if(ui->sb_Iterations->value() > iterationsCount && sollEnergie < residuumEnergie)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        else if(ui->chb_Iterations->isChecked())
        {
            ui->progressBarCalc->setMaximum(ui->sb_Iterations->value());
            processValue++;
            ui->progressBarCalc->setValue(processValue);

            if(ui->sb_Iterations->value() > iterationsCount)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        else if(ui->chb_ResEnergy->isChecked())
        {
            ui->progressBarCalc->setMaximum((1-sollEnergie)*100);
            processValue = (1 - residuumEnergie / signalEnergie + sollEnergie)*100;
            ui->progressBarCalc->setValue(processValue);

            if(sollEnergie < residuumEnergie)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }*/
    }
    return result_list;
}


// calc scalarproduct of Atom and Signal
QStringList FixDictMp::correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName)
{
    qreal sum = 0;
    qint32 index = 0;
    qreal maximum = 0;
    //qreal sumAtom = 0;

    VectorXd originalSignalList = signalSamples;
    QList<qreal> tempList;
    QList<qreal> scalarList;
    QStringList resultList;

    resultList.clear();
    tempList.clear();

    /*// Quadratische Normierung des Atoms auf den Betrag 1
    //**************************** Im Moment weil Testwoerterbuecher nicht nomiert ***************************************

    for(qint32 i = 0; i < atomSamples.length(); i++)
        sumAtom += atomSamples.at(i)* atomSamples.at(i);
    sumAtom = sqrt(sumAtom);

    for(qint32 i = 0; i < atomSamples.length(); i++)
    {
        qreal tempVarAtom = atomSamples.at(i) / sumAtom;
        atomSamples.removeAt(i);
        atomSamples.insert(i, tempVarAtom);
    }

    // Fuellt das Signal vorne und hinten mit nullen auf damit Randwertproblem umgangen wird
    for(qint32 l = 0; l < atomSamples.length() - 1; l++)
    {
        //signalSamples.append(0);
        //signalSamples.prepend(0);
    }
    */
    //******************************************************************************************************************

    for(qint32 j = 0; j < originalSignalList.rows() + atomSamples.length() -1; j++)
    {
        // Inner Product des of Atom and Signal
        for(qint32 g = 0; g < atomSamples.length(); g++)
        {
            tempList.append(signalSamples[g + j] * atomSamples.at(g));
            sum += tempList.at(g);
        }
        scalarList.append(sum);
        tempList.clear();
        sum = 0;
    }

    //find Maximum and Index of Scalarproduct
    for(qint32 k = 0; k < scalarList.length(); k++)
    {
        if(fabs(maximum) < fabs(scalarList.at(k)))
        {
            maximum = scalarList.at(k);
            index = k;
        }
    }

    // List of Atomname, Index and max correlation coefficient
    resultList.append(atomName);
    resultList.append(QString("%1").arg(index -atomSamples.length() + 1));     // for translation
    resultList.append(QString("%1").arg(maximum)); //for scaling

    return resultList;
    // die Stelle, an der die Korrelation am groessten ist ergibt sich aus:
    // dem Index des hoechsten Korrelationswertes minus die halbe Atomlaenge,

}

//******************************************************************************************************************
//******************************************************************************************************************
//******************************************************************************************************************
//******************************************************************************************************************
//******************************************************************************************************************
//******************************************************************************************************************
//******************************************************************************************************************

void FixDictMp::create_tree_dict(QString save_path)
{
    QFile file(save_path);
    QDomDocument atom_xml_file;
    atom_xml_file.setContent(&file);
    GaborAtom* gabor_Atom = new GaborAtom;

    qint32 sample_count = 0;
    qreal scale = 0;
    quint32 translation = 0;
    qreal modulation = 0;
    qreal phase = 0;

    QDomElement xml_element = atom_xml_file.documentElement();

    QString root_tag = xml_element.tagName();

    if(root_tag == "builtAtomsTreebasedMP")
        sample_count = xml_element.attribute("sample_count").toInt();

    VectorXd compare_atom = VectorXd::Zero(sample_count);

    QString write_save_path = save_path;
    write_save_path.replace(".tbd", ".dict");
    QFile write_file(write_save_path);
    write_file.open(QIODevice::WriteOnly);

    QXmlStreamWriter write_molecules_to_xml(&write_file);
    write_molecules_to_xml.setAutoFormatting(true);

    write_molecules_to_xml.writeStartDocument();
    write_molecules_to_xml.writeStartElement("TreebasedStructureFor_TBMP");
    write_molecules_to_xml.writeAttribute("sample_count", QString::number(sample_count));

    QDomNodeList node_list = xml_element.elementsByTagName("Atom");
    qint32 max_it = node_list.count();
    qint32 number_of_molecs = 0;//only debugout

    for(qint32 i = 0; i < max_it; i++)
    {
        QDomElement current_element = node_list.at(0).toElement();

        if(!current_element.isNull())
        {
            scale = (current_element.attribute("scale", current_element.text())).toDouble();
            translation = (current_element.attribute("translation", current_element.text())).toInt();
            modulation = (current_element.attribute("modulation", current_element.text())).toDouble();
            phase = (current_element.attribute("phase", current_element.text())).toDouble();
        }

        compare_atom = gabor_Atom->create_real(sample_count,scale,translation,modulation,phase);
        qreal threshold = 0.8 * compare_atom.dot(compare_atom); //vary this to find optimum of seperation

        QList<qint32> similar_atoms;        
        VectorXd temp_atom = VectorXd::Zero(sample_count);

        //finding atoms with low differences
        for (qint32 next = 0; next < node_list.count(); next++)//;32)
        {
            //try the next atom
            current_element = node_list.at(next).toElement();

            if(!current_element.isNull())
            {
                scale = (current_element.attribute("scale", current_element.text())).toDouble();
                translation = (current_element.attribute("translation", current_element.text())).toInt();
                modulation = (current_element.attribute("modulation", current_element.text())).toDouble();
                phase = (current_element.attribute("phase", current_element.text())).toDouble();
            }


            //set atoms out of xml to compare with compare_atom
            temp_atom = gabor_Atom->create_real(sample_count,scale,translation,modulation,phase);

            //fill list of similar atoms to save as molecule
            if( compare_atom.dot(temp_atom) > threshold)
                similar_atoms.append(current_element.attribute("ID", current_element.text()).toInt());

            if (similar_atoms.length() == 32)//vary this for testing
                break;
        }

        qreal molec_scale = 0;
        quint32 molec_translation = 0;
        qreal molec_modulation = 0;
        qreal molec_phase = 0;

        //calc molecule params

        for(qint32 j = 0; j < similar_atoms.length(); j++)
        {            
            current_element = node_list.at(0).toElement();

            while(!current_element.isNull())
            {
                if(current_element.attribute("ID", current_element.text()).toInt() == similar_atoms.at(j))
                {
                        molec_scale += (current_element.attribute("scale", current_element.text())).toDouble();
                        molec_translation += (current_element.attribute("translation", current_element.text())).toInt();
                        molec_modulation += (current_element.attribute("modulation", current_element.text())).toDouble();
                        molec_phase += (current_element.attribute("phase", current_element.text())).toDouble();
                }
                current_element = current_element.nextSibling().toElement();
            }
        }

        molec_scale /= similar_atoms.length();
        molec_translation /= similar_atoms.length();
        molec_modulation /= similar_atoms.length();
        molec_phase /= similar_atoms.length();

        write_molecules_to_xml.writeStartElement("Molecule");
        write_molecules_to_xml.writeAttribute("ID", QString::number(i));
        write_molecules_to_xml.writeAttribute("level", "0");
        write_molecules_to_xml.writeAttribute("scale", QString::number(molec_scale));
        write_molecules_to_xml.writeAttribute("translation", QString::number(molec_translation));
        write_molecules_to_xml.writeAttribute("modulation", QString::number(molec_modulation));
        write_molecules_to_xml.writeAttribute("phase", QString::number(molec_phase));

        // write atoms to molecule
        for(qint32 k = 0; k < similar_atoms.length(); k++)
        {
            current_element = node_list.at(0).toElement();

            while(!current_element.isNull())
            {
                if(current_element.attribute("ID", current_element.text()).toInt() == similar_atoms.at(k))
                {
                    write_molecules_to_xml.writeStartElement("Atom");
                    write_molecules_to_xml.writeAttribute("ID", current_element.attribute("ID", current_element.text()));
                    write_molecules_to_xml.writeAttribute("scale", current_element.attribute("scale", current_element.text()));
                    write_molecules_to_xml.writeAttribute("translation", current_element.attribute("translation", current_element.text()));
                    write_molecules_to_xml.writeAttribute("modulation", current_element.attribute("modulation", current_element.text()));
                    write_molecules_to_xml.writeAttribute("phase", current_element.attribute("phase", current_element.text()));

                    //recursive_node_built(current_element, write_file);

                    write_molecules_to_xml.writeEndElement();//atom
                }
                current_element = current_element.nextSibling().toElement();
            }
        }
        write_molecules_to_xml.writeEndElement();//molecule

        //remove found atoms from node list
        for(qint32 n = 0; n < similar_atoms.length(); n++)
        {
            current_element = node_list.at(0).toElement();
            QDomNode to_remove = current_element.parentNode();

            while(!current_element.isNull())
            {
                if(current_element.attribute("ID", current_element.text()).toInt() == similar_atoms.at(n))
                    to_remove.removeChild(current_element);

                current_element = current_element.nextSibling().toElement();
            }
        }

        if(node_list.count() == 0)
            break;

        number_of_molecs++;
    }//for max_it of nodelist

    write_molecules_to_xml.writeEndElement();//header
    write_molecules_to_xml.writeEndDocument();

    cout << "...start to build tree recursive... number of built molecules:  " << number_of_molecs + 1 << "\n";

    //close reader and writer
    file.close();
    write_file.close();
    delete gabor_Atom;

    //ToDo: recursion
    build_molecule_xml_file(0);

    cout << "finished treebuilding\n";


}

//******************************************************************************************************************

void FixDictMp::recursive_node_built(QDomElement current_element, QFile &temp_file)
{
    QXmlStreamWriter write_molecules_to_xml(&temp_file);
    write_molecules_to_xml.setAutoFormatting(true);

    qint32 end_element_counter = 0;
    while (current_element.hasChildNodes())
    {
        QDomNodeList child_list = current_element.childNodes();

        for(qint32 z = 0; z < child_list.length(); z++)
        {
            current_element = child_list.at(z).toElement();

            write_molecules_to_xml.writeStartElement(current_element.nodeName());

            if(current_element.nodeName() == "Molecule")
                write_molecules_to_xml.writeAttribute("level", current_element.attribute("level", current_element.text()));

            write_molecules_to_xml.writeAttribute("ID", current_element.attribute("ID", current_element.text()));
            write_molecules_to_xml.writeAttribute("scale", current_element.attribute("scale", current_element.text()));
            write_molecules_to_xml.writeAttribute("translation", current_element.attribute("translation", current_element.text()));
            write_molecules_to_xml.writeAttribute("modulation", current_element.attribute("modulation", current_element.text()));
            write_molecules_to_xml.writeAttribute("phase", current_element.attribute("phase", current_element.text()));

            if(current_element.nodeName() == "Atom")
                write_molecules_to_xml.writeEndElement();

            //recursive_node_built(current_element, temp_file);

        }
        end_element_counter++;

    }

    for(qint32 close = end_element_counter; close > 1; --close)
        write_molecules_to_xml.writeEndElement();

}

void FixDictMp::build_molecule_xml_file(qint32 level_counter)
{
    //qint32 i = 0;
    if(level_counter < 10)
    {
        QString save_path = QString("Matching-Pursuit-Toolbox/my_first_16atoms.dict");//ToDo: make it flexible
        QFile file(save_path);
        QString temp_path("Matching-Pursuit-Toolbox/__temp.dict");
        file.copy(temp_path);
        QFile temp_file(temp_path);

        QDomDocument atom_xml_file;
        atom_xml_file.setContent(&file);
        QDomElement xml_element = atom_xml_file.documentElement();
        xml_element = atom_xml_file.documentElement();

        GaborAtom* gabor_Atom = new GaborAtom;
        qint32 sample_count = 0;
        qreal scale = 0;
        quint32 translation = 0;
        qreal modulation = 0;
        qreal phase = 0;
        //qint32 level_counter = 0;

        QString root_tag = xml_element.tagName();
        root_tag = xml_element.tagName();

        if(root_tag == "TreebasedStructureFor_TBMP")
            sample_count = xml_element.attribute("sample_count").toInt();

        VectorXd compare_molec = VectorXd::Zero(sample_count);

        QDomNodeList node_list = xml_element.elementsByTagName("Molecule");
        qint32 max_it = node_list.count();
        QDomElement current_element;

        temp_file.open(QIODevice::WriteOnly);

        QXmlStreamWriter write_molecules_to_xml(&temp_file);
        write_molecules_to_xml.setAutoFormatting(true);

        write_molecules_to_xml.writeStartDocument();
        write_molecules_to_xml.writeStartElement("TreebasedStructureFor_TBMP");
        write_molecules_to_xml.writeAttribute("sample_count", QString::number(sample_count));

        for(qint32 i = 0; i < max_it; i++)
        {
            current_element = node_list.at(0).toElement();
            QList<qint32> similar_molecs;

            if(current_element.attribute("level").toInt() == level_counter)
            {
                if(!current_element.isNull())
                {
                    scale = (current_element.attribute("scale", current_element.text())).toDouble();
                    translation = (current_element.attribute("translation", current_element.text())).toInt();
                    modulation = (current_element.attribute("modulation", current_element.text())).toDouble();
                    phase = (current_element.attribute("phase", current_element.text())).toDouble();
                }
                compare_molec = gabor_Atom->create_real(sample_count,scale,translation,modulation,phase);
                qreal threshold = 0.8 * compare_molec.dot(compare_molec); //vary this to find optimum of seperation



                VectorXd temp_molec = VectorXd::Zero(sample_count);

                //finding molecules with low differences
                for (qint32 next = 0; next < node_list.count(); next++)//;32)
                {
                    //try the next atom
                    //if(current_element.attribute("level").toInt() == level_counter)
                        current_element = node_list.at(next).toElement();

                    if(!current_element.isNull())
                    {
                        scale = (current_element.attribute("scale", current_element.text())).toDouble();
                        translation = (current_element.attribute("translation", current_element.text())).toInt();
                        modulation = (current_element.attribute("modulation", current_element.text())).toDouble();
                        phase = (current_element.attribute("phase", current_element.text())).toDouble();
                    }
                    //set molecules out of xml to compare with molecule
                    temp_molec = gabor_Atom->create_real(sample_count,scale,translation,modulation,phase);

                    //fill list of similar molecules to save as new molecule
                    if( compare_molec.dot(temp_molec) > threshold && current_element.attribute("level").toInt() == level_counter)
                        similar_molecs.append(current_element.attribute("ID", current_element.text()).toInt());

                    if (similar_molecs.length() == 32)//vary this for testing
                        break;
                }

                qreal molec_scale = 0;
                quint32 molec_translation = 0;
                qreal molec_modulation = 0;
                qreal molec_phase = 0;

                //calc molecule params
                for(qint32 j = 0; j < similar_molecs.length(); j++)
                {
                    current_element = node_list.at(0).toElement();

                    while(!current_element.isNull())
                    {
                        if(current_element.attribute("ID", current_element.text()).toInt() == similar_molecs.at(j) && current_element.attribute("level").toInt() == level_counter)
                        {
                            molec_scale += (current_element.attribute("scale", current_element.text())).toDouble();
                            molec_translation += (current_element.attribute("translation", current_element.text())).toInt();
                            molec_modulation += (current_element.attribute("modulation", current_element.text())).toDouble();
                            molec_phase += (current_element.attribute("phase", current_element.text())).toDouble();
                        }
                        current_element = current_element.nextSiblingElement("Molecule").toElement();
                    }
                }

                molec_scale /= similar_molecs.length();
                molec_translation /= similar_molecs.length();
                molec_modulation /= similar_molecs.length();
                molec_phase /= similar_molecs.length();

                write_molecules_to_xml.writeStartElement("Molecule");
                write_molecules_to_xml.writeAttribute("level", QString::number(level_counter + 1));
                write_molecules_to_xml.writeAttribute("ID", QString::number(i));
                write_molecules_to_xml.writeAttribute("scale", QString::number(molec_scale));
                write_molecules_to_xml.writeAttribute("translation", QString::number(molec_translation));
                write_molecules_to_xml.writeAttribute("modulation", QString::number(molec_modulation));
                write_molecules_to_xml.writeAttribute("phase", QString::number(molec_phase));

                //treedepth building

                for(qint32 k = 0; k < similar_molecs.length(); k++)
                {
                    current_element = node_list.at(0).toElement();

                    while(!current_element.isNull())
                    {
                        if(current_element.attribute("ID", current_element.text()).toInt() == similar_molecs.at(k) && current_element.attribute("level").toInt() == level_counter)
                        {
                            write_molecules_to_xml.writeStartElement("Molecule");
                            write_molecules_to_xml.writeAttribute("level", current_element.attribute("level", current_element.text()));
                            write_molecules_to_xml.writeAttribute("ID", current_element.attribute("ID", current_element.text()));
                            write_molecules_to_xml.writeAttribute("scale", current_element.attribute("scale", current_element.text()));
                            write_molecules_to_xml.writeAttribute("translation", current_element.attribute("translation", current_element.text()));
                            write_molecules_to_xml.writeAttribute("modulation", current_element.attribute("modulation", current_element.text()));
                            write_molecules_to_xml.writeAttribute("phase", current_element.attribute("phase", current_element.text()));


                            //recursive_node_built(current_element, temp_file);
                            qint32 end_element_counter = 0;
                            if (current_element.hasChildNodes())
                            {
                                //recursive_node_built(current_element, temp_file);
                                QDomNodeList child_list = current_element.childNodes();

                                for(qint32 z = 0; z < child_list.length(); z++)
                                {
                                    current_element = child_list.at(z).toElement();

                                    write_molecules_to_xml.writeStartElement(current_element.nodeName());

                                    if(current_element.nodeName() == "Molecule")
                                        write_molecules_to_xml.writeAttribute("level", current_element.attribute("level", current_element.text()));

                                    write_molecules_to_xml.writeAttribute("ID", current_element.attribute("ID", current_element.text()));
                                    write_molecules_to_xml.writeAttribute("scale", current_element.attribute("scale", current_element.text()));
                                    write_molecules_to_xml.writeAttribute("translation", current_element.attribute("translation", current_element.text()));
                                    write_molecules_to_xml.writeAttribute("modulation", current_element.attribute("modulation", current_element.text()));
                                    write_molecules_to_xml.writeAttribute("phase", current_element.attribute("phase", current_element.text()));

                                    if(current_element.nodeName() == "Atom")
                                        write_molecules_to_xml.writeEndElement();

                                    if(current_element.nodeName() == "Molecule")
                                    {
                                        current_element = current_element.firstChildElement();

                                        while(!current_element.isNull())
                                        {
                                            write_molecules_to_xml.writeStartElement(current_element.nodeName());

                                            if(current_element.nodeName() == "Molecule")
                                                write_molecules_to_xml.writeAttribute("level", current_element.attribute("level", current_element.text()));

                                            write_molecules_to_xml.writeAttribute("ID", current_element.attribute("ID", current_element.text()));
                                            write_molecules_to_xml.writeAttribute("scale", current_element.attribute("scale", current_element.text()));
                                            write_molecules_to_xml.writeAttribute("translation", current_element.attribute("translation", current_element.text()));
                                            write_molecules_to_xml.writeAttribute("modulation", current_element.attribute("modulation", current_element.text()));
                                            write_molecules_to_xml.writeAttribute("phase", current_element.attribute("phase", current_element.text()));

                                            if(current_element.nodeName() == "Atom")
                                                write_molecules_to_xml.writeEndElement();

                                            current_element = current_element.nextSibling().toElement();
                                        }
                                        write_molecules_to_xml.writeEndElement();
                                    }
                                    current_element = child_list.at(z).toElement();
                                }
                                end_element_counter++;

                            }

                            for(qint32 close = end_element_counter; close > 1; close--)
                                write_molecules_to_xml.writeEndElement();

                            write_molecules_to_xml.writeEndElement();//molecules
                        }
                        current_element = current_element.nextSiblingElement("Molecule").toElement();
                    }
                }
                write_molecules_to_xml.writeEndElement(); // next level molecule

                //remove found molecules from node list
                for(qint32 n = 0; n < similar_molecs.length(); n++)
                {
                    current_element = node_list.at(0).toElement();
                    QDomNode to_remove = current_element.parentNode();

                    while(!current_element.isNull())
                    {
                        if(current_element.attribute("ID", current_element.text()).toInt() == similar_molecs.at(n) && current_element.attribute("level", current_element.text()).toInt() == level_counter)
                            to_remove.removeChild(current_element);

                        current_element = current_element.nextSiblingElement("Molecule").toElement();
                    }
                }
            }//if

            cout << node_list.count() <<"\n";

            if(node_list.count() == 0)
                break;
        }//for nodelist

        write_molecules_to_xml.writeEndElement();//header
        write_molecules_to_xml.writeEndDocument();

        file.close();
        temp_file.close();

        file.remove();
        temp_file.copy(save_path);
        temp_file.remove();

        level_counter++;

        build_molecule_xml_file(level_counter);
    }

}
