#ifndef CL_TYPES_H
#define CL_TYPES_H

#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

typedef unsigned int uint_t;    // Shorthand notation for this system's default unsigned integer
typedef uint64_t     uintcw_t;  // Default integer size chosen cluster-wide
typedef int64_t      intcw_t;  // Default integer size chosen cluster-wide
typedef void* (*Func)(void*);

#endif
