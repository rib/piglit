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
 * \file api.c
 *
 * Basic INTEL_performance_query infrastructure tests.  These test the
 * mechanism to retrieve counter and query information, string
 * processing, and various error conditions.  They do not actually
 * activate monitoring.
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
get_queries(GLuint ** queries, unsigned *num_queries)
{
	GLuint queryid;
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
get_counters(GLuint query, GLuint ** counters, unsigned *num_counters)
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
 * Return true if x is in xs.
 */
static bool
in_list(int x, unsigned *xs, int elts)
{
	int i;
	for (i = 0; i < elts; ++i) {
		if (x == xs[i])
			return true;
	}
	return false;
}

/**
 * Find an invalid query ID.
 */
static GLuint
find_invalid_query(unsigned *queries, int num_queries)
{
	unsigned invalid_query = ~0;

	/* Most implementations probably use small consecutive integers, so
	 * start at ~0 and work backwards.  Hopefully we shouldn't loop.
	 */
	while (in_list(invalid_query, queries, num_queries))
		--invalid_query;

	return invalid_query;
}

/**
 * Find an invalid counter ID.
 */
static unsigned
find_invalid_counter(unsigned *counters, int num_counters)
{
	unsigned invalid_counter = ~0;

	/* Most implementations probably use small consecutive integers, so
	 * start at ~0 and work backwards.  Hopefully we shouldn't loop.
	 */
	while (in_list(invalid_counter, counters, num_counters))
		--invalid_counter;

	return invalid_counter;
}

#define report(pass) \
	do { \
		piglit_report_subtest_result((pass) ? \
		PIGLIT_PASS : PIGLIT_FAIL, __FUNCTION__); \
		return; \
	} while (0)

/******************************************************************************/

/** Call glGetFirstPerfQueryIdINTEL() with a NULL queryId pointer.
 *
 * Verify that it doesn't attempt to write the query id and crash.
 */
static void
test_first_query_null_queryid_pointer(void)
{
	glGetFirstPerfQueryIdINTEL(NULL);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/******************************************************************************/

/** Call glGetNextPerfQueryIdINTEL() with a NULL nextQueryId.
 *
 * Verify that it doesn't attempt to write the query id and crash.
 */
static void
test_next_query_null_nextqueryid_pointer(unsigned validquery)
{
	glGetNextPerfQueryIdINTEL(validquery, NULL);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/** Call glGetNextPerfQueryIdINTEL() with an invalid query id.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_next_query_invalid_queryid(unsigned invalidquery)
{
	unsigned dummy;

	glGetNextPerfQueryIdINTEL(invalidquery, &dummy);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/******************************************************************************/

/** Call glGetPerfQueryIdByNameINTEL() with a NULL query id pointer.
 *
 * Verify that it doesn't attempt to write a query id and crash.
 */
static void
test_get_query_by_name_null_queryid_pointer(const char *validname)
{
	glGetPerfQueryIdByNameINTEL(validname, NULL);
	/* The specification does not say that this should produce an error. */
}

/** Call glGetPerfQueryIdByNameINTEL() with a NULL name pointer.
 *
 * Verify that it doesn't attempt to read the name and crash.
 */
static void
test_get_query_by_name_null_name_pointer(void)
{
	unsigned dummy;

	glGetPerfQueryIdByNameINTEL(NULL, &dummy);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/** Call glGetPerfQueryIdByNameINTEL() with an invalid name.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_get_query_by_name_invalid_name(const char *invalidname)
{
	unsigned dummy;

	glGetPerfQueryIdByNameINTEL(invalidname, &dummy);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/******************************************************************************/

/** Call glGetPerfQueryInfoINTEL() with an invalid query id.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_get_perf_query_info_invalid_queryid(unsigned invalidquery)
{
	char name[256];
	GLuint datasize;
	GLuint counters;
	GLuint instances;
	GLuint caps;

	glGetPerfQueryInfoINTEL(invalidquery,
				sizeof(name), name,
				&datasize, &counters, &instances, &caps);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/** Call glGetPerfQueryInfoINTEL() with NULL pointers.
 *
 * Verify that it doesn't attempt to write data to them and crash.
 * Verify that it doesn't produce an error.
 */
static void
test_get_perf_query_info_null_pointers(unsigned validquery)
{
	glGetPerfQueryInfoINTEL(validquery, 0, NULL, NULL, NULL, NULL, NULL);
	report(piglit_check_gl_error(GL_NO_ERROR));
}

/** Call glGetPerfQueryInfoINTEL() with a single character buffer.
 *
 * Verify that length is correct, the string is zero-terminated,
 * and no buffer overflows occur.
 */
static void
test_get_perf_query_info_single_character_buffer(unsigned validquery)
{
	char name[3] = "```";
	bool pass = true;

	glGetPerfQueryInfoINTEL(validquery, 1, name, NULL, NULL, NULL, NULL);
	pass = piglit_check_gl_error(GL_NO_ERROR);

	/* Verify buffer contents: only the first character should change,
	 * and it should be the terminator.
	 */
	pass = name[0] == '\0' && pass;
	pass = name[1] == '`' && pass;
	pass = name[2] == '`' && pass;

	report(pass);
}

/******************************************************************************/

/** Call glGetPerfCounterInfoINTEL() with an invalid query id.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_get_perf_counter_info_invalid_queryid(unsigned invalidquery)
{
	char name[256];
	char desc[1024];
	GLuint offset;
	GLuint datasize;
	GLuint type;
	GLuint datatype;
	GLuint64 maxvalue;

	/* 1 is a valid counter ID */
	glGetPerfCounterInfoINTEL(invalidquery, 1,
				  sizeof(name), name,
				  sizeof(desc), desc,
				  &offset, &datasize, &type,
				  &datatype, &maxvalue);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/** Call glGetPerfCounterInfoINTEL() with a valid query id but invalid
 * counter id.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_get_perf_counter_info_invalid_counterid(unsigned validquery,
					     unsigned invalidcounter)
{
	char name[256];
	char desc[1024];
	GLuint offset;
	GLuint datasize;
	GLuint type;
	GLuint datatype;
	GLuint64 maxvalue;

	glGetPerfCounterInfoINTEL(validquery, invalidcounter,
				  sizeof(name), name,
				  sizeof(desc), desc,
				  &offset, &datasize, &type,
				  &datatype, &maxvalue);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/** Call glGetPerfCounterInfoINTEL() with NULL pointers.
 *
 * Verify that it doesn't attempt to write data to them and crash.
 * Verify that it doesn't produce an error.
 */
static void
test_get_perf_counter_info_null_pointers(unsigned validquery,
					 unsigned validcounter)
{
	glGetPerfCounterInfoINTEL(validquery, validcounter,
				  0, NULL,
				  0, NULL, NULL, NULL, NULL, NULL, NULL);
	report(piglit_check_gl_error(GL_NO_ERROR));
}

/** Call glGetPerfCounterInfoINTEL() with single character buffers.
 *
 * Verify that length is correct, the string is zero-terminated,
 * and no buffer overflows occur.
 */
static void
test_get_perf_counter_info_single_character_buffer(unsigned validquery,
						   unsigned validcounter)
{
	char name[3] = "```";
	char desc[3] = "```";
	bool pass = true;

	glGetPerfCounterInfoINTEL(validquery, validcounter,
				  1, name,
				  1, desc, NULL, NULL, NULL, NULL, NULL);
	pass = piglit_check_gl_error(GL_NO_ERROR);

	/* Verify buffer contents: only the first character should change,
	 * and it should be the terminator.
	 */
	pass = name[0] == '\0' && pass;
	pass = name[1] == '`' && pass;
	pass = name[2] == '`' && pass;

	pass = desc[0] == '\0' && pass;
	pass = desc[1] == '`' && pass;
	pass = desc[2] == '`' && pass;

	report(pass);
}

/**
 * Call glGetPerfQueryInfoINTEL() on every query and verify that all
 * queries have a valid capsMask value.
 */
static void
test_query_info(unsigned *queries, unsigned num_queries)
{
	int i;

	for (i = 0; i < num_queries; ++i) {
		char name[256];
		GLuint datasize;
		GLuint counters;
		GLuint instances;
		GLuint caps;

		glGetPerfQueryInfoINTEL(queries[i],
					sizeof(name), name,
					&datasize, &counters,
					&instances, &caps);

		if (caps != GL_PERFQUERY_SINGLE_CONTEXT_INTEL &&
		    caps != GL_PERFQUERY_GLOBAL_CONTEXT_INTEL) {
			printf("Query %u has an invalid capability mask: %x\n",
			       queries[i], caps);
			report(false);
			break;
		}
	}

	report(true);
}

/**
 * Call glGetPerfCounterInfoINTEL() on every query/counter pair and verify that
 * all counters have a valid type and datatype.
 */
static void
test_counter_info(unsigned *queries, unsigned num_queries)
{
	int i;
	int j;

	for (i = 0; i < num_queries; ++i) {
		unsigned *counters;
		unsigned num_counters;
		get_counters(queries[i], &counters, &num_counters);

		if (!piglit_automatic) {
			char queryname[256];

			glGetPerfQueryInfoINTEL(queries[i], sizeof(queryname),
						queryname, NULL, NULL, NULL,
						NULL);
			printf("Query %u [%s]:\n", queries[i], queryname);
		}

		for (j = 0; j < num_counters; ++j) {
			char name[256];
			char desc[1024];
			GLuint offset;
			GLuint datasize;
			GLuint type;
			GLuint datatype;
			GLuint64 maxvalue;

			memset(name, 0, sizeof(name));
			memset(desc, 0, sizeof(desc));

			glGetPerfCounterInfoINTEL(queries[i], counters[j],
						  sizeof(name), name,
						  sizeof(desc), desc,
						  &offset, &datasize, &type,
						  &datatype, &maxvalue);

			if (!piglit_automatic)
				printf(" Counter %u [%s]: %s\n", counters[j],
				       name, desc);

			switch (datatype) {
			default:
				printf("Query %u/Counter %u has an invalid datatype: %x\n",
				       queries[i], counters[j], datatype);
				report(false);
				break;
			case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL:
			case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL:
			case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL:
			case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL:
			case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL:
				break;
			}

			switch (type) {
			default:
				printf("Query %u/Counter %u has an invalid type: %x\n",
				       queries[i], counters[j], type);
				report(false);
				break;
			case GL_PERFQUERY_COUNTER_EVENT_INTEL:
			case GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL:
			case GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL:
			case GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL:
			case GL_PERFQUERY_COUNTER_RAW_INTEL:
			case GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL:
				break;
			}
		}
		free(counters);
	}

	report(true);
}

/******************************************************************************/

/**
 * Call glBeginPerfQueryINTEL() on an invalid query handle.
 * (Should be run before any Gen tests to ensure this handle is invalid.)
 *
 * Verify that it produces INVALID_VALUE.
 */
void
test_begin_invalid_query_handle(void)
{
	glBeginPerfQueryINTEL(777);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glEndPerfQueryINTEL() on an invalid query handle.
 * (Should be run before any Gen tests to ensure this ID is invalid.)
 *
 * Verify that it produces INVALID_VALUE or INVALID_OPERATION.
 * XXX: Only INVALID_OPERATION is actually specified, assume
 * INVALID_VALUE can also be generated.
 */
void
test_end_invalid_query_handle(void)
{
	GLenum error;
	glEndPerfQueryINTEL(777);
	error = glGetError();
	report(error == GL_INVALID_VALUE || error == GL_INVALID_OPERATION);
}

/**
 * Call glCreatePerfQueryINTEL() with an invalid query id.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_create_perf_query_invalid_query(unsigned invalidquery)
{
	GLuint handle;
	glCreatePerfQueryINTEL(invalidquery, &handle);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glCreatePerfQueryINTEL() with a NULL queryhandle pointer.
 *
 * Verify that it doesn't attempt to write a handle and crash,
 * and produces INVALID_VALUE.
 * XXX: This isn't actually specified, but it seems like it ought to be.
 */
static void
test_create_perf_query_null_handle_pointer(unsigned validquery)
{
	glCreatePerfQueryINTEL(validquery, NULL);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glDeletePerfQueryINTEL() with an invalid query handle.
 *
 * Verify that it produces INVALID_VALUE.
 */
static void
test_delete_perf_query_invalid_handle(void)
{
	glDeletePerfQueryINTEL(777);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glGetPerfQueryDataINTEL() with an invalid query handle.
 * (Should be run before any Gen tests to ensure this ID is invalid.)
 *
 * Verify that it produces INVALID_VALUE.
 * XXX: This isn't actually specified, but it seems like it ought to be.
 */
static void
test_get_query_data_invalid_query_handle(void)
{
	char data[1024];
	GLuint bytes;
	glGetPerfQueryDataINTEL(777, GL_PERFQUERY_DONOT_FLUSH_INTEL,
				sizeof(data), data, &bytes);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glGetPerfQueryDataINTEL() with a NULL data pointer.
 *
 * Verify that it doesn't attempt to write data and crash, and
 * produces INVALID_VALUE.
 */
static void
test_get_query_data_null_data_pointer(void)
{
	GLuint query;
	GLuint handle;
	GLuint bytes;

	glGetFirstPerfQueryIdINTEL(&query);
	glCreatePerfQueryINTEL(query, &handle);

	glGetPerfQueryDataINTEL(handle, GL_PERFQUERY_DONOT_FLUSH_INTEL,
				128, NULL, &bytes);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glGetPerfQueryDataINTEL() with a NULL byteswritten pointer.
 *
 * Verify that it doesn't attempt to write data and crash, and
 * produces INVALID_VALUE.
 */
static void
test_get_query_data_null_byteswritten_pointer(void)
{
	GLuint query;
	GLuint handle;
	char data[1024];

	glGetFirstPerfQueryIdINTEL(&query);
	glCreatePerfQueryINTEL(query, &handle);

	glGetPerfQueryDataINTEL(handle, GL_PERFQUERY_DONOT_FLUSH_INTEL,
				sizeof(data), data, NULL);
	report(piglit_check_gl_error(GL_INVALID_VALUE));
}

/**
 * Call glGetPerfQueryDataINTEL on a query handle that hasn't been started.
 *
 * Verify that no data is written in such case.
 */
static void
test_initial_state(void)
{
	bool pass = true;
	unsigned query;
	unsigned handle;
	unsigned bytes = 0xd0d0d0d0;
	char data[1024];

	glGetFirstPerfQueryIdINTEL(&query);
	glCreatePerfQueryINTEL(query, &handle);
	pass = piglit_check_gl_error(GL_NO_ERROR) && pass;

	memset(data, '`', sizeof(data));

	glGetPerfQueryDataINTEL(handle, GL_PERFQUERY_DONOT_FLUSH_INTEL,
				sizeof(data), data, &bytes);
	pass = piglit_check_gl_error(GL_NO_ERROR) && pass;

	pass = bytes == 0 && pass;
	pass = data[0] == '`' && pass;

	glDeletePerfQueryINTEL(handle);
	report(pass);
}

/**
 * "If a performance query is not currently started, an
 * INVALID_OPERATION error will be generated."
 */
static void
test_end_without_begin(void)
{
	bool pass = true;
	unsigned query;
	unsigned handle;

	glGetFirstPerfQueryIdINTEL(&query);
	glCreatePerfQueryINTEL(query, &handle);
	pass = piglit_check_gl_error(GL_NO_ERROR) && pass;

	glEndPerfQueryINTEL(handle);
	report(piglit_check_gl_error(GL_INVALID_OPERATION));

	glDeletePerfQueryINTEL(handle);
}

/**
 * "Note that some query types, they cannot be collected in the same
 * time. Therefore calls of BeginPerfQueryINTEL() cannot be nested if
 * they refer to queries of such different types. In such case
 * INVALID_OPERATION error is generated."
 *
 * Impossible to know for sure which different queries cannot be used
 * at the same time.  Assume that the same query cannot be started
 * twice, so verify that calling Begin twice on the same query
 * produces INVALID_OPERATION.
 */
static void
test_double_begin(void)
{
	bool pass = true;
	unsigned query;
	unsigned handle;
	GLenum error;

	glGetFirstPerfQueryIdINTEL(&query);
	glCreatePerfQueryINTEL(query, &handle);
	pass = piglit_check_gl_error(GL_NO_ERROR) && pass;

	glBeginPerfQueryINTEL(handle);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		glDeletePerfQueryINTEL(handle);
		/* Query couldn't start for some reason; bail. */
		if (error == GL_INVALID_OPERATION)
			return;
		/* We weren't expecting this other error. */
		report(false);
	}

	/* Double begin */
	glBeginPerfQueryINTEL(handle);
	pass = piglit_check_gl_error(GL_INVALID_OPERATION) && pass;

	glDeletePerfQueryINTEL(handle);
	report(pass);
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
	unsigned *counters;
	unsigned num_counters;
	unsigned invalid_query;
	unsigned valid_query;
	unsigned invalid_counter;
	unsigned valid_counter;
	char valid_name[256];

	piglit_require_extension("GL_INTEL_performance_query");

	test_first_query_null_queryid_pointer();

	get_queries(&queries, &num_queries);

	/* If there are no valid queries, the rest of the tests can't run.
	 * Bail.
	 */
	if (num_queries == 0)
		exit(0);

	invalid_query = find_invalid_query(queries, num_queries);
	valid_query = queries[0];

	test_next_query_null_nextqueryid_pointer(valid_query);
	test_next_query_invalid_queryid(invalid_query);

	glGetPerfQueryInfoINTEL(valid_query, sizeof(valid_name), valid_name,
				NULL, NULL, NULL, NULL);

	test_get_query_by_name_null_queryid_pointer(valid_name);
	test_get_query_by_name_null_name_pointer();
	test_get_query_by_name_invalid_name
		("We assume this is an invalid name of a query");
	test_get_perf_query_info_invalid_queryid(invalid_query);
	test_get_perf_query_info_null_pointers(valid_query);
	test_get_perf_query_info_single_character_buffer(valid_query);
	test_get_perf_counter_info_invalid_queryid(invalid_query);

	get_counters(valid_query, &counters, &num_counters);

	/* If there are no counters, the rest of the tests can't run.
	 * Bail.
	 */
	if (num_counters == 0)
		exit(0);

	invalid_counter = find_invalid_counter(counters, num_counters);
	valid_counter = counters[0];

	test_get_perf_counter_info_invalid_counterid(valid_query,
						     invalid_counter);
	test_get_perf_counter_info_null_pointers(valid_query, valid_counter);
	test_get_perf_counter_info_single_character_buffer(valid_query,
							   valid_counter);
	test_query_info(queries, num_queries);
	test_counter_info(queries, num_queries);
	test_begin_invalid_query_handle();
	test_end_invalid_query_handle();
	test_create_perf_query_invalid_query(invalid_query);
	test_create_perf_query_null_handle_pointer(valid_query);
	test_delete_perf_query_invalid_handle();
	test_get_query_data_invalid_query_handle();
	test_get_query_data_null_data_pointer();
	test_get_query_data_null_byteswritten_pointer();
	test_initial_state();
	test_end_without_begin();
	test_double_begin();

	free(counters);
	free(queries);

	exit(0);
}
