// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of NVIDIA CORPORATION nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* fixme:  need to split this into multiple files.  One for the general
bucket-based dot3 method (A and B both sparse/hyper), one for non-bucket-
based dot3 methods (A and/or B bitmap/full), one for reduction, etc

Otherwise, this will get too large when constructing all the CUDA kernels
for all of GraphBLAS.
*/

/*
  Extended example for building on-the-fly kernels with C interface.
  Simple examples demonstrating different ways to load source code
    and call kernels.
 */

#ifndef GB_JITFACTORY_H
#define GB_JITFACTORY_H

#pragma once

extern "C" {
#include "GraphBLAS.h"
};
#include "GB_jit_launcher.h"
#include "GB_cuda_mxm_factory.hpp"
#include "GB_cuda_reduce_factory.hpp"
#include "GB_cuda_buckets.h"
#include "GB_cuda_type_wrap.hpp"
#include "GB_cuda_error.h"
#include "../rmm_wrap/rmm_wrap.h"

// fixme: C11 is already required for all of GraphBLAS.  No need to test here:
#if __cplusplus >= 201103L

constexpr unsigned int SMEM = 0;

/**
 * This file is responsible for picking all the parameters and what kernel variaiton we will use for a given instance
 * - data types
 * - semiring types
 * - binary ops
 * - monoids
 *
 * Kernel factory says "Here's the actual instance I want you to build with the given parameters"
 */

//bool GB_cuda_reduce(int64_t *index, void *in_data, void *output, unsigned int N, GrB_Monoid op);

//Kernel jitifiers
class reduceFactory ;
template<typename T1, typename T2, typename T3> class dotFactory ;
template<typename T1, typename T2, typename T3> class spdotFactory ;

inline std::istream* (*file_callback)(std::string, std::iostream&);

//AxB_dot3_phase1 kernel launchers
template<int threads_per_block, int chunk_size> class phase1launchFactory ;

//AxB_dot3_phase3 kernel launchers

template<  typename T_C, typename T_M, 
         typename T_A, typename T_B, typename T_xy, typename T_z> class launchFactory ;


static const std::vector<std::string> compiler_flags{
   "-std=c++14",
   //"-G",
   "-remove-unused-globals",
   "-w",
   "-D__CUDACC_RTC__",
   "-I.",
   "-I..",
   "-I../../Source",
   "-I../../Source/Template",
   "-I../templates",

   // Add includes relative to GRAPHBLAS_SOURCE_PATH variable
   "-I" + jit::get_user_graphblas_source_path() + "/CUDA",
   "-I" + jit::get_user_graphblas_source_path() + "/CUDA/templates",
   "-I" + jit::get_user_graphblas_source_path() + "/Source",
   "-I" + jit::get_user_graphblas_source_path() + "/Source/Template",
   "-I/usr/local/cuda/include",
};

static const std::vector<std::string> header_names ={};

// FIXME: We probably want to remove this type template altogether and provide a
// macro/function that can convert from a GrB_Type instance to the name of a type
// that the jitifier will accept.
template<int threads_per_block=32, int chunk_size = 128>
class phase1launchFactory 
{
  std::string base_name = "GB_jit";
  std::string kernel_name = "AxB_phase1";

  GB_cuda_mxm_factory &mxm_factory_;

public:

  int get_number_of_blocks(GrB_Matrix M) {
      int number_of_sms = GB_Global_gpu_sm_get (0);
      int nblks = ( GB_nnz (M) + chunk_size - 1)/chunk_size;
      return GB_IMIN( nblks,  chunk_size * number_of_sms);
  }

  int get_threads_per_block() {
      return threads_per_block;
  }

  // This assumes the needed state on the GB_cuda_mxm_factory
  // has already been populated
  phase1launchFactory(GB_cuda_mxm_factory &mxm_factory): mxm_factory_(mxm_factory){}

