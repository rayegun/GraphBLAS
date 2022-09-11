//------------------------------------------------------------------------------
// GrB_mxm: matrix-matrix multiply
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2022, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// C<M> = accum (C,A*B) and variations.

// The input matrices A and B are optionally transposed, as determined by the
// Descriptor desc.

#include "GB_mxm.h"
#include "GB_get_mask.h"

int enzyme_dup;
int enzyme_out;
int enzyme_const;

GrB_Info GrB_mxm                    // C<M> = accum (C, A*B)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix M_in,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C,T)
    const GrB_Semiring semiring,    // defines '+' and '*' for T=A*B
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Matrix B,             // second input: matrix B
    const GrB_Descriptor desc       // descriptor for C, M, A, and B,
                                    // and method used for C=A*B
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE (C, "GrB_mxm (C, M, accum, semiring, A, B, desc)") ;
    GB_BURBLE_START ("GrB_mxm") ;
    GB_RETURN_IF_NULL_OR_FAULTY (C) ;
    GB_RETURN_IF_FAULTY (M_in) ;
    GB_RETURN_IF_NULL_OR_FAULTY (A) ;
    GB_RETURN_IF_NULL_OR_FAULTY (B) ;

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,
        A_transpose, B_transpose, AxB_method, do_sort) ;

    // get the mask
    GrB_Matrix M = GB_get_mask (M_in, &Mask_comp, &Mask_struct) ;

    //--------------------------------------------------------------------------
    // C<M> = accum (C,A*B) and variations, using the mxm kernel
    //--------------------------------------------------------------------------

    // C<M> = accum (C,T) where T = A*B, A'*B, A*B', or A'*B'
    info = GB_mxm (
        C,          C_replace,      // C matrix and its descriptor
        M, Mask_comp, Mask_struct,  // mask matrix and its descriptor
        accum,                      // for accum (C,T)
        semiring,                   // semiring that defines T=A*B
        A,          A_transpose,    // A matrix and its descriptor
        B,          B_transpose,    // B matrix and its descriptor
        false,                      // use fmult(x,y), flipxy = false
        AxB_method, do_sort,        // algorithm selector
        Context) ;

    GB_BURBLE_END ;
    return (info) ;
}

InfoAndTape GxB_fwdmxm
(
    GrB_Matrix C,                   // input/output matrix for results
    GrB_Matrix dC,
    GrB_Matrix Mask,          // optional mask for C, unused if NULL
    GrB_BinaryOp accum,       // optional accum for Z=accum(C,T)
    GrB_Semiring semiring,    // defines '+' and '*' for A*B
    GrB_Matrix A,             // first input:  matrix A
    GrB_Matrix dA,
    GrB_Matrix B,             // second input: matrix B
    GrB_Matrix dB,
    GrB_Descriptor desc 
)
{
    return __enzyme_augmentfwd((void *)GrB_mxm, enzyme_dup, C, dC, enzyme_const, Mask, enzyme_const, accum, enzyme_const, semiring, enzyme_dup, A, dA, enzyme_dup, B, dB, enzyme_const, desc);
}

void GjB_drevmxm
(
    GrB_Matrix C,
    GrB_Matrix dC,
    GrB_Matrix Mask,
    GrB_BinaryOp accum,
    GrB_Semiring semiring,
    GrB_Matrix A,
    GrB_Matrix dA,
    GrB_Matrix B,
    GrB_Matrix dB,
    GrB_Descriptor desc,
    void* tape
)
{
    __enzyme_reverse((void *)GrB_mxm, enzyme_dup, C, dC, enzyme_const, Mask, enzyme_const, accum, enzyme_const, semiring, enzyme_dup, A, dA, enzyme_dup, B, dB, enzyme_const, desc, tape);
}