//=============================================================================================================
/**
 * @file     %{CppFileName}
 * @author   %{author} <%{eMail}>
 * @since    0.1.0
 * @date     %{Month}, %{Year}
 *
 * @section  LICENSE
 *
 * Copyright (C) %{Year}, %{author}. All rights reserved.
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
 * @brief     %{testDescription}.
 *
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