  bool jitGridBlockLaunch(int64_t *nanobuckets, int64_t *blockBucket,
                          GrB_Matrix C, GrB_Matrix M, GrB_Matrix A, GrB_Matrix B, cudaStream_t stream = 0) {

    // Idea is to have each task work on a continguous block of columns of C
    // Note: for small tests, mnz is small so ntasks is be governed by
    // chunksize, not chunk_size*number_of_sms.  For large problems in
    // production, chunksize is less important since ntasks will likely be
    // bounded by chunk_size*number_of_sms (say 128*80 = 10,240 on a V100, for
    // the default chunk_size of 128).

    // Defining dummy instance only so we can introspect type
//    // (1) create the mxm code and name

//    // (2) ensure the jitifier has "GB_mxm_[mymxm.sr_code].h"
    jit::GBJitCache filecache = jit::GBJitCache::Instance() ;
    filecache.getFile (mxm_factory_) ;

    auto sr_code = std::to_string(mxm_factory_.sr_code);

    std::stringstream string_to_be_jitted ;
    // FIXME: use mask_ecode instead, not even M->type->name
    std::vector<std::string> template_types = {M->type->name, sr_code};

    std::string hashable_name = base_name + "_" + kernel_name;
    string_to_be_jitted << hashable_name << std::endl <<
    R"(#include ")" << jit::get_user_home_cache_dir() << "/" << mxm_factory_.filename << R"(")" << std::endl <<
    R"(#include "templates/)" << hashable_name << R"(.cuh")" << std::endl;

    bool result = false;

    dim3 grid(get_number_of_blocks(M));
    dim3 block(get_threads_per_block());

    jit::launcher( hashable_name + "_" + sr_code,   // FIXME: use mask_ecode
                   string_to_be_jitted.str(),
                   header_names,
                   compiler_flags,
                   file_callback)
                 .set_kernel_inst(  kernel_name, template_types)
                 .configure(grid, block, SMEM, stream)
                 .launch( nanobuckets, blockBucket, C, M, A, B);

      result = true;

      return result;
     }
};

template<int threads_per_block = 32, int chunk_size = 128>
class phase2launchFactory
{

  std::string base_name = "GB_jit";
  std::string kernel_name = "AxB_phase2";

public:

  int get_threads_per_block() {
        return threads_per_block;
  }

  int get_number_of_blocks(GrB_Matrix M) {
    const int64_t mnz = GB_nnz (M) ;
    int ntasks = ( mnz +chunk_size -1)/chunk_size;
    // Idea is to have each task work on a continguous block of columns of C
    ntasks = GB_IMIN( ntasks,  chunk_size*GB_Global_gpu_sm_get (0)) ;    // ntasks will be grid.x
    return (ntasks + threads_per_block - 1) / threads_per_block ;
  }

  int get_number_of_phase1_blocks( GrB_Matrix M){
    const int64_t mnz = GB_nnz (M) ;
    int number_of_sms = GB_Global_gpu_sm_get (0);
    int nblks = ( GB_nnz (M) + chunk_size - 1)/chunk_size;
    return GB_IMIN( nblks,  chunk_size * number_of_sms);
  }

  bool jitGridBlockLaunch(// parameters to AxB_phase2:
                          int64_t *blockBucket, int64_t *offset, GrB_Matrix M, cudaStream_t stream = 0) {

    bool result = false;

      dim3 grid(get_number_of_blocks(M));
      dim3 block(get_threads_per_block());

      std::string hashable_name = base_name + "_" + kernel_name;
      std::stringstream string_to_be_jitted ;
      string_to_be_jitted <<
      hashable_name << std::endl << R"(#include ")" << hashable_name << R"(.cuh")" << std::endl;

      const int64_t mnz = GB_nnz (M) ;
      jit::launcher( hashable_name,
                     string_to_be_jitted.str(),
                     header_names,
                     compiler_flags,
                     file_callback)
                   .set_kernel_inst( kernel_name, {})
                   .configure(grid, block, SMEM, stream)
                   // parameters to AxB_phase2:
                   .launch( blockBucket, offset, get_number_of_phase1_blocks(M));

      result= true;

      return result;
     }

};

template< int threads_per_block = 32, int chunk_size = 128>
class phase2endlaunchFactory
{

  std::string base_name = "GB_jit";
  std::string kernel_name = "AxB_phase2end";

public:

  int get_threads_per_block() {
        return threads_per_block;
  }

