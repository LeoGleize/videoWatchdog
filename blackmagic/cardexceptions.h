/*
 * exceptions.h
 *
 *  Created on: Oct 20, 2015
 *      Author: fcaldas
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <iostream>
#include <exception>

enum exceptionType{NO_INPUT_EXCEPTION};

class CardException: public std::exception
{
public:
    /** Constructor (C strings).
     *  @param message C-style string error message.
     *                 The string contents are copied upon construction.
     *                 Hence, responsibility for deleting the \c char* lies
     *                 with the caller.
     */
    explicit CardException(const char* message);

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit CardException(const std::string& message);


    explicit CardException(const std::string& message, exceptionType eType);

    virtual const char* what() const throw();
    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~CardException() throw ();

    exceptionType getExceptionType() const;
    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a \c const \c char*. The underlying memory
     *          is in posession of the \c Exception object. Callers \a must
     *          not attempt to free the memory.
     */
protected:
    /** Error message.
     */
    std::string msg_;
    exceptionType exType;
};

#endif /* EXCEPTIONS_H_ */
