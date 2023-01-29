

//------------------------------------------------------------------------------
// GB_red:  hard-coded functions for reductions
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// If this file is in the Generated2/ folder, do not edit it
// (it is auto-generated from Generator/*).

#include "GB.h"
#ifndef GBCUDA_DEV
#include "GB_control.h" 
#include "GB_red__include.h"

// The reduction is defined by the following types and operators:

// Reduce to scalar:   GB (_red__times_fp32)

// A type:   float
// Z type:   float

// Reduce:   z *= aij
// Identity: 1
// Terminal: ;

#define GB_A_TYPENAME \
    float

#define GB_Z_TYPENAME \
    float

// declare a scalar and set it equal to the monoid identity value

    #define GB_DECLARE_MONOID_IDENTITY(z)           \
        float z = 1

// reduction operator:

    // z += y, no typecast
    #define GB_UPDATE(z,y) \
        z *= y

    // s += (ztype) Ax [p], no typecast here however
    #define GB_GETA_AND_UPDATE(s,Ax,p)              \
        GB_UPDATE (s, Ax [p])

// break the loop if terminal condition reached

    #define GB_MONOID_IS_TERMINAL                   \
        0

    #define GB_TERMINAL_CONDITION(z,zterminal)      \
        (false)

    #define GB_IF_TERMINAL_BREAK(z,zterminal)       \
        ;

// panel size for built-in operators

    #define GB_PANEL                                \
        64

// special case for the ANY monoid

    #define GB_IS_ANY_MONOID                        \
        0

// disable this operator and use the generic case if these conditions hold
#define GB_DISABLE \
    (GxB_NO_TIMES || GxB_NO_FP32 || GxB_NO_TIMES_FP32)

//------------------------------------------------------------------------------
// reduce to a non-iso matrix to scalar, for monoids only
//------------------------------------------------------------------------------

GrB_Info GB (_red__times_fp32)
(
    float *result,
    const GrB_Matrix A,
    GB_void *restrict W_space,
    bool *restrict F,
    int ntasks,
    int nthreads
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    float z = (*result) ;
    float *restrict W = (float *) W_space ;
    if (A->nzombies > 0 || GB_IS_BITMAP (A))
    {
        #include "GB_reduce_to_scalar_template.c"
    }
    else
    {
        #include "GB_reduce_panel.c"
    }
    (*result) = z ;
    return (GrB_SUCCESS) ;
    #endif
}

#endif

