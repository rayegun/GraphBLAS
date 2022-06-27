//------------------------------------------------------------------------------
// GB_xalloc_memory: allocate an array for n entries, or 1 if iso
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2022, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"

// TODO: This needs to be type based not type_size based most likely.
void *GB_xalloc_memory      // return the newly-allocated space
(
    // input
    bool use_calloc,        // if true, use calloc
    bool iso,               // if true, only allocate a single entry
    int64_t n,              // # of entries to allocate if non iso
    size_t type_size,       // size of each entry
    // output
    size_t *size,           // resulting size
    GB_Context Context
)
{
    void *p ;
    if (iso)
    { 
        // always calloc the iso entry
        p = GB_MALLOC (GB_void, type_size, GrB_UINT8, size) ;  // x:OK
    }
    else if (use_calloc)
    { 
        p = GB_MALLOC (GB_void, n * type_size, GrB_UINT8, size) ; // x:OK
    }
    else
    { 
        p = GB_MALLOC (GB_void, n * type_size, GrB_UINT8, size) ; // x:OK
    }
    return (p) ;
}