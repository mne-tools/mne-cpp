#ifndef FILTER_H
#define FILTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtprocessing_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMetaType>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * @brief The Filter class
 */
class Filter
{
public:
    Filter();
};

//=============================================================================================================
/**
 * @brief The FilterParameter class
 */
class RTPROCESINGSHARED_EXPORT FilterParameter{
friend class FilterKernel;

public:
    explicit FilterParameter(QString);
    explicit FilterParameter(QString, QString);

    QString getName() const;

    friend bool operator == (const FilterParameter& in1, const FilterParameter& in2){
        return (in1.m_sName == in2.m_sName);
    }
protected:
    QString m_sName;            /**< Item name */
    QString m_sDescription;     /**< Item description */
};

}//namespace

#endif // FILTER_H
