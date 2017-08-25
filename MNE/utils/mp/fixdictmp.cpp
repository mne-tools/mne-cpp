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


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QThread>
#include <QtConcurrent>
#include <QFuture>
#include <QFile>
#include <QStringList>


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
, epsilon(0)
, max_iterations(0)
{

}


//*************************************************************************************************************

FixDictMp::~FixDictMp()
{

}


//*************************************************************************************************************

Dictionary::Dictionary()
: type(AtomType::GABORATOM)
, sample_count(0)
{

}


//*************************************************************************************************************

Dictionary::~Dictionary()
{

}


//*************************************************************************************************************

void FixDictMp::matching_pursuit(MatrixXd signal, qint32 max_iterations, qreal epsilon, qint32 boost, QString path, qreal delta)
{
    Eigen::initParallel();

    std::cout << "\nFixDict Matching Pursuit Algorithm started...\n";

    QList<Dictionary> parsed_dicts;

    qint32 sample_count = signal.rows();
    qint32 channel_count = signal.cols();

    signal_energy = 0;
    qreal residuum_energy = 0;
    qreal energy_threshold = 0;
    qreal last_energy = 0;
    bool sample_count_mismatch = false;

    this->residuum = signal;
    parsed_dicts = parse_xml_dict(path);

    //calculate signal_energy
    for(qint32 channel = 0; channel < channel_count; channel++)
    {
        for(qint32 sample = 0; sample < sample_count; sample++)
            signal_energy += (signal(sample, channel) * signal(sample, channel));

        energy_threshold = 0.01 * epsilon * signal_energy;
        residuum_energy = signal_energy;
    }

    std::cout << "absolute energy of signal: " << residuum_energy << "\n";

    while(it < max_iterations && energy_threshold < residuum_energy)
    {
        FixDictAtom global_best_matching;

        QList<find_best_matching> list_of_best;
        for(qint32 i = 0; i < parsed_dicts.length(); i++)
        {
            find_best_matching current_best_matching;
            current_best_matching.pdict = parsed_dicts.at(i);
            current_best_matching.current_resid = this->residuum;
            current_best_matching.boost = boost;
            list_of_best.append(current_best_matching);
        }

        QFuture<FixDictAtom> mapped_best_matchings = QtConcurrent::mapped(list_of_best, &find_best_matching::parallel_correlation);// parse_threads;
        mapped_best_matchings.waitForFinished();

        QFuture<FixDictAtom>::const_iterator i;
        for (i = mapped_best_matchings.constBegin(); i != mapped_best_matchings.constEnd(); i++)
        {
            //FixDictAtom current_best_matching = correlation(parsed_dicts.at(i));
            if(i == mapped_best_matchings.constBegin())
                global_best_matching = *i;

            else if(std::fabs(i->max_scalar_product) > std::fabs(global_best_matching.max_scalar_product))
                global_best_matching = *i;
        }

        global_best_matching.display_text = create_display_text(global_best_matching);

        VectorXd fitted_atom = VectorXd::Zero(this->residuum.rows());
        VectorXd resized_atom = VectorXd::Zero(this->residuum.rows());

        if(global_best_matching.atom_samples.rows() > this->residuum.rows())
            for(qint32 k = 0; k < this->residuum.rows(); k++)    
                resized_atom[k] = global_best_matching.atom_samples[k + floor(global_best_matching.atom_samples.rows() / 2) - floor(this->residuum.rows() / 2)];
        else    resized_atom = global_best_matching.atom_samples;


        for(qint32 k = 0; k < resized_atom.rows(); k++)
            if((k + global_best_matching.translation - floor(resized_atom.rows() / 2)) >= 0
                    && (k + global_best_matching.translation - floor(resized_atom.rows() / 2)) < fitted_atom.rows())
                    fitted_atom[(k + global_best_matching.translation - floor(resized_atom.rows() / 2))] += resized_atom[k];

        //normalization
        qreal norm = 0;
        norm = fitted_atom.norm();
        if(norm != 0) fitted_atom /= norm;

        for(qint32 chn = 0; chn < this->residuum.cols(); chn++)
        {
            qreal scalarproduct = 0;
            for(qint32 sample = 0; sample < this->residuum.rows(); sample++)
                scalarproduct += (this->residuum(sample, chn) * fitted_atom[sample]);

            global_best_matching.max_scalar_list.append(scalarproduct);//residuum(global_best_matching.translation, chn) / fitted_atom[global_best_matching.translation]);

            for(qint32 k = 0; k < fitted_atom.rows(); k++)
            {
                this->residuum(k,chn) -= global_best_matching.max_scalar_list.at(chn) * fitted_atom[k];
                global_best_matching.energy += pow(global_best_matching.max_scalar_list.at(chn) * fitted_atom[k], 2); //  * global_best_matching.max_scalar_list.at(chn) * fitted_atom[k];
            }
        }

        global_best_matching.atom_samples = fitted_atom;


        last_energy = residuum_energy;
        /*residuum_energy = 0;
        for(qint32 channel = 0; channel < residuum.cols(); channel++)
        {
            for(qint32 sample = 0; sample < residuum.rows(); sample++)
                residuum_energy += pow(residuum(sample, channel), 2);
        }*/

        residuum_energy -= global_best_matching.energy;
        current_energy += global_best_matching.energy;

        fix_dict_list.append(global_best_matching);

        if(global_best_matching.sample_count != signal.rows() && !sample_count_mismatch)
        {
            std::cout <<  "\n=============================================\n"
                      << "  INFO\n\n(part-)dictionary does not fit the signal!\n\nResults might be better by creating dictionaries\ncontaining atoms of the same length as the signal\n"
                      << "\nsignal samples: " << signal.rows() << "\n"
                      << "atom samples: " << global_best_matching.sample_count << "\n"
                      <<  "=============================================\n";
            sample_count_mismatch = true;
        }

        std::cout << "\n" << "===============" << it + 1 <<"th atom found" << "===============" << ":\n\n"<<
                     qPrintable(global_best_matching.display_text) << "\n\n" <<  "sample_count: " << global_best_matching.sample_count <<
                     " source_dict: " << qPrintable(global_best_matching.dict_source) << " Atom ID: " << global_best_matching.id <<
                     "\nsclr_prdct: " << global_best_matching.max_scalar_product << "\n" <<
                     "\nabsolute energy of residue: " << residuum_energy << "\n";

        it++;

        emit current_result(it, max_iterations, current_energy, signal_energy, residuum, adaptive_list, fix_dict_list);


        if(((last_energy * 100 / signal_energy) - (residuum_energy * 100 / signal_energy)) < delta)
        {
            std::cout <<  "\n=============================================\n"
                      << "  ALGORITHM ABORTED\n\ndictionary excludes atoms to reduce further residual energy\n"
                      <<  "=============================================\n";
            emit send_warning(1);
            break;
        }
        if( QThread::currentThread()->isInterruptionRequested())
        {
            send_warning(10);
            break;
        }

    }//end while iterations

    std::cout << "\nFixDict Matching Pursuit Algorithm finished.\n";
    emit finished_calc();
    //return atom_list;
}


