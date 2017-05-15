//=============================================================================================================
/**
* @file     fheap.h
* @author   Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Mai, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Sugandha Sachdeva and Matti Hamalainen. All rights reserved.
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
* @brief     FHeap class declaration
*
*/


#ifndef FHEAP_H
#define FHEAP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "FNode.h"
#include <cstddef>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <vector>

/**
* Description of what this class is intended to do (in detail).
*
* @brief Header File for fibonacci Heap
*/


class FHeap{

private:
    FNode * min;
    std::string printCycle(FNode * f);

public:
    FHeap();
    ~FHeap();

    FHeap * operator=(FHeap * f);

    void insert(qint32 v);
    void insert(FNode * f);
    FNode * find(int target);
    FNode * getMin();
    void deleteMin();
    void deleteNode(int target);
    void decreaseKey(int target, int delta);
    void setKey(int target, int newValue);
    void meldWith(FHeap * f);
    std::string print();
};

#endif // FHEAP_H
