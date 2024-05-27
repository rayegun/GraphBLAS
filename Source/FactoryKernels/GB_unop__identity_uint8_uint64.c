//------------------------------------------------------------------------------
// GB_unop:  hard-coded functions for each built-in unary operator
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"
#include "builtin/factory/GB_control.h"
#include "FactoryKernels/GB_unop__include.h"

// unary operator: z = f(x)
#define GB_UNARYOP(z,x) z = (uint8_t) x
#define GB_Z_TYPE uint8_t
#define GB_X_TYPE uint64_t

// A matrix
#define GB_A_TYPE uint64_t
#define GB_DECLAREA(aij) uint64_t aij
#define GB_GETA(aij,Ax,pA,A_iso) aij = Ax [pA]

// C matrix
#define GB_C_TYPE uint8_t

// cij = op (aij)
#define GB_APPLY_OP(pC,pA)          \
{                                   \
    /* aij = Ax [pA] */             \
    GB_DECLAREA (aij) ;             \
    GB_GETA (aij, Ax, pA, false) ;  \
    /* Cx [pC] = unop (aij) */      \
    GB_UNARYOP (Cx [pC], aij) ;     \
}

// disable this operator and use the generic case if these conditions hold
#if (defined(GxB_NO_IDENTITY) || defined(GxB_NO_UINT8) || defined(GxB_NO_UINT64))
#define GB_DISABLE 1
#else
#define GB_DISABLE 0
#endif

#include "apply/include/GB_apply_shared_definitions.h"

//------------------------------------------------------------------------------
// Cx = op (cast (Ax)): apply a unary operator
//------------------------------------------------------------------------------

GrB_Info GB (_unop_apply__identity_uint8_uint64)
(
    GB_void *Cx_out,            // Cx and Ax may be aliased
    const GB_void *Ax_in,       // A is always non-iso for this kernel
    const int8_t *restrict Ab,  // A->b if A is bitmap
    int64_t anz,
    int nthreads
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    #include "apply/template/GB_apply_unop_template.c"
    return (GrB_SUCCESS) ;
    #endif
}

//------------------------------------------------------------------------------
// C = op (cast (A')): transpose, typecast, and apply a unary operator
//------------------------------------------------------------------------------

GrB_Info GB (_unop_tran__identity_uint8_uint64)
(
    GrB_Matrix C,
    const GrB_Matrix A,
    int64_t *restrict *Workspaces,
    const int64_t *restrict A_slice,
    int nworkspaces,
    int nthreads
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    #include "transpose/template/GB_transpose_template.c"
    return (GrB_SUCCESS) ;
    #endif
}

