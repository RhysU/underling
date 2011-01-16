//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// underling 0.0.1: underling library for parallel, 3D pencil decompositions
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

#ifndef __TEST_UNDERLING_TOOLS_HPP
#define __TEST_UNDERLING_TOOLS_HPP

#include <mpi.h>
#include <fftw3-mpi.h>
#include <underling/underling.hpp>

/** A test fixture to setup and teardown MPI and FFTW MPI */
struct FFTWMPIFixture {

    FFTWMPIFixture() {
        MPI_Init(NULL, NULL); // NULL valid per MPI standard section 8.7
#ifdef HAVE_FFTW3_THREADS
        assert(fftw_init_threads());
        int nthreads = 1;
#if defined HAVE_OPENMP
        const char * const envstr = getenv("OMP_NUM_THREADS");
        if (envstr) {
            const int envnum = atoi(envstr);
            if (envnum > 0) nthreads = envnum;
        }
#elif defined HAVE_PTHREAD
    // TODO Provide sane nthreads default for FFTW pthread environment
#else
#error "Sanity check failed; unknown FFTW threading model in use."
#endif
        BOOST_TEST_MESSAGE("Using FFTW with " << nthreads << " threads.");
        fftw_plan_with_nthreads(nthreads);
#endif
        fftw_mpi_init();
    }

    ~FFTWMPIFixture() {
        fftw_mpi_cleanup();
#ifdef HAVE_FFTW3_THREADS
        fftw_cleanup_threads();
#endif
        MPI_Finalize();
    }
};

/* A test fixture to setup and teardown an underling use case */
struct UnderlingFixture {

    UnderlingFixture(MPI_Comm comm,
                     const int n0, const int n1, const int n2,
                     const int howmany,
                     const unsigned transposed_flags = 0)
        : grid(comm, n0, n1, n2),
          problem(grid, howmany, transposed_flags),
          data((underling_real *) fftw_malloc(
                    problem.local_memory()*sizeof(underling_real)),
                &fftw_free),
          plan(problem, data.get(),
               underling::transpose::all, FFTW_ESTIMATE)
    {
        BOOST_REQUIRE(grid);
        BOOST_REQUIRE(problem);
        BOOST_REQUIRE(data);
        BOOST_REQUIRE(plan);
    }

    underling::grid grid;
    underling::problem problem;
    boost::shared_array<underling_real> data;
    underling::plan plan;

};

#endif // __TEST_UNDERLING_TOOLS_HPP
