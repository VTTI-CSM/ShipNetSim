/**
 * @file gdal_compat.h
 * @brief GDAL backward compatibility utilities.
 *
 * This file provides compatibility macros for GDAL API changes across versions.
 * GDAL 3.x changed the C API to use opaque handles, making some C API functions
 * incompatible with C++ object pointers.
 *
 * Author: Ahmed Aredah
 * Date: 2024
 */

#ifndef GDAL_COMPAT_H
#define GDAL_COMPAT_H

#include <gdal_version.h>
#include <ogr_spatialref.h>

// Backward-compatible helper for destroying OGRCoordinateTransformation objects.
// GDAL 3.x changed the C API to use opaque handles, making OCTDestroyCoordinateTransformation
// incompatible with C++ OGRCoordinateTransformation* pointers.
// Use DestroyCT for GDAL >= 2.3, or delete for older versions.
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2, 3, 0)
    #define DESTROY_COORD_TRANSFORM(ct) OGRCoordinateTransformation::DestroyCT(ct)
#else
    #define DESTROY_COORD_TRANSFORM(ct) delete (ct)
#endif

#endif // GDAL_COMPAT_H