//*************************************************************************************************************

// calc scalarproduct of Atom and Signal
FixDictAtom FixDictMp::correlation(Dictionary current_pdict, MatrixXd current_resid, qint32 boost)
{
    qint32 channel_count = current_resid.cols() * (boost / 100.0); //reducing the number of observed channels in the algorithm to increase speed performance
    if(boost == 0 || channel_count == 0)
        channel_count = 1;

    Eigen::FFT<double> fft;
    std::ptrdiff_t max_index;
    VectorXcd fft_atom = VectorXcd::Zero(current_resid.rows());

    FixDictAtom best_matching;
    qreal max_scalar_product = 0;

    for(qint32 i = 0; i < current_pdict.atoms.length(); i++)
    {
        VectorXd fitted_atom = VectorXd::Zero(current_resid.rows());
        qint32 p = floor(current_resid.rows() / 2);//translation

        VectorXd resized_atom = VectorXd::Zero(current_resid.rows());

        if(current_pdict.atoms.at(i).atom_samples.rows() > current_resid.rows())
            for(qint32 k = 0; k < current_resid.rows(); k++)
                resized_atom[k] = current_pdict.atoms.at(i).atom_samples[k + floor(current_pdict.atoms.at(i).atom_samples.rows() / 2) - floor(current_resid.rows() / 2)];
        else resized_atom = current_pdict.atoms.at(i).atom_samples;

        if(resized_atom.rows() < current_resid.rows())
            for(qint32 k = 0; k < resized_atom.rows(); k++)
                fitted_atom[(k + p - floor(resized_atom.rows() / 2))] = resized_atom[k];
        else fitted_atom = resized_atom;

        //normalization
        qreal norm = 0;
        norm = fitted_atom.norm();
        if(norm != 0) fitted_atom /= norm;

        fft.fwd(fft_atom, fitted_atom);

        for(qint32 chn = 0; chn < channel_count; chn++)
        {
            p = floor(current_resid.rows() / 2);//translation
            VectorXd corr_coeffs = VectorXd::Zero(current_resid.rows());
            VectorXcd fft_signal = VectorXcd::Zero(current_resid.rows());
            VectorXcd fft_sig_atom = VectorXcd::Zero(current_resid.rows());

            fft.fwd(fft_signal, current_resid.col(chn));

            for( qint32 m = 0; m < current_resid.rows(); m++)
                fft_sig_atom[m] = fft_signal[m] * conj(fft_atom[m]);

            fft.inv(corr_coeffs, fft_sig_atom);

            //find index of maximum correlation-coefficient to use in translation
            max_scalar_product = corr_coeffs.maxCoeff(&max_index);

            if(i == 0)
            {
                best_matching = current_pdict.atoms.at(i);
                best_matching.max_scalar_product = max_scalar_product;

                //adapting translation p to create atomtranslation correctly
                if(max_index >= p && current_resid.rows() % (2) == 0) p = max_index - p;
                else if(max_index >= p && current_resid.rows() % (2) != 0) p = max_index - p - 1;
                else p = max_index + p;

                best_matching.translation = p;
            }

            else if(std::fabs(max_scalar_product) > std::fabs(best_matching.max_scalar_product))
            {
                best_matching = current_pdict.atoms.at(i);
                best_matching.max_scalar_product = max_scalar_product;

                //adapting translation p to create atomtranslation correctly
                if(max_index >= p && current_resid.rows() % (2) == 0) p = max_index - p;
                else if(max_index >= p && current_resid.rows() % (2) != 0) p = max_index - p - 1;
                else p = max_index + p;

                best_matching.translation = p;
            }
        }
    }
    best_matching.atom_formula = current_pdict.atom_formula;
    best_matching.dict_source = current_pdict.source;
    best_matching.type = current_pdict.type;
    best_matching.sample_count = current_pdict.sample_count;

    return best_matching;
}


