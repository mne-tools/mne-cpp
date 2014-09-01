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
*           approximation of signals.
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
: it(0)
, signal_energy(0)
, current_energy(0)
{

}

//*************************************************************************************************************

void FixDictMp::matching_pursuit(MatrixXd signal, qint32 max_iterations, qreal epsilon, QString path)
{
    std::cout << "\nFixDict Matching Pursuit Algorithm started...\n";

    bool isDouble = false;

    qint32 atomCount = 0;
    qint32 bestCorrStartIndex;
    qreal sample;
    qreal bestCorrValue = 0;

    QString contents;
    QString atomName;
    QString bestCorrName;

    QList<qreal> atom_samples;
    QList<QStringList> correlationList;    

    MatrixXd residuum = signal; //residuum initialised with signal
    qint32 sample_count = signal.rows();
    qint32 channel_count = signal.cols();
    qint32 atom_sample_count = 0;

    signal_energy = 0;
    qreal residuum_energy = 0;
    qreal energy_threshold = 0;
    qreal temp_energy = 0;
    bool sample_count_mismatch = false;


    //calculate signal_energy
    for(qint32 channel = 0; channel < channel_count; channel++)
    {
        for(qint32 sample = 0; sample < sample_count; sample++)
            signal_energy += (signal(sample, channel) * signal(sample, channel));

        energy_threshold = 0.01 * epsilon * signal_energy;
        residuum_energy = signal_energy;
    }
    std::cout << "absolute energy of signal: " << residuum_energy << "\n";

    VectorXd signalSamples = VectorXd::Zero(sample_count);    

    QFile current_dict(path);

    while(it < max_iterations && (energy_threshold < residuum_energy))
    {
        FixDictAtom *fix_dict_atom = new FixDictAtom();

        for(qint32 i = 0; i < sample_count; i++)
            signalSamples[i] = residuum(i,0);

        // read dictionary and give samples and name of atom to correlation function
        if (current_dict.open (QIODevice::ReadOnly))
        {
            while(!current_dict.atEnd())
            {
                contents = current_dict.readLine();
                if(contents.startsWith("atomcount"))
                {
                    atomCount = contents.mid(12).toInt();
                    break;
                }
            }
            while(!current_dict.atEnd())
            {
                while(!current_dict.atEnd())
                {
                    if(contents.contains("_ATOM_"))
                    {
                        atomName = contents;
                        break;
                    }
                    contents = current_dict.readLine();
                }

                contents = "";
                fix_dict_atom->atom_formula = current_dict.readLine();

                while(!contents.contains("_ATOM_"))
                {
                    contents = current_dict.readLine();
                    sample = contents.toDouble(&isDouble);
                    if(isDouble)
                        atom_samples.append(sample);
                    if(current_dict.atEnd())
                        break;
                }
                correlationList.append(correlation(signalSamples, atom_samples, atomName));

                if(atom_samples.length() != sample_count && !sample_count_mismatch)
                {
                    std::cout <<  "\n=============================================\n"
                              << "this dictionary does not fit the signal,\nplease choose or create another dictionary containing\nonly atoms of the same length as the signal vector\n"
                              << "signal samples: " << sample_count << "\n"
                              << "atom samples: " << atom_samples.length() << "\n"
                              <<  "=============================================\n";
                    sample_count_mismatch = true;
                }

                atom_sample_count = atom_samples.length();
                atom_samples.clear();
            }
            current_dict.close();

            // find best matching atom
            for(qint32 i = 0; i < correlationList.length(); i++)
            {
                if(abs(correlationList.at(i).at(2).toDouble()) > abs(bestCorrValue))
                {
                    bestCorrName =  correlationList.at(i).at(0);
                    bestCorrStartIndex = correlationList.at(i).at(1).toInt();
                    bestCorrValue = correlationList.at(i).at(2).toDouble();
                }
            }
            correlationList.clear();

            // find best matching atom in dictionary and write values into list
            if (current_dict.open (QIODevice::ReadOnly))
            {
                bool hasFound = false;
                //qint32 j = 0;
                while(!current_dict.atEnd() )
                {
                    contents = current_dict.readLine();
                    if(QString::compare(contents, bestCorrName) == 0)
                    {
                        fix_dict_atom->atom_formula = current_dict.readLine();
                        contents = current_dict.readLine();


                        while (!contents.contains("_ATOM_") && atom_sample_count > 0)
                        {
                            qint32 read_offset = 0;

                            if(bestCorrStartIndex - floor(sample_count/2) >= 0)//atom translated in direction to signalvector end
                            {
                                for(qint32 translation = 0; translation < sample_count; translation++)
                                {
                                    if(translation <= bestCorrStartIndex - floor(sample_count/2))
                                    {
                                        read_offset++;
                                        atom_samples.append(0);
                                    }
                                    else if(translation > bestCorrStartIndex - floor(sample_count/2))
                                    {
                                        contents = current_dict.readLine();
                                        sample = contents.toDouble(&isDouble);
                                        if(isDouble)
                                            atom_samples.append(sample);
                                    }

                                    if(current_dict.atEnd())
                                        break;
                                }
                                for(qint32 i = 0; i < read_offset; i++)
                                    contents = current_dict.readLine();

                                if(current_dict.atEnd())
                                    break;
                            }                            
                            else // atom translated more in direction to signalvector start
                            {
                                for(qint32 translation = bestCorrStartIndex - floor(sample_count/2); translation < sample_count; translation++)
                                {
                                    if(translation < 0)
                                        contents = current_dict.readLine();

                                    else if(translation >= 0 && translation < sample_count - bestCorrStartIndex - floor(sample_count/2))
                                    {
                                        contents = current_dict.readLine();
                                        sample = contents.toDouble(&isDouble);
                                        if(isDouble)
                                            atom_samples.append(sample);
                                    }
                                    else
                                        atom_samples.append(0);

                                    if(current_dict.atEnd())
                                        break;
                                }

                                if(current_dict.atEnd())
                                    break;
                            }
                            if(current_dict.atEnd())
                                break;

                            atom_sample_count--;
                        }

                        for(qint32 k = 0; k < sample_count; k++)
                        {
                                residuum(k, 0) -= bestCorrValue * atom_samples.at(k);
                                temp_energy += bestCorrValue * atom_samples.at(k) * bestCorrValue * atom_samples.at(k);
                        }

                        std::cout << "\n" << "===============" << " found atom " << it << "===============" << ":\n\n";

                        hasFound = true;
                    }
                    if(hasFound) break;
                }
            }            
            current_dict.close();
        }   

        residuum_energy -= temp_energy;
        current_energy += temp_energy;
        VectorXd atom_sample_vector = VectorXd::Zero(atom_samples.length());
        for(qint32 i = 0; i < atom_samples.length(); i++)
            atom_sample_vector[i] = atom_samples.at(i);


        fix_dict_atom->vector_list.append(atom_sample_vector);
        fix_dict_atom->energy = temp_energy;
        fix_dict_atom->max_scalar_list.append(bestCorrValue);
        fix_dict_list.append(*fix_dict_atom);

        std::cout << "absolute energy of residuum: " << residuum_energy << "\n";

        atom_samples.clear();
        signalSamples = VectorXd::Zero(sample_count);
        bestCorrValue = 0;
        temp_energy = 0;

        delete fix_dict_atom;
        it++;

        emit current_result(it, max_iterations, current_energy, signal_energy, residuum, adaptive_list, fix_dict_list);

        if( QThread::currentThread()->isInterruptionRequested())
            break;

    }//end while iterations

    std::cout << "\nFixDict Matching Pursuit Algorithm finished.\n";
    emit finished_calc();
    //return atom_list;
}

