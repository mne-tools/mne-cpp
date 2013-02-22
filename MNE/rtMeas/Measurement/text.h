//=============================================================================================================
/**
* @file		text.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the Text class.
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
// DEFINE NAMESPACE CSART
//=============================================================================================================

namespace CSART
{


//=============================================================================================================
/**
* DECLARE CLASS Text
*
* @brief The Text class is the base class of every Text Measurement.
*/

class RTMEASSHARED_EXPORT Text : public Measurement
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
