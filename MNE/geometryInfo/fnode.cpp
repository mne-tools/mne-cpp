/**
 * File: FNode.cpp
 *
 * This files contains the implementations of the Nodes
 * to be used in the Fibonacci-Heap
 *
 */

#include "FNode.h"

//#define DEBUG

#ifdef DEBUG
#include <iostream>
using namespace std;
#endif

/**
 * Function: FNode
 *
 * This is the default constructor for the FNode data
 * structure. It creates a node with no value and no siblings,
 * parent, or children.
 *
 *
 */

FNode::FNode(){
        right = this;
        left = this;
        parent = NULL;
        children = NULL;
        marked = false;
        degree = 0;
}

/**
 * Function: FNode
 *
 * A constructor that takes an integer and sets the node
 * value to that parameter.
 *
 * @param v The value of the node
 */
FNode::FNode(int v){
        value = v;
        right = this;
        left = this;
        parent = NULL;
        children = NULL;
        marked = false;
        degree = 0;
}

/**
 * Function: FNode
 *
 * A deep-copy constructor
 *
 * @param f A pointer to an existing FNode
 */
FNode::FNode(FNode * f){
        if ( f == NULL ){
                throw "ERROR: tried to operate on a NULL node pointer.";
        }

        #ifdef DEBUG
        cout << "Making a copy of " << f->getValue() << endl;
        #endif

        value = f->getValue();
        left = this;
        right = this;
        marked = f->isMarked();
        parent = NULL;
        degree = 0;
        children = NULL;
        if ( f->children != NULL ){
                FNode * current = f->children;
                do {
                        #ifdef DEBUG
                                cout << "Copying " << current->getValue() << " to " << getValue() << "..." << endl;
                        #endif
                        this->addChild(current);
                        #ifdef DEBUG
                                cout << "Added " << current->getValue() << " to " << getValue() << " successfully!" << endl;
                        #endif
                        current = current->right;
                } while ( current != f->children );
        } else {
                children = NULL;
        }
}

/**
 * Function: getValue
 *
 * Returns the integer value of the function
 *
 * @return The value of the node
 * @throws Exception if the node has not been properly allocated
 */

int FNode::getValue(){
        if ( this != NULL ){
                return value;
        } else {
                throw "ERROR. Tried to get the value of a null node.";
        }
}

/**
 * Function: unlink
 *
 * This removes all connections of the node. All children pointers
 * are removed as well as the parent pointer and siblings.
 * Also performs cascading cuts if necessary.
 *
 * @return A pointer to the unlinked node
 *
 */
FNode * FNode::unlink(){
        FNode * tmp, * tmp2;
        if( this->right == this ){
                if ( this->parent != NULL ){
                        this->parent->children = NULL;
                        if ( this->parent->isMarked() ){
                                tmp = this->parent;
                                tmp2 = tmp;
                                while ( tmp2->parent != NULL ){
                                        tmp2 = tmp2->parent;
                                }
                                if ( tmp != tmp2 ){
                                        tmp->unMark();
                                        tmp2->appendNode(tmp);
                                        tmp->unlink();
                                }
                        } else {
                                this->parent->mark();
                        }
                        this->parent = NULL;
                }
                return this;
        } else {
                this->right->left = left;
                this->left->right = right;
                if ( this->parent && this->parent->children == this ){
                        this->parent->children = this->right;
                }
                if ( this->parent && this->parent->isMarked() ){
                        tmp = this->parent;
                        tmp2 = tmp;
                        while ( tmp2->parent ){
                                tmp2 = tmp2->parent;
                        }
                        if ( tmp != tmp2 ){
                                tmp->unMark();
                                tmp2->appendNode(tmp);
                        }
                        tmp->unlink();
                } else if ( this->parent ) {
                        this->parent->mark();
                }
                this->right = this;
                this->left = this;
                this->parent = NULL;
        }
        return this;
}

/**
 * Function: appendNode
 *
 * This adds a sibling (A deep copy of the paramter node) to the right of the node
 *
 * @param f The node to be appended
 * @return A pointer to the node
 * @throws Exception if the parameter is NULL
 */
FNode * FNode::appendNode(FNode * f){
        if ( f == NULL ){
                throw "ERROR: tried to operate on a NULL node pointer.";
        }

        #ifdef DEBUG
        cout << "Appending: " << f->getValue() << " to node " << this->value << endl;
        #endif

        FNode * n = new FNode (f);

        n->right = right;
        n->left = this;
        if ( right != this ){
                right->left = n;
        } else {
                left = n;
        }
        right = n;
        return n;
}

/**
 * Function: appendNode
 *
 * This adds a sibling (the same node that is passed).
 *
 * @param f The node to be appended.
 * @throws Exception if the parameter is NULL
 */
void FNode::appendSameNode(FNode * f){
        if ( f == NULL ){
                throw "ERROR: tried to operate on a NULL node pointer.";
        }

        f->right = right;
        f->left = this;
        if ( right != this ){
                right->left = f;
        } else {
                left = f;
        }
        right = f;
}

/**
 * Function: prependNode
 *
 * This adds a node to the left of the calling node
 *
 * @param f The node to be copied and prepended
 * @return A pointer to the node
 * @throws Exception if the parameter node is NULL
 */

FNode * FNode::prependNode(FNode * f){
        if ( f == NULL ){
                throw "ERROR: tried to operate on a NULL node pointer.";
        }

        FNode * n = new FNode(f);
        n->right = right;
        n->left = this;
        if ( right != this ){
                right->left = n;
        } else {
                left = n;
        }
        right = n;
        return n;
}

/**
 * Function: addChild
 *
 * This function adds a child as appropriate. It checks to see
 * if there already children or not then behaves accordingly.
 *
 * @param f A pointer to the new child
 * @return A pointer to the new node
 * @throws If the new child is NULL
 */
FNode * FNode::addChild(FNode * f){
        if ( f == NULL ){
                throw "ERROR: tried to operate on a NULL node pointer.";
        }

        #ifdef DEBUG
        cout << "Adding child: " << f->getValue() << " to node " << value << endl;
        #endif

        FNode * n = new FNode(f);

        if ( children == NULL ){

                #ifdef DEBUG
                cout << "Node " << value << " currently has no children." << endl;
                cout << "Starting children of " << value << " with " << f->getValue() << endl;
                #endif

                this->children = new FNode(f);
                this->children->parent = this;
                this->children->unMark();
        } else {
                this->children->appendNode(n)->parent = this;
        }
        degree++;

        return n;
}

/**
 * Function: addChild
 *
 * This adds a new node with the specified value.
 *
 * @param i The value of the new node
 * @return A pointer to the new node
 *
 */

FNode * FNode::addChild(int i){
        return addChild(new FNode(i));
}

/**
 * Function: findInChildren
 *
 * This function recursively searches a node and its children
 * for the node with a particular value.
 *
 * @param target The integer value of the target node
 * @return A pointer to the target node if it exists or NULL if not
 */

FNode * FNode::findInChildren(int target){
        if ( target == value ){
                return this;
        } else if ( children ){
                FNode * tmp = children;
                FNode * test;
                do {
                        test = tmp->findInChildren(target);
                        if ( test != NULL ){
                                return test;
                        } else {
                                tmp = tmp->right;
                        }
                } while ( tmp != children);
        }
        return NULL;
}
