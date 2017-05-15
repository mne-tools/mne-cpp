//=============================================================================================================
/**
* @file     fheap.cpp
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
* @brief    Header File for fibonacci Heap
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================




#include "FHeap.h"
#include <iostream>

using namespace std;

// Function: FHeap
// The default constructor of the heap
FHeap::FHeap(){
    min = NULL;
}

//Function: ~FHeap
// The default destructor of the heap

FHeap::~FHeap(){
    FNode * tmp = min;
    FNode * tmp2;
    while ( tmp ){
        tmp2 = tmp->right;
        delete tmp;
        tmp = tmp2;
    }
}

/**
 * Function: operator=
 *
 * An overloaded assignment operator for FHeap
 * Performs a deep copy of the paramter heap
 *
 * @param f The heap to be assigned
 */
FHeap * FHeap::operator=(FHeap * f){
    if ( this != f ){
        min = new FNode();
        min->setValue(f->getMin()->getValue());
        min->right = min;
        min->left = min;
        FNode * iterator = f->getMin();
        while ( iterator->right != f->getMin() ){
            FNode * newNode = new FNode(iterator->getValue());
            insert(newNode);
        }
    }
    return this;
}

/**
 * Function: insert
 *
 * Creates then inserts a new node with a specific value
 *
 * @param v The value of the new node
 */
void FHeap::insert(int v){
    insert(new FNode(v));
}

/**
 * Function: insert
 *
 * Inserts a given node into the heap. This uses the meld
 * operation as prescribed by the general algorithm given in
 * class
 *
 * @param f A pointer to the new node
 */
void FHeap::insert(FNode * f){
    if ( min == NULL ){
        min = new FNode(f);
    } else {
        if ( find(f->getValue()) != NULL ){
            throw "No duplicate keys allowed.";
        }
        FHeap * H = new FHeap();
        H->insert(f);
        this->meldWith(H);
    }
}

/**
 * Function: find
 *
 * This recursively searches the nodes to find the one
 * with a particular value.
 * @param target The value of the desired node
 *
 * @return A pointer to the node or NULL if it is not present
 */
FNode * FHeap::find(int target){
    FNode * tmp = min;
    FNode * t;
    do {
        t = tmp->findInChildren(target);
        if ( t ){
            return t;
        }
        tmp = tmp->right;
    } while ( tmp != min );
    return NULL;
}

/**
 * Functio: getMin
 *
 * Returns the pointer to the minimum node on the FHeap
 *
 *
 * @return The min pointer
 */
FNode * FHeap::getMin(){
    return min;
}

/**
 * Function: deleteMin
 *
 * This performs the deleteMin operation
 *
 * @throws Exception if there is no min key
 *
 */
void FHeap::deleteMin(){
    if ( min == NULL ){
        throw "ERROR: no minimum node.";
    }
    // remove node with the min. key from the root cycle
    FNode * tmpMin;
    FNode * tmp, * tmp2;
    int roots = 0;

    if ( min->right == min && min->children == NULL ){
        delete min->unlink();
        min = NULL;
        return;
    } else if ( min->right == min ){
        tmpMin = NULL;
    } else {
        tmpMin = min->right;
    }

    min->unlink();

    // merge the root cycle with the cycle of children of this node
    tmp = min->children;
    while ( tmp ){
        if ( tmpMin == NULL ){
            tmpMin = new FNode(tmp);
        } else {
            tmpMin->appendNode(tmp);
        }
        tmp->parent = NULL;
        tmp = tmp->right;
        if ( tmp == min->children ){ break; }
    }

    // while two roots have the same degree, link them
    // Adjust the minimum key along the way
    tmp = tmpMin;
    do {
        roots++;
        tmp = tmp->right;
    } while ( tmp != tmpMin );

    min = tmpMin;

    //Create the array to keep track of the root node degrees
    vector<int> rootDegrees (roots+1);
    //int rootDegrees [roots+1];

    // Initialize the array to zeros
    for ( int i = 0; i < roots+1; i++ ){ rootDegrees[i] = 0; }

    // @TODO: make sure this runs enough times
    for ( int i = 0; i < roots+1; i++ ){
        tmpMin = min;
        tmp = tmpMin;
        do {
            if ( tmp->getValue() < min->getValue() ){
                min = tmp;
            }

            if ( tmp->getDegree() == i ){
                if ( rootDegrees[i] == 0 ){
                    rootDegrees[i] = tmp->getValue();
                    tmp = tmp->right;
                } else {
                    tmp2 = find(rootDegrees[i]);
                    if ( tmp2->getValue() <= tmp->getValue() ){
                        tmp2->addChild(tmp);
                        tmp2 = tmp->right;
                        tmp->unlink();
                        tmp = tmp2;
                    } else {
                        rootDegrees[i] = tmp->getValue();
                        tmp->addChild(tmp2);
                        if ( tmp2 == tmpMin ){
                            tmpMin = tmp2->left;
                        }
                        tmp2->unlink();
                        tmp = tmp->right;
                    }
                }
            } else {
                tmp = tmp->right;
            }
        } while ( tmp != tmpMin );
    }
}