//*************************************************************************************************************

// calc scalarproduct of Atom and Signal
QStringList FixDictMp::correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName)
{
    qreal maximum = 0;
    qint32 max_index = 0;
    qint32 p = floor(signalSamples.rows() / 2);//translation
    Eigen::FFT<double> fft;

    QStringList resultList;

    resultList.clear();

    VectorXd atom = VectorXd::Zero(signalSamples.rows());

    for(qint32 i = 0; i < atomSamples.length(); i++)
        atom[i] = atomSamples.at(i);

    VectorXcd fft_atom = VectorXcd::Zero(signalSamples.rows());

    fft.fwd(fft_atom, atom);

    VectorXcd fft_signal = VectorXcd::Zero(signalSamples.rows());
    VectorXcd fft_sig_atom = VectorXcd::Zero(signalSamples.rows());
    VectorXd corr_coeffs = VectorXd::Zero(signalSamples.rows());

    fft.fwd(fft_signal, signalSamples);

    for( qint32 m = 0; m < signalSamples.rows(); m++)
        fft_sig_atom[m] = fft_signal[m] * conj(fft_atom[m]);

    fft.inv(corr_coeffs, fft_sig_atom);
    maximum = corr_coeffs[0];

    //find index of maximum correlation-coefficient to use in translation
    for(qint32 i = 1; i < corr_coeffs.rows(); i++)
        if(maximum < corr_coeffs[i])
        {
            maximum = corr_coeffs[i];
            max_index = i;
        }

    //adapting translation p to create atomtranslation correctly
    if(max_index >= p) p = max_index - p + 1;
    else p = max_index + p;

    // List of Atomname, Index and max correlation coefficient
    resultList.append(atomName);
    resultList.append(QString("%1").arg(p));     // for translation
    resultList.append(QString("%1").arg(maximum)); //for scaling

    return resultList;
}

