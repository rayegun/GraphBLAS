function e = nnz (G)
%NNZ the number of nonzeros in a GraphBLAS matrix.
% e = nnz (G) is the number of nonzeros in a GraphBLAS matrix G.  A
% GraphBLAS matrix G may have explicit zero entries, but these are
% excluded from the count e.  Thus, nnz (G) <= GrB.entries (G).
%
% See also GrB.entries, GrB.prune, GrB/nonzeros, GrB/size, GrB/numel.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights
% Reserved. http://suitesparse.com.  See GraphBLAS/Doc/License.txt.

G = G.opaque ;
e = gb_nnz (G) ;

