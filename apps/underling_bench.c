//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// underling 0.2.1: underling library for parallel, 3D pencil decompositions
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/file.h>
#include <unistd.h>

#include "argp.h"
#include "mpi_argp.h"

#include <mpi.h>
#include <fftw3.h>
#include <fftw3-mpi.h>
#include <underling/error.h>
#include <underling/underling.h>

#ifdef HAVE_GRVY
#include <grvy.h>
#define GRVY_TIMER_BEGIN(id)   grvy_timer_begin(id)
#define GRVY_TIMER_END(id)     grvy_timer_end(id)
#define GRVY_TIMER_FINALIZE()  grvy_timer_finalize()
#define GRVY_TIMER_INIT(id)    grvy_timer_init(id)
#define GRVY_TIMER_RESET()     grvy_timer_reset()
#define GRVY_TIMER_SUMMARIZE() grvy_timer_summarize()
#else
#define GRVY_TIMER_BEGIN(id)
#define GRVY_TIMER_END(id)
#define GRVY_TIMER_FINALIZE()
#define GRVY_TIMER_INIT(id)
#define GRVY_TIMER_RESET()
#define GRVY_TIMER_SUMMARIZE()
#endif

// TODO Allow UNDERLING_TRANSPOSED_LONG_{N2,N0}
// TODO Allow FFTs in particular directions
// TODO Allow different transform_flags
// TODO Verify memory contents after all transposes complete

//****************************************************************
// DATA STRUCTURES DATA STRUCTURES DATA STRUCTURES DATA STRUCTURES
//****************************************************************

struct details {
    int world_rank;
    int world_size;
    int verbose;
    int repeat;
    int nfields;
    int howmany;
    int nthreads;
    int n0;
    int n1;
    int n2;
    int pA;
    int pB;
    long bytes;
    unsigned transposed_flags;
    unsigned transform_flags;
    unsigned fftw_rigor_flags;
    char *wisdom_file;
    int inplace;
};

//*************************************************************
// STATIC PROTOTYPES STATIC PROTOTYPES STATIC PROTOTYPES STATIC
//*************************************************************

static void print_version(FILE *stream, struct argp_state *state);

static void trim(char *a);

static inline int min(int a, int b);

static inline int max(int a, int b);

static void to_human_readable_byte_count(long bytes,
                                         int si,
                                         double *coeff,
                                         const char **units);

static long from_human_readable_byte_count(const char *str);

static void* malloc_and_fill(struct details *d,
                             const long bytes,
                             unsigned salt);

//*******************************************************************
// ARGP DETAILS: http://www.gnu.org/s/libc/manual/html_node/Argp.html
//*******************************************************************

const char *argp_program_version      = "underling_bench " PACKAGE_VERSION;
void (*argp_program_version_hook)(FILE *stream, struct argp_state *state)
                                      = &print_version;
const char *argp_program_bug_address  = PACKAGE_BUGREPORT;
static const char doc[]               =
"Simulate and benchmark underling-based application transformation operations."
"\v"
"Transform parallel, 3D pencil decompositions using underling's data "
"movement capabilities.  Timing information is collected "
"over one or more iterations.  If provided, FFTW wisdom is imported from "
"and accumulated within WISDOM_FILE.\n"
"\n"
"Options taking a 'bytes' parameter can be given common byte-related "
"units.  For example --field-memory=5G indicates that approximately "
"5 gigabytes of memory should be used on each rank to store field data. "
"SI units like 'Ki' or 'MiB' are also accepted.\n"
;

static const char args_doc[] = "\nWISDOM_FILE";

enum {
    KEY_ESTIMATE = 1024, // !isascii
    KEY_MEASURE,
    KEY_PATIENT,
    KEY_EXHAUSTIVE
};

