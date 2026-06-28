# ===========================================================================
#         http://autoconf-archive.cryp.to/ax_prog_cc_c99_cflags.html
# ===========================================================================
#
# SYNOPSIS
#
#   Ensure the C compiler operates in ISO C99 mode.
#
#   AX_PROG_CC_C99_CFLAGS([ACTION-IF-AVAILABLE], [ACTION-IF-UNAVAILABLE])
#
# DESCRIPTION
#
#   Historically this macro used the now-obsolete AC_PROG_CC_C99 to find
#   the flag enabling C99 mode and appended it to CFLAGS rather than CC, so
#   that other tools using CFLAGS (e.g. mpicc via AX_MPI) would also pick it
#   up.  Since Autoconf 2.70, AC_PROG_CC already selects the newest C
#   standard supported by the compiler (C99 or later) and folds any required
#   flag directly into CC, so no separate flag juggling is necessary.  This
#   macro therefore merely confirms that a C99 feature compiles with the
#   configured compiler and dispatches the optional ACTION arguments.  The
#   default actions are to do nothing.
#
# LAST MODIFICATION
#
#   2026-06-28
#
# COPYLEFT
#
#   Copyright (c) 2009 Rhys Ulerich <rhys.ulerich@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AX_PROG_CC_C99_CFLAGS],[
    AC_REQUIRE([AC_PROG_CC])
    AC_LANG_PUSH([C])
    AC_CACHE_CHECK([whether $CC supports ISO C99], [ax_cv_prog_cc_c99_cflags],
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
              #include <stdbool.h>
              #include <stdint.h>
            ]], [[
              for (int ax_i = 0; ax_i < 1; ++ax_i) { bool ax_b = true; (void) ax_b; }
              long long ax_ll = 0; (void) ax_ll;
            ]])],
            [ax_cv_prog_cc_c99_cflags=yes],
            [ax_cv_prog_cc_c99_cflags=no])])
    AS_IF([test "x$ax_cv_prog_cc_c99_cflags" = xyes],
          [m4_default([$1],[:])],
          [m4_default([$2],[:])])
    AC_LANG_POP([C])
])
