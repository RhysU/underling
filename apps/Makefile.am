## Process this file with automake to produce Makefile.in

AM_CFLAGS       = $(GRVY_CFLAGS) $(FFTW3_THREADS_CFLAGS)
AM_CPPFLAGS     = -I$(top_builddir)/gnulib -I$(top_srcdir)/gnulib
AM_CPPFLAGS    += -I$(top_builddir)        -I$(top_srcdir)
AM_LDFLAGS      = # Append below
noinst_PROGRAMS = # Append below
noinst_HEADERS  = mpi_argp.h
bin_PROGRAMS    = # Append below
BUILT_DOCS      = # Append below
BUILT_SOURCES   = # Append below
CLEANFILES      = # Append below

# Use MPI compiler wrappers
CC=@MPICC@

## C-based benchmarking tool
bin_PROGRAMS             += underling_bench
underling_bench_SOURCES   = underling_bench.c mpi_argp.c
underling_bench_LDADD     = ../underling/libunderling.la
underling_bench_LDADD    += ../gnulib/libgnu.la $(LTLIBINTL)
underling_bench_LDADD    += $(GRVY_LIBS)

# Additional files to clean
CLEANFILES += *.gcno *.gcda

# License updates
include $(top_srcdir)/license.am

# Automatic dependency generation for Fortran
SFMAKEDEPEND_FLAGS = -i underling
include $(top_srcdir)/sfmakedepend.am

# Required for AX_AM_MACROS
@INC_AMINCLUDE@
