//=============================================================================================================
/**
 * @file     selectionio.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the SelectionIO class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionio.h"

#include <iostream>
#include <fstream>
#include <sstream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QScreen>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SelectionIO::SelectionIO()
{
}

//=============================================================================================================

bool SelectionIO::readMNESelFile(QString path, QMultiMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".sel"))
        return false;

    //clear the map first
    selectionMap.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening selection file";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);

    while(!in.atEnd()) {
        QString line = in.readLine();

        if(line.contains("%") == false && line.contains(":") == true) //Skip commented areas in file
        {
            QStringList firstSplit = line.split(":");

            //Create new key
            QString key = firstSplit.at(0);

            QStringList secondSplit = firstSplit.at(1).split("|");

            //Delete last element if it is a blank character
            if(secondSplit.at(secondSplit.size()-1) == "")
                secondSplit.removeLast();

            //Add to map
            selectionMap.insert(key, secondSplit);
        }
    }

    file.close();

    return true;
}

//=============================================================================================================

bool SelectionIO::readMNESelFile(const std::string& path, std::multimap<std::string,std::vector<std::string>> &selectionMap)
{
    //Open .sel file
    if(path.find(".sel") != std::string::npos)
        return false;

    //clear the map first
    selectionMap.clear();

    std::ifstream inFile(path);
    if(!inFile.is_open()){
        qDebug()<<"Error opening selection file";
        return false;
    }

    std::string line;

    while(std::getline(inFile, line)){
        if(line.find('%') == std::string::npos && line.find(':') != std::string::npos){
            std::stringstream stream{line};
            std::vector<std::string> firstSplit;
            for (std::string element; std::getline(stream, line, ':');){
                firstSplit.push_back(element);
            }
            std::string key = firstSplit.at(0);

            std::vector<std::string> secondSplit;
            for (std::string element; std::getline(stream, line, '|');){
                secondSplit.push_back(element);
            }
            if(secondSplit.back() == ""){
                secondSplit.pop_back();
            }
            selectionMap.insert({key, secondSplit});
        }
    }

    return true;
}

//=============================================================================================================

bool SelectionIO::readBrainstormMonFile(QString path, QMultiMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".mon"))
        return false;

    //clear the map first
    selectionMap.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening montage file";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);
    QString groupName = in.readLine();
    QStringList channels;

    while(!in.atEnd()) {
        QString line = in.readLine();

        if(line.contains(":") == true) {
            QStringList secondSplit = line.split(":");
            QString key = secondSplit.at(0);
            channels.append(key);
        }
    }

    //Add to map
    selectionMap.insert(groupName, channels);

    file.close();

    return true;
}

//=============================================================================================================

bool SelectionIO::readBrainstormMonFile(const std::string& path, std::multimap<std::string,std::vector<std::string>>& selectionMap)
{
    //Open file
    if(path.find(".mon") != std::string::npos)
        return false;

    //clear the map first
    selectionMap.clear();

    std::ifstream inFile(path);
    if(!inFile.is_open()){
        qDebug()<<"Error opening montage file";
        return false;
    }
    std::vector<std::string> channels;

    std::string groupName;
    std::getline(inFile, groupName);

    std::string line;
    while(std::getline(inFile, line)){
        if(line.find(':') != std::string::npos){
            std::stringstream stream{line};
            std::vector<std::string> split;
            for (std::string element; std::getline(stream, line, ':');){
                split.push_back(std::move(element));
            }
            channels.push_back(split.at(0));
        }
    }

    selectionMap.insert({groupName, channels});

    return true;
}

//=============================================================================================================

bool SelectionIO::writeMNESelFile(QString path, const QMultiMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".sel"))
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug()<<"Error opening sel file for writing";
        return false;
    }

    //Write selections to file
    QTextStream out(&file);

    QMultiMap<QString, QStringList>::const_iterator i = selectionMap.constBegin();
    while (i != selectionMap.constEnd()) {
        out << i.key() << ":";

        for(int u=0; u<i.value().size() ; u++)
            out << i.value().at(u) << "|";

        out << "\n" << "\n";

        ++i;
    }

    file.close();

    return true;
}

//=============================================================================================================

bool SelectionIO::writeMNESelFile(const std::string& path, const std::map<std::string,std::vector<std::string>>& selectionMap)
{
    //Open .sel file
    if(path.find(".sel") == std::string::npos)
        return false;

    std::ofstream outFile(path);
    if (outFile.is_open()){
        qDebug()<<"Error opening sel file for writing";
        return false;
    }

    for(auto& mapElement : selectionMap){
        outFile << mapElement.first << ":";
        for(auto& vectorElement : mapElement.second){
            outFile << vectorElement << "|";
        }
        outFile << "\n" << "\n";
    }

    return true;
}

//=============================================================================================================

bool SelectionIO::writeBrainstormMonFiles(QString path, const QMultiMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".mon"))
        return false;

    for(auto i = selectionMap.constBegin(); i != selectionMap.constEnd(); i++) {
        QFileInfo fileInfo(path);

        QString newPath = QString("%1/%2.mon").arg(fileInfo.absolutePath()).arg(i.key());

        //std::cout<<newPath.toStdString()<<std::endl;

        QFile file(newPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug()<<"Error opening mon file for writing";
            return false;
        }

        //Write selections to file
        QTextStream out(&file);

        out<<i.key()<<"\n";

        for(int u=0; u<i.value().size() ; u++)
            out << i.value().at(u) << " : " << i.value().at(u) << "\n";

        file.close();
    }

    return true;
}

//=============================================================================================================

bool SelectionIO::writeBrainstormMonFiles(const std::string& path, const std::map<std::string,std::vector<std::string>>& selectionMap)
{
    //Open file
    if(path.find(".mon") == std::string::npos)
        return false;

    for(auto& mapElement : selectionMap){
        //logic of using parent_path here might not be "correct"
        std::string newPath{path.substr(0, path.find_last_of("/") + 1) + mapElement.first + ".mon"};
        std::ofstream outFile(newPath);
        if(!outFile.is_open()){
            qDebug()<<"Error opening mon file for writing";
            return false;
        }

        outFile << mapElement.first << "\n";
        for(auto& vectorElement : mapElement.second){
            outFile << vectorElement << " : " << vectorElement << "\n";
        }
    }

    return true;
}
