//=============================================================================================================
/**
* @file     text.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the Text class.
*
*/

#ifndef TEXT_H
#define TEXT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurement.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{


//=============================================================================================================
/**
* DECLARE CLASS Text
*
* @brief The Text class is the base class of every Text Measurement.
*/

class XMEASSHARED_EXPORT Text : public Measurement
{
public:

    //=========================================================================================================
    /**
    * Constructs a Text.
    */
    Text();
    //=========================================================================================================
    /**
    * Destroys the Text.
    */
    virtual ~Text();

    //=========================================================================================================
    /**
    * Sets a new text and notifies its observers.
    *
    * @param [in] text which should be set.
    */
    void setText(const QString& text);
    //=========================================================================================================
    /**
    * Returns the current text.
    *
    * @return the current text.
    */
    inline const QString& getText() const;

    //=========================================================================================================
    /**
    * Sets a value.
    * Not used. Method inherited by Measurement.
    *
    * @param [in] value which should be set.
    */
    virtual void setValue(double) {;};
    //=========================================================================================================
    /**
    * Returns the current value.
    * Not used. Method inherited by Subject.
    *
    * @return always -1, because values are not used.
    */
    virtual double getValue() const {return -1;};

private:
    QString m_Text;		/**< Holds the current text.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//*************************************************************************************************************

inline const QString& Text::getText() const
{
    return m_Text;
}

} // NAMESPACE

#endif // TEXT_H
