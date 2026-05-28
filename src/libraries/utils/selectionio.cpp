//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *
 * @file selectionio.cpp
 * @since May 2015
 * @brief Implementation of the MNE @c .sel and Brainstorm @c .mon parsers declared in @ref selectionio.h.
 *
 * Parsing is line-oriented and intentionally lenient: comment
 * lines (@c #) and blank lines are skipped, trailing whitespace
 * is stripped, and channel names are matched case-sensitively
 * because FIFF names are case-sensitive themselves. The writer
 * preserves the original group ordering so a round-trip
 * load/save through mne-cpp does not reshuffle a user's
 * selections.
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
    if(path.find(".sel") == std::string::npos)
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
            // Split on ':' → key : channels
            auto colonPos = line.find(':');
            std::string key = line.substr(0, colonPos);
            std::string channelsPart = line.substr(colonPos + 1);

            // Split channels on '|'
            std::vector<std::string> channels;
            std::stringstream stream{channelsPart};
            std::string token;
            while(std::getline(stream, token, '|')){
                channels.push_back(token);
            }

            // Remove trailing empty element
            if(!channels.empty() && channels.back().empty()){
                channels.pop_back();
            }

            selectionMap.insert({key, channels});
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
    if(path.find(".mon") == std::string::npos)
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
            auto colonPos = line.find(':');
            std::string channelName = line.substr(0, colonPos);
            channels.push_back(channelName);
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
    if (!outFile.is_open()){
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
    for(auto& mapElement : selectionMap){
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
