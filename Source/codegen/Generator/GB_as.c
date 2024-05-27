//------------------------------------------------------------------------------
// GB_as:  assign/subassign kernels with no accum
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// C(I,J)<M> = A

#include "GB.h"
#include "builtin/factory/GB_control.h"
#include "slice/GB_ek_slice.h"
#include "FactoryKernels/GB_as__include.h"

// A and C matrices
GB_atype
GB_ctype
GB_declarec
GB_copy_aij_to_cwork
GB_copy_aij_to_c
GB_copy_scalar_to_c
GB_ax_mask

// disable this operator and use the generic case if these conditions hold
GB_disable

#include "shared/GB_assign_shared_definitions.h"

//------------------------------------------------------------------------------
// C<M> = scalar, when C is dense
//------------------------------------------------------------------------------

GrB_Info GB (_subassign_05d)
(
    GrB_Matrix C,
    const GrB_Matrix M,
    const bool Mask_struct,
    const GB_void *scalar,      // of type C->type
    GB_Werk Werk
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    GB_C_TYPE cwork = (*((GB_C_TYPE *) scalar)) ;
    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;
    #include "assign/template/GB_subassign_05d_template.c"
    return (GrB_SUCCESS) ;
    #endif
}

//------------------------------------------------------------------------------
// C<A> = A, when C is dense
//------------------------------------------------------------------------------

GrB_Info GB (_subassign_06d)
(
    GrB_Matrix C,
    const GrB_Matrix A,
    const bool Mask_struct,
    GB_Werk Werk
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    ASSERT (C->type == A->type) ;
    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;
    #include "assign/template/GB_subassign_06d_template.c"
    return (GrB_SUCCESS) ;
    #endif
}

//------------------------------------------------------------------------------
// C<M> = A, when C is empty and A is dense
//------------------------------------------------------------------------------

GrB_Info GB (_subassign_25)
(
    GrB_Matrix C,
    const GrB_Matrix M,
    const GrB_Matrix A,
    GB_Werk Werk
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    ASSERT (C->type == A->type) ;
    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;
    #include "assign/template/GB_subassign_25_template.c"
    return (GrB_SUCCESS) ;
    #endif
}

