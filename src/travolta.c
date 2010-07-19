
/******************************************************************************
*
*            M4RIE: Linear Algebra over GF(2^e)
*
*    Copyright (C) 2010 Martin Albrecht <martinralbrecht@googlemail.com>
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

#include "travolta.h"

/**
 * \brief Perform Gaussian reduction to reduced row echelon form on a
 * submatrix.
 * 
 * The submatrix has dimension at most k starting at r x c of A. Checks
 * for pivot rows up to row endrow (exclusive). Terminates as soon as
 * finding a pivot column fails.
 *
 * \param A Matrix.
 * \param r First row.
 * \param c First column.
 * \param k Maximal dimension of identity matrix to produce.
 * \param end_row Maximal row index (exclusive) for rows to consider
 * for inclusion.
 */

int _mzed_gauss_submatrix_full(mzed_t *A, size_t r, size_t c, size_t end_row, int k) {
  size_t i,j,l;
  size_t start_row = r;
  int found;
  word tmp;

  gf2e *ff = A->finite_field;

  for (j=c; j<c+k; j++) {
    found = 0;
    for (i=start_row; i< end_row; i++) {
      /* first we need to clear the first columns */
      for (l=0; l<j-c; l++) {
        tmp = mzed_read_elem(A, i, c+l);
        if (tmp) mzed_add_multiple_of_row(A, i, A, r+l, ff->mul[tmp], c+l);
      }
      /* pivot? */
      const word x = mzed_read_elem(A, i, j);
      if (x) {
        mzed_rescale_row(A, i, j, ff->mul[ff->inv[x]]);
        mzd_row_swap(A->x, i, start_row);

        /* clear above */
        for (l=r; l<start_row; l++) {
          tmp = mzed_read_elem(A, l, j);
          if (tmp) mzed_add_multiple_of_row(A, l, A, start_row, ff->mul[tmp], j);
        }
        start_row++;
        found = 1;
        break;
      }
    }
    if (found==0) {
      return j - c;
    }
  }
  return j - c;
}


void mzed_make_table(mzed_t *A, size_t r, size_t c, mzed_t *T,  size_t *L, gf2e *ff) {
  mzd_set_ui(T->x,0);

  for(size_t i=0; i< TWOPOW(ff->degree); i++) {
    word *X = ff->mul[i];
    L[i] = i;
    mzed_add_multiple_of_row(T, i, A, r, X, c);
  }
}

