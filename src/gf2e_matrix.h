/**
 * \file gf2e_matrix.h
 *
 * \brief Dense matrices over GF(2^k) (2<= k <= 10) represented by M4RI matrices.
 *
 * This file implements the data type mzed_t. 
 * That is, matrices over GF(2^k) in row major representation. 

 * For example, let a = \sum a_i x_i / <f> and b = \sum b_i x_i / <f> 
 * be elements in GF(2^6) with minimal polynomial f. Then, the 
 * 1 x 2 matrix [b a] would be stored as 
\verbatim
 [...| 0 0 b5 b4 b3 b2 b1 b0 | 0 0 a5 a4 a3 a2 a1 a0]
\endverbatim
 * 
 * Internally M4RI matrices are used to store bits with allows to
 * re-use existing M4RI methods (such as mzd_add) when implementing
 * functions for mzed_t.
 *
 * \author Martin Albrecht <martinralbrecht@googlemail.com>
 */

#ifndef M4RIE_GF2E_MATRIX_H
#define M4RIE_GF2E_MATRIX_H

/******************************************************************************
*
*            M4RIE: Linear Algebra over GF(2^e)
*
*    Copyright (C) 2010,2011 Martin Albrecht <martinralbrecht@googlemail.com>
*
*  Distributed under the terms of the GNU General Public License (GEL)
*  version 2 or higher.
*
*    This code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*  The full text of the GPL is available at:
*
*                  http://www.gnu.org/licenses/
******************************************************************************/

#include <m4ri/m4ri.h>
#include "finite_field.h"
#include "m4ri_functions.h"

/**
 * \brief Dense matrices over GF(2^k) in a packed representation.
 * 
 * \ingroup Definitions
 */

typedef struct {

  /**
   * m x n matrices over GF(2^k) are represented as m x (kn) matrices over GF(2).
   */
  mzd_t *x;

  /**
   * A finite field GF(2^k).
   */

  gf2e *finite_field;

  /**
   * Number of rows.
   */

  rci_t nrows;

  /**
   * Number of columns.
   */

  rci_t ncols;

  /**
   * The internal width of elements (must divide 64)
   */

  wi_t w;

} mzed_t;


/**
 * \brief Create a new matrix of dimension m x n over ff
 *
 * Use mzed_free to kill it.
 *
 * \param ff Finite field
 * \param m Number of rows
 * \param n Number of columns
 *
 * \ingroup Constructions
 */

mzed_t *mzed_init(gf2e *ff, const rci_t m, const rci_t n);

/**
 * \brief Free a matrix created with mzed_init.
 * 
 * \param A Matrix
 *
 * \ingroup Constructions
 */

void mzed_free(mzed_t *A);


/**
 * \brief Concatenate B to A and write the result to C.
 * 
 * That is,
 *
 \verbatim
 [ A ], [ B ] -> [ A  B ] = C
 \endverbatim
 *
 * The inputs are not modified but a new matrix is created.
 *
 * \param C Matrix, may be NULL for automatic creation
 * \param A Matrix
 * \param B Matrix
 *
 * \note This is sometimes called augment.
 *
 * \ingroup Constructions
 */

static inline mzed_t *mzed_concat(mzed_t *C, const mzed_t *A, const mzed_t *B) {
  if(C==NULL)
    C = mzed_init(A->finite_field, A->nrows, A->ncols + B->ncols);
  mzd_concat(C->x, A->x, B->x);
  return C;
}

/**
 * \brief Stack A on top of B and write the result to C.
 *
 * That is, 
 *
 \verbatim
 [ A ], [ B ] -> [ A ] = C
                 [ B ]
 \endverbatim
 *
 * The inputs are not modified but a new matrix is created.
 *
 * \param C Matrix, may be NULL for automatic creation
 * \param A Matrix
 * \param B Matrix
 *
 * \ingroup Constructions
 */

static inline mzed_t *mzed_stack(mzed_t *C, const mzed_t *A, const mzed_t *B) {
  if(C==NULL)
    C = mzed_init(A->finite_field, A->nrows + B->nrows, A->ncols);
  mzd_stack(C->x, A->x, B->x);
  return C;
}