  int get_number_of_blocks(GrB_Matrix M) {
    const int64_t mnz = GB_nnz (M) ;
    int ntasks = ( mnz +chunk_size -1)/chunk_size;
    int number_of_sms = GB_Global_gpu_sm_get (0);

    // Idea is to have each task work on a continguous block of columns of C
    return GB_IMIN( ntasks,  chunk_size*number_of_sms) ;    // ntasks will be grid.x
  }

  bool jitGridBlockLaunch(int64_t *nanobuckets, int64_t *blockBucket,
                          int64_t *bucketp, int64_t *bucket, int64_t *offset,
                          GrB_Matrix C, GrB_Matrix M, cudaStream_t stream = 0)
     {

      bool result = false;

      dim3 grid(get_number_of_blocks(M));
      dim3 block(get_threads_per_block());

      std::string hashable_name = base_name + "_" + kernel_name;
      std::stringstream string_to_be_jitted ;
      string_to_be_jitted <<
      hashable_name << std::endl << R"(#include ")" << hashable_name << R"(.cuh")" << std::endl;

      jit::launcher( hashable_name,
                     string_to_be_jitted.str(),
                     header_names,
                     compiler_flags,
                     file_callback)
                   .set_kernel_inst(  kernel_name , {})
                   .configure(grid, block, SMEM, stream)
                   .launch( nanobuckets, blockBucket, bucketp, bucket, offset, C, GB_nnz (M));

      result= true;

      return result;
     }

};

class phase3launchFactory
{
  std::string base_name = "GB_jit";
  std::string kernel_name = "AxB_dot3";

  GB_cuda_mxm_factory &mxm_factory_;

  GB_bucket_code bucket_code_;

public:

   std::string Opname;

  /**
   * This assumes the needed state on the GB_cuda_mxm_factory has already been populated.
   * The `bucket_code` determines which kernel is launched
   */
  phase3launchFactory(GB_cuda_mxm_factory &mymxmfactory, GB_bucket_code bucket_code):
      mxm_factory_(mymxmfactory), bucket_code_(bucket_code) {}

