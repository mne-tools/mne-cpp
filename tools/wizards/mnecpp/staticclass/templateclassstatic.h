//=============================================================================================================
/**
 * @file     %{HdrFileName}
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
 * @brief     %{CN} class declaration.
 *
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
