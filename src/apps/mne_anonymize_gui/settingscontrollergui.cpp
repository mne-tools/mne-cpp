//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     settingscontrollergui.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.3
 * @date     May, 2020
 * @brief    SettingsControllerGUI class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontrollergui.h"
#include "mainwindow.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimeZone>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr qint64 kJulianDayOfEpoch = 2440588; // Julian day of 1970-01-01

QDate toQDate(const std::chrono::year_month_day& date)
{
    if (!date.ok()) {
        return QDate();
    }
    return QDate(static_cast<int>(date.year()), static_cast<int>(static_cast<unsigned>(date.month())), static_cast<int>(static_cast<unsigned>(date.day())));
}

std::chrono::year_month_day toYearMonthDay(const QDate& date)
{
    if (!date.isValid()) {
        return std::chrono::year_month_day{std::chrono::year{0}, std::chrono::month{0}, std::chrono::day{0}};
    }
    return std::chrono::year_month_day{std::chrono::sys_days{std::chrono::days{date.toJulianDay() - kJulianDayOfEpoch}}};
}

QDateTime localDateTime(std::int64_t secs)
{
    return QDateTime::fromSecsSinceEpoch(secs, QTimeZone::LocalTime);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsControllerGui::SettingsControllerGui(const std::vector<std::string>& arguments)
: m_pWin(QSharedPointer<MainWindow>(new MainWindow))
{
    initParser();
    m_bGuiMode = true;
    parseInputs(arguments);
    setupCommunication();

    initializeOptionsState();

    m_pWin->show();

    QString msg("Ready.");
    m_pWin->statusMsg(msg, 2000);
}

//=============================================================================================================

QString SettingsControllerGui::absoluteFilePath(const std::filesystem::path& path)
{
    return QFileInfo(QString::fromStdString(mne::utf8_from_path(path))).absoluteFilePath();
}

//=============================================================================================================

void SettingsControllerGui::executeAnonymizer()
{
    if (!m_pAnonymizer->isFileInSet()) {
        m_pWin->winPopup("Please specify a valid input file first.");
        return;
    }
    if (!m_pAnonymizer->isFileOutSet()) {
        m_pWin->winPopup("Please specify a valid output file first.");
        return;
    }
    m_pWin->statusMsg("Anonymizing the input file into the output file.", 2000);
    m_pAnonymizer->anonymizeFile();
    m_pWin->outputFileReady();
}

//=============================================================================================================

void SettingsControllerGui::readData()
{
    if (m_pAnonymizer->isFileInSet()) {
        m_pWin->statusMsg("Reading input file information...", 0);
        QString stringTempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        QString fileOutStr(QDir(stringTempDir).filePath(QString::fromStdString(generateRandomFileName())));
        m_pAnonymizer->setOutFile(fileOutStr.toStdString());
        m_pWin->setDefaultStateExtraInfo();
        bool verboseMode(m_pAnonymizer->getVerboseMode());
        m_pAnonymizer->setVerboseMode(false);
        m_pAnonymizer->anonymizeFile();
        QFile fileOut(fileOutStr);
        fileOut.remove();
        m_pAnonymizer->setOutFile(absoluteFilePath(m_fiOutFile).toStdString());
        m_pAnonymizer->setVerboseMode(verboseMode);
        QString msg2("Input file information read correctly.");
        m_pWin->statusMsg(msg2, 2000);
        m_pWin->repaintTabWidget();
    } else {
        m_pWin->winPopup("Cannot read data. Please select a valid input file first.");
    }
}

//=============================================================================================================

void SettingsControllerGui::fileInChanged(const QString& strInFile)
{
    QFileInfo newfiInFile(strInFile);

    if (newfiInFile.isDir()) {
        m_pWin->statusMsg("Invalid input file. That's a directory");
        m_pWin->setInFile(absoluteFilePath(m_fiInFile));
        return;
    }
    if (QString::compare(newfiInFile.suffix(), QString("fif")) != 0) {
        m_pWin->statusMsg("The input file extension must be \".fif\".");
        m_pWin->setInFile(absoluteFilePath(m_fiInFile));
        return;
    }

    if (!newfiInFile.isReadable()) {
        m_pWin->statusMsg("You might not have reading permissions to this folder");
        m_pWin->setInFile(absoluteFilePath(m_fiInFile));
        return;
    }

    m_fiInFile = mne::path_from_utf8(newfiInFile.absoluteFilePath().toStdString());
    m_pAnonymizer->setInFile(absoluteFilePath(m_fiInFile).toStdString());

    generateDefaultOutputFileName();
    m_pWin->setOutFile(absoluteFilePath(m_fiOutFile));
    m_pAnonymizer->setOutFile(absoluteFilePath(m_fiOutFile).toStdString());
}

//=============================================================================================================

void SettingsControllerGui::fileOutChanged(const QString& strOutFile)
{
    QFileInfo newfiOutFile(strOutFile);
    if (newfiOutFile.isDir()) {
        const QFileInfo fiInFile(absoluteFilePath(m_fiInFile));
        QString fileOutDefaultName(newfiOutFile.absolutePath() + fiInFile.baseName() +
                                   "_anonymized." + fiInFile.completeSuffix());
        m_fiOutFile = mne::path_from_utf8(fileOutDefaultName.toStdString());
        m_pWin->setOutFile(absoluteFilePath(m_fiOutFile));
        return;
    }

    if (QString::compare(newfiOutFile.suffix(), QString("fif")) != 0) {
        m_pWin->statusMsg("The output file extension must be \".fif\".");
        m_pWin->setOutFile(absoluteFilePath(m_fiOutFile));
        return;
    }

    m_fiOutFile = mne::path_from_utf8(newfiOutFile.absoluteFilePath().toStdString());
    m_pAnonymizer->setOutFile(absoluteFilePath(m_fiOutFile).toStdString());
}

//=============================================================================================================

void SettingsControllerGui::setupCommunication()
{
    //view to controller
    QObject::connect(m_pWin.data(), &MainWindow::fileInChanged,
                     this, &SettingsControllerGui::fileInChanged,
                     Qt::ConnectionType::DirectConnection);

    QObject::connect(m_pWin.data(), &MainWindow::fileOutChanged,
                     this, &SettingsControllerGui::fileOutChanged,
                     Qt::ConnectionType::DirectConnection);

    QObject::connect(m_pWin.data(), &MainWindow::readInputDataButtonClicked,
                     this, &SettingsControllerGui::readData);
    QObject::connect(m_pWin.data(), &MainWindow::saveOutputFileClicked,
                     this, &SettingsControllerGui::executeAnonymizer);

    //from view to model
    FiffAnonymizer* anonymizer = m_pAnonymizer.get();
    QObject::connect(m_pWin.data(), &MainWindow::bruteModeChanged, this, [anonymizer](bool b) {
        anonymizer->setBruteMode(b);
    });
    QObject::connect(m_pWin.data(), &MainWindow::measurementDateChanged, this, [anonymizer](const QDateTime& d) {
        anonymizer->setMeasurementDate(toYearMonthDay(d.date()));
    });
    QObject::connect(m_pWin.data(), &MainWindow::useMeasurementOffset, this, [anonymizer](bool b) {
        anonymizer->setUseMeasurementDateOffset(b);
    });
    QObject::connect(m_pWin.data(), &MainWindow::measurementDateOffsetChanged, this, [anonymizer](int o) {
        anonymizer->setMeasurementDateOffset(o);
    });
    QObject::connect(m_pWin.data(), &MainWindow::birthdayDateChanged, this, [anonymizer](const QDate& d) {
        anonymizer->setSubjectBirthday(toYearMonthDay(d));
    });
    QObject::connect(m_pWin.data(), &MainWindow::useBirthdayOffset, this, [anonymizer](bool b) {
        anonymizer->setUseSubjectBirthdayOffset(b);
    });
    QObject::connect(m_pWin.data(), &MainWindow::birthdayOffsetChanged, this, [anonymizer](int o) {
        anonymizer->setSubjectBirthdayOffset(o);
    });
    QObject::connect(m_pWin.data(), &MainWindow::subjectHisIdChanged, this, [anonymizer](const QString& text) {
        anonymizer->setSubjectHisId(text.toStdString());
    });

    //from model to view
    MainWindow* win = m_pWin.data();
    const auto text = [](const std::string& s) {
        return QString::fromStdString(s);
    };
    FiffAnonymizerCallbacks callbacks;
    callbacks.readingIdFileVersion = [win](double v) {
        win->setLineEditIdFileVersion(v);
    };
    callbacks.readingIdMeasurementDate = [win](std::int64_t d) {
        win->setLineEditIdMeasurementDate(localDateTime(d));
    };
    callbacks.readingIdMac = [win, text](const std::string& mac) {
        win->setLineEditIdMacAddress(text(mac));
    };

    callbacks.readingFileMeasurementDate = [win](std::int64_t d) {
        win->setLineEditFileMeasurementDate(localDateTime(d));
    };
    callbacks.readingFileComment = [win, text](const std::string& c) {
        win->setLineEditFileComment(text(c));
    };
    callbacks.readingFileExperimenter = [win, text](const std::string& e) {
        win->setLineEditFileExperimenter(text(e));
    };

    callbacks.readingSubjectId = [win](int i) {
        win->setLineEditSubjectId(i);
    };
    callbacks.readingSubjectFirstName = [win, text](const std::string& fn) {
        win->setLineEditSubjectFirstName(text(fn));
    };
    callbacks.readingSubjectMiddleName = [win, text](const std::string& mn) {
        win->setLineEditSubjectMiddleName(text(mn));
    };
    callbacks.readingSubjectLastName = [win, text](const std::string& ln) {
        win->setLineEditSubjectLastName(text(ln));
    };

    callbacks.readingSubjectBirthday = [win](std::int64_t julianDay) {
        win->setLineEditSubjectBirthday(QDate::fromJulianDay(julianDay));
    };
    callbacks.readingSubjectSex = [win](int s) {
        win->setComboBoxSubjectSex(s);
    };
    callbacks.readingSubjectHand = [win](int h) {
        win->setLineEditSubjectHand(h);
    };
    callbacks.readingSubjectWeight = [win](float w) {
        win->setLineEditSubjectWeight(w);
    };
    callbacks.readingSubjectHeight = [win](float h) {
        win->setLineEditSubjectHeight(h);
    };
    callbacks.readingSubjectComment = [win, text](const std::string& c) {
        win->setLineEditSubjectComment(text(c));
    };
    callbacks.readingSubjectHisId = [win, text](const std::string& his) {
        win->setLineEditSubjectHisId(text(his));
    };

    callbacks.readingProjectId = [win](int i) {
        win->setLineEditProjectId(i);
    };
    callbacks.readingProjectName = [win, text](const std::string& n) {
        win->setLineEditProjectName(text(n));
    };
    callbacks.readingProjectAim = [win, text](const std::string& a) {
        win->setLineEditProjectAim(text(a));
    };
    callbacks.readingProjectPersons = [win, text](const std::string& p) {
        win->setLineEditProjectPersons(text(p));
    };
    callbacks.readingProjectComment = [win, text](const std::string& c) {
        win->setLineEditProjectComment(text(c));
    };
    callbacks.mriDataFoundInFile = [win](bool b) {
        win->setLabelMriDataFoundVisible(b);
    };

    callbacks.readingMNEWorkingDir = [win, text](const std::string& s) {
        win->setLineEditMNEWorkingDir(text(s));
    };
    callbacks.readingMNECommandLine = [win, text](const std::string& s) {
        win->setLineEditMNECommand(text(s));
    };
    m_pAnonymizer->setCallbacks(std::move(callbacks));
}

//=============================================================================================================

void SettingsControllerGui::initializeOptionsState()
{
    if (m_pAnonymizer->isFileInSet()) {
        m_pWin->setInFile(absoluteFilePath(m_fiInFile));
    }
    if (m_pAnonymizer->isFileOutSet()) {
        m_pWin->setOutFile(absoluteFilePath(m_fiOutFile));
    }

    m_pWin->setCheckBoxBruteMode(m_pAnonymizer->getBruteMode());
    m_pWin->setMeasurementDate(QDateTime(toQDate(m_pAnonymizer->getMeasurementDate()), QTime(1, 1, 0)));
    m_pWin->setCheckBoxMeasurementDateOffset(m_pAnonymizer->getUseMeasurementDayOffset());
    m_pWin->setMeasurementDateOffset(m_pAnonymizer->getMeasurementDayOffset());
    m_pWin->setCheckBoxSubjectBirthdayOffset(m_pAnonymizer->getUseSubjectBirthdayOffset());
    m_pWin->setSubjectBirthday(toQDate(m_pAnonymizer->getSubjectBirthday()));
    m_pWin->setSubjectBirthdayOffset(m_pAnonymizer->getSubjectBirthdayOffset());
    if (m_bHisIdSpecified) {
        m_pWin->setSubjectHis(QString::fromStdString(m_pAnonymizer->getSubjectHisID()));
    }
}

//=============================================================================================================

int SettingsControllerGui::run()
{
    return 0;
}
