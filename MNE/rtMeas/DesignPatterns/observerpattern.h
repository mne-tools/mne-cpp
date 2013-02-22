//=============================================================================================================
/**
* @file		observerpattern.h
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
* @brief	Contains declarations of the observer design pattern: Subject class and IObserver interface.
*
*/

#ifndef OBSERVERPATTERN_H
#define OBSERVERPATTERN_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QSet>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Subject;


//=============================================================================================================
/**
* DECLARE INTERFACE OBSERVER
*
* @brief The IObserver interface provides the base class of every observer of the observer design pattern.
*/
class IObserver
{
public:

    //=========================================================================================================
    /**
    * Destroys the IObserver.
    */
    virtual ~IObserver() {};
    //=========================================================================================================
    /**
    * Updates the IObserver.
    *
    * @param [in] pSubject pointer to the subject where observer is attached to.
    */
    virtual void update(Subject* pSubject) = 0;
};


//=========================================================================================================
/**
* DECLARE BASE CLASS SUBJECT
*
* @brief The Subject class provides the base class of every subject of the observer design pattern.
*/
class RTMEASSHARED_EXPORT Subject
{
public:

    //=========================================================================================================
    /**
    * Destroys the Subject.
    */
    virtual ~Subject();

    //=========================================================================================================
    /**
    * Attaches an observer to the subject.
    *
    * @param [in] pObserver pointer to the observer which should be attached to the subject.
    */
    void attach(IObserver* pObserver);
    //=========================================================================================================
    /**
    * Detaches an observer of the subject.
    *
    * @param [in] pObserver pointer to the observer which should be detached of the subject.
    */
    void detach(IObserver* pObserver);


    //=========================================================================================================
    /**
    * Notifies all attached servers by calling there update method. Is used when subject has updates to provide.
    * This method is enabled when nothifiedEnabled is true.
    */
    void notify();

    //=========================================================================================================
    /**
    * Holds the status whether notification is enabled.
    * This is used to block notify() to make the observer pattern thread safe. It's working like a mutex. The different is that data aren't stored. -> it's okay when values are queued in their own buffer.
    */
    static bool notifyEnabled;

    //=========================================================================================================
    /**
    * Returns number of attached observers.
    * ToDo only for debug purpose -> could be removed
    *
    * @return the number of attached observers.
    */
    int observerNumDebug(){return m_Observers.size();};


protected:
    //=========================================================================================================
    /**
    * Constructs a Subject.
    */
    Subject() {};

private:
    typedef QSet<IObserver*>    t_Observers;	/**< Defines a new IObserver set type. */
    t_Observers                 m_Observers;	/**< Holds the attached observers.*/
};

#endif // OBSERVERPATTERN_H