static struct argp_option options[] = {
    {"verbose",     'v', 0,       0, "produce verbose output",       0 },
    {"repeat",      'r', "count", 0, "number of repetitions",        0 },
    {"nthreads",    't', "count", 0, "number of concurrent threads", 0 },
    {"nfields",     'n', "count", 0, "number of independent fields", 0 },
    {"howmany",     'h', "count", 0, "howmany components per field", 0 },
    {"in-place",    'i', 0,       0, "perform in-place transposes",  0 },
    {0, 0, 0, 0,
     "Controlling global problem size (specify at most one)",     0 },
    {"field-memory", 'f', "bytes",    0, "per-rank field memory", 0 },
    {"field-global", 'F', "n0xn1xn2", 0, "field global extents",  0 },
    {0, 0, 0, 0,
     "Controlling parallel decomposition per MPI_Dims_create semantics", 0 },
    {"dims",       'P', "pAxpB",    0, "process grid for decomposition", 0 },
    {0, 0, 0, 0,
     "Controlling FFTW planning rigor",                                      0},
    {"estimate",   KEY_ESTIMATE,   0, 0, "plan with FFTW_ESTIMATE",          0},
    {"measure",    KEY_MEASURE,    0, 0, "plan with FFTW_MEASURE (default)", 0},
    {"patient",    KEY_PATIENT,    0, 0, "plan with FFTW_PATIENT",           0},
    {"exhaustive", KEY_EXHAUSTIVE, 0, 0, "plan with FFTW_EXHAUSTIVE",        0},
    {"timeout",    'T',    "seconds", 0, "use fftw_set_timelimit(seconds)",  0},
    { 0, 0, 0, 0,  0, 0 }
};

