/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * \file measure.c
 *
 * Some INTEL_performance_query tests that actually measure things.
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 30;
	config.supports_gl_es_version = 20;
	config.window_visual = PIGLIT_GL_VISUAL_RGB;

PIGLIT_GL_TEST_CONFIG_END

/******************************************************************************/

/**
 * Get a list of query IDs.
 */
static void
get_queries(unsigned **queries, unsigned *num_queries)
{
	unsigned queryid;
	unsigned i;

	/* Enumerate all query ids first to get their count */
	*num_queries = 0;
	for (glGetFirstPerfQueryIdINTEL(&queryid);
	     queryid != 0;
	     glGetNextPerfQueryIdINTEL(queryid, &queryid)) {
		++(*num_queries);
	}

	*queries = calloc(*num_queries, sizeof(unsigned));

	/* And now collect them */
	i = 0;
	for (glGetFirstPerfQueryIdINTEL(&queryid);
	     queryid != 0;
	     glGetNextPerfQueryIdINTEL(queryid, &queryid)) {
		(*queries)[i++] = queryid;
	}
}

/**
 * Get a list of counter IDs in a given query.
 */
static void
get_counters(unsigned query, unsigned **counters, unsigned *num_counters)
{
	unsigned i;

	glGetPerfQueryInfoINTEL(query, 0, NULL, NULL,
				num_counters, NULL, NULL);

	/* Counters start from 1 and are continuous */
	*counters = calloc(*num_counters, sizeof(unsigned));
	for (i = 0; i < *num_counters; ++i) {
		(*counters)[i] = i + 1;
	}
}

/**
 * Get the size of a counter value of a given datatype enum in bytes.
 */
static unsigned
value_size(GLuint datatype)
{
	switch (datatype) {
	default:
		/* Return 0 so the caller can report correct data to piglit. */
		return 0;
	case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL:
	case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL:
	case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL:
		return sizeof(uint32_t);
	case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL:
	case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL:
		return sizeof(uint64_t);
	}
}

#define verify(x)						\
  if (!(x)) {							\
    piglit_report_subtest_result(PIGLIT_FAIL, "%s", test_name); \
    return;                                                     \
  }

/******************************************************************************/

/**
 * Basic functional test: create a query of the first query (this
 * terminology is too overloaded) begin monitoring, end monitoring,
 * make sure results are available, sanity check the result size, and
 * get the results.
 */
static void
test_basic_measurement(unsigned query)
{
	unsigned handle;
	unsigned *counters;
	unsigned num_counters;
	unsigned result_size = 0;
	GLuint bytes_written = 0;
	char *data;
	unsigned i;

	const char *test_name;

	/**
	* Test #1: Initialization
	*
	* Begin monitoring, end monitoring.
	*/
	test_name = "initialization";

	get_counters(query, &counters, &num_counters);
	verify(num_counters > 0);
	verify(piglit_check_gl_error(GL_NO_ERROR));

	glCreatePerfQueryINTEL(query, &handle);
	verify(piglit_check_gl_error(GL_NO_ERROR));

	/* Start monitoring */
	glBeginPerfQueryINTEL(handle);
	verify(piglit_check_gl_error(GL_NO_ERROR));

	/* Drawing...meh */
	piglit_draw_triangle(0.0, 0.0, 1.0, 1.0, 0.5, 0.5);

	/* End monitoring */
	glEndPerfQueryINTEL(handle);
	verify(piglit_check_gl_error(GL_NO_ERROR));

	piglit_report_subtest_result(PIGLIT_PASS, "%s", test_name);

	/**
	* Test #2: Receiving results
	*
	* Grab the data and sanity check its size as much as possible.
	*/

	test_name = "result retrieval";

	/* Get the result size. */
	glGetPerfQueryInfoINTEL(query, 0, NULL, &result_size, NULL, NULL,
				NULL);
	verify(piglit_check_gl_error(GL_NO_ERROR));

	/* Get the results. */

	/* Allocate more than the query info says, in case the
	 * implementation lied about the size.
	 */
	data = calloc(1, result_size + 32);
	/* Pass GL_PERFQUERY_WAIT_INTEL which will make the function
	 * return only when results are available.
	 */
	glGetPerfQueryDataINTEL(handle, GL_PERFQUERY_WAIT_INTEL,
				result_size + 32, data, &bytes_written);
	verify(piglit_check_gl_error(GL_NO_ERROR));
	verify(bytes_written == result_size);

	piglit_report_subtest_result(PIGLIT_PASS, "%s", test_name);

	/**
	* Test #3: Data
	*
	* Check that the data sizes and offsets are correct.
	*/
	test_name = "data";

	/* Counter values are one of float, double, uint32_t, uint64_t or
	 * bool32_t. Now, the specification implicitly allows counters to
	 * alias and extra data to exist between values (GetPerfCounterInfo
	 * gives counter data offset in bytes) so we don't have many sanity
	 * checks on the result_size the spec would restrict.
	 */
	verify(result_size >= sizeof(uint32_t));
	verify(result_size % sizeof(uint32_t) == 0);

	for (i = 0; i < num_counters; ++i) {
		char name[256];
		GLuint offset;
		GLuint size;
		GLuint datatype;
		glGetPerfCounterInfoINTEL(query, counters[i],
					  sizeof(name), name,
					  0, NULL,
					  &offset, &size,
					  NULL, &datatype, NULL);
		verify(piglit_check_gl_error(GL_NO_ERROR));
		verify(offset + size <= result_size);
		/* This tests that the datatype enum is valid */
		verify(value_size(datatype) != 0);
		/* In theory, the specification allows counter values to be
		 * _multiple_ values of the reported data type.
		 */
		verify(size >= value_size(datatype));
		verify(size % value_size(datatype) == 0);

		/* Print the counter value for manual inspection. */
		if (!piglit_automatic) {
			char *p;

			p = data + offset;
			printf("%u [%s]: ", i, name);
			switch (datatype) {
			case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL:
				printf("%" PRIu32 "\n", *((uint32_t *) p));
				break;
			case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL:
				printf("%" PRIu64 "\n", *((uint64_t *) p));
				break;
			case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL:
				printf("%f\n", *((float *) p));
				break;
			case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL:
				printf("%f\n", *((double *) p));
				break;
			case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL:
				printf("%" PRIu32 "\n", *((uint32_t *) p));
				break;
			default:
				verify(!"not reached, should have already checked for validity");
				break;
			}
		}
	}

	piglit_report_subtest_result(PIGLIT_PASS, "%s", test_name);

	free(data);

	glDeletePerfQueryINTEL(handle);
}


/******************************************************************************/

enum piglit_result
piglit_display(void)
{
	return PIGLIT_FAIL;
}

/**
 * The main test program.
 */
void
piglit_init(int argc, char **argv)
{
	unsigned *queries;
	unsigned num_queries;

	piglit_require_extension("GL_INTEL_performance_query");

	get_queries(&queries, &num_queries);

	/* If there are no queries, the rest of the tests can't run.  Bail. */
	if (num_queries == 0)
		exit(0);

	test_basic_measurement(queries[0]);

	exit(0);
}
