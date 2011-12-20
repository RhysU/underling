//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// underling 0.3.1: an FFTW MPI-based library for 3D pencil decompositions
// http://pecos.ices.utexas.edu/
//
// Copyright (C) 2010, 2011 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-
// $Id$
#ifndef __UNDERLING_COMMON_H
#define __UNDERLING_COMMON_H

#if __GNUC__ >= 3
/**
Provides hint to the compiler to optimize for the expression being false.
@param expr boolean expression.
@returns value of expression.
*/
#define UNDERLING_UNLIKELY(expr) __builtin_expect(expr, 0)
#else
/**
Provides hint to the compiler to optimize for the expression being false.
@param expr boolean expression.
@returns value of expression.
**/
#define UNDERLING_UNLIKELY(expr) expr
#endif

/**
 * Explicitly marks a variable as unused to avoid compiler warnings.
 *
 * @param variable Variable to be marked.
 */
#define UNDERLING_UNUSED(variable) do {(void)(variable);} while (0)

/* The MPI standard uses MPI_MAX_OBJECT_NAME for MPI_Comm_get_name, etc.  */
#ifndef MPI_MAX_OBJECT_NAME
/* mpvapich only provides MPI_MAX_NAME_STRING */
#ifdef MPI_MAX_NAME_STRING
#define MPI_MAX_OBJECT_NAME MPI_MAX_NAME_STRING
#endif /* MPI_MAX_NAME_STRING */
#endif /* MPI_MAX_OBJECT_NAME */

#endif // __UNDERLING_COMMON_H