//*************************************************************************************************************

void FixDictMp::recieve_input(MatrixXd signal, qint32 max_iterations, qreal epsilon, QString path)
{
    matching_pursuit(signal, max_iterations, epsilon, path);
}

//*************************************************************************************************************

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

            //if (similar_atoms.length() == 32)//vary this for testing
            //    break;
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
        write_molecules_to_xml.writeAttribute("level", "0");
        write_molecules_to_xml.writeAttribute("ID", QString::number(i));
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

            if(current_element.attribute("level").toInt() == level_counter && current_element.nodeName() == "Molecule")
            {
                if(!current_element.isNull())
                {
                    scale = (current_element.attribute("scale", current_element.text())).toDouble();
                    translation = (current_element.attribute("translation", current_element.text())).toInt();
                    modulation = (current_element.attribute("modulation", current_element.text())).toDouble();
                    phase = (current_element.attribute("phase", current_element.text())).toDouble();
                }
                compare_molec = gabor_Atom->create_real(sample_count,scale,translation,modulation,phase);
                qreal threshold = 0.6 * compare_molec.dot(compare_molec); //vary this to find optimum of seperation

                VectorXd temp_molec = VectorXd::Zero(sample_count);

                //finding molecules with low differences
                for (qint32 next = 0; next < node_list.count(); next++)//;32)
                {
                    current_element = node_list.at(next).toElement();
                    //try the next molecule
                    if(current_element.attribute("level").toInt() == level_counter && current_element.nodeName() == "Molecule")
                    {    //current_element = node_list.at(next).toElement();

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
                        if( compare_molec.dot(temp_molec) > threshold)
                            similar_molecs.append(current_element.attribute("ID", current_element.text()).toInt());

                        //if (similar_molecs.length() == 32)//vary this for testing
                        //    break;
                    }
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

                current_element = node_list.at(0).toElement();
                qint32 root_counter = 0;

                for(qint32 k = 0; k < similar_molecs.length(); k++)
                {                    
                    //current_element = node_list.at(current_element.attribute("ID", current_element.text()).toInt() == similar_molecs.at(k) && current_element.attribute("level").toInt() == level_counter).toElement();
                    current_element = node_list.at(0).toElement();

                    while(!current_element.isNull())
                    {
                        root_counter = 0;

                        if(current_element.attribute("ID").toInt() == similar_molecs.at(k) && current_element.attribute("level").toInt() == level_counter)
                        {                           
                            QDomElement temp_element = current_element;
                            qint32 end_element_counter = 0;
                            qint32 child_counter = 0;//ToDo: set right child for right position in tree to write all atoms and molecules
                            root_counter = 0;

                            while(temp_element.hasChildNodes())
                            {
                                end_element_counter = 0;
                                child_counter = 0;

                                while(!temp_element.isNull())
                                {                                    
                                    write_molecules_to_xml.writeStartElement(temp_element.nodeName());

                                    if(temp_element.nodeName() == "Molecule")
                                        write_molecules_to_xml.writeAttribute("level", temp_element.attribute("level", temp_element.text()));

                                    write_molecules_to_xml.writeAttribute("ID", temp_element.attribute("ID", temp_element.text()));
                                    write_molecules_to_xml.writeAttribute("scale", temp_element.attribute("scale", temp_element.text()));
                                    write_molecules_to_xml.writeAttribute("translation", temp_element.attribute("translation", temp_element.text()));
                                    write_molecules_to_xml.writeAttribute("modulation", temp_element.attribute("modulation", temp_element.text()));
                                    write_molecules_to_xml.writeAttribute("phase", temp_element.attribute("phase", temp_element.text()));

                                    if(temp_element.nodeName() == "Atom")
                                        write_molecules_to_xml.writeEndElement();//close atoms immediately
                                    else
                                        end_element_counter++;//keep in mind how many molecules are opened

                                    if(temp_element.hasChildNodes())//if there are children, that means the current element is not an atom: go to next child
                                    {
                                        temp_element = temp_element.firstChild().toElement();
                                        child_counter++;//keep in mind how many children exist
                                    }
                                    else
                                        temp_element = temp_element.nextSibling().toElement();//else there are no children, there must be an atom , so take the next atom

                                }
                                root_counter++;
                                write_molecules_to_xml.writeEndElement();

                                temp_element = current_element;

                                //for(qint32 node_depth = 2; node_depth < child_counter; node_depth++)
                                //    temp_element = temp_element.firstChildElement();

                                for(qint32 node_depth = 2; node_depth < child_counter; node_depth++)
                                    temp_element = temp_element.childNodes().at(root_counter).toElement();

                            }
                            //for(qint32 close_root_molecs = end_element_counter; close_root_molecs > 1; close_root_molecs--)
                            for(qint32 close_root_molecs = level_counter; close_root_molecs > 0; close_root_molecs--)
                                write_molecules_to_xml.writeEndElement();//end elements for one submolecule in a "parentmolecule"

                        }

                        //for(qint32 close_parent_molecs = root_counter; close_parent_molecs > 1; close_parent_molecs--)
                        //    write_molecules_to_xml.writeEndElement();//close all submolecules inside a new parentmolecule

                        current_element = current_element.nextSiblingElement("Molecule").toElement();
                    }

                }//for all similar molecules

                //for(qint32 close_parent_molecs = root_counter; close_parent_molecs > 1; close_parent_molecs--)
                write_molecules_to_xml.writeEndElement();

                //remove found molecules from node list
                for(qint32 n = 0; n < similar_molecs.length(); n++)
                {
                    current_element = node_list.at(0).toElement();
                    QDomNode to_remove = current_element.parentNode();

                    while(!current_element.isNull())
                    {
                        if(current_element.attribute("ID").toInt() == similar_molecs.at(n) && current_element.attribute("level").toInt() == level_counter)
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

        file.flush();
        temp_file.flush();
        file.close();
        temp_file.close();

        file.remove();        

        temp_file.copy(save_path);
        temp_file.remove();

        level_counter++;

        build_molecule_xml_file(level_counter);
    }

}