size_t mzed_echelonize_travolta(mzed_t *A, int full) {
  gf2e* ff = A->finite_field;

  size_t r,c;

  size_t k = ff->degree;

  /** cf. mzd_echelonize_m4ri **/
  size_t kk = m4ri_opt_k(A->x->nrows, A->x->ncols, 0);
  if (kk>=7) 
    kk = 7;
  if ( (6*(1<<kk)*A->ncols / 8.0) > CPU_L2_CACHE / 2.0 )
    kk -= 1;
  kk = (6*kk)/k;

  /** enforcing bounds **/
  if (kk == 0)
    kk = 1;
  else if (kk > 6)
    kk = 6;

  size_t kbar = 0;

  mzed_t *T0 = mzed_init(ff, TWOPOW(k), A->ncols);
  mzed_t *T1 = mzed_init(ff, TWOPOW(k), A->ncols);
  mzed_t *T2 = mzed_init(ff, TWOPOW(k), A->ncols);
  mzed_t *T3 = mzed_init(ff, TWOPOW(k), A->ncols);
  mzed_t *T4 = mzed_init(ff, TWOPOW(k), A->ncols);
  mzed_t *T5 = mzed_init(ff, TWOPOW(k), A->ncols);
  
  /* this is dummy, we keep it for compatibility with the M4RI functions */
  size_t *L0 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L1 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L2 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L3 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L4 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L5 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));


  r = 0;
  c = 0;
  while(c < A->ncols) {
    /**
     * \todo: If full == False we should switch over to naive once the
     *        remain matrix is small.
     */
    if(c+kk > A->ncols) kk = A->ncols - c;

    /**
     * \todo: we don't really compute the upper triangular form yet,
     *        we need to implement _mzed_gauss_submatrix and a better
     *        table creation for that.
     */ 
    kbar = _mzed_gauss_submatrix_full(A, r, c, A->nrows, kk);

    if (kbar == 6)  {
      mzed_make_table( A, r,   c,   T0, L0, ff);
      mzed_make_table( A, r+1, c+1, T1, L1, ff);
      mzed_make_table( A, r+2, c+2, T2, L2, ff);
      mzed_make_table( A, r+3, c+3, T3, L3, ff);
      mzed_make_table( A, r+4, c+4, T4, L4, ff);
      mzed_make_table( A, r+5, c+5, T5, L5, ff);
      if(kbar == kk)
        mzed_process_rows6( A, r+6, A->nrows, c, T0, L0, T1, L1, T2, L2, T3, L3, T4, L4, T5, L5);
      if(full)
        mzed_process_rows6( A,   0,        r, c, T0, L0, T1, L1, T2, L2, T3, L3, T4, L4, T5, L5);
    } else if(kbar == 5) {
      mzed_make_table( A, r,     c, T0, L0, ff);
      mzed_make_table( A, r+1, c+1, T1, L1, ff);
      mzed_make_table( A, r+2, c+2, T2, L2, ff);
      mzed_make_table( A, r+3, c+3, T3, L3, ff);
      mzed_make_table( A, r+4, c+4, T4, L4, ff);
      if(kbar == kk)
        mzed_process_rows5( A, r+5, A->nrows, c, T0, L0, T1, L1, T2, L2, T3, L3, T4, L4);
      if(full)
        mzed_process_rows5( A,   0,        r, c, T0, L0, T1, L1, T2, L2, T3, L3, T4, L4);

    } else if(kbar == 4) {
      mzed_make_table( A, r,   c,   T0, L0, ff);
      mzed_make_table( A, r+1, c+1, T1, L1, ff);
      mzed_make_table( A, r+2, c+2, T2, L2, ff);
      mzed_make_table( A, r+3, c+3, T3, L3, ff);
      if(kbar == kk)
        mzed_process_rows4( A, r+4, A->nrows, c, T0, L0, T1, L1, T2, L2, T3, L3);
      if(full)
        mzed_process_rows4( A,   0,        r, c, T0, L0, T1, L1, T2, L2, T3, L3);

    } else if(kbar == 3) {
      mzed_make_table( A, r,   c,   T0, L0, ff);
      mzed_make_table( A, r+1, c+1, T1, L1, ff);
      mzed_make_table( A, r+2, c+2, T2, L2, ff);
      if(kbar == kk)
        mzed_process_rows3( A, r+3, A->nrows, c, T0, L0, T1, L1, T2, L2);
      if(full)
        mzed_process_rows3( A,   0,        r, c, T0, L0, T1, L1, T2, L2);

    } else if(kbar == 2) {
      mzed_make_table( A, r,   c,   T0, L0, ff);
      mzed_make_table( A, r+1, c+1, T1, L1, ff);
      if(kbar == kk)
        mzed_process_rows2( A, r+2, A->nrows, c, T0, L0, T1, L1);
      if(full)
        mzed_process_rows2( A,   0,        r, c, T0, L0, T1, L1);

    } else if (kbar == 1) {
      mzed_make_table(A, r, c, T0, L0, ff);
      if(kbar == kk)
        mzed_process_rows( A, r+1, A->nrows, c, T0, L0);
      if(full)
        mzed_process_rows( A,   0,        r, c, T0, L0);

    } else {
      c++;
    }
    r += kbar;
    c += kbar;
  }

  m4ri_mm_free(L0); m4ri_mm_free(L1); m4ri_mm_free(L2);
  m4ri_mm_free(L3); m4ri_mm_free(L4); m4ri_mm_free(L5);
  mzed_free(T0); mzed_free(T1); mzed_free(T2);
  mzed_free(T3); mzed_free(T4); mzed_free(T5);
  return r;
}

