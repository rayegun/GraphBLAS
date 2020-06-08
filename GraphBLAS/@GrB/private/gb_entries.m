function result = gb_entries (A, varargin)
%GB_ENTRIES count or query the entries of a matrix.
% Implements GrB.entries (A, ...) and GrB.nonz (A, ...).

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights
% Reserved. http://suitesparse.com.  See GraphBLAS/Doc/License.txt.

% get the string arguments
dim = 'all' ;           % 'all', 'row', or 'col'
kind = 'count' ;        % 'count', 'list', or 'degree'
for k = 1:nargin-1
    arg = varargin {k} ;
    switch arg
        case { 'all', 'row', 'col' }
            dim = arg ;
        case { 'count', 'list', 'degree' }
            kind = arg ;
        otherwise
            gb_error ('unknown option') ;
    end
end

if (isequal (dim, 'all'))

    switch kind
        case 'count'
            % number of entries in A
            % e = GrB.entries (A)
            result = gbnvals (A) ;
        case 'list'
            % list of values of unique entries
            % X = GrB.entries (A, 'list')
            result = unique (gbextractvalues (A)) ;
        otherwise
            gb_error ('''all'' and ''degree'' cannot be combined') ;
    end

else

    % get the row or column degree
    result = gbdegree (A, dim) ;

    switch kind
        case 'count'
            % number of non-empty rows/cols
            % e = GrB.entries (A, 'row')
            % e = GrB.entries (A, 'col')
            result = gbnvals (gbselect (result, 'nonzero')) ;
        case 'list'
            % list of non-empty rows/cols
            % I = GrB.entries (A, 'row', 'list')
            % J = GrB.entries (A, 'col', 'list')
            result = gbextracttuples (gbselect (result, 'nonzero')) ;
        % case 'degree'
            % degree of all rows/cols
            % d = GrB.entries (A, 'row', 'degree')
            % d = GrB.entries (A, 'col', 'degree')
            % result is returned as a GraphBLAS struct
    end
end

