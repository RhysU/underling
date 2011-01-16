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

#ifndef __TEST_TOOLS_HPP
#define __TEST_TOOLS_HPP

#include <complex>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <underling/error.h>

#pragma warning(push,disable:1418)

// Like BOOST_CHECK_EQUAL_COLLECTION but uses BOOST_CHECK_CLOSE, which allows
// for a floating point tolerance on each comparison.  Have submitted a request
// to Boost Test maintainer to add an official BOOST_<level>_CLOSE_COLLECTION
// test predicate.
template<typename FPT>
bool check_close_collections(const FPT *left_begin, const FPT *left_end,
                             const FPT *right_begin, const FPT *right_end,
                             FPT percent_tolerance)
{
    const ::boost::test_tools::close_at_tolerance<FPT> is_close
        = ::boost::test_tools::close_at_tolerance<FPT>(
                ::boost::test_tools::percent_tolerance(percent_tolerance));

    int pos = 0;
    bool res = true;
    std::ostringstream errormsg;

    for( ;
         left_begin != left_end && right_begin != right_end;
         ++left_begin, ++right_begin, ++pos ) {

        bool pos_okay = true;

        if( *left_begin == 0 && *right_begin == 0 ) {
            // NOP on both identically zero
        } else if ( *left_begin == 0 ) {
            if (std::abs(*right_begin) > percent_tolerance) pos_okay = false;
        } else if ( *right_begin == 0 ) {
            if (std::abs(*left_begin) > percent_tolerance) pos_okay = false;
        } else if( !is_close(*left_begin,*right_begin) ) {
            pos_okay = false;
        }

        if (!pos_okay) {
            errormsg << "\nMismatch to tolerance "
                << percent_tolerance << "% at position " << pos << ": ";
            const ::std::ios_base::fmtflags flags = errormsg.flags();
            const ::std::streamsize         prec  = errormsg.precision();
            errormsg.flags(::std::ios::scientific | ::std::ios::showpos);
            errormsg.precision(::std::numeric_limits<FPT>::digits10 + 1);
            errormsg << *left_begin << " != " << *right_begin;
            errormsg.flags(flags);
            errormsg.precision(prec);
            res = false;
        }
    }

    if( left_begin != left_end ) {
        std::size_t r_size = pos;
        while( left_begin != left_end ) {
            ++pos;
            ++left_begin;
        }

        errormsg << "\nCollections size mismatch: " << pos << " != " << r_size;
        res = false;
    }

    if( right_begin != right_end ) {
        std::size_t l_size = pos;
        while( right_begin != right_end ) {
            ++pos;
            ++right_begin;
        }

        errormsg << "\nCollections size mismatch: " << l_size << " != " << pos;
        res = false;
    }

    BOOST_CHECK_MESSAGE(res, errormsg.str());

    return res;
}

// Like BOOST_CHECK_EQUAL_COLLECTION but uses BOOST_CHECK_SMALL and
// compares the absolute value of a complex difference.
template<typename FPT>
bool check_close_complex_collections(
        const FPT (*left_begin )[2], const FPT (*left_end )[2],
        const FPT (*right_begin)[2], const FPT (*right_end)[2],
        FPT abs_tolerance)
{
    int pos = 0;
    bool res = true;
    std::ostringstream errormsg;

    for( ;
         left_begin != left_end && right_begin != right_end;
         ++left_begin, ++right_begin, ++pos ) {

        const FPT diff_re  = (*left_begin)[0] - (*right_begin)[0];
        const FPT diff_im  = (*left_begin)[1] - (*right_begin)[1];
        const FPT diff_abs = std::sqrt(diff_re*diff_re + diff_im*diff_im);

        const bool pos_okay = diff_abs <= abs_tolerance;

        if (!pos_okay) {
            errormsg << "\nMismatch to abs tolerance "
                << abs_tolerance << " at position " << pos << ": ";
            const ::std::ios_base::fmtflags flags = errormsg.flags();
            const ::std::streamsize         prec  = errormsg.precision();
            errormsg.flags(::std::ios::scientific | ::std::ios::showpos);
            errormsg.precision(::std::numeric_limits<FPT>::digits10 + 1);
            errormsg <<'{'<<(*left_begin)[0] <<','<<(*left_begin)[1] <<'}'
                     << " != "
                     <<'{'<<(*right_begin)[0]<<','<<(*right_begin)[1]<<'}';
            errormsg.flags(flags);
            errormsg.precision(prec);
            res = false;
        }
    }

    if( left_begin != left_end ) {
        std::size_t r_size = pos;
        while( left_begin != left_end ) {
            ++pos;
            ++left_begin;
        }

        errormsg << "\nCollections size mismatch: " << pos << " != " << r_size;
        res = false;
    }

    if( right_begin != right_end ) {
        std::size_t l_size = pos;
        while( right_begin != right_end ) {
            ++pos;
            ++right_begin;
        }

        errormsg << "\nCollections size mismatch: " << l_size << " != " << pos;
        res = false;
    }

    BOOST_CHECK_MESSAGE(res, errormsg.str());

    return res;
}

/** Provides a periodic function useful for testing FFT behavior */
template<typename FPT = double, typename Integer = int>
class periodic_function {
public:

    /**
     * Produce a periodic real signal with known frequency content on domain of
     * supplied length.
     *
     * @param N Number of points in the physical domain
     * @param max_mode_exclusive Exclusive upper bound on the signal's
     *        frequency content.
     * @param shift Phase shift in the signal
     * @param length Domain size over which the signal is periodic
     * @param constant The constant content of the signal and an amplitude
     *                 factor used to scale all modes.
     */
     explicit periodic_function(const Integer N,
                                const Integer max_mode_exclusive = -1,
                                const FPT shift = M_PI/3,
                                const FPT length = 2*M_PI,
                                const FPT constant = 17)
        : N(N),
          max_mode_exclusive(
                    max_mode_exclusive >= 0 ? max_mode_exclusive : N/2+1),
          shift(shift),
          length(length),
          constant(constant)
        {
            assert(max_mode_exclusive <= (N/2+1));
        }

    /**
     * Retrieve the real-valued signal amplitude at the given location.
     *
     * @param x The desired grid location.
     *          Should be within <tt>[0,length)</tt>.
     * @param derivative Desired derivative of the signal with zero
     *        indicating the signal itself.
     *
     * @param Returns the requested value.
     */
    FPT physical_evaluate(const FPT x, const Integer derivative = 0) const;

    /**
     * Retrieve the real-valued signal amplitude at the given gridpoint.
     *
     * @param i Zero-indexed value of the desired grid location.
     *        Must be within <tt>[0,N)</tt>
     * @param derivative Desired derivative of the signal with zero
     *        indicating the signal itself.
     *
     * @param Returns the requested value.
     */
    FPT physical(const Integer i, const Integer derivative = 0) const;

    /**
     * Retrieve the requested complex-valued signal modes in wave space.
     *
     * @param i Zero-indexed mode number, where zero is the constant
     *        mode.
     * @param derivative Desired derivative of the signal with zero
     *        indicating the signal itself.
     *
     * @param Returns the requested value.
     */
    typename std::complex<FPT> wave(const Integer i,
                                    const Integer derivative = 0) const;

    const Integer N;
    const Integer max_mode_exclusive;
    const FPT shift;
    const FPT length;
    const FPT constant;
};

template<typename FPT, typename Integer>
FPT periodic_function<FPT,Integer>::physical_evaluate(
        const FPT x,
        const Integer derivative) const
{
    assert(0 <= (derivative % 4) && (derivative % 4) <= 3);

    FPT retval = (max_mode_exclusive > 0 && derivative == 0 )
        ? constant : 0;
    for (Integer j = 1; j < max_mode_exclusive; ++j) {
        switch (derivative % 4) {
            case 0:
                retval +=   j * pow(j*(2.0*M_PI/length), derivative)
                          * constant
                          * sin(j*(2.0*M_PI/length)*x + shift);
                break;
            case 1:
                retval +=   j * pow(j*(2.0*M_PI/length), derivative)
                          * constant
                          * cos(j*(2.0*M_PI/length)*x + shift);
                break;
            case 2:
                retval -=   j * pow(j*(2.0*M_PI/length), derivative)
                          * constant
                          * sin(j*(2.0*M_PI/length)*x + shift);
                break;
            case 3:
                retval -=   j * pow(j*(2.0*M_PI/length), derivative)
                          * constant
                          * cos(j*(2.0*M_PI/length)*x + shift);
                break;
        }
    }
    return retval;
}

template<typename FPT, typename Integer>
FPT periodic_function<FPT,Integer>::physical(
        const Integer i,
        const Integer derivative) const
{
    assert(0 <= i);
    assert(i <  N);

    const FPT xi = i*length/N;
    return physical_evaluate(xi, derivative);
}

template<typename FPT, typename Integer>
typename std::complex<FPT> periodic_function<FPT,Integer>::wave(
        const Integer i,
        const Integer derivative) const
{
    assert(0 <= i && i < N);

    typedef typename std::complex<FPT> complex_type;
    complex_type retval;
    if (i == 0) {
        if (i < max_mode_exclusive) {
            retval = complex_type(1, 0);
        }
    } else if (i < (N/2)) {
        if (i < max_mode_exclusive) {
            retval = complex_type(   (i)*sin(shift)/2, -  (i)*cos(shift)/2 );
        }
    } else if (i == (N/2) &&  (N%2)) {
        if (i < max_mode_exclusive) {
            retval = complex_type(   (i)*sin(shift)/2, -  (i)*cos(shift)/2 );
        }
    } else if (i == (N/2) && !(N%2)) { // Highest half mode on even grid
        if (i < max_mode_exclusive) {
            retval = complex_type(   (i)*sin(shift), 0    );
        }
    } else {
        if ((N-i) < max_mode_exclusive) {
            retval = complex_type( (N-i)*sin(shift)/2, +(N-i)*cos(shift)/2 );
        }
    }
    retval *= constant;

    for (int j = 0; j < derivative; ++j) {
        retval *= complex_type(0, 2*M_PI/length);
    }

    return retval;
}

/** A fixture for the Boost.Test that replaces underling_error */
#pragma warning(push,disable:2017 2021)
class BoostFailErrorHandlerFixture {
public:
    /** A underling_error_handler_t that invokes BOOST_FAIL */
    static void boost_fail_error_handler(
            const char *reason, const char *file, int line, int underling_errno)
    {
        std::ostringstream oss;
        oss << "Encountered '"
            << underling_strerror(underling_errno)
            << "' at "
            << file
            << ':'
            << line
            << " with reason '"
            << reason
            << "'";
        BOOST_FAIL(oss.str());
    }

    BoostFailErrorHandlerFixture()
        : previous_(underling_set_error_handler(&boost_fail_error_handler)) {}

    ~BoostFailErrorHandlerFixture() {
        underling_set_error_handler(previous_);
    }
private:
    underling_error_handler_t * previous_;
};
#pragma warning(pop)

#pragma warning(pop)
#endif // __TEST_TOOLS_HPP