//*************************************************************************************************************

QList<Dictionary> FixDictMp::parse_xml_dict(QString path)
{
    QFile current_dict(path);
    std::cout << "\nparsing dictionary, please be patient...";

    QDomDocument dictionary;
    dictionary.setContent(&current_dict);
    QList<Dictionary> parsed_dict;
    QDomElement current_element = dictionary.documentElement();
    QDomNodeList node_list = current_element.childNodes();
    QList<parse_node> pdict_nodes;
    QList<QDomNode> nodes_listed;
    bool is_emitted = false;

    for(qint32 i = 0; i < node_list.length(); i++)
    {
        parse_node temp_parse;
        temp_parse.node = node_list.at(i);
        pdict_nodes.append(temp_parse);
        nodes_listed.append(node_list.at(i));
    }

    //multithreading
    QFuture<Dictionary> mapped_dicts = QtConcurrent::mapped(pdict_nodes, &parse_node::fill_dict_in_map);// parse_threads;
    mapped_dicts.waitForFinished();

    QFuture<Dictionary>::const_iterator i;

    for (i = mapped_dicts.constBegin(); i != mapped_dicts.constEnd(); i++)
    {
        //find_best_matching current_best_matching;
        //current_best_matching.pdict = *i;
        //current_best_matching.current_resid = this->residuum;
        if(i->sample_count != this->residuum.rows() && !is_emitted)
        {
            is_emitted = true;
            emit send_warning(2);
        }

        parsed_dict.append(*i);
    }

    std::cout << "   done.\n\n";

    return parsed_dict;
}


//*************************************************************************************************************

