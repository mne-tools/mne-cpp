//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) %{Year} MNE-CPP Authors
 *
 * @file     %{HdrFileName}
 * @author   %{author} <%{eMail}>
 * @since    0.1.0
 * @date     %{Month} %{Year}
 * @brief    %{CN} class declaration.
 */

#ifndef %{GUARD}
#define %{GUARD}

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

%{JS: QtSupport.qtIncludes([ 'QtCore/QSharedPointer' ,
						     ( '%{IncludeQObject}' )          ? 'QtCore/%{IncludeQObject}'     : '',
                             ( '%{IncludeQWidget}' )          ? 'QtGui/%{IncludeQWidget}'      : '',
                             ( '%{IncludeQMainWindow}' )      ? 'QtGui/%{IncludeQMainWindow}'  : '' ],
						   [ 'QtCore/QSharedPointer',
						     ( '%{IncludeQObject}' )          ? 'QtCore/%{IncludeQObject}'     : '',
                             ( '%{IncludeQWidget}' )          ? 'QtGui/%{IncludeQWidget}'      : '',
                             ( '%{IncludeQMainWindow}' )      ? 'QtGui/%{IncludeQMainWindow}'  : ''])}\

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE %{JS: Cpp.namespaces('%{Class}')[0]}
//=============================================================================================================
%{JS: Cpp.openNamespaces('%{Class}')}

//=============================================================================================================
// %{JS: Cpp.namespaces('%{Class}')[0]} FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */

@if '%{Base}'
class %{CN} : public %{Base}
@else
class %{CN}
@endif
{
@if %{isQObject}
     Q_OBJECT
@endif

public:
@if '%{Base}' === 'QObject'
    typedef QSharedPointer<%{CN}> SPtr;            /**< Shared pointer type for %{CN}. */
    typedef QSharedPointer<const %{CN}> ConstSPtr; /**< Const shared pointer type for %{CN}. */

    //=========================================================================================================
    /**
    * Constructs a %{CN} object.
    */
    explicit %{CN}(QObject *parent = 0);
@elsif '%{Base}' === 'QWidget' || '%{Base}' === 'QMainWindow'
    typedef QSharedPointer<%{CN}> SPtr;            /**< Shared pointer type for %{CN}. */
    typedef QSharedPointer<const %{CN}> ConstSPtr; /**< Const shared pointer type for %{CN}. */

    //=========================================================================================================
    /**
    * Constructs a %{CN} object.
    */
    explicit %{CN}(QWidget *parent = 0);
@else
    //=========================================================================================================
    /**
    * Delete constructor since this is a static class.
    */
    %{CN}() = delete;
@endif

protected:
	
private:
	
@if %{isQObject}
signals:
@endif
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

%{JS: Cpp.closeNamespaces('%{Class}')}
#endif // %{GUARD}
ass}')}
#endif // %{GUARD}
