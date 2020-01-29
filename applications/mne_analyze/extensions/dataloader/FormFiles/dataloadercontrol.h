//=============================================================================================================
/**
* @file     dataloadercontrol.h
* @author   Lorenz Esch <lorenzesch@hotmail.com>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019 Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the DataLoaderControl class.
*
*/

#ifndef DATALOADERCONTROL_H
#define DATALOADERCONTROL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class DataLoaderControl;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DATALOADEREXTENSION
//=============================================================================================================

namespace DATALOADEREXTENSION
{


//*************************************************************************************************************
//=============================================================================================================
// DATALOADEREXTENSION FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DataLoaderControl Extension Control
*
* @brief The DataLoaderControl class provides the extension control.
*/
class DataLoaderControl : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs the DataLoaderControl
    *
    * @param[in] parent     If parent is not NULL the QWidget becomes a child of QWidget inside parent.
    */
    explicit DataLoaderControl(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the DataLoaderControl.
    */
    virtual ~DataLoaderControl();

private:
    Ui::DataLoaderControl *ui;   /**< The user interface */

signals:
    //=========================================================================================================
    /**
    * Signal is emitted when user wants to load a Dipole Fit from a file
    */
    void loadFiffFile();
};
}

#endif // DATALOADERCONTROL_H