/**
 * Function: deleteNode
 *
 * This function finds and removes the specified node. It
 * then merges the children of the deleted node with the
 * root cylce of the heap.
 *
 * @param target The value of the node to be deleted
 *
 */
void FHeap::deleteNode(int target){
    FNode * tmp = find(target);
    if ( tmp == min ){
        deleteMin();
    } else {
        FNode * tmpChildren = tmp->children;
        tmp->unlink();
        if ( tmpChildren ){
            do {
                min->appendNode(tmpChildren);
                tmpChildren = tmpChildren->right;
            } while ( tmpChildren != tmp->children );
        }
    }
}

/**
 * Function: decreaseKey
 *
 * Decreases the key of the specified node by the specified
 * amout (can increase if the delta is negative)
 * @param target The node to be changed
 * @param delta The amount to decrease
 *
 * @throws Exception If the target is not present
 */
void FHeap::decreaseKey(int target, int delta){
    // Unlink tree rooted at the target
    FNode * t = find(target);
    FNode * tmp;
    if ( t == NULL ){
        throw "ERROR: Cannot find node to modify.";
    } else if ( find(target - delta) != NULL ){
        cout << "found a duplicate: " << target + delta << endl;
        throw "ERROR: No duplicate keys allowed.";
    }

    if ( t == min ){
        min->setValue(min->getValue() - delta);
        return;
    }

    // Decrease the key by delta
    tmp = new FNode(t);
    tmp->setValue(t->getValue() - delta);

    // Add the target to the root cycle
    min->appendNode(tmp);
    if ( min->getValue() > tmp->getValue() ){
        min = find(tmp->getValue());
    }

    delete tmp;

    // Unlink and do cascading cuts
    t->unlink();
}

/**
 * Function: setKey
 *
 * Sets the key of the target to the specified value. Uses
 * the decrease key operation with the delta between old and
 * new values.
 *
 * @param target The node to be changed
 * @param newValue The new value of the target node
 *
 */
void FHeap::setKey(int target, int newValue){
    FNode * tmp = find(target);
    if ( tmp ){
        decreaseKey(target, tmp->getValue() - newValue);
    } else {
        throw "Error: couldn't find key.";
    }
}

/**
 * Function: mergeWith
 *
 * Merges the given heap into the current heap. Joins the
 * new heap root cycle into the current root cycle and adjusts
 * the min key accordingly.
 *
 * @author Joseph T. Anderson <jtanderson@ratiocaeli.com>
 * @since 2012-11-04
 * @version 2012-11-04
 *
 * @param f The heap to be merged
 */
void FHeap::mergeWith(FHeap * f){
    int t;
    if ( f->getMin() ){
        FNode * tmp = f->getMin();

        do {
            min->appendNode(tmp);
            tmp = tmp->right;
        } while ( tmp && tmp != f->getMin() );

        t = f->getMin()->getValue();
        if ( min->getValue() > t ){
            min = find(t);
        }
    }
}

/**
 * Function: print
 *
 * This makes a string representation of the heap where
 * parent-child relationships are shown on the child node
 * by a parenthetical key.
 *
 * @return The string of the heap or [Empty F-Heap] if there are no nodes
 */
std::string FHeap::print(){
    if ( min != NULL ){
        std::stringstream sstm;
        std::string result;
        // FNode * tmp = min;

        sstm << printCycle(min);

        result = sstm.str();
        return result;
    } else {
        return "[Empty F-Heap]";
    }
}

/**
 * Function: printCycle
 *
 * This prints a cycle of the heap and returns it. Used by
 * the print function.
 *
 * @param f The node to print the cycle of
 *
 * @return A string of the cycle
 *
 * @throws Exception if the node is NULL
 */
std::string FHeap::printCycle(FNode * f){
    std::stringstream result;
    std::string parent;
    FNode * tmp = f;
    int childrenCycles = 0;
    int childrenIndex = 0;

    if ( f ){

        do {
            if ( tmp->children ){
                childrenCycles++;
            }
            tmp = tmp->right;
        } while ( tmp != f);

        vector <FNode *> next (childrenCycles);
        //FNode * next [childrenCycles];

        if ( childrenCycles > 0 ){
            for ( int i = 0; i < childrenCycles; i++ ){ next[i] = NULL; }
        }

        do {
            result << tmp->getValue() << (tmp->isMarked() ? "*" : "" );
            if ( tmp->parent ){
                result << "(" << tmp->parent->getValue() << ")";
            }
            result << " ";

            if ( childrenCycles > 0 && tmp->children ){
                next[childrenIndex] = tmp->children;
                childrenIndex++;
            }

            tmp = tmp->right;
        } while ( tmp != f);

        if ( childrenCycles > 0 ) {
            for ( int i = 0; i < childrenCycles; i++ ){
                if ( next[i] ){
                    if ( i == 0 ){
                        result << endl;
                    }
                    result << printCycle(next[i]);
                }
            }
        }

        if ( childrenCycles > 0 ){
            result << endl;
        }

        return result.str();
    } else {
        throw "ERROR: Cannot print a cycle with a NULL pointer.";
    }
}