  bool jitGridBlockLaunch(int64_t start, int64_t end, int64_t *bucketp, int64_t *bucket,
                          GrB_Matrix C,  GrB_Matrix M, GrB_Matrix A, GrB_Matrix B,
                          cudaStream_t stream = 0) {

      bool result = false;

    //----------------------------------------------------------------------
    // phase3: do the numerical work
    //----------------------------------------------------------------------

    C->jumbled = true;
    const int64_t nz = end - start; // number of dots in this bucket  
    const int64_t mnvec = M->nvec ;

    int gridsz, blocksz, sz = 4;

    std::stringstream final_kernel_name_ss;
    final_kernel_name_ss << kernel_name << "_";

    /**
     * Configure geometry and kernel function name based on sparsity of C and number of vectors in M
     */
    configure( nz, mnvec, final_kernel_name_ss, blocksz, gridsz, sz);

    auto sr_code = std::to_string(mxm_factory_.sr_code);

    GrB_BinaryOp mult = mxm_factory_.semiring->multiply ;

    std::string hashable_name = base_name + "_" + final_kernel_name_ss.str();
    std::stringstream string_to_be_jitted ;
    std::vector<std::string> template_types =
    {
        C->type->name, A->type->name, B->type->name,
        mult->ztype->name, mult->xtype->name, mult->ytype->name,
        sr_code
    };

    jit::GBJitCache filecache = jit::GBJitCache::Instance() ;
    filecache.getFile (mxm_factory_) ;

    string_to_be_jitted << hashable_name << std::endl <<
    R"(#include ")" << jit::get_user_home_cache_dir() << "/" << mxm_factory_.filename << R"(")" << std::endl <<
    R"(#include ")" << hashable_name << R"(.cuh")" << std::endl;

    dim3 grid(gridsz);
    dim3 block(blocksz);

    GBURBLE ("(GPU phase3 launch %s st,end=%ld,%ld nblocks,blocksize= %d,%d )\n", this->Opname.c_str(),
              start,end,gridsz,blocksz) ;
    jit::launcher( hashable_name + "_" + sr_code,
                   string_to_be_jitted.str(),
                   header_names,
                   compiler_flags,
                   file_callback)
               .set_kernel_inst(final_kernel_name_ss.str(), template_types )
                               // { C->type->name,
                               //   A->type->name,
                               //   B->type->name })
               .configure(grid, block, SMEM, stream) //if commented, use implicit 1D configure in launch
               .launch(
                        start,             // input/output:
                        end,               // global bucket cumsum, of size NBUCKETS+1
                        bucket,            // global buckets, of size cnz (== mnz)
                        C,                 // final output matrix
                                           // inputs, not modified:
                        M,                 // Mi used for column index
                        A,                 // A matrix
                        B,                 // B matrix
                        sz                 // only used for sparse-sparse cases
                    );

    result= true;

    return result;
  }

private:
    void configure(std::int64_t Cnz, std::int64_t mnvec, std::stringstream &opname,
                   int &blocksz, int &gridsz, int &sz) {
    int number_of_sms = GB_Global_gpu_sm_get (0) ;

    int work_per_thread;

    // TODO: make sure this works with different geometry

    /* fixme: the final bucket-based dot3 method should only have the following
        buckets:

        case GB_BUCKET_ZOMBIE : // C(i,j) is a zombie (not a bucket)
        case GB_BUCKET_VSSP :       // one vector very sparse, other longer
        case GB_BUCKET_VSVS_256 :   // both vectors very sparse
        case GB_BUCKET_VSVS_64 :    // (or just one VSVS bucket, not 4)
        case GB_BUCKET_VSVS_16 :
        case GB_BUCKET_VSVS_4 :
        case GB_BUCKET_MERGEPATH :  // both vectors are long
        case GB_BUCKET_WARP_IX :    // currently unused; remove this

    These buckets should be handled in different kernels, mostly non-bucket-
    based:

        case GB_BUCKET_DNDN :   both A and B bitmap/full

        case GB_BUCKET_ZOMBIE : // C(i,j) is a zombie (not a bucket)
        case GB_BUCKET_DNVS :   A bitmap/full, B sparse/hyper
        case GB_BUCKET_DNSP :   A bitmap/full, B very sparse/hyper

        case GB_BUCKET_ZOMBIE : // C(i,j) is a zombie (not a bucket)
        case GB_BUCKET_VSDN :   A sparse/hyper, B bitmap/full
        case GB_BUCKET_SPDN :   A very sparse/hyper, B bitmap/full
    */

    switch (bucket_code_)
    {

        //--------------------------------------------------------------
        // not a bucket ... bring out your dead:
        //--------------------------------------------------------------

        case GB_BUCKET_ZOMBIE : // C(i,j) is a zombie (not a bucket)
            break ;

        //--------------------------------------------------------------
        // CUDA kernel: dndn, handles a single bucket:
        //--------------------------------------------------------------

//      // both A(:,i) and B(:,j) are dense
//      case GB_BUCKET_DNDN : // fixme: remove bucket, use dedicated kernel
//          Opname = "phase3_dndn" ;
//          blocksz = 32;
//          gridsz = ( Cnz -1 + blocksz)/blocksz;
//          break ;

        //--------------------------------------------------------------
        // CUDA kernel: spdn, handles 4 buckets:
        //--------------------------------------------------------------

//      // A(:,i) is dense and B(:,j) is very sparse (< 256 entries)
//      case GB_BUCKET_DNVS :  // fixme: remove bucket, use dedicated kernel
//      // A(:,i) is very sparse (< 256 entries) and B(:,j) is dense
//      case GB_BUCKET_VSDN : // fixme: remove bucket, use dedicated kernel
//          sz = 64 ;
//          Opname = "phase3_spdn" ;
//          blocksz = 32;
//          gridsz = ( Cnz -1 + blocksz)/blocksz;
//          break ;

//      // A(:,i) is dense and B(:,j) is sparse (>= 256 entries)
//      case GB_BUCKET_DNSP : // fixme: remove bucket, use dedicated kernel
//      // A(:,i) is sparse (>= 256 entries) and B(:,j) is dense
//      case GB_BUCKET_SPDN : // fixme: remove bucket, use dedicated kernel
//          sz = 256 ;
//          Opname = "phase3_spdn" ;
//          blocksz = 32;
//          gridsz = ( Cnz -1 + blocksz)/blocksz;
//          break ;

        //--------------------------------------------------------------
        // CUDA kernel: vssp, handles 1 bucket, uses binary search:
        //--------------------------------------------------------------

        // A(:,i) is very sparse compared to B(:,j), or visa versa
        case GB_BUCKET_VSSP :
            Opname = "phase3_vssp" ;
            blocksz = 256;
            work_per_thread = 4;
            if( Cnz < 2048)
            {
              blocksz = 32;
              work_per_thread = 1;
            }
            gridsz = ( Cnz -1 + work_per_thread*blocksz)/(work_per_thread*blocksz);
            break ;

        //--------------------------------------------------------------
        // CUDA kernel: vsvs, handles 4 buckets:
        //--------------------------------------------------------------

        // let len = nnz (A (:,i) + nnz (B (:,j)), then:
//      case GB_BUCKET_VSVS_256 : sz += 256-64 ;
//      case GB_BUCKET_VSVS_64 :  sz += 64-4   ;
//      case GB_BUCKET_VSVS_16 :  sz += 16-4   ;
//      case GB_BUCKET_VSVS_4 :   sz += 4      ;

        case GB_BUCKET_VSVS :
            Opname = "phase3_vsvs" ;
            blocksz = 512;
            work_per_thread = 4;
            
            if( Cnz < 1024){
              blocksz = 64;
              work_per_thread = 8;
            }
            

            // FIXME: Is the first line not needed?
            //gridsz = GB_IMIN( 1024*number_of_sms, ( Cnz  + work_per_thread*blocksz -1 )/(work_per_thread*blocksz));
            gridsz =  ( Cnz  + work_per_thread*blocksz -1 )/(work_per_thread*blocksz);
            break ;

        //--------------------------------------------------------------
        // CUDA kernel: mp, use the merge-path method:
        //--------------------------------------------------------------

        case GB_BUCKET_MERGEPATH :
            Opname = "phase3_mp" ;
            blocksz = 32;
            work_per_thread = 32*8 ;
            gridsz = ( Cnz -1 + work_per_thread)/(work_per_thread);
            //gridsz = GB_IMIN( 1024*number_of_sms, ( Cnz  + work_per_thread*blocksz -1 )/(work_per_thread*blocksz));
            break ;

//      case GB_BUCKET_WARP_IX :   sz = 32      ;
//          Opname = "phase3_warpix" ;
//          blocksz = 32;
//          gridsz =  GB_IMIN( (mnvec+15)/16, 256*number_of_sms);
//          break ;

        default:
            break ;
    }

    opname << Opname;
  }
};

