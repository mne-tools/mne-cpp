/* Coverity Scan model
 *
 * This is the source code for our Coverity user model file. The 
 * purpose of user models is to increase scanning accuracy by explaining 
 * code Coverity can't see (out of tree libraries) or doesn't 
 * sufficiently understand.  Better accuracy means both fewer false 
 * positives and more true defects.  Memory leaks in particular. 
 * 
 *
 * - A model file can't import any header files.
 * - Therefore only some built-in primitives like int, char and void are
 *   available but not wchar_t, NULL etc.
 * - Modeling doesn't need full structs and typedefs. Rudimentary structs
 *   and similar types are sufficient.
 * - An uninitialized local variable signifies that the variable could be 
 *   any value. 
 * - An uninitialized local pointer is not an error. It signifies that the
 *   variable could be either NULL or have some data.
 *
 * Coverity Scan doesn't pick up modifications automatically. The model file
 * must be uploaded by an admin in the analysis settings of
 * http://scan.coverity.com/projects/200
 */

/* empty modeling file for now */