/**
 * \brief Copy a submatrix.
 * 
 * Note that the upper bounds are not included.
 *
 * \param S Preallocated space for submatrix, may be NULL for automatic creation.
 * \param M Matrix
 * \param lowr start rows
 * \param lowc start column
 * \param highr stop row (this row is \em not included)
 * \param highc stop column (this column is \em not included)
 *
 * \ingroup Constructions
 */
static inline mzed_t *mzed_submatrix(mzed_t *S, const mzed_t *M, const rci_t lowr, const rci_t lowc, const rci_t highr, const rci_t highc) {
  if(S==NULL)
    S = mzed_init(M->finite_field, highr - lowr, highc - lowc);

  mzd_submatrix(S->x, M->x, lowr, lowc*M->w, highr, highc*M->w);
  return S;
}

/**
 * \brief Create a window/view into the matrix M.
 *
 * A matrix window for M is a meta structure on the matrix M. It is
 * setup to point into the matrix so M \em must \em not be freed while the
 * matrix window is used.
 *
 * This function puts the restriction on the provided parameters that
 * all parameters must be within range for M which is not currently
 * enforced.
 *
 * Use mzed_free_window to free the window.
 *
 * \param M Matrix
 * \param lowr Starting row (inclusive)
 * \param lowc Starting column (inclusive)
 * \param highr End row (exclusive)
 * \param highc End column (exclusive)
 *
 * \ingroup Constructions
 */

static inline mzed_t *mzed_init_window(const mzed_t *A, const rci_t lowr, const rci_t lowc, const rci_t highr, const rci_t highc) {
  mzed_t *B = (mzed_t *)m4ri_mm_malloc(sizeof(mzed_t));
  B->finite_field = A->finite_field;
  B->w = gf2e_degree_to_w(A->finite_field);
  B->nrows = highr - lowr;
  B->ncols = highc - lowc;
  B->x = mzd_init_window(A->x, lowr, B->w*lowc, highr, B->w*highc);
  return B;
}

/**
 * \brief Free a matrix window created with mzed_init_window.
 * 
 * \param A Matrix
 *
 * \ingroup Constructions
 */

static inline void mzed_free_window(mzed_t *A) {
  mzd_free_window(A->x);
  m4ri_mm_free(A);
}

/**
 * \brief Set C = A+B.
 *
 * C is also returned. If C is NULL then a new matrix is created which
 * must be freed by mzed_free.
 *
 * \param C Preallocated sum matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \ingroup Addition
 */