class reduceFactory
{
  std::string base_name = "GB_jit";
  std::string kernel_name = "reduceNonZombiesWarp";

  int threads_per_block = 256 ;
  int work_per_thread = 128;
  int number_of_sms = GB_Global_gpu_sm_get (0);

  GB_cuda_reduce_factory &reduce_factory_;

public:

  reduceFactory(GB_cuda_reduce_factory &myreducefactory) : reduce_factory_(myreducefactory) {}

  int get_threads_per_block() {
    return threads_per_block;
  }

  int get_number_of_blocks(unsigned int N) {
      return (N + work_per_thread*threads_per_block - 1)/(work_per_thread*threads_per_block);
  }

  // Note: this does assume the erased types are compatible w/ the monoid's ztype
  bool jitGridBlockLaunch(GrB_Matrix A, void* output,
                          GrB_Monoid op, cudaStream_t stream = 0)
  {
      GBURBLE ("\n(launch reduce factory) \n") ;

      GrB_Scalar temp_scalar;
      GrB_Scalar_new(&temp_scalar, op->op->ztype);

      cuda::jit::scalar_set_element(temp_scalar, 0);
      GrB_Scalar_wait(temp_scalar, GrB_MATERIALIZE);

      jit::GBJitCache filecache = jit::GBJitCache::Instance() ;
      filecache.getFile (reduce_factory_) ;

      auto rcode = std::to_string(reduce_factory_.rcode);

      std::string hashable_name = base_name + "_" + kernel_name;
      std::stringstream string_to_be_jitted ;
      string_to_be_jitted <<
      hashable_name << std::endl <<
      R"(#include ")" << jit::get_user_home_cache_dir() << "/" << reduce_factory_.filename << R"(")" << std::endl <<
      R"(#include ")" << hashable_name << R"(.cuh")" << std::endl;

      bool is_sparse = GB_IS_SPARSE(A);
      int64_t N = is_sparse ? GB_nnz(A) : GB_NCOLS(A) * GB_NROWS(A);

      int blocksz = get_threads_per_block();
      int gridsz = get_number_of_blocks(N);
      dim3 grid(gridsz);
      dim3 block(blocksz);

      // FIXME: call GB_stringify_reduce to create GB_ADD and related
      // macros, in an include file: GB_reduce_123412341234.h
      GBURBLE ("(cuda reduce launch %d threads in %d blocks)", blocksz, gridsz ) ;

      jit::launcher(hashable_name + "_" + rcode,
                    string_to_be_jitted.str(),
                    header_names,
                    compiler_flags,
                    file_callback)
               .set_kernel_inst(  kernel_name , { A->type->name, op->op->ztype->name, rcode, "true" })
               .configure(grid, block, SMEM, stream)
               // FIXME: GB_ADD is hardcoded into kernel for now
               .launch( A, temp_scalar, N, is_sparse);

      // Need to synchronize before copying result to host
      CHECK_CUDA( cudaStreamSynchronize(stream) );

      memcpy(output, temp_scalar->x, op->op->ztype->size);

      rmm_wrap_free(temp_scalar);
      return true;
  }
};

