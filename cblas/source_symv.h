/* blas/source_symv.h
 * 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Gerard Jungman
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Author:  G. Jungman
 * RCS:     $Id$
 */

{
    size_t i, j;

    if (alpha == 0.0 && beta == 1.0)
	return;

    /* form  y := beta*y */
    if (beta == 0.0) {
        size_t iy = OFFSET(N, incY);
	for (i = 0; i < N; i++) {
	    Y[iy] = 0.0;
	    iy += incY;
	}
    } else if (beta != 1.0) {
	size_t iy = OFFSET(N, incY);
	for (i = 0; i < N; i++) {
	    Y[iy] *= beta;
	    iy += incY;
	}
    }

    if (alpha == 0.0)
	return;

    /* form  y := alpha*A*x + y */

    if ((order == CblasRowMajor && Uplo == CblasUpper)
        || (order == CblasColMajor && Uplo == CblasLower)) {
      size_t ix = OFFSET(N, incX);
      size_t iy = OFFSET(N, incY);
      for (i = 0; i < N; i++) {
        BASE temp1 = alpha * X[ix];
        BASE temp2 = 0.0;
        const size_t j_min = i + 1;
        const size_t j_max = N;
        size_t jx = OFFSET(N, incX) + j_min * incX;
        size_t jy = OFFSET(N, incY) + j_min * incY;
        Y[iy] += temp1 * A[lda * i + i];
        for (j = j_min; j < j_max; j++) {
          Y[jy] += temp1 * A[lda * i + j];
          temp2 += X[jx] * A[lda * i + j];
          jx += incX;
          jy += incY;
        }
        Y[iy] += alpha * temp2;
        ix += incX;
        iy += incY;
      }
    } else if ((order == CblasRowMajor && Uplo == CblasLower)
               || (order == CblasColMajor && Uplo == CblasUpper)) {
      size_t ix = OFFSET(N, incX) + (N - 1) * incX;
      size_t iy = OFFSET(N, incY) + (N - 1) * incY;
      for (i = N; i > 0 && i--;) {
        BASE temp1 = alpha * X[ix];
        BASE temp2 = 0.0;
        const size_t j_min = 0;
        const size_t j_max = i;
        size_t jx = OFFSET(N, incX) + j_min * incX;
        size_t jy = OFFSET(N, incY) + j_min * incY;
        Y[iy] += temp1 * A[lda * i + i];
        for (j = j_min; j < j_max; j++) {
          Y[jy] += temp1 * A[lda * i + j];
          temp2 += X[jx] * A[lda * i + j];
          jx += incX;
          jy += incY;
        }
        Y[iy] += alpha * temp2;
        ix -= incX;
        iy -= incY;
      }
    } else {
      BLAS_ERROR ("unrecognized operation");
    }
}