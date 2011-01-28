//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// underling 0.1.0: underling library for parallel, 3D pencil decompositions
// http://pecos.ices.utexas.edu/
//
// Copyright (C) 2010 The PECOS Development Team
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

#endif // __UNDERLING_COMMON_H
