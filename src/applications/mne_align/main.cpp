//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Entry point for the MNE Align application.
 */

#include "mne_align.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MNE-CPP"));
    QCoreApplication::setApplicationName(QStringLiteral("MNE Align"));
    QCoreApplication::setApplicationVersion(QStringLiteral("2.3.0"));

    MNEALIGN::MneAlign w;
    w.show();
    return app.exec();
}
