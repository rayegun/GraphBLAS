//------------------------------------------------------------------------------
// GB_subassign_25: C(:,:)<M,s> = A; C empty, A dense, M structural
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2022, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// JIT: needed (now).

// Method 25: C(:,:)<M,s> = A ; C is empty, M structural, A bitmap/as-if-full

// M:           present
// Mask_comp:   false
// Mask_struct: true
// C_replace:   effectively false (not relevant since C is empty)
// accum:       NULL
// A:           matrix
// S:           none

// C and M are sparse or hypersparse.  A can have any sparsity structure, even
// bitmap, but it must either be bitmap, or as-if-full.  M may be jumbled.  If
// so, C is constructed as jumbled.  C is reconstructed with the same structure
// as M and can have any sparsity structure on input.  The only constraint on C
// is nnz(C) is zero on input.

// C is iso if A is iso

#include "GB_subassign_shared_definitions.h"
#include "GB_subassign_methods.h"
#include "GB_subassign_dense.h"
#ifndef GBCUDA_DEV
#include "GB_as__include.h"
#endif

#undef  GB_FREE_ALL
#define GB_FREE_ALL                         \
{                                           \
    GB_WERK_POP (M_ek_slicing, int64_t) ;   \
}

GrB_Info GB_subassign_25
(
    GrB_Matrix C,
    // input:
    const GrB_Matrix M,
    const GrB_Matrix A,
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (!GB_IS_BITMAP (M)) ; ASSERT (!GB_IS_FULL (M)) ;
    ASSERT (!GB_aliased (C, M)) ;   // NO ALIAS of C==M
    ASSERT (!GB_aliased (C, A)) ;   // NO ALIAS of C==A

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT_MATRIX_OK (C, "C for subassign method_25", GB0) ;
    ASSERT (GB_nnz (C) == 0) ;
    ASSERT (!GB_ZOMBIES (C)) ;
    ASSERT (!GB_JUMBLED (C)) ;
    ASSERT (!GB_PENDING (C)) ;

    ASSERT_MATRIX_OK (M, "M for subassign method_25", GB0) ;
    ASSERT (!GB_ZOMBIES (M)) ;
    ASSERT (GB_JUMBLED_OK (M)) ;
    ASSERT (!GB_PENDING (M)) ;

    ASSERT_MATRIX_OK (A, "A for subassign method_25", GB0) ;
    ASSERT (GB_as_if_full (A) || GB_IS_BITMAP (A)) ;

    const GB_Type_code ccode = C->type->code ;
    const GB_Type_code acode = A->type->code ;
    const size_t asize = A->type->size ;
    const bool C_iso = A->iso ;       // C is iso if A is iso

    //--------------------------------------------------------------------------
    // Method 25: C(:,:)<M> = A ; C is empty, A is dense, M is structural
    //--------------------------------------------------------------------------

    // Time: Optimal:  the method must iterate over all entries in M,
    // and the time is O(nnz(M)).  This is also the size of C.

    //--------------------------------------------------------------------------
    // Parallel: slice M into equal-sized chunks
    //--------------------------------------------------------------------------

    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;

    //--------------------------------------------------------------------------
    // slice the entries for each task
    //--------------------------------------------------------------------------

    GB_WERK_DECLARE (M_ek_slicing, int64_t) ;
    int M_nthreads, M_ntasks ;
    GB_SLICE_MATRIX (M, 8, chunk) ;

    //--------------------------------------------------------------------------
    // allocate C and create its pattern
    //--------------------------------------------------------------------------

    // clear prior content and then create a copy of the pattern of M.  Keep
    // the same type and CSR/CSC for C.  Allocate the values of C but do not
    // initialize them.

    bool C_is_csc = C->is_csc ;
    GB_phybix_free (C) ;
    // set C->iso = C_iso   OK
    GB_OK (GB_dup_worker (&C, C_iso, M, false, C->type)) ;
    C->is_csc = C_is_csc ;

    //--------------------------------------------------------------------------
    // C<M> = A for built-in types
    //--------------------------------------------------------------------------

    info = GrB_NO_VALUE ;

    if (C_iso)
    { 

        //----------------------------------------------------------------------
        // via the iso kernel
        //----------------------------------------------------------------------

        #define GB_ISO_ASSIGN
        GB_cast_scalar (C->x, ccode, A->x, acode, asize) ;
        #include "GB_subassign_25_template.c"
        info = GrB_SUCCESS ;

    }
    else
    {

        //----------------------------------------------------------------------
        // via the factory kernel
        //----------------------------------------------------------------------

        #ifndef GBCUDA_DEV

            //------------------------------------------------------------------
            // define the worker for the switch factory
            //------------------------------------------------------------------

            #define GB_sub25(cname) GB (_subassign_25_ ## cname)
            #define GB_WORKER(cname)                            \
            {                                                   \
                info = GB_sub25 (cname) (C, M, A,               \
                    M_ek_slicing, M_ntasks, M_nthreads) ;       \
            }                                                   \
            break ;

            //------------------------------------------------------------------
            // launch the switch factory
            //------------------------------------------------------------------

            if (C->type == A->type && ccode < GB_UDT_code)
            {
                // FUTURE: use cases 1,2,4,8,16
                // C<M> = A
                switch (ccode)
                {
                    case GB_BOOL_code   : GB_WORKER (_bool  )
                    case GB_INT8_code   : GB_WORKER (_int8  )
                    case GB_INT16_code  : GB_WORKER (_int16 )
                    case GB_INT32_code  : GB_WORKER (_int32 )
                    case GB_INT64_code  : GB_WORKER (_int64 )
                    case GB_UINT8_code  : GB_WORKER (_uint8 )
                    case GB_UINT16_code : GB_WORKER (_uint16)
                    case GB_UINT32_code : GB_WORKER (_uint32)
                    case GB_UINT64_code : GB_WORKER (_uint64)
                    case GB_FP32_code   : GB_WORKER (_fp32  )
                    case GB_FP64_code   : GB_WORKER (_fp64  )
                    case GB_FC32_code   : GB_WORKER (_fc32  )
                    case GB_FC64_code   : GB_WORKER (_fc64  )
                    default: ;
                }
            }

        #endif

        //----------------------------------------------------------------------
        // via the JIT kernel
        //----------------------------------------------------------------------

        #if GB_JIT_ENABLED
        // JIT TODO: type: subassign 25
        #endif

        //----------------------------------------------------------------------
        // via the generic kernel
        //----------------------------------------------------------------------

        if (info == GrB_NO_VALUE)
        { 

            //-----------------------------------------------------------------
            // get operators, functions, workspace, contents of A and C
            //------------------------------------------------------------------

            #include "GB_generic.h"
            GB_BURBLE_MATRIX (A, "(generic C(:,:)<M,struct>=A assign, "
                "method 25) ") ;

            const size_t csize = C->type->size ;
            GB_cast_function cast_A_to_C = GB_cast_factory (ccode, acode) ;

            #define C_iso false
            #include "GB_subassign_25_template.c"
            info = GrB_SUCCESS ;
        }
    }

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    GB_FREE_ALL ;
    if (info == GrB_SUCCESS)
    {
        ASSERT_MATRIX_OK (C, "C output for subassign method_25", GB0) ;
        ASSERT (GB_ZOMBIES_OK (C)) ;
        ASSERT (GB_JUMBLED_OK (C)) ;
        ASSERT (!GB_PENDING (C)) ;
    }
    return (info) ;
}

