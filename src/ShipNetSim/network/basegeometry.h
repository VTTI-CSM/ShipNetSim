/**
 * @file basegeometry.h
 *
 * @brief Defines the BaseGeometry class. BaseGeometry is an abstract class
 * which is the base class for all geometric shapes in the system.
 *
 * The class provides a virtual destructor and a pure virtual method
 * toString() which should be overridden by all derived classes to
 * provide a string representation of the geometric shape.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef BASEGEOMETRY_H
#define BASEGEOMETRY_H

#include <QString>
#include <string>

/**
 * @class BaseGeometry
 *
 * @brief An abstract base class for all geometric shapes.
 *
 * BaseGeometry class provides the base functionality and interface
 * that all geometric shapes in the system must adhere to.
 * It has a virtual destructor and a pure virtual function
 * `toString()` which must be overridden by derived classes.
 */
class BaseGeometry
{

public:
    /**
     * @brief Virtual destructor.
     *
     * A virtual destructor ensures that the derived class's
     * destructor is called when a base class pointer is used to
     * delete an object.
     */
    virtual ~BaseGeometry() = default; // Virtual destructor

    /**
     * @brief Pure virtual function to return string representation.
     *
     * All derived classes must provide an implementation for this
     * function which returns a string representation of the
     * geometric shape.
     *
     * @return QString representing the geometric shape.
     */
    virtual QString toString() = 0;
};

#endif // BASEGEOMETRY_H