Dictionary FixDictMp::fill_dict(const QDomNode &pdict)
{
    Dictionary current_dict;
    QDomElement atom = pdict.firstChildElement("ATOM");

    current_dict.source = pdict.toElement().attribute("source_dict");
    current_dict.atom_count();// = pdict.toElement().attribute("atom_count").toInt();
    current_dict.atom_formula = pdict.toElement().attribute("formula");
    current_dict.sample_count = pdict.toElement().attribute("sample_count").toInt();

    if(current_dict.atom_formula == QString("Gaboratom"))
    {
        current_dict.type = GABORATOM;

        while(!atom.isNull())
        {
            FixDictAtom current_atom;

            current_atom.id = atom.attribute("ID").toInt();
            current_atom.gabor_atom.scale = atom.attribute("scale").toDouble();
            current_atom.gabor_atom.modulation = atom.attribute("modu").toDouble();
            current_atom.gabor_atom.phase = atom.attribute("phase").toDouble();

            if(atom.hasChildNodes())
            {
                QString sample_string = atom.firstChild().toElement().attribute("samples");
                QStringList sample_list = sample_string.split(":",  QString::SkipEmptyParts);
                current_atom.atom_samples = VectorXd::Zero(sample_list.length());
                for(qint32 i = 0; i < sample_list.length(); i++)
                    current_atom.atom_samples[i] = sample_list.at(i).toDouble();
            }
            current_dict.atoms.append(current_atom);
            atom = atom.nextSiblingElement("ATOM");
        }        
    }
    else if(current_dict.atom_formula == QString("Chirpatom"))
    {
        current_dict.type = CHIRPATOM;

        while(!atom.isNull())
        {
            FixDictAtom current_atom;

            current_atom.id = atom.attribute("id").toInt();
            current_atom.chirp_atom.scale = atom.attribute("scale").toDouble();
            current_atom.chirp_atom.modulation = atom.attribute("modu").toDouble();
            current_atom.chirp_atom.phase = atom.attribute("phase").toDouble();
            current_atom.chirp_atom.chirp = atom.attribute("chirp").toDouble();

            if(atom.hasChildNodes())
            {
                QString sample_string = atom.firstChild().toElement().attribute("samples");
                QStringList sample_list = sample_string.split(":", QString::SkipEmptyParts);
                current_atom.atom_samples = VectorXd::Zero(sample_list.length());
                for(qint32 i = 0; i < sample_list.length(); i++)
                    current_atom.atom_samples[i] = sample_list.at(i).toDouble();
            }
            current_dict.atoms.append(current_atom);
            atom = atom.nextSiblingElement("ATOM");
        }
    }
    else
    {
        current_dict.type = FORMULAATOM;

        while(!atom.isNull())
        {
            FixDictAtom current_atom;

            current_atom.id = atom.attribute("id").toInt();
            current_atom.formula_atom.a = atom.attribute("a").toDouble();
            current_atom.formula_atom.b = atom.attribute("b").toDouble();
            current_atom.formula_atom.c = atom.attribute("c").toDouble();
            current_atom.formula_atom.d = atom.attribute("d").toDouble();
            current_atom.formula_atom.e = atom.attribute("e").toDouble();
            current_atom.formula_atom.f = atom.attribute("f").toDouble();
            current_atom.formula_atom.g = atom.attribute("g").toDouble();
            current_atom.formula_atom.h = atom.attribute("h").toDouble();

            if(atom.hasChildNodes())
            {
                QString sample_string = atom.firstChild().toElement().attribute("samples");
                QStringList sample_list = sample_string.split(":", QString::SkipEmptyParts);
                current_atom.atom_samples = VectorXd::Zero(sample_list.length());
                for(qint32 i = 0; i < sample_list.length(); i++)
                    current_atom.atom_samples[i] = sample_list.at(i).toDouble();
            }
            current_dict.atoms.append(current_atom);
            atom = atom.nextSiblingElement("ATOM");
        }
    }

    return current_dict;
}


//*************************************************************************************************************

QString FixDictMp::create_display_text(const FixDictAtom& global_best_matching)
{
    QString display_text = "";
    if(global_best_matching.type == GABORATOM)
    {
        display_text = QString("Gaboratom: scale: %0, translation: %1, modulation: %2, phase: %3")
                .arg(QString::number(global_best_matching.gabor_atom.scale, 'f', 2))
                .arg(QString::number(global_best_matching.translation, 'f', 2))
                .arg(QString::number(global_best_matching.gabor_atom.modulation, 'f', 2))
                .arg(QString::number(global_best_matching.gabor_atom.phase, 'f', 2));
    }
    else if(global_best_matching.type == CHIRPATOM)
    {
        display_text = QString("Chripatom: scale: %0, translation: %1, modulation: %2, phase: %3, chirp: %4")
                .arg(QString::number(global_best_matching.chirp_atom.scale, 'f', 2))
                .arg(QString::number(global_best_matching.translation, 'f', 2))
                .arg(QString::number(global_best_matching.chirp_atom.modulation, 'f', 2))
                .arg(QString::number(global_best_matching.chirp_atom.phase, 'f', 2))
                .arg(QString::number(global_best_matching.chirp_atom.chirp, 'f', 2));
    }
    else if(global_best_matching.type == FORMULAATOM)
    {
        display_text = QString("%0:  transl: %1 a: %2, b: %3 c: %4, d: %5, e: %6, f: %7, g: %8, h: %9")
                .arg(global_best_matching.atom_formula)
                .arg(QString::number(global_best_matching.translation,    'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.a, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.b, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.c, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.d, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.e, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.f, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.g, 'f', 2))
                .arg(QString::number(global_best_matching.formula_atom.h, 'f', 2));
    }

    return display_text;
}


//*************************************************************************************************************

void FixDictMp::recieve_input(MatrixXd signal, qint32 max_iterations, qreal epsilon, qint32 boost, QString path, qreal delta)
{
    matching_pursuit(signal, max_iterations, epsilon, boost, path, delta);
}


//*************************************************************************************************************

qint32 Dictionary::atom_count()
{
    return atoms.length();
}


//*************************************************************************************************************

 void Dictionary::clear()
 {
     this->atoms.clear();
     this->atom_formula = "";
     this->sample_count = 0;
     this->source = "";
 }


 //*************************************************************************************************************

/*
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
*/