template<  int threads_per_block=32, int chunk_size = 256>
inline bool GB_cuda_mxm_phase1(GB_cuda_mxm_factory &mxm_factory, int64_t *nanobuckets, int64_t *blockBucket,
                        GrB_Matrix C, GrB_Matrix M, GrB_Matrix A, GrB_Matrix B,
                        cudaStream_t stream = 0) {
    phase1launchFactory<threads_per_block, chunk_size> lf(mxm_factory);
    return lf.jitGridBlockLaunch(nanobuckets, blockBucket, C, M, A, B, stream);
}


template<int threads_per_block = 32, int chunk_size = 256>
bool GB_cuda_mxm_phase2(int64_t *nanobuckets, int64_t *blockBucket,
                          int64_t *bucketp, int64_t *bucket, int64_t *offset,
                          GrB_Matrix M, cudaStream_t stream = 0) {

  phase2launchFactory<threads_per_block, chunk_size> lf;
  return lf.jitGridBlockLaunch(nanobuckets, blockBucket, bucketp, bucket, offset, M, stream);
}

template<int threads_per_block = 32, int chunk_size = 128>
inline bool GB_cuda_mxm_phase2end(int64_t *nanobuckets, int64_t *blockBucket,
                           int64_t *bucketp, int64_t *bucket, int64_t *offset,
                           GrB_Matrix C, GrB_Matrix M, cudaStream_t stream) {
    phase2endlaunchFactory lf;
    return lf.jitGridBlockLaunch(nanobuckets, blockBucket, bucketp, bucket, offset, C, M, stream);
}



inline bool GB_cuda_mxm_phase3(GB_cuda_mxm_factory &mymxmfactory, GB_bucket_code bucket_code,
                        int64_t start, int64_t end, int64_t *bucketp, int64_t *bucket,
                          GrB_Matrix C,  GrB_Matrix M, GrB_Matrix A, GrB_Matrix B, cudaStream_t stream = 0) {
    phase3launchFactory lf(mymxmfactory, bucket_code);
    return lf.jitGridBlockLaunch(start, end, bucketp, bucket, C, M, A, B, stream);
}


inline bool GB_cuda_reduce(GB_cuda_reduce_factory &myreducefactory,
                           GrB_Matrix A, void *output, GrB_Monoid op,
                           cudaStream_t stream = 0) {
    reduceFactory rf(myreducefactory);
    GBURBLE ("(starting cuda reduce)" ) ;
    bool result = rf.jitGridBlockLaunch(A, output, op, stream);
    GBURBLE ("(ending cuda reduce)" ) ;
    return (result) ;
}

#endif  // C++11
#endif
