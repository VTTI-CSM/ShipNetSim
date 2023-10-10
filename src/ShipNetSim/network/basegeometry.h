#ifndef BASEGEOMETRY_H
#define BASEGEOMETRY_H

#include <QString>
#include <string>

class BaseGeometry
{

public:
    virtual ~BaseGeometry() = default; // Virtual destructor

    virtual QString toString() = 0;
};

#endif // BASEGEOMETRY_H
