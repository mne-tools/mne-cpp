//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Read the data sets of a FIFF file through the generic FiffIO interface and write its raw data back to disk.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <fiff/fiff_io.h>

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    // The files to read; raw and evoked data are collected from all of them
    const QString sample = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/";
    const std::vector<std::filesystem::path> files{(sample + "sample_audvis_raw.fif").toStdU16String()};

    FiffIO p_fiffIO(files);

    std::cout << p_fiffIO << std::endl;

    bool readSuccessful = false;

    //Read raw data samples
    MatrixXd data, times;
    readSuccessful = p_fiffIO.raw[0]->read_raw_segment_times(data, times, 100, 102);

    if (readSuccessful) {
        //Write some raw data
        p_fiffIO.write((sample + "sample_write/sample_out.fif").toStdU16String(), FIFFB_RAW_DATA);
    } else {
        qDebug() << "Could not read raw segment.";
    }

    return 0;
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