mzed_t *mzed_add(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Same as mzed_add but without any checks on the input.
 *
 * \param C Preallocated sum matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 *
 * \ingroup Addition
 */

mzed_t *_mzed_add(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Same as mzed_add.
 *
 * \param C Preallocated difference matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 *
 * \ingroup Addition
 */

#define mzed_sub mzed_add

/**
 * \brief Same as mzed_sub but without any checks on the input.
 *
 * \param C Preallocated difference matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 *
 * \ingroup Addition
 */

#define _mzed_sub _mzed_add

/**
 * \brief Compute C such that C == AB.
 *
 * \param C Preallocated return matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */
 
mzed_t *mzed_mul(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Compute C such that C == C + AB.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_addmul(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *_mzed_mul(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == C + AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *_mzed_addmul(mzed_t *C, const mzed_t *A, const mzed_t *B);


/**
 * \brief Compute C such that C == C + AB using naive cubic multiplication.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_addmul_naive(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Compute C such that C == AB using naive cubic multiplication.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_mul_naive(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *_mzed_mul_naive(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == aB.
 *
 * \param C Preallocated product matrix or NULL.
 * \param a finite field element.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_mul_scalar(mzed_t *C, const word a, const mzed_t *B);

/**
 * Check whether C, A and B match in sizes and fields for
 * multiplication
 *
 * \param C Output matrix, if NULL a new matrix is created.
 * \param A Input matrix.
 * \param B Input matrix.
 * \param clear Write zeros to C or not.
 */

mzed_t *_mzed_mul_init(mzed_t *C, const mzed_t *A, const mzed_t *B, int clear);

/**
 * \brief Fill matrix A with random elements.
 *
 * \param A Matrix
 *
 * \todo Allow the user to provide a RNG callback.
 *
 * \ingroup Assignment
 */

void mzed_randomize(mzed_t *A);

/**
 * \brief Copy matrix A to B.
 *
 * \param B May be NULL for automatic creation.
 * \param A Source matrix.
 *
 * \ingroup Assignment
 */

mzed_t *mzed_copy(mzed_t *B, const mzed_t *A);

/**
 * \brief Return diagonal matrix with value on the diagonal.
 *
 * If the matrix is not square then the largest possible square
 * submatrix is used.
 *
 * \param M Matrix
 * \param value Finite Field element
 *
 * \ingroup Assignment
 */

void mzed_set_ui(mzed_t *A, word value);


/**
 * Get the element at position (row,col) from the matrix A.
 *
 * \param A Source matrix.
 * \param row Starting row.
 * \param col Starting column.
 *
 * \ingroup Assignment
 */ 

static inline word mzed_read_elem(const mzed_t *A, const rci_t row, const rci_t col) {
  return __mzd_read_bits(A->x, row, A->w*col, A->w);
}

/**
 * At the element elem to the element at position (row,col) in the matrix A.
 *
 * \param A Target matrix.
 * \param row Starting row.
 * \param col Starting column.
 * \param elem finite field element.
 *
 * \ingroup Assignment
 */ 

static inline void mzed_add_elem(mzed_t *a, const rci_t row, const rci_t col, const word elem) {
  __mzd_xor_bits(a->x, row, a->w*col, a->w, elem);
}

/**
 * Write the element elem to the position (row,col) in the matrix A.
 *
 * \param A Target matrix.
 * \param row Starting row.
 * \param col Starting column.
 * \param elem finite field element.
 *
 * \ingroup Assignment
 */ 

static inline void mzed_write_elem(mzed_t *a, const rci_t row, const rci_t col, const word elem) {
  __mzd_clear_bits(a->x, row, a->w*col, a->w);
  __mzd_xor_bits(a->x, row, a->w*col, a->w, elem);
}

/**
 * \brief Return -1,0,1 if if A < B, A == B or A > B respectively.
 *
 * \param A Matrix.
 * \param B Matrix.
 *
 * \note This comparison is not well defined mathematically and
 * relatively arbitrary since elements of GF(2^k) don't have an
 * ordering.
 *
 * \ingroup Comparison
 */

static inline int mzed_cmp(mzed_t *A, mzed_t *B) {
  return mzd_cmp(A->x,B->x);
}


/**
 * \brief Zero test for matrix.
 *
 * \param A Input matrix.
 *
 * \ingroup Comparison
 */
static inline int mzed_is_zero(mzed_t *A) {
  return mzd_is_zero(A->x);
}

/**
 *  Compute A[ar,c] = A[ar,c] + X*B[br,c] for all c >= startcol.
 *
 * \param A Matrix.
 * \param ar Row index in A.
 * \param B Matrix.
 * \param br Row index in B.
 * \param X Lookup table for multiplication with some finite field element x.
 * \param start_col Column index.
 *
 * \ingroup RowOperations
 */

void mzed_add_multiple_of_row(mzed_t *A, rci_t ar, const mzed_t *B, rci_t br, word *X, rci_t start_col);

/**
 * \brief Recale the row r in A by X starting c.
 * 
 * \param A Matrix
 * \param r Row index.
 * \param start_col Column index.
 * \param X Multiplier 
 *
 * \ingroup RowOperations
 */

static inline void mzed_rescale_row(mzed_t *A, rci_t r, rci_t start_col, const word *X) {
  assert(A->x->offset == 0);
  assert(start_col < A->ncols);

  const size_t startblock = (A->w*start_col)/m4ri_radix;
  word *_a = A->x->rows[r];
  int j;
  register word __a, __t;

  if(A->w == 2) {    
    const word bitmask_begin = __M4RI_RIGHT_BITMASK(m4ri_radix - ((2*start_col)%m4ri_radix));
    const word bitmask_end = __M4RI_LEFT_BITMASK(A->x->ncols % m4ri_radix);

    __a = _a[startblock]>>(2*(start_col%32));
    __t = 0;
    switch(start_col % 32) {
    case  0:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<0;   __a >>= 2;
    case  1:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<2;   __a >>= 2;
    case  2:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<4;   __a >>= 2;
    case  3:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<6;   __a >>= 2;
    case  4:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<8;   __a >>= 2;
    case  5:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<10;  __a >>= 2;
    case  6:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<12;  __a >>= 2;
    case  7:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<14;  __a >>= 2;
    case  8:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<16;  __a >>= 2;
    case  9:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<18;  __a >>= 2;
    case 10:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<20;  __a >>= 2;
    case 11:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<22;  __a >>= 2;
    case 12:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<24;  __a >>= 2;
    case 13:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<26;  __a >>= 2;
    case 14:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<28;  __a >>= 2;
    case 15:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<30;  __a >>= 2;
    case 16:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<32;  __a >>= 2;
    case 17:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<34;  __a >>= 2;
    case 18:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<36;  __a >>= 2;
    case 19:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<38;  __a >>= 2;
    case 20:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<40;  __a >>= 2;
    case 21:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<42;  __a >>= 2;
    case 22:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<44;  __a >>= 2;
    case 23:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<46;  __a >>= 2;
    case 24:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<48;  __a >>= 2;
    case 25:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<50;  __a >>= 2;
    case 26:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<52;  __a >>= 2;
    case 27:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<54;  __a >>= 2;
    case 28:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<56;  __a >>= 2;
    case 29:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<58;  __a >>= 2;
    case 30:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<60;  __a >>= 2;
    case 31:  __t ^= (X[((__a)& 0x0000000000000003ULL)])<<62;  break;
    default: m4ri_die("impossible");
    }
    if(A->x->width-startblock == 1) {
      _a[startblock] &= ~(bitmask_begin & bitmask_end);
      _a[startblock] ^= __t & bitmask_begin & bitmask_end;
      return;
    } else {
      _a[startblock] &= ~bitmask_begin;
      _a[startblock] ^= __t & bitmask_begin;
    }

    for(j=startblock+1; j<A->x->width -1; j++) {
      __a = _a[j], __t = 0;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<0;   __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<2;   __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<4;   __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<6;   __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<8;   __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<10;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<12;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<14;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<16;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<18;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<20;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<22;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<24;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<26;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<28;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<30;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<32;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<34;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<36;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<38;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<40;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<42;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<44;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<46;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<48;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<50;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<52;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<54;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<56;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<58;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<60;  __a >>= 2;
      __t ^= (X[((__a)& 0x0000000000000003ULL)])<<62;
      _a[j] = __t;
    }

    __t = _a[j] & ~bitmask_end;
    switch(A->x->ncols % m4ri_radix) {
    case  0: __t ^= ((word)X[(int)((_a[j] & 0xC000000000000000ULL)>>62)])<<62;
    case 62: __t ^= ((word)X[(int)((_a[j] & 0x3000000000000000ULL)>>60)])<<60;
    case 60: __t ^= ((word)X[(int)((_a[j] & 0x0C00000000000000ULL)>>58)])<<58;
    case 58: __t ^= ((word)X[(int)((_a[j] & 0x0300000000000000ULL)>>56)])<<56;
    case 56: __t ^= ((word)X[(int)((_a[j] & 0x00C0000000000000ULL)>>54)])<<54;
    case 54: __t ^= ((word)X[(int)((_a[j] & 0x0030000000000000ULL)>>52)])<<52;
    case 52: __t ^= ((word)X[(int)((_a[j] & 0x000C000000000000ULL)>>50)])<<50;
    case 50: __t ^= ((word)X[(int)((_a[j] & 0x0003000000000000ULL)>>48)])<<48;
    case 48: __t ^= ((word)X[(int)((_a[j] & 0x0000C00000000000ULL)>>46)])<<46;
    case 46: __t ^= ((word)X[(int)((_a[j] & 0x0000300000000000ULL)>>44)])<<44;
    case 44: __t ^= ((word)X[(int)((_a[j] & 0x00000C0000000000ULL)>>42)])<<42;
    case 42: __t ^= ((word)X[(int)((_a[j] & 0x0000030000000000ULL)>>40)])<<40;
    case 40: __t ^= ((word)X[(int)((_a[j] & 0x000000C000000000ULL)>>38)])<<38;
    case 38: __t ^= ((word)X[(int)((_a[j] & 0x0000003000000000ULL)>>36)])<<36;
    case 36: __t ^= ((word)X[(int)((_a[j] & 0x0000000C00000000ULL)>>34)])<<34;
    case 34: __t ^= ((word)X[(int)((_a[j] & 0x0000000300000000ULL)>>32)])<<32;
    case 32: __t ^= ((word)X[(int)((_a[j] & 0x00000000C0000000ULL)>>30)])<<30;
    case 30: __t ^= ((word)X[(int)((_a[j] & 0x0000000030000000ULL)>>28)])<<28;
    case 28: __t ^= ((word)X[(int)((_a[j] & 0x000000000C000000ULL)>>26)])<<26;
    case 26: __t ^= ((word)X[(int)((_a[j] & 0x0000000003000000ULL)>>24)])<<24;
    case 24: __t ^= ((word)X[(int)((_a[j] & 0x0000000000C00000ULL)>>22)])<<22;
    case 22: __t ^= ((word)X[(int)((_a[j] & 0x0000000000300000ULL)>>20)])<<20;
    case 20: __t ^= ((word)X[(int)((_a[j] & 0x00000000000C0000ULL)>>18)])<<18;
    case 18: __t ^= ((word)X[(int)((_a[j] & 0x0000000000030000ULL)>>16)])<<16;
    case 16: __t ^= ((word)X[(int)((_a[j] & 0x000000000000C000ULL)>>14)])<<14;
    case 14: __t ^= ((word)X[(int)((_a[j] & 0x0000000000003000ULL)>>12)])<<12;
    case 12: __t ^= ((word)X[(int)((_a[j] & 0x0000000000000C00ULL)>>10)])<<10;
    case 10: __t ^= ((word)X[(int)((_a[j] & 0x0000000000000300ULL)>> 8)])<< 8;
    case  8: __t ^= ((word)X[(int)((_a[j] & 0x00000000000000C0ULL)>> 6)])<< 6;
    case  6: __t ^= ((word)X[(int)((_a[j] & 0x0000000000000030ULL)>> 4)])<< 4;
    case  4: __t ^= ((word)X[(int)((_a[j] & 0x000000000000000CULL)>> 2)])<< 2;
    case  2: __t ^= ((word)X[(int)((_a[j] & 0x0000000000000003ULL)>> 0)])<< 0;
    };
    _a[j] = __t;

  } else if(A->w == 4) {

    const word bitmask_begin = __M4RI_RIGHT_BITMASK(m4ri_radix - ((4*start_col)%m4ri_radix));
    const word bitmask_end = __M4RI_LEFT_BITMASK(A->x->ncols % m4ri_radix);

    __a = _a[startblock]>>(4*(start_col%16));
    __t = 0;
    switch(start_col%16) {
    case  0: __t ^= (X[((__a)& 0x000000000000000FULL)])<<0;   __a >>= 4;
    case  1: __t ^= (X[((__a)& 0x000000000000000FULL)])<<4;   __a >>= 4;
    case  2: __t ^= (X[((__a)& 0x000000000000000FULL)])<<8;   __a >>= 4;
    case  3: __t ^= (X[((__a)& 0x000000000000000FULL)])<<12;  __a >>= 4;
    case  4: __t ^= (X[((__a)& 0x000000000000000FULL)])<<16;  __a >>= 4;
    case  5: __t ^= (X[((__a)& 0x000000000000000FULL)])<<20;  __a >>= 4;
    case  6: __t ^= (X[((__a)& 0x000000000000000FULL)])<<24;  __a >>= 4;
    case  7: __t ^= (X[((__a)& 0x000000000000000FULL)])<<28;  __a >>= 4;
    case  8: __t ^= (X[((__a)& 0x000000000000000FULL)])<<32;  __a >>= 4;
    case  9: __t ^= (X[((__a)& 0x000000000000000FULL)])<<36;  __a >>= 4;
    case 10: __t ^= (X[((__a)& 0x000000000000000FULL)])<<40;  __a >>= 4;
    case 11: __t ^= (X[((__a)& 0x000000000000000FULL)])<<44;  __a >>= 4;
    case 12: __t ^= (X[((__a)& 0x000000000000000FULL)])<<48;  __a >>= 4;
    case 13: __t ^= (X[((__a)& 0x000000000000000FULL)])<<52;  __a >>= 4;
    case 14: __t ^= (X[((__a)& 0x000000000000000FULL)])<<56;  __a >>= 4;
    case 15: __t ^= (X[((__a)& 0x000000000000000FULL)])<<60;  break;
    default: m4ri_die("impossible");
    }
    if(A->x->width-startblock == 1) {
      _a[startblock] &= ~(bitmask_begin & bitmask_end);
      _a[startblock] ^= __t & bitmask_begin & bitmask_end;
      return;
    } else {
      _a[startblock] &= ~bitmask_begin;
      _a[startblock] ^= __t & bitmask_begin;
    }

    for(j=startblock+1; j<A->x->width -1; j++) {
      __a = _a[j], __t = 0;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<0;   __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<4;   __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<8;   __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<12;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<16;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<20;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<24;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<28;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<32;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<36;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<40;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<44;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<48;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<52;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<56;  __a >>= 4;
      __t ^= (X[((__a)& 0x000000000000000FULL)])<<60;
      _a[j] = __t;
    }

    __t = _a[j] & ~bitmask_end;
    switch(A->x->ncols % m4ri_radix) {
    case  0: __t ^= ((word)X[(int)((_a[j] & 0xF000000000000000ULL)>>60)])<<60;
    case 60: __t ^= ((word)X[(int)((_a[j] & 0x0F00000000000000ULL)>>56)])<<56;
    case 56: __t ^= ((word)X[(int)((_a[j] & 0x00F0000000000000ULL)>>52)])<<52;
    case 52: __t ^= ((word)X[(int)((_a[j] & 0x000F000000000000ULL)>>48)])<<48;
    case 48: __t ^= ((word)X[(int)((_a[j] & 0x0000F00000000000ULL)>>44)])<<44;
    case 44: __t ^= ((word)X[(int)((_a[j] & 0x00000F0000000000ULL)>>40)])<<40;
    case 40: __t ^= ((word)X[(int)((_a[j] & 0x000000F000000000ULL)>>36)])<<36;
    case 36: __t ^= ((word)X[(int)((_a[j] & 0x0000000F00000000ULL)>>32)])<<32;
    case 32: __t ^= ((word)X[(int)((_a[j] & 0x00000000F0000000ULL)>>28)])<<28;
    case 28: __t ^= ((word)X[(int)((_a[j] & 0x000000000F000000ULL)>>24)])<<24;
    case 24: __t ^= ((word)X[(int)((_a[j] & 0x0000000000F00000ULL)>>20)])<<20;
    case 20: __t ^= ((word)X[(int)((_a[j] & 0x00000000000F0000ULL)>>16)])<<16;
    case 16: __t ^= ((word)X[(int)((_a[j] & 0x000000000000F000ULL)>>12)])<<12;
    case 12: __t ^= ((word)X[(int)((_a[j] & 0x0000000000000F00ULL)>> 8)])<< 8;
    case  8: __t ^= ((word)X[(int)((_a[j] & 0x00000000000000F0ULL)>> 4)])<< 4;
    case  4: __t ^= ((word)X[(int)((_a[j] & 0x000000000000000FULL)>> 0)])<< 0;
    };
    _a[j] = __t;

  } else if (A->w == 8) {

    const word bitmask_begin = __M4RI_RIGHT_BITMASK(m4ri_radix - ((8*start_col)%m4ri_radix));
    const word bitmask_end = __M4RI_LEFT_BITMASK(A->x->ncols % m4ri_radix);

    register word __a0 = _a[startblock]>>(8*(start_col%8));
    register word __a1;
    register word __t0 = 0;
    register word __t1;

    switch(start_col%8) {
    case 0: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<0;  __a0 >>= 8;
    case 1: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<8;  __a0 >>= 8;
    case 2: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<16; __a0 >>= 8;
    case 3: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<24; __a0 >>= 8;
    case 4: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<32; __a0 >>= 8;
    case 5: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<40; __a0 >>= 8;
    case 6: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<48; __a0 >>= 8;
    case 7: __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<56; break;
    default: m4ri_die("impossible");
    }
    if(A->x->width-startblock == 1) {
      _a[startblock] &= ~(bitmask_begin & bitmask_end);
      _a[startblock] ^= __t0 & bitmask_begin & bitmask_end;
      return;
    } else {
      _a[startblock] &= ~bitmask_begin;
      _a[startblock] ^= __t0 & bitmask_begin;
    }

    for(j=startblock+1; j+2 < A->x->width; j+=2) {
      __a0 = _a[j], __t0 = 0;
      __a1 = _a[j+1], __t1 = 0;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<0;  __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<0;  __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<8;  __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<8;  __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<16; __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<16; __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<24; __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<24; __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<32; __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<32; __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<40; __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<40; __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<48; __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<48; __a1 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<56; __a0 >>= 8;
      __t1 ^= (X[((__a1)& 0x00000000000000FFULL)])<<56;
      _a[j] = __t0;
      _a[j+1] = __t1;
    }

    for(; j < A->x->width-1; j++) {
      __a0 = _a[j], __t0 = 0;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<0;  __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<8;  __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<16; __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<24; __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<32; __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<40; __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<48; __a0 >>= 8;
      __t0 ^= (X[((__a0)& 0x00000000000000FFULL)])<<56;
      _a[j] = __t0;
    }
    
    __t = _a[j] & ~bitmask_end;
    switch(A->x->ncols % m4ri_radix) {
    case  0: __t ^= ((word)X[(int)((_a[j] & 0xFF00000000000000ULL)>>56)])<<56;
    case 56: __t ^= ((word)X[(int)((_a[j] & 0x00FF000000000000ULL)>>48)])<<48;
    case 48: __t ^= ((word)X[(int)((_a[j] & 0x0000FF0000000000ULL)>>40)])<<40;
    case 40: __t ^= ((word)X[(int)((_a[j] & 0x000000FF00000000ULL)>>32)])<<32;
    case 32: __t ^= ((word)X[(int)((_a[j] & 0x00000000FF000000ULL)>>24)])<<24;
    case 24: __t ^= ((word)X[(int)((_a[j] & 0x0000000000FF0000ULL)>>16)])<<16;
    case 16: __t ^= ((word)X[(int)((_a[j] & 0x000000000000FF00ULL)>> 8)])<< 8;
    case  8: __t ^= ((word)X[(int)((_a[j] & 0x00000000000000FFULL)>> 0)])<< 0;
    };
    _a[j] = __t;

  } else if (A->w == 16) {
    const word bitmask_begin = __M4RI_RIGHT_BITMASK(m4ri_radix - ((16*start_col)%m4ri_radix));
    const word bitmask_end = __M4RI_LEFT_BITMASK(A->x->ncols % m4ri_radix);

    __a = _a[startblock]>>(16*(start_col%4));
    __t = 0;
    switch(start_col%4) {
    case 0: __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<0;  __a >>= 16;
    case 1: __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<16; __a >>= 16;
    case 2: __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<32; __a >>= 16;
    case 3: __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<48; break;
    default: m4ri_die("impossible");
    }
    if(A->x->width-startblock == 1) {
      _a[startblock] &= ~(bitmask_begin & bitmask_end);
      _a[startblock] ^= __t & bitmask_begin & bitmask_end;
      return;
    } else {
      _a[startblock] &= ~bitmask_begin;
      _a[startblock] ^= __t & bitmask_begin;
    }

    for(j=startblock+1; j+4<A->x->width; j+=4) {
      __a = _a[j], __t = 0;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<0;  __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<16; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<32; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<48;
      _a[j] = __t;

      __a = _a[j+1], __t = 0;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<0;  __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<16; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<32; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<48;
      _a[j+1] = __t;


      __a = _a[j+2], __t = 0;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<0;  __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<16; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<32; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<48;
      _a[j+2] = __t;

      __a = _a[j+3], __t = 0;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<0;  __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<16; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<32; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<48;
      _a[j+3] = __t;
    }
    for( ; j<A->x->width-1; j++) {
      __a = _a[j], __t = 0;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<0;  __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<16; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<32; __a >>= 16;
      __t ^= (X[((__a)& 0x000000000000FFFFULL)])<<48;
      _a[j] = __t;
    }

    __t = _a[j] & ~bitmask_end;
    switch(A->x->ncols % m4ri_radix) {
    case  0: __t ^= ((word)X[(int)((_a[j] & 0xFFFF000000000000ULL)>>48)])<<48;
    case 48: __t ^= ((word)X[(int)((_a[j] & 0x0000FFFF00000000ULL)>>32)])<<32;
    case 32: __t ^= ((word)X[(int)((_a[j] & 0x00000000FFFF0000ULL)>>16)])<<16;
    case 16: __t ^= ((word)X[(int)((_a[j] & 0x000000000000FFFFULL)>> 0)])<< 0;
    };
    _a[j] = __t;

  } else {
    for(rci_t j=start_col; j<A->ncols; j++) {
      mzed_write_elem(A, r, j, X[mzed_read_elem(A, r, j)]);
    }
  }
}

/**
 * \brief Swap the two rows rowa and rowb.
 * 
 * \param M Matrix
 * \param rowa Row index.
 * \param rowb Row index.
 *
 * \ingroup RowOperations
 *
 * \wordoffset
 */

static inline void mzed_row_swap(mzed_t *M, const rci_t rowa, const rci_t rowb) {
  mzd_row_swap(M->x, rowa, rowb);
}

/**
 * \brief copy row j from A to row i from B.
 *
 * The offsets of A and B must match and the number of columns of A
 * must be less than or equal to the number of columns of B.
 *
 * \param B Target matrix.
 * \param i Target row index.
 * \param A Source matrix.
 * \param j Source row index.
 *
 * \ingroup RowOperations
 */

static inline void mzed_copy_row(mzed_t* B, rci_t i, const mzed_t* A, rci_t j) {
  mzd_copy_row(B->x, i, A->x, j);
}

/**
 * \brief Swap the two columns cola and colb.
 * 
 * \param M Matrix.
 * \param cola Column index.
 * \param colb Column index.
 *
 * \ingroup RowOperations
 */
 
static inline void mzed_col_swap(mzed_t *M, const rci_t cola, const rci_t colb) {
  for(rci_t i=0; i<M->w; i++)
    mzd_col_swap(M->x,M->w*cola+i, M->w*colb+i);
}

/**
 * \brief Swap the two columns cola and colb but only between start_row and stop_row.
 * 
 * \param M Matrix.
 * \param cola Column index.
 * \param colb Column index.
 * \param start_row Row index.
 * \param stop_row Row index (exclusive).
 *
 * \ingroup RowOperations
 */

static inline void mzed_col_swap_in_rows(mzed_t *A, const rci_t cola, const rci_t colb, const rci_t start_row, rci_t stop_row) {
  for(int e=0; e < A->finite_field->degree; e++) {
    mzd_col_swap_in_rows(A->x, A->w*cola+e, A->w*colb+e, start_row, stop_row);
  };
}

/**
 * \brief Add the rows sourcerow and destrow and stores the total in
 * the row destrow.
 *
 * \param M Matrix
 * \param sourcerow Index of source row
 * \param destrow Index of target row
 *
 * \note this can be done much faster with mzed_combine.
 *
 * \ingroup RowOperations
 */

static inline void mzed_row_add(mzed_t *M, const rci_t sourcerow, const rci_t destrow) {
  mzd_row_add(M->x, sourcerow, destrow);
}

/**
 * \brief Return the first row with all zero entries.
 *
 * If no such row can be found returns nrows.
 *
 * \param A Matrix
 *
 * \ingroup RowOperations
 */

static inline rci_t mzed_first_zero_row(mzed_t *A) {
  return mzd_first_zero_row(A->x);
}


/**
 * \brief Clear the given row, but only begins at the column coloffset.
 *
 * \param M Matrix
 * \param row Index of row
 * \param coloffset Column offset
 *
 * \ingroup RowOperations
 */

static inline void mzed_row_clear_offset(mzed_t *M, const rci_t row, const rci_t coloffset) {
  mzd_row_clear_offset(M->x, row, coloffset*M->w);
}

/**
 * \brief Gaussian elimination.
 * 
 * Perform Gaussian elimination on the matrix A.  If full=0, then it
 * will do triangular style elimination, and if full=1, it will do
 * Gauss-Jordan style, or full elimination.
 *
 * \param A Matrix
 * \param full Gauss-Jordan style or upper unit-triangular form only.
 *
 * \ingroup Echelon
 */

rci_t mzed_echelonize_naive(mzed_t *A, int full);

/**
 * \brief Print a matrix to stdout. 
 *
 * The output will contain colons between every 4-th column.
 *
 * \param M Matrix
 *
 * \ingroup StringConversions
 */

void mzed_print(const mzed_t *M);

#endif //M4RIE_MATRIX_H
