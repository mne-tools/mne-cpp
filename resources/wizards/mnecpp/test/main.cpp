//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) %{Year} MNE-CPP Authors
 *
 * @file     %{CppFileName}
 * @author   %{author} <%{eMail}>
 * @since    0.1.0
 * @date     %{Month} %{Year}
 * @brief    %{testDescription}.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QtTest>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
/**
 * DECLARE CLASS %{TestClassName}
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class %{TestClassName}: public QObject
{
    Q_OBJECT

public:
    %{TestClassName}();

private slots:
    void initTestCase();
    void compareValue();
    // add other compareFunctions here
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    double dEpsilon;
    Eigen::MatrixXd mFirstInData; 
    Eigen::MatrixXd mSecondInData;
    
};

//=============================================================================================================

%{TestClassName}::%{TestClassName}()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void %{TestClassName}::initTestCase()
{
	// test your function here
}

//=============================================================================================================

void %{TestClassName}::compareValue()
{
    // compare your data here, think about usefull metrics
    Eigen::MatrixXd mDataDiff = mFirstInData - mSecondInData;
    QVERIFY( mDataDiff.sum() < dEpsilon );
}

//=============================================================================================================

void %{TestClassName}::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(%{TestClassName})
#include "%{ProjectName}.moc"

