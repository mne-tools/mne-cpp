//=============================================================================================================
/**
* @file     fmain.cpp
* @author   Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Mai, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017,Sugandha Sachdeva and Matti Hamalainen. All rights reserved.
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
* The driver file for the Fibonacci-heap project. This handles
* processing user input throgh the command file as specified by the
* project specifications.
*
* @brief    Header File for fibonacci Heap
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fmain.h"

// #define DEBUG

int main(int argc, char *argv[]){
        try{
                if ( argc == 2 ){
                        // Open the file, read it, and do stuff
                        ifstream f(argv[1]);
                        if ( ! f.fail() ){
                                FHeap * heap = new FHeap();
                                string line;
                                int i, arg1, arg2;
                                while ( ! f.eof() ){
                                        getline(f,line);
                                        switch ( line[0] ){
                                                case 'i':
                                                        i = line.find(" ");
                                                        arg1 = atoi(line.substr(i+1).c_str());
                                                        #ifdef DEBUG
                                                                cout << "Inserting: " << arg1 << endl;
                                                        #endif
                                                        heap->insert(arg1);
                                                        break;
                                                case 'm':
                                                        #ifdef DEBUG
                                                                cout << "Printing Min: " << ( heap->getMin() != NULL ? heap->getMin()->getValue() : 0 ) << endl;
                                                        #endif
                                                        cout << heap->getMin()->getValue() << endl;
                                                        break;
                                                case 'd':
                                                        #ifdef DEBUG
                                                                cout << "Deleting Min: " << ( heap->getMin() != NULL ? heap->getMin()->getValue() : 0 ) << endl;
                                                        #endif
                                                        heap->deleteMin();
                                                        break;
                                                case 'x':
                                                        i = line.find(" ");
                                                        arg1 = atoi(line.substr(i+1).c_str());
                                                        #ifdef DEBUG
                                                                cout << "Deleting Node: " << arg1 << endl;
                                                        #endif
                                                        heap->deleteNode(arg1);
                                                        break;
                                                case 'c':
                                                        i = line.find(" ");
                                                        arg1 = atoi(line.substr(i+1).c_str());
                                                        arg2 = atoi(line.substr(line.find(" ", i+1)).c_str());
                                                        #ifdef DEBUG
                                                                cout << "Setting key: " << arg1 << " to: " << arg2 << endl;
                                                        #endif
                                                        heap->setKey(arg1, arg2);
                                                        break;
                                                default:
                                                        break;
                                        }
                                        #ifdef DEBUG
                                                cout << "Current tree: " << endl << heap->print() << endl;
                                        #endif
                                }
                                f.close();
                        } else {
                                cerr << "Error: the specified file could not be opened for reading." << endl;
                        }
                } else {
                        cerr << "Error: no file specified." << endl;
                        cerr << "Usage: fheap file" << endl;
                        exit(2);
                }
        } catch ( const char* message ){
                #ifdef DEBUG
                        cerr << message << endl;
                #else
                        cout << "error" << endl;
                #endif
                return 1;
        }
        return 0;
}
