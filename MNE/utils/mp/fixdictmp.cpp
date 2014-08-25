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


    bool isDouble = false;

    qint32 atomCount = 0;
    qint32 bestCorrStartIndex;
    qreal sample;
    qreal bestCorrValue = 0;
    qreal residuumEnergie = 0;

    QString contents;
    QString atomName;
    QString bestCorrName;

    QList<qreal> atomSamples;
    QList<QStringList> correlationList;
    VectorXd residuum = signalSamples;


    //originalSignalSamples = signalSamples;

    // Liest das Woerterbuch aus und gibt die Samples und den Namen an die Skalarfunktion weiter
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
            correlationList.append(correlation(signalSamples, atomSamples, atomName));

            atomSamples.clear();
        }
        currentDict.close();

        // Sucht aus allen verglichenen Atomen das beste passende herraus
        for(qint32 i = 0; i < correlationList.length(); i++)
        {
            if(fabs(correlationList.at(i).at(2).toDouble()) > fabs(bestCorrValue))
            {
                bestCorrName =  correlationList.at(i).at(0);
                bestCorrStartIndex = correlationList.at(i).at(1).toInt();
                bestCorrValue = correlationList.at(i).at(2).toDouble();
            }
        }

        // Sucht das passende Atom im Woerterbuch und traegt dessen Werte in eine Liste
        if (currentDict.open (QIODevice::ReadOnly))
        {
            bool hasFound = false;
            qint32 j = 0;
            while(!currentDict.atEnd() )
            {
                contents = currentDict.readLine();
                if(QString::compare(contents, bestCorrName) == 0)
                {
                    contents = currentDict.readLine();

                    QStringList list = contents.split(':');
                    gabor_Atom->sample_count = 256;
                    QString t = list.at(1);
                    qreal scale = t.remove(t.length() - 5, 5).toDouble(&isDouble);
                    if(isDouble)
                        gabor_Atom->scale = scale;
                    gabor_Atom->translation = bestCorrStartIndex - 128;
                    t = list.at(2);
                    qreal modu = t.remove(t.length() - 6, 6).toDouble(&isDouble);
                    if(isDouble)
                        gabor_Atom->modulation = modu;
                    t = list.at(3);
                    qreal phase = t.remove(t.length() - 6, 6).toDouble(&isDouble);
                    if(isDouble)
                        gabor_Atom->phase = phase;
                    gabor_Atom->max_scalar_product = bestCorrValue;

                    //---------------------------------

                    for(qint32 chn = 0; chn < 1; chn++)
                    {
                        //simplexfunction to find minimum of target among parameters s, p, k
                        std::vector<double> init;

                        init.push_back(gabor_Atom->scale);
                        init.push_back(gabor_Atom->translation);
                        init.push_back(gabor_Atom->modulation);

                        double tol = 1E8 * std::numeric_limits<double>::epsilon();
                        std::vector<std::vector<double> > x = std::vector<std::vector<double> >();
                        qint32 iterations = 1E3;
                        qint32 N = init.size();                     //space dimension

                        VectorXd atom_fxc_params = VectorXd::Zero(5); //initialisation for contraction coefficients

                        const qreal a=1.0, b=0.2, g=0.5, h=0.5;  //coefficients a = 1, b = 0.2, g = 0.5, h = 0.5
                                                                 //a: reflection  -> xr step away from worst siplex found
                                                                 //b: expansion   -> xe if better with a so go in this direction with b
                                                                 //g: contraction -> xc calc new worst point an bring closer to middle of simplex
                                                                 //h: full contraction to x1
                        std::vector<double> xcentroid_old(N,0);  //simplex center * (N+1)
                        std::vector<double> xcentroid_new(N,0);  //simplex center * (N+1)
                        std::vector<double> vf(N+1,0);           //f evaluated at simplex vertices
                        qint32 x1 = 0, xn = 0, xnp1 = 0;         //x1:   f(x1) = min { f(x1), f(x2)...f(x_{n+1} }
                                                                 //xnp1: f(xnp1) = max { f(x1), f(x2)...f(x_{n+1} }
                                                                 //xn:   f(xn)<f(xnp1) && f(xn)> all other f(x_i)
                        qint32 cnt = 0; //iteration step number

                        if(x.size()== 0) //if no initial simplex is specified
                        {
                            //construct the trial simplex
                            //based upon the initial guess parameters
                            std::vector<double> del( init );
                            std::transform(del.begin(), del.end(), del.begin(),
                            std::bind2nd( std::divides<double>() , 20) );//'20' is picked
                                                                 //assuming initial trail close to true

                            for(qint32 i = 0; i < N; ++i)
                            {
                                std::vector<double> tmp( init );
                                tmp[i] +=  del[i];
                                x.push_back( tmp );
                            }

                            x.push_back(init);//x.size()=N+1, x[i].size()=N

                            //xcentriod
                            std::transform(init.begin(), init.end(), xcentroid_old.begin(), std::bind2nd(std::multiplies<double>(), N+1) );
                        }//constructing the simplex finished

                        qint32 sample_count = 256;

                        //optimization begins
                        for(cnt=0; cnt<iterations; ++cnt)
                        {
                            for(qint32 i=0; i < N+1; ++i)
                            {
                                VectorXd atom_fx = VectorXd::Zero(sample_count);

                                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                    atom_fx = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), x[i][2], chn, residuum, RETURNATOM, false);

                                else
                                    atom_fx = AdaptiveMp::calculate_atom(sample_count, x[i][0], x[i][1], x[i][2], chn, residuum, RETURNATOM, false);

                                //create targetfunction of realGaborAtom and Residuum
                                double target = 0;
                                for(qint32 k = 0; k < atom_fx.rows(); k++)
                                {
                                    target -=atom_fx[k]*residuum(k,0);
                                }

                                vf[i] = target;
                            }

                            x1=0; xn=0; xnp1=0;//find index of max, second max, min of vf.

                            for(quint32 i=0; i < vf.size(); ++i)
                            {
                                if(vf[i]<vf[x1])      x1 = i;
                                if(vf[i]>vf[xnp1])    xnp1 = i;
                            }

                            xn = x1;

                            for(quint32 i=0; i<vf.size();++i) if(vf[i]<vf[xnp1] && vf[i]>vf[xn])  xn=i;

                            //x1, xn, xnp1 are found

                            std::vector<double> xg(N, 0);//xg: centroid of the N best vertexes

                            for(quint32 i=0; i<x.size(); ++i) if(i!=xnp1) std::transform(xg.begin(), xg.end(), x[i].begin(), xg.begin(), std::plus<double>() );

                            std::transform(xg.begin(), xg.end(), x[xnp1].begin(), xcentroid_new.begin(), std::plus<double>());
                            std::transform(xg.begin(), xg.end(), xg.begin(), std::bind2nd(std::divides<double>(), N) );
                            //xg found, xcentroid_new updated

                            //termination condition
                            double diff=0;          //calculate the difference of the simplex centers

                            //see if the difference is less than the termination criteria
                            for(qint32 i=0; i<N; ++i) diff += fabs(xcentroid_old[i]-xcentroid_new[i]);

                            if (diff/N < tol) break;              //terminate the optimizer
                            else xcentroid_old.swap(xcentroid_new); //update simplex center

                            //reflection:
                            std::vector<double> xr(N,0);

                            for( qint32 i=0; i<N; ++i) xr[i]=xg[i]+a*(xg[i]-x[xnp1][i]);
                            //reflection, xr found

                            VectorXd atom_fxr = VectorXd::Zero(sample_count);

                            if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                atom_fxr = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xr[2], chn, residuum, RETURNATOM, false);

                            else
                                atom_fxr = AdaptiveMp::calculate_atom(sample_count, xr[0], xr[1], xr[2], chn, residuum, RETURNATOM, false);

                            //create targetfunction of realGaborAtom and Residuum
                            double fxr = 0;
                            for(qint32 k = 0; k < atom_fxr.rows(); k++) fxr -=atom_fxr[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                            //double fxr = target;//record function at xr

                            if(vf[x1]<=fxr && fxr<=vf[xn]) std::copy(xr.begin(), xr.end(), x[xnp1].begin());

                            //expansion:
                            else if(fxr<vf[x1])
                            {
                                std::vector<double> xe(N,0);

                                for( qint32 i=0; i<N; ++i) xe[i]=xr[i]+b*(xr[i]-xg[i]);

                                VectorXd atom_fxe = VectorXd::Zero(sample_count);

                                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                    atom_fxe = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xe[2], chn, residuum, RETURNATOM, false);

                                else
                                    atom_fxe = AdaptiveMp::calculate_atom(sample_count, xe[0], xe[1], xe[2], chn, residuum, RETURNATOM, false);

                                //create targetfunction of realGaborAtom and Residuum
                                double fxe = 0;
                                for(qint32 k = 0; k < atom_fxe.rows(); k++) fxe -=atom_fxe[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                                if( fxe < fxr ) std::copy(xe.begin(), xe.end(), x[xnp1].begin() );
                                else std::copy(xr.begin(), xr.end(), x[xnp1].begin() );
                            }//expansion finished,  xe is not used outside the scope

                            //contraction:
                            else if( fxr > vf[xn] )
                            {
                                std::vector<double> xc(N,0);

                                for( qint32 i=0; i<N; ++i)
                                    xc[i]=xg[i]+g*(x[xnp1][i]-xg[i]);

                                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                    atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xc[2], chn, residuum, RETURNPARAMETERS, false);

                                else
                                    atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, xc[0], xc[1], xc[2], chn, residuum, RETURNPARAMETERS, false);

                                VectorXd atom_fxc = gabor_Atom->create_real(gabor_Atom->sample_count, atom_fxc_params[0], atom_fxc_params[1], atom_fxc_params[2], atom_fxc_params[3]);

                                atom_fxc_params[4] = 0;

                                for(qint32 i = 0; i < sample_count; i++)
                                    atom_fxc_params[4] += atom_fxc[i] * residuum(i, chn);

                                //create targetfunction of realGaborAtom and Residuum
                                double fxc = 0;

                                for(qint32 k = 0; k < atom_fxc.rows(); k++)
                                    fxc -=atom_fxc[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                                if( fxc < vf[xnp1] )
                                    std::copy(xc.begin(), xc.end(), x[xnp1].begin() );

                                else
                                    for( quint32 i=0; i<x.size(); ++i )
                                        if( i!=x1 )
                                            for(qint32 j=0; j<N; ++j)
                                                x[i][j] = x[x1][j] + h * ( x[i][j]-x[x1][j] );
                            }//contraction finished, xc is not used outside the scope
                        }//optimization is finished

                        if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                            atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), x[x1][2], chn, residuum, RETURNPARAMETERS, false);

                        else
                            atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, x[x1][0], x[x1][1], x[x1][2], chn, residuum, RETURNPARAMETERS, false);

                        if(abs(atom_fxc_params[4]) > abs(bestCorrValue) /*&& atom_fxc_params[0] < sample_count && atom_fxc_params[0] > 0*/ && atom_fxc_params[1] < sample_count && atom_fxc_params[1] > 0)//ToDo: find a way to make the simplex not running out of bounds
                        {
                            bestCorrValue = atom_fxc_params[4];             //scalarProduct
                            gabor_Atom->scale              = atom_fxc_params[0];//scale
                            gabor_Atom->translation        = atom_fxc_params[1];//translation
                            gabor_Atom->modulation         = atom_fxc_params[2];//phase
                            gabor_Atom->phase              = atom_fxc_params[3];
                            gabor_Atom->max_scalar_product   = bestCorrValue;
                        }

                        if(cnt==iterations)//max number of iteration achieves before tol is satisfied
                            std::cout<<"Simplex Iteration limit of "<<iterations<<" achieved in channel " << chn << ", result may not be optimal";

                    }//end Maximisation for channels Copyright (C) 2010 Botao Jia


                    //-----------------------------------

                    std::cout << "\n" << "===============" << " found parameters " << 1 << "===============" << ":\n\n"<<
                                 "scale: " << gabor_Atom->scale << " trans: " << gabor_Atom->translation <<
                                 " modu: " << gabor_Atom->modulation << " phase: " << gabor_Atom->phase << " scalarproduct: " << gabor_Atom->max_scalar_product << "\n";

                   atom_list.append(*gabor_Atom);
                    //atom_res_list.append(*gabor_Atom);
                    hasFound = true;
                }
                if(hasFound) break;
            }
        }

        currentDict.close();

        //recieve the resulting atomparams
        GaborAtom gaborAtom = atom_list.last();

        VectorXd discret_atom = gaborAtom.create_real(gaborAtom.sample_count, gaborAtom.scale, gaborAtom.translation, gaborAtom.modulation, gaborAtom.phase);

        //_atom_sum_matrix.col(0) += gaborAtom.max_scalar_product * discret_atom;
        //_residuum_matrix.col(0) -= gaborAtom.max_scalar_product  * discret_atom;
        //residuum.col(0) -= gaborAtom.max_scalar_product  * discret_atom;
    }
    iterationsCount--;

    return atom_list;
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
