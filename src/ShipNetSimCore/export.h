#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #if defined(SHIPNETSIM_LIBRARY)
    #define SHIPNETSIM_EXPORT __declspec(dllexport)
  #else
    #define SHIPNETSIM_EXPORT __declspec(dllimport)
  #endif
#else
  #define SHIPNETSIM_EXPORT
#endif
