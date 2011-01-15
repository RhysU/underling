## Process this file with automake to produce Makefile.in

AM_CPPFLAGS       = -I$(top_srcdir) -I$(top_builddir)
AM_CPPFLAGS      += -I$(top_builddir)/lib -I$(top_srcdir)/lib
AM_CPPFLAGS      += $(BOOST_CPPFLAGS)
AM_LDFLAGS        = # Append below
bin_PROGRAMS      = # Append below
check_PROGRAMS    = # Append below
noinst_HEADERS    = # Append below
CLEANFILES        = # Append below
EXTRA_DIST        = # Append below
TESTS             = # Append below
TESTS_ENVIRONMENT = # Append below

noinst_HEADERS += test_tools.hpp
noinst_HEADERS += test_underling_tools.hpp

# FIXME: Ugly, separate MPI compilation into separate dir, maybe
CXX=@MPICXX@
CC=@MPICC@

# Demand that all tests build for 'make all' target to be successful.
# Much of the C++ functionality is header-only; this ensures it builds.
all-local: $(check_PROGRAMS)

TESTS              += test_error
check_PROGRAMS     += test_error
test_error_SOURCES  = test_error.cpp
test_error_LDADD    = ../suzerain/libsuzerain.la
test_error_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)
test_error_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

TESTS                += test_fftw_1D
check_PROGRAMS       += test_fftw_1D
test_fftw_1D_SOURCES  = test_fftw_1D.cpp
test_fftw_1D_CPPFLAGS = $(AM_CPPFLAGS) $(FFTW3_CFLAGS)
test_fftw_1D_LDADD    = $(FFTW3_LIBS)
test_fftw_1D_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)
test_fftw_1D_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

TESTS                  += test_underling.sh          # NB Wrapper script
EXTRA_DIST             += test_underling.sh          # NB Wrapper script
check_PROGRAMS         += test_underling
test_underling_SOURCES  = test_underling.cpp
test_underling_CPPFLAGS = $(AM_CPPFLAGS) $(FFTW3_THREADS_CFLAGS)
test_underling_LDADD    = ../suzerain/libunderling.la
test_underling_LDADD   += $(FFTW3_MPI_THREADS_LIBS)
test_underling_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)
test_underling_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

TESTS                      += test_underling_fft.sh  # NB Wrapper script
EXTRA_DIST                 += test_underling_fft.sh  # NB Wrapper script
check_PROGRAMS             += test_underling_fft
test_underling_fft_SOURCES  = test_underling_fft.cpp
test_underling_fft_CPPFLAGS = $(AM_CPPFLAGS) $(FFTW3_THREADS_CFLAGS)
test_underling_fft_LDADD    = ../suzerain/libunderling.la
test_underling_fft_LDADD   += $(FFTW3_MPI_THREADS_LIBS)
test_underling_fft_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)
test_underling_fft_LDADD   += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

# Required for AX_AM_MACROS
@INC_AMINCLUDE@
