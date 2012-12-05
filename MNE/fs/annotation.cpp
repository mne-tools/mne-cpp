//=============================================================================================================
/**
* @file     annotation.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Bruce Fischl
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Annotation::Annotation()
{
}


//*************************************************************************************************************

void Annotation::read_annotation(QString& t_sFileName)
{
    QFile t_File(t_sFileName);

//    fp = fopen(filename, 'r', 'b');

//    verbosity = 1;
//    if length(varargin)
//        verbosity       = varargin{1};
//    end;

//    if(fp < 0)
//       if verbosity, disp('Annotation file cannot be opened'); end;
//       return;
//    end

//    A = fread(fp, 1, 'int');

//    tmp = fread(fp, 2*A, 'int');
//    vertices = tmp(1:2:end);
//    label = tmp(2:2:end);

//    bool = fread(fp, 1, 'int');
//    if(isempty(bool)) %means no colortable
//       if verbosity, disp('No Colortable found.'); end;
//       colortable = struct([]);
//       fclose(fp);
//       return;
//    end

//    if(bool)

//        %Read colortable
//        numEntries = fread(fp, 1, 'int');

//        if(numEntries > 0)

//            if verbosity, disp(['Reading from Original Version']); end;
//            colortable.numEntries = numEntries;
//            len = fread(fp, 1, 'int');
//            colortable.orig_tab = fread(fp, len, '*char')';
//            colortable.orig_tab = colortable.orig_tab(1:end-1);

//            colortable.struct_names = cell(numEntries,1);
//            colortable.table = zeros(numEntries,5);
//            for i = 1:numEntries
//                len = fread(fp, 1, 'int');
//                colortable.struct_names{i} = fread(fp, len, '*char')';
//                colortable.struct_names{i} = colortable.struct_names{i}(1:end-1);
//                colortable.table(i,1) = fread(fp, 1, 'int');
//                colortable.table(i,2) = fread(fp, 1, 'int');
//                colortable.table(i,3) = fread(fp, 1, 'int');
//                colortable.table(i,4) = fread(fp, 1, 'int');
//                colortable.table(i,5) = colortable.table(i,1) + colortable.table(i,2)*2^8 + colortable.table(i,3)*2^16 + colortable.table(i,4)*2^24;
//            end
//            if verbosity
//                disp(['colortable with ' num2str(colortable.numEntries) ' entries read (originally ' colortable.orig_tab ')']);
//            end
//        else
//            version = -numEntries;
//            if verbosity
//              if(version~=2)
//                disp(['Error! Does not handle version ' num2str(version)]);
//              else
//                disp(['Reading from version ' num2str(version)]);
//              end
//            end
//            numEntries = fread(fp, 1, 'int');
//            colortable.numEntries = numEntries;
//            len = fread(fp, 1, 'int');
//            colortable.orig_tab = fread(fp, len, '*char')';
//            colortable.orig_tab = colortable.orig_tab(1:end-1);

//            colortable.struct_names = cell(numEntries,1);
//            colortable.table = zeros(numEntries,5);

//            numEntriesToRead = fread(fp, 1, 'int');
//            for i = 1:numEntriesToRead
//                structure = fread(fp, 1, 'int')+1;
//                if (structure < 0)
//                  if verbosity, disp(['Error! Read entry, index ' num2str(structure)]); end;
//                end
//                if(~isempty(colortable.struct_names{structure}))
//                  if verbosity, disp(['Error! Duplicate Structure ' num2str(structure)]); end;
//                end
//                len = fread(fp, 1, 'int');
//                colortable.struct_names{structure} = fread(fp, len, '*char')';
//                colortable.struct_names{structure} = colortable.struct_names{structure}(1:end-1);
//                colortable.table(structure,1) = fread(fp, 1, 'int');
//                colortable.table(structure,2) = fread(fp, 1, 'int');
//                colortable.table(structure,3) = fread(fp, 1, 'int');
//                colortable.table(structure,4) = fread(fp, 1, 'int');
//                colortable.table(structure,5) = colortable.table(structure,1) + colortable.table(structure,2)*2^8 + colortable.table(structure,3)*2^16 + colortable.table(structure,4)*2^24;
//            end
//            if verbosity
//              disp(['colortable with ' num2str(colortable.numEntries) ' entries read (originally ' colortable.orig_tab ')']);
//            end
//        end
//    else
//        if verbosity
//            disp('Error! Should not be expecting bool = 0');
//        end;
//    end

//    fclose(fp);

}

