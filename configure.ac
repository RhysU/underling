dnl ----------------------------------------------------
dnl Package name, release number, description, and URL
dnl ----------------------------------------------------
AC_INIT([underling], [0.0.1], [rhys@ices.utexas.edu])
AC_REVISION([$Id$])
PACKAGE_DESCRIPTION="underling library for parallel, 3D pencil decompositions"
AC_SUBST([PACKAGE_DESCRIPTION])
PACKAGE_URL="http://pecos.ices.utexas.edu/"
AC_SUBST([PACKAGE_URL])

dnl ------------------------------------------------
dnl Initialization macros
dnl ------------------------------------------------
AC_CONFIG_SRCDIR([underling/underling.h])
AM_CONFIG_HEADER(underling/config.h)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AX_PREFIX_CONFIG_H([underling/underling-config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(1.11 no-define tar-ustar -Wall -Werror foreign)
AC_PREREQ([2.61])
AX_AM_MACROS
AX_ADD_AM_CTAGSFLAGS
AX_AM_JOBSERVER([no])
AX_SPLIT_VERSION
AC_SUBST([AX_MAJOR_VERSION])
AC_SUBST([AX_MINOR_VERSION])
AC_SUBST([AX_POINT_VERSION])
AC_PATH_PROG([PERL],[perl],[no])
AM_CONDITIONAL([HAVE_PERL],[test x$PERL != xno])
AC_CACHE_SAVE

dnl ------------------------------------------------
dnl Manipulate C settings and find MPI/FFTW
dnl ------------------------------------------------
AX_CHECK_ENABLE_DEBUG
AC_PROG_CC
gl_EARLY
gl_INIT
AC_LANG_PUSH([C])
if test "x$ax_enable_debug" != "xno"; then
    AX_TRACEBACK()
fi
AX_PATH_GRVY_NEW([0.27],[no])
AX_CFLAGS_WARN_ALL
AX_PROG_CC_C99_CFLAGS(,AC_MSG_ERROR([Compiler cannot support ISO C99]))
ACX_MPI(,AC_MSG_ERROR(Could not find MPI compilation tools for C, e.g. mpicc.))
MPICC_LOCATION=$(eval command -v $MPICC)
AC_SUBST([MPICC_LOCATION])
AX_MPIP()
AX_FFTW3()
if test "x$ax_fftw3_found_mpi" != "xyes"; then
    AC_MSG_ERROR([FFTW3 installation with MPI support not detected.])
fi
AX_VISIBILITY([hidden])
AX_COMPILER_VENDOR()
AX_WARNINGS_SANITIZE()
AC_LANG_POP([C])
AC_CACHE_SAVE

dnl ------------------------------------------------
dnl Manipulate C++ settings and find MPI
dnl ------------------------------------------------
AC_LANG_PUSH([C++])
ACX_MPI(,AC_MSG_ERROR(Could not find MPI compilation tools for C++, e.g. mpicxx.))
BOOST_REQUIRE([1.39])
BOOST_TEST
AC_LANG_POP([C++])
AC_CACHE_SAVE

dnl ------------------------------------------------
dnl Enable GNU libtool
dnl Current version checks also in Makefile.am
dnl ------------------------------------------------
LT_INIT([disable_static])
LT_PREREQ([2.2.6])
AC_SUBST(LIBTOOL_DEPS)
AC_CACHE_SAVE

dnl ------------------------------------------------
dnl Documentation generation
dnl ------------------------------------------------
DX_DOXYGEN_FEATURE([ON])
DX_HTML_FEATURE([ON])
DX_CHM_FEATURE([OFF])
DX_CHI_FEATURE([OFF])
DX_MAN_FEATURE([ON])
DX_RTF_FEATURE([OFF])
DX_XML_FEATURE([OFF])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([OFF])
DX_INIT_DOXYGEN([underling],[doxygen/doxygen.cfg],[docs])

#--------------------------
# Checks for code coverage
#-------------------------
AX_CODE_COVERAGE

dnl ------------------------------------------------
dnl Generate Makefiles
dnl ------------------------------------------------
AC_CONFIG_FILES([
    LICENSE
    Makefile
    underling.pc
    underling-fftw.pc
    apps/Makefile
    doxygen/Makefile
    underling/Makefile
    underling/version.h
    examples/Makefile
    gnulib/Makefile
    tests/Makefile
])

AC_OUTPUT()