mzed_t *_mzed_mul_travolta0(mzed_t *C, mzed_t *A, mzed_t *B) {
  mzed_t *T0 = mzed_init(C->finite_field, TWOPOW(A->finite_field->degree), B->ncols);
  size_t *L0 = (size_t*)m4ri_mm_calloc(TWOPOW(A->finite_field->degree), sizeof(size_t));

  for(size_t i=0; i < A->ncols; i++) {
    mzed_make_table(B, i, 0, T0, L0, A->finite_field);
    for(size_t j=0; j<A->nrows; j++)
      mzd_combine(C->x, j, 0, C->x, j, 0, T0->x, mzed_read_elem(A, j, i), 0);
  }
  mzed_free(T0);
  m4ri_mm_free(L0);
  return C;
}

mzed_t *_mzed_mul_travolta1(mzed_t *C, mzed_t *A, mzed_t *B) {
  mzed_t *T0 = mzed_init(C->finite_field, TWOPOW(A->finite_field->degree), B->ncols);
  mzed_t *T1 = mzed_init(C->finite_field, TWOPOW(A->finite_field->degree), B->ncols);
  mzed_t *T2 = mzed_init(C->finite_field, TWOPOW(A->finite_field->degree), B->ncols);
  mzed_t *T3 = mzed_init(C->finite_field, TWOPOW(A->finite_field->degree), B->ncols);

  size_t *L0 = (size_t*)m4ri_mm_calloc(TWOPOW(A->finite_field->degree), sizeof(size_t));
  size_t *L1 = (size_t*)m4ri_mm_calloc(TWOPOW(A->finite_field->degree), sizeof(size_t));
  size_t *L2 = (size_t*)m4ri_mm_calloc(TWOPOW(A->finite_field->degree), sizeof(size_t));
  size_t *L3 = (size_t*)m4ri_mm_calloc(TWOPOW(A->finite_field->degree), sizeof(size_t));
  
  const size_t kk = 4;
  const size_t end = A->ncols/kk;

  for(size_t i=0; i < end; i++) {
    mzed_make_table( B, kk*i  , 0, T0, L0, A->finite_field);
    mzed_make_table( B, kk*i+1, 0, T1, L1, A->finite_field);
    mzed_make_table( B, kk*i+2, 0, T2, L2, A->finite_field);
    mzed_make_table( B, kk*i+3, 0, T3, L3, A->finite_field);
    for(size_t j=0; j<A->nrows; j++) {
      size_t x0 = L0[ mzed_read_elem(A, j, kk*  i) ];
      size_t x1 = L1[ mzed_read_elem(A, j, kk*i+1) ];
      size_t x2 = L2[ mzed_read_elem(A, j, kk*i+2) ];
      size_t x3 = L3[ mzed_read_elem(A, j, kk*i+3) ];
      mzed_combine4(C, j, T0, x0, T1, x1, T2, x2, T3, x3);
    }
  }
  if (A->ncols%kk) {
    for(size_t i=kk*end; i < A->ncols; i++) {
      mzed_make_table(B, i, 0, T0, L0, A->finite_field);
      for(size_t j=0; j<A->nrows; j++)
        mzd_combine(C->x, j, 0, C->x, j, 0, T0->x, mzed_read_elem(A, j, i), 0);
    }
  }

  mzed_free(T0);
  return C;
}

mzed_t *mzed_addmul_travolta(mzed_t *C, mzed_t *A, mzed_t *B) {
  if (C->nrows != A->nrows || C->ncols != B->ncols || C->finite_field != A->finite_field) {
    m4ri_die("mzd_mul_naive: Provided return matrix has wrong dimensions or wrong base field.\n");
  }
  return _mzed_mul_travolta1(C, A, B);
}

mzed_t *mzed_mul_travolta(mzed_t *C, mzed_t *A, mzed_t *B) {
  if (C==NULL) {
    C = mzed_init(A->finite_field, A->nrows, B->ncols);
  } else {
    if (C->nrows != A->nrows || C->ncols != B->ncols || C->finite_field != A->finite_field) {
      m4ri_die("mzd_mul_naive: Provided return matrix has wrong dimensions or wrong base field.\n");
    }
    mzd_set_ui(C->x, 0);
  }
  return _mzed_mul_travolta1(C, A, B);
}