// Parse a single option following Argp semantics
static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    // Get the input argument from argp_parse.
    struct details *d = state->input;

    // Trim any leading/trailing whitespace from arg
    if (arg) trim(arg);

    // Want to ensure we consume the entire argument for many options
    // Many sscanf calls provide an extra sentinel %c dumping into &ignore
    char ignore = '\0';

    switch (key) {
        case ARGP_KEY_ARG:
            if (state->arg_num > 0) {
                argp_usage(state);
            } else {
                d->wisdom_file = strdup(arg);
                assert(d->wisdom_file);
            }
            break;

        case ARGP_KEY_END:
            if (state->arg_num > 1) {
                argp_usage(state);
            }
            break;

        case KEY_ESTIMATE:
            d->fftw_rigor_flags = FFTW_ESTIMATE;
            break;

        case KEY_MEASURE:
            d->fftw_rigor_flags = FFTW_MEASURE;
            break;

        case KEY_PATIENT:
            d->fftw_rigor_flags = FFTW_PATIENT;
            break;

        case KEY_EXHAUSTIVE:
            d->fftw_rigor_flags = FFTW_EXHAUSTIVE;
            break;

        case 'T':
            errno = 0;
            {
                double seconds;
                if (1 != sscanf(arg ? arg : "", "%lf %c", &seconds, &ignore)) {
                    argp_failure(state, EX_USAGE, errno,
                            "timeout option is malformed: '%s'", arg);
                }
                if (seconds < 0) {
                    argp_failure(state, EX_USAGE, 0,
                            "timeout value %lf must be nonnegative",
                            seconds);
                }
                fftw_set_timelimit(seconds);
            }
            break;

        case 'i':
            d->inplace = 1;
            break;

        case 'v':
            d->verbose = 1;
            break;

        case 'r':
            errno = 0;
            if (1 != sscanf(arg ? arg : "", "%d %c", &d->repeat, &ignore)) {
                argp_failure(state, EX_USAGE, errno,
                        "repeat option is malformed: '%s'", arg);
            }
            if (d->repeat < 1) {
                argp_failure(state, EX_USAGE, 0,
                        "repeat value %d must be strictly positive",
                        d->repeat);
            }
            break;

        case 't':
            errno = 0;
            if (1 != sscanf(arg ? arg : "", "%d %c", &d->nthreads, &ignore)) {
                argp_failure(state, EX_USAGE, errno,
                        "nthreads is malformed: '%s'", arg);
            }
            if (d->nthreads < 0) {
                argp_failure(state, EX_USAGE, 0,
                        "nthreads value %d must be nonnegative", d->nthreads);
            }
            break;

        case 'n':
            errno = 0;
            if (1 != sscanf(arg ? arg : "", "%d %c", &d->nfields, &ignore)) {
                argp_failure(state, EX_USAGE, errno,
                        "nfields is malformed: '%s'", arg);
            }
            if (d->nfields < 0) {
                argp_failure(state, EX_USAGE, 0,
                        "nfields value %d must be nonnegative", d->nfields);
            }
            break;

        case 'h':
            errno = 0;
            if (1 != sscanf(arg ? arg : "", "%d %c", &d->howmany, &ignore)) {
                argp_failure(state, EX_USAGE, errno,
                        "howmany option is malformed: '%s'", arg);
            }
            if (d->howmany < 1) {
                argp_failure(state, EX_USAGE, 0,
                        "howmany value %d must be strictly positive",
                        d->howmany);
            }
            break;

        case 'f':
            if (d->n0 || d->n1 || d->n2 ) {
                argp_error(state, "only one of --field-{memory,global}"
                           " may be specified");
            }
            if (arg) {
                d->bytes = from_human_readable_byte_count(arg);
                if (d->bytes < 1)
                    argp_failure(state, EX_USAGE, 0,
                            "field-memory option is malformed: '%s'", arg);
            }
            break;

        case 'F':
            if (d->bytes) {
                argp_error(state, "only one of --field-{memory,global}"
                           " may be specified");
            }
            errno = 0;
            if (3 != sscanf(arg ? arg : "", "%d x %d x %d %c",
                            &d->n0, &d->n1, &d->n2, &ignore)) {
                argp_failure(state, EX_USAGE, errno,
                        "field-global option is malformed: '%s'", arg);
            }
            if (d->n0 < 0 || d->n1 < 0 || d->n2 < 0) {
                argp_failure(state, EX_USAGE, 0,
                        "field-global values %dx%dx%d must be nonnegative",
                        d->n0, d->n1, d->n2);
            }
            break;

        case 'P':
            errno = 0;
            if (2 != sscanf(arg ? arg : "", "%d x %d %c",
                            &d->pA, &d->pB, &ignore)) {
                argp_failure(state, EX_USAGE, errno,
                        "dims option not of form pAxpB: '%s'", arg);
            }
            if (d->pA < 0 || d->pB < 0) {
                argp_failure(state, EX_USAGE, 0,
                        "field-global values %dx%d must be nonnegative",
                        d->pA, d->pB);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {
    options, parse_opt, args_doc, doc,
    0 /*children*/, 0 /*help_filter*/, 0 /*argp_domain*/
};

// Rank-dependent output streams established in main().
static FILE *rankout, *rankerr;

int main(int argc, char *argv[])
{
    // Initialize default argument storage and default values
    struct details d;
    memset(&d, 0, sizeof(struct details));
    d.repeat           = 1;
    d.nthreads         = 0;
    d.nfields          = 1;
    d.howmany          = 2;

    // Initialize/finalize MPI with profiling initially disabled
    MPI_Init(&argc, &argv);
    atexit((void (*) ()) MPI_Finalize);
    MPI_Pcontrol(0);
    MPI_Comm_size(MPI_COMM_WORLD, &d.world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &d.world_rank);

    // Establish rank-dependent output streams
    errno   = 0;
    if (d.world_rank == 0) {
        rankout = stdout;
        rankerr = stderr;
    } else {
        rankout = fopen("/dev/null", "w");
        rankerr = rankout;
    }
    if (!rankout) {
        perror("Unable to open rank-dependent output streams"),
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Parse command line arguments using MPI-savvy argp extension
    mpi_argp_parse(d.world_rank, &argp, argc, argv, 0, 0, &d);

    // Initialize/finalize FFTW threads, FFTW MPI, underling
    underling_init(&argc, &argv, d.nthreads);
    atexit(&underling_cleanup);

    // Print program banner that shows version and program arguments
    fprintf(rankout, "%s invoked as\n\t", argp_program_version);
    for (int i = 0; i < argc; ++i) {
        fprintf(rankout, " %s", argv[i]);
    }
    fprintf(rankout, "\n");

    // If available, load wisdom from disk on rank 0 and broadcast it
    // Attempt advisory locking to reduce processes stepping on each other
    if (d.wisdom_file && d.world_rank == 0) {
        FILE *w = fopen(d.wisdom_file, "r");
        if (w) {
            fprintf(rankout, "Loading wisdom from file %s\n", d.wisdom_file);
            if (flock(fileno(w), LOCK_SH)) {
                fprintf(rankout, "WARNING: LOCK_SH failed on wisdom file: %s\n",
                        strerror(errno));
            }
            fftw_import_wisdom_from_file(w);
            if (flock(fileno(w), LOCK_UN)) {
                fprintf(rankout, "WARNING: LOCK_UN failed on wisdom file: %s\n",
                        strerror(errno));
            }
            fclose(w);
        } else {
            fprintf(rankout, "WARNING: Unable to open file %s: %s\n",
                    d.wisdom_file, strerror(errno));
        }
    }
    fftw_mpi_broadcast_wisdom(MPI_COMM_WORLD);

    // If necessary, compute global grid size from per-rank memory constraint
    if (d.bytes) {
        const double nvectors = (d.bytes * d.world_size)
                              / ((double) d.howmany * sizeof(underling_real))
                              / ((double) d.nfields);
        d.n0 = d.n1 = d.n2 = ceil(cbrt(nvectors));

        double coeff;
        const char *units;
        to_human_readable_byte_count(d.bytes, 0, &coeff, &units);
        fprintf(rankout,
            "Per-rank %.2f %s memory requested => %d x %d x %d problem\n",
            coeff, units, d.n0, d.n1, d.n2);
    }

    // Ensure a non-trivial grid was requested
    if (!d.n0 || !d.n1 || !d.n2) {
        fprintf(rankout,
                "You must specify either --field-memory or --field-global!\n");
        MPI_Abort(MPI_COMM_WORLD, EX_USAGE);
    }

    // Initialize underling_grid and print decomposition banner
    underling_grid grid = underling_grid_create(
            MPI_COMM_WORLD, d.n0, d.n1, d.n2, d.pA, d.pB);
    assert(grid);
    d.pA = underling_grid_pA_size(grid); // Obtain any automagic pA value
    d.pB = underling_grid_pB_size(grid); // Obtain any automagic pB value
    fprintf(rankout,
        "\n"
        "Global pencil decomposition details (for transposed_flags == 0)\n"
        "------------------------------------------------------------------------------\n"
        "Long n2:                                     (%1$5d/%5$4d x %2$5d/%4$4d) x %3$5d\n"
        "Long n1: %3$5d/%4$4d x (%1$5d/%5$4d x %2$5d) = (%3$5d/%4$4d x %1$5d/%5$4d) x %2$5d\n"
        "Long n0: %2$5d/%5$4d x (%3$5d/%4$4d x %1$5d) = (%2$5d/%5$4d x %3$5d/%4$4d) x %1$5d\n"
        "------------------------------------------------------------------------------\n"
        "\n", d.n0, d.n1, d.n2, d.pA, d.pB);

    // Initialize underling_problem and find per-field memory requirements
    underling_problem problem = underling_problem_create(
            grid, d.howmany, d.transposed_flags);
    assert(grid);
    const size_t local_memory = underling_local_memory(problem);

    // Display some information about the problem's memory requirements
    {
        double coeff;
        const char *units;

        to_human_readable_byte_count(underling_global_memory_optimum(
                    grid, problem) * sizeof(underling_real), 0, &coeff, &units);
        fprintf(rankout, "Optimum global, per-field memory is %.4f %s\n",
                coeff, units);
        to_human_readable_byte_count(underling_global_memory(grid, problem)
                * sizeof(underling_real), 0, &coeff, &units);
        fprintf(rankout, "Actual global,  per-field memory is %.4f %s\n",
                coeff, units);

        to_human_readable_byte_count(underling_local_memory_optimum(problem)
                * sizeof(underling_real), 0, &coeff, &units);
        fprintf(rankout, "Optimum per-rank, per-field memory is %.4f %s\n",
                coeff, units);
        to_human_readable_byte_count(underling_local_memory_minimum(
                grid, problem) * sizeof(underling_real), 0, &coeff, &units);
        fprintf(rankout, "Minimum per-rank, per-field memory is %.4f %s\n",
                coeff, units);
        to_human_readable_byte_count(underling_local_memory_maximum(
                grid, problem) * sizeof(underling_real), 0, &coeff, &units);
        fprintf(rankout, "Maximum per-rank, per-field memory is %.4f %s\n",
                coeff, units);
    }

    // Adjust for in-place operation
    int off;
    if (d.inplace) {
        fprintf(rankout, "Performing operations in-place\n");
        off = 0;
    } else {
        fprintf(rankout, "Performing operations out-of-place\n");
        off = 1;
    }

    // Allocate memory for each field plus one optional scratch buffer
    underling_real *f[d.nfields + off]; // C99
    for (int i = 0; i < d.nfields + off; ++i) {
        f[i] = (underling_real *) malloc_and_fill(
                &d, local_memory * sizeof(underling_real), i);
        assert(f[i]);
    }

    // Create the transpose plan
    fprintf(rankout, "Invoking underling_plan_create...\n");
    const double plan_start = MPI_Wtime();
    underling_plan plan = underling_plan_create(
            problem, f[0], f[off], d.transform_flags, d.fftw_rigor_flags);
    const double plan_end = MPI_Wtime();
    fprintf(rankout, "...underling_plan_create took %lf seconds"
                     " and returned (on rank 0):\n", plan_end - plan_start);
    underling_fprint_plan(plan, rankout);
    fprintf(rankout, "\n");

    fprintf(rankout, "Beginning benchmark main loop...\n");
    GRVY_TIMER_INIT(argp_program_version);
    const double start = MPI_Wtime();
    for (int i = 0; i < d.repeat; ++i) {
        fprintf(rankout, "\tIteration %d\n", i);

        // Fields pointed to by f[0..(d.nfields-1)] are, say, long n2 to start

        for (int j = d.nfields-1; j >= 0; --j) {
            GRVY_TIMER_BEGIN("underling_execute_long_n2_to_long_n1");
            underling_execute_long_n2_to_long_n1(plan, f[j], f[j+off]);
            GRVY_TIMER_END("underling_execute_long_n2_to_long_n1");
        }

        // For out-of-place,
        // Fields pointed to by f[1..(d.nfields)] are now long n1

        for (int j = 0; j < d.nfields; ++j) {
            GRVY_TIMER_BEGIN("underling_execute_long_n1_to_long_n0");
            underling_execute_long_n1_to_long_n0(plan, f[j+off], f[j]);
            GRVY_TIMER_END("underling_execute_long_n1_to_long_n0");
        }

        // For out-of-place,
        // Fields pointed to by f[0..(d.nfields-1)] are now long n0

        for (int j = d.nfields-1; j >= 0; --j) {
            GRVY_TIMER_BEGIN("underling_execute_long_n0_to_long_n1");
            underling_execute_long_n0_to_long_n1(plan, f[j], f[j+off]);
            GRVY_TIMER_END("underling_execute_long_n0_to_long_n1");
        }

        // For out-of-place,
        // Fields pointed to by f[1..(d.nfields)] are now long n1

        for (int j = 0; j < d.nfields; ++j) {
            GRVY_TIMER_BEGIN("underling_execute_long_n1_to_long_n2");
            underling_execute_long_n1_to_long_n2(plan, f[j+off], f[j]);
            GRVY_TIMER_END("underling_execute_long_n1_to_long_n2");
        }

        // For out-of-place,
        // Fields pointed to by f[0..(d.nfields-1)] are now long n2
    }
    const double end = MPI_Wtime();
    GRVY_TIMER_FINALIZE();
    fprintf(rankout, "...ended benchmark main loop\n");

    const double elapsed = end - start;
    const double mean = elapsed / d.repeat;
    fprintf(rankout, "Mean time across %d iteration(s) was %8.6g seconds\n",
            d.repeat, mean);

    // TODO Get timing information back from multiple ranks
    if (d.world_rank == 0) { GRVY_TIMER_SUMMARIZE(); }

    // Deallocate memory for each field plus one possible scratch buffer
    for (int i = 0; i < d.nfields + off; ++i) {
        fftw_free(f[i]);
        f[i] = NULL;
    }

    // Tear down underling_plan, underling_problem, underling_grid
    underling_plan_destroy(plan);
    underling_problem_destroy(problem);
    underling_grid_destroy(grid);

    // If available, gather wisdom and then write to disk on rank 0
    // Attempt advisory locking to reduce processes stepping on each other
    fftw_mpi_gather_wisdom(MPI_COMM_WORLD);
    if (d.wisdom_file && d.world_rank == 0) {
        FILE *w = fopen(d.wisdom_file, "w+");
        if (w) {
            fprintf(rankout, "Saving wisdom to file %s\n", d.wisdom_file);
            if (flock(fileno(w), LOCK_EX)) {
                fprintf(rankout, "WARNING: LOCK_EX failed on wisdom file: %s\n",
                        strerror(errno));
            }
            fftw_export_wisdom_to_file(w);
            if (flock(fileno(w), LOCK_UN)) {
                fprintf(rankout, "WARNING: LOCK_UN failed on wisdom file: %s\n",
                        strerror(errno));
            }
            fclose(w);
        } else {
            fprintf(rankout, "WARNING: Unable to open file %s: %s\n",
                    d.wisdom_file, strerror(errno));
        }
    }

    // Finalizing of MPI, FFTW, underling handled by atexit()

    return 0;
}

void print_version(FILE *stream, struct argp_state *state)
{
    (void) state; // Unused

    fputs(argp_program_version, stream);
    fprintf(stream, " linked against FFTW3 %s", fftw_version);
    int version, subversion;
    if (MPI_SUCCESS == MPI_Get_version(&version, &subversion)) {
        fprintf(stream, " running atop MPI %d.%d", version, subversion);
    }
    fputc('\n', stream);
}


void trim(char *a)
{
    char *b = a;
    while (isspace(*b))   ++b;
    while (*b)            *a++ = *b++;
    *a = '\0';
    while (isspace(*--a)) *a = '\0';
}


static inline int min(int a, int b)
{
    return a < b ? a : b;
}


static inline int max(int a, int b)
{
    return a > b ? a : b;
}


// Adapted from http://stackoverflow.com/questions/3758606/
// how-to-convert-byte-size-into-human-readable-format-in-java
static void to_human_readable_byte_count(long bytes,
                                         int si,
                                         double *coeff,
                                         const char **units)
{
    // Static lookup table of byte-based SI units
    static const char *suffix[][2] = { { "B",  "B"   },
                                       { "kB", "KiB" },
                                       { "MB", "MiB" },
                                       { "GB", "GiB" },
                                       { "TB", "TiB" },
                                       { "EB", "EiB" },
                                       { "ZB", "ZiB" },
                                       { "YB", "YiB" } };
    int unit = si ? 1000 : 1024;
    int exp = 0;
    if (bytes > 0) {
        exp = min( (int) (log(bytes) / log(unit)),
                   (int) sizeof(suffix) / sizeof(suffix[0]) - 1);
    }
    *coeff = bytes / pow(unit, exp);
    *units  = suffix[exp][!!si];
}


// Convert strings like the following into byte counts
//    5MB, 5 MB, 5M, 3.7GB, 123b, 456kBytes
// with some amount of forgiveness baked into the parsing.
static long from_human_readable_byte_count(const char *str)
{
    // Parse leading numeric factor
    char *endptr;
    errno = 0;
    const double coeff = strtod(str, &endptr);
    if (errno) return -1;

    // Skip any intermediate white space
    while (isspace(*endptr)) ++endptr;

    // Read off first character which should be an SI prefix
    int exp  = 0;
    int unit = 1024;
    switch (toupper(*endptr)) {
        case 'B':  exp =  0; break;
        case 'K':  exp =  3; break;
        case 'M':  exp =  6; break;
        case 'G':  exp =  9; break;
        case 'T':  exp = 12; break;
        case 'E':  exp = 15; break;
        case 'Z':  exp = 18; break;
        case 'Y':  exp = 21; break;

        case ' ':
        case '\t':
        case '\0': exp =  0; goto done;

        default:   return -1;
    }
    ++endptr;

    // If an 'i' or 'I' is present use SI factor-of-1000 units
    if (toupper(*endptr) == 'I') {
        ++endptr;
        unit = 1000;
    }

    // Next character must be one of B/empty/whitespace
    switch (toupper(*endptr)) {
        case 'B':
        case ' ':
        case '\t': ++endptr;  break;

        case '\0': goto done;

        default:   return -1;
    }

    // Skip any remaining white space
    while (isspace(*endptr)) ++endptr;

    // Parse error on anything but a null terminator
    if (*endptr) return -1;

done:
    return exp ? coeff * pow(unit, exp / 3) : coeff;
}

static void* malloc_and_fill(struct details *d,
                             const long bytes,
                             unsigned salt)
{
    void *p = fftw_malloc(bytes); // Aligned malloc
    if (!p) {
        double coeff;
        const char *units;
        to_human_readable_byte_count(bytes, 0, &coeff, &units);
        fprintf(stderr, "Unable to malloc %.2f %s on rank %d\n",
                coeff, units, d->world_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Store previous random seed and provide a new one based on rank, salt
    char state[64];
    initstate(d->world_rank + salt, state, sizeof(state)/sizeof(state[0]));
    char *previous = setstate(state);

    // Fill
    const size_t count = bytes / sizeof(underling_real);
    for (size_t i = 0; i < count; ++i)
        ((double *) p)[i] = (double) random();

    // Restore previous random state
    setstate(previous);

    return p;
}
