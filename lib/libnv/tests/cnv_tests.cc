/*-
 * Copyright (c) 2016 Adam Starak <starak.adam@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/cnv.h>
#include <sys/nv.h>
#include <sys/types.h>

#include <atf-c++.hpp>
#include <fcntl.h>
#include <errno.h>

#define	fd_is_valid(fd)	(fcntl((fd), F_GETFL) != -1 || errno != EBADF)

/* ATF cnvlist_get tests */

static void
check_nvlist(const nvlist_t *nvl)
{
	ATF_REQUIRE(nvl != NULL);
	ATF_REQUIRE_EQ(nvlist_error(nvl), 0);
	ATF_REQUIRE(nvlist_empty(nvl));
}

static void
check_element(const nvlist_t *nvl, int type, int res_type, const char *key)
{
	void *cookie;
	int dummy;

	cookie = NULL;

	ATF_REQUIRE_EQ(nvlist_error(nvl), 0);
	ATF_REQUIRE_EQ(type, res_type);
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE(nvlist_exists(nvl, key));

	nvlist_next(nvl, &dummy, &cookie);
	nvlist_next(nvl, &dummy, &cookie);

	/* Ensure that it has only our one key. */

	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), static_cast<const char *>(NULL));
}

static void
check_empty(const nvlist_t *nvl, const char *key)
{
	void *cookie;
	int dummy;

	cookie = NULL;
	nvlist_next(nvl, &dummy, &cookie);

	ATF_REQUIRE_EQ(nvlist_error(nvl), 0);
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(!nvlist_exists(nvl, key));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &dummy, &cookie), static_cast<const char *>(NULL));
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_bool);
ATF_TEST_CASE_BODY(cnvlist_get_bool)
{
	nvlist_t *nvl;
	const char *key;
	bool value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = true;

	nvlist_add_bool(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BOOL, key);
	ATF_REQUIRE(nvlist_exists_bool(nvl, key));

	ATF_REQUIRE_EQ(cnvlist_get_bool(cookie), value);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_number);
ATF_TEST_CASE_BODY(cnvlist_get_number)
{
	nvlist_t *nvl;
	const char *key;
	uint64_t value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = 420;

	nvlist_add_number(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NUMBER, key);
	ATF_REQUIRE(nvlist_exists_number(nvl, key));

	ATF_REQUIRE_EQ(cnvlist_get_number(cookie), value);

	nvlist_destroy(nvl);
}


ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_string);
ATF_TEST_CASE_BODY(cnvlist_get_string)
{
	nvlist_t *nvl;
	const char *key;
	const char *value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = "text";

	nvlist_add_string(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_STRING, key);
	ATF_REQUIRE(nvlist_exists_string(nvl, key));

	ATF_REQUIRE_EQ(strcmp(cnvlist_get_string(cookie), value), 0);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_nvlist);
ATF_TEST_CASE_BODY(cnvlist_get_nvlist)
{
	nvlist_t *nvl, *value;
	const nvlist_t *result;
	const char *key, *subkey;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	value = nvlist_create(0);
	check_nvlist(value);

	key = "name";
	subkey = "subname";
	cookie = NULL;

	/* Add null to 'value' nvlist. */

	nvlist_add_null(value, subkey);
	nvlist_next(value, &type, &cookie);
	check_element(value, type, NV_TYPE_NULL, subkey);
	ATF_REQUIRE(nvlist_exists_null(value, subkey));

	/* Add 'value' nvlist. */

	cookie = NULL;
	nvlist_add_nvlist(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NVLIST, key);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, key));

	/* Assuming nvlist_get_nvlist is correct check if cnvlist returns the same pointer. */

	result = cnvlist_get_nvlist(cookie);
	ATF_REQUIRE_EQ(result, nvlist_get_nvlist(nvl, key));
	ATF_REQUIRE(result != value);

	/* Validate data inside nvlist. */

	cookie = NULL;
	nvlist_next(result, &type, &cookie);
	check_element(result, type, NV_TYPE_NULL, subkey);
	ATF_REQUIRE(nvlist_exists_null(result, subkey));

	nvlist_destroy(nvl);
	nvlist_destroy(value);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_descriptor);
ATF_TEST_CASE_BODY(cnvlist_get_descriptor)
{
	nvlist_t *nvl;
	const char *key;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_descriptor(nvl, key, STDERR_FILENO);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_DESCRIPTOR, key);
	ATF_REQUIRE(nvlist_exists_descriptor(nvl, key));

	ATF_REQUIRE_EQ(fd_is_valid(cnvlist_get_descriptor(cookie)), 1);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_binary);
ATF_TEST_CASE_BODY(cnvlist_get_binary)
{
	nvlist_t *nvl;
	const char *key;
	void *in_binary;
	const void *out_binary;
	void *cookie;
	int type;
	size_t in_size, out_size;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	in_size = 13;
	in_binary = malloc(in_size);
	ATF_REQUIRE(in_binary != NULL);
	memset(in_binary, 0xa5, in_size);

	nvlist_add_binary(nvl, key, in_binary, in_size);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BINARY, key);
	ATF_REQUIRE(nvlist_exists_binary(nvl, key));

	out_binary = cnvlist_get_binary(cookie, &out_size);
	ATF_REQUIRE_EQ(out_size, in_size);
	ATF_REQUIRE_EQ(memcmp(in_binary, out_binary, out_size), 0);

	nvlist_destroy(nvl);
}

/* ATF cnvlist_get array tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_bool_array);
ATF_TEST_CASE_BODY(cnvlist_get_bool_array)
{
	nvlist_t *nvl;
	bool in_array[16];
	const bool *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = (i % 2 == 0);

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	nvlist_add_bool_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	check_element(nvl, type, NV_TYPE_BOOL_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_bool_array(nvl, key));

	out_array = cnvlist_get_bool_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 16);
	ATF_REQUIRE(out_array != NULL);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_number_array);
ATF_TEST_CASE_BODY(cnvlist_get_number_array)
{
	nvlist_t *nvl;
	uint64_t in_array[16];
	const uint64_t *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = i;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	nvlist_add_number_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NUMBER_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_number_array(nvl, key));

	out_array = cnvlist_get_number_array(cookie, &nitems);
	ATF_REQUIRE(out_array != NULL);
	ATF_REQUIRE_EQ(nitems, 16);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_string_array);
ATF_TEST_CASE_BODY(cnvlist_get_string_array)
{
	nvlist_t *nvl;
	const char *in_array[4] = {"inequality", "sucks", ".", ""};
	const char * const *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_string_array(nvl, key, in_array, 4);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_STRING_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_string_array(nvl, key));

	out_array = cnvlist_get_string_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 4);
	ATF_REQUIRE(out_array != NULL);
	for (i = 0; i < 4; i++) {
		ATF_REQUIRE(out_array[i] != NULL);
		ATF_REQUIRE_EQ(strcmp(out_array[i], in_array[i]), 0);
	}

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_nvlist_array);
ATF_TEST_CASE_BODY(cnvlist_get_nvlist_array)
{
	nvlist_t *nvl;
	nvlist_t *in_array[6];
	const nvlist_t * const *out_array;
	const nvlist_t * const *out_result;
	void *cookie;
	const char *key;
	const char *subkeys;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	subkeys = "123456";
	for (i = 0; i < 6; i++) {
		in_array[i] = nvlist_create(0);
		check_nvlist(in_array[i]);

		cookie = NULL;

		nvlist_add_null(in_array[i], subkeys+i);
		nvlist_next(in_array[i], &type, &cookie);
		check_element(in_array[i], type, NV_TYPE_NULL, subkeys+i);
		ATF_REQUIRE(nvlist_exists_null(in_array[i], subkeys+i));
	}

	cookie = NULL;
	key = "name";

	nvlist_add_nvlist_array(nvl, key, in_array, 6);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NVLIST_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, key));

	/* Get nvlist array by cnvlist function. */

	out_array = cnvlist_get_nvlist_array(cookie, &nitems);
	ATF_REQUIRE(out_array != NULL);
	ATF_REQUIRE_EQ(nitems, 6);

	/* Get nvlist array by nvlist function. */

	out_result = nvlist_get_nvlist_array(nvl, key, &nitems);
	ATF_REQUIRE(out_result != NULL);
	ATF_REQUIRE_EQ(nitems, 6);

	/* Validate assuming that nvlist returned a proper pointer */

	for (i = 0; i < 6; i++) {
		ATF_REQUIRE_EQ(out_result[i], out_array[i]);
		ATF_REQUIRE(out_array[i] != in_array[i]);

		/* Validate data inside nvlist. */
		cookie = NULL;
		nvlist_next(out_array[i], &type, &cookie);
		check_element(out_array[i], type, NV_TYPE_NULL, subkeys+i);
		ATF_REQUIRE(nvlist_exists_null(out_array[i], subkeys+i));
	}

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_descriptor_array);
ATF_TEST_CASE_BODY(cnvlist_get_descriptor_array)
{

	nvlist_t *nvl;
	int *in_array, type, i, count;
	const int *out_array;
	void *cookie;
	const char *key;
	size_t nitems;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	count = 50;

	in_array = (int*)malloc(sizeof(*in_array) * count);
	ATF_REQUIRE(in_array != NULL);
	for (i = 0; i < count; i++) {
		in_array[i] = dup(STDERR_FILENO);
		ATF_REQUIRE(fd_is_valid(in_array[i]));
	}

	nvlist_add_descriptor_array(nvl, key, in_array, count);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_DESCRIPTOR_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_descriptor_array(nvl, key));

	out_array = cnvlist_get_descriptor_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, count);
	ATF_REQUIRE(out_array != NULL);
	for (i = 0; i < count; i++)
		ATF_REQUIRE_EQ(fd_is_valid(out_array[i]), 1);

	nvlist_destroy(nvl);
}

/* ATF cnvlist_take tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_bool);
ATF_TEST_CASE_BODY(cnvlist_take_bool)
{
	nvlist_t *nvl;
	const char *key;
	bool value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = true;

	nvlist_add_bool(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BOOL, key);
	ATF_REQUIRE(nvlist_exists_bool(nvl, key));

	ATF_REQUIRE_EQ(cnvlist_take_bool(nvl, cookie), value);

	check_empty(nvl, key);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_number);
ATF_TEST_CASE_BODY(cnvlist_take_number)
{
	nvlist_t *nvl;
	const char *key;
	uint64_t value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = 69;

	nvlist_add_number(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NUMBER, key);
	ATF_REQUIRE(nvlist_exists_number(nvl, key));

	ATF_REQUIRE_EQ(cnvlist_take_number(nvl, cookie), value);

	check_empty(nvl, key);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_string);
ATF_TEST_CASE_BODY(cnvlist_take_string)
{
	nvlist_t *nvl;
	const char *key;
	const char *value;
	char *out_string;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = "text";

	nvlist_add_string(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_STRING, key);
	ATF_REQUIRE(nvlist_exists_string(nvl, key));

	out_string = cnvlist_take_string(nvl, cookie);
	ATF_REQUIRE(out_string != NULL);
	ATF_REQUIRE_EQ(strcmp(out_string, value), 0);

	check_empty(nvl, key);
	free(out_string);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_nvlist);
ATF_TEST_CASE_BODY(cnvlist_take_nvlist)
{
	nvlist_t *nvl, *value, *result;
	const char *key, *subkey;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	value = nvlist_create(0);
	check_nvlist(value);

	key = "name";
	subkey = "subname";
	cookie = NULL;

	/* Add null to 'value' nvlist. */

	nvlist_add_null(value, subkey);
	nvlist_next(value, &type, &cookie);
	check_element(value, type, NV_TYPE_NULL, subkey);
	ATF_REQUIRE(nvlist_exists_null(value, subkey));

	/* Add 'value' nvlist. */

	cookie = NULL;
	nvlist_move_nvlist(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NVLIST, key);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, key));

	result = cnvlist_take_nvlist(nvl, cookie);
	ATF_REQUIRE(!nvlist_exists_nvlist(nvl, key));
	ATF_REQUIRE(result == value);

	/* Validate data inside nvlist. */

	cookie = NULL;
	nvlist_next(result, &type, &cookie);
	check_element(result, type, NV_TYPE_NULL, subkey);
	ATF_REQUIRE(nvlist_exists_null(result, subkey));

	nvlist_destroy(nvl);
	nvlist_destroy(value);
}

/* ATF cnvlist_take array tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_bool_array);
ATF_TEST_CASE_BODY(cnvlist_take_bool_array)
{
	nvlist_t *nvl;
	bool in_array[16];
	const bool *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = (i % 2 == 0);

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_bool_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BOOL_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_bool_array(nvl, key));

	out_array = cnvlist_take_bool_array(nvl, cookie, &nitems);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_bool_array(nvl, key));

	ATF_REQUIRE_EQ(nitems, 16);
	ATF_REQUIRE(out_array != NULL);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_number_array);
ATF_TEST_CASE_BODY(cnvlist_take_number_array)
{
	nvlist_t *nvl;
	uint64_t in_array[16];
	const uint64_t *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = i;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_number_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NUMBER_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_number_array(nvl, key));

	out_array = cnvlist_take_number_array(nvl, cookie, &nitems);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_number_array(nvl, key));

	ATF_REQUIRE(out_array != NULL);
	ATF_REQUIRE_EQ(nitems, 16);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_string_array);
ATF_TEST_CASE_BODY(cnvlist_take_string_array)
{
	nvlist_t *nvl;
	const char *in_array[4] = {"inequality", "sucks", ".", ""};
	char **out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_string_array(nvl, key, in_array, 4);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_STRING_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_string_array(nvl, key));

	out_array = cnvlist_take_string_array(nvl, cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 4);
	for (i = 0; i < 4; i++) {
		ATF_REQUIRE(out_array[i] != NULL);
		ATF_REQUIRE_EQ(strcmp(out_array[i], in_array[i]), 0);
	}
	ATF_REQUIRE(nvlist_empty(nvl));

	free(out_array);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_nvlist_array);
ATF_TEST_CASE_BODY(cnvlist_take_nvlist_array)
{
	nvlist_t *testnvl[8];
	nvlist_t **result;
	nvlist_t *nvl;
	void *cookie;
	size_t num_items;
	unsigned int i;
	int type;
	const char *somestr[8] = { "a", "b", "c", "d", "e", "f", "g", "h" };
	const char *key;

	for (i = 0; i < 8; i++) {
		testnvl[i] = nvlist_create(0);
		check_nvlist(testnvl[i]);
		nvlist_add_string(testnvl[i], "nvl/string", somestr[i]);

		cookie = NULL;
		nvlist_next(testnvl[i], &type, &cookie);
		check_element(testnvl[i], type, NV_TYPE_STRING, "nvl/string");
	}

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	key = "nvl/nvlist";
	cookie = NULL;

	nvlist_add_nvlist_array(nvl, key, (const nvlist_t * const *)testnvl, 8);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NVLIST_ARRAY, key);

	result = cnvlist_take_nvlist_array(nvl, cookie, &num_items);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_nvlist_array(nvl, key));

	ATF_REQUIRE(result != NULL);
	ATF_REQUIRE_EQ(num_items, 8);
	for (i = 0; i < num_items; i++) {
		ATF_REQUIRE_EQ(nvlist_error(result[i]), 0);
		ATF_REQUIRE(nvlist_get_array_next(result[i]) == NULL);
	}

	ATF_REQUIRE(!nvlist_exists_string_array(nvl, key));
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_error(nvl), 0);

	for (i = 0; i < 8; i++) {
		nvlist_destroy(result[i]);
		nvlist_destroy(testnvl[i]);
	}

	free(result);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_binary);
ATF_TEST_CASE_BODY(cnvlist_take_binary)
{
	nvlist_t *nvl;
	const char *key;
	void *in_binary;
	const void *out_binary;
	void *cookie;
	int type;
	size_t in_size, out_size;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	in_size = 13;
	in_binary = malloc(in_size);
	ATF_REQUIRE(in_binary != NULL);
	memset(in_binary, 0xa5, in_size);

	nvlist_add_binary(nvl, key, in_binary, in_size);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BINARY, key);
	ATF_REQUIRE(nvlist_exists_binary(nvl, key));

	out_binary = cnvlist_take_binary(nvl, cookie, &out_size);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_binary(nvl, key));

	ATF_REQUIRE_EQ(out_size, in_size);
	ATF_REQUIRE_EQ(memcmp(in_binary, out_binary, out_size), 0);

	nvlist_destroy(nvl);
}

/* ATF cnvlist_free tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_bool);
ATF_TEST_CASE_BODY(cnvlist_free_bool)
{
	nvlist_t *nvl;
	const char *key;
	bool value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = true;

	nvlist_add_bool(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BOOL, key);
	ATF_REQUIRE(nvlist_exists_bool(nvl, key));

	cnvlist_free_bool(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_bool(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_number);
ATF_TEST_CASE_BODY(cnvlist_free_number)
{
	nvlist_t *nvl;
	const char *key;
	uint64_t value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = 69;

	nvlist_add_number(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NUMBER, key);
	ATF_REQUIRE(nvlist_exists_number(nvl, key));

	cnvlist_free_number(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_number(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_string);
ATF_TEST_CASE_BODY(cnvlist_free_string)
{
	nvlist_t *nvl;
	const char *key;
	const char *value;
	char *out_string;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	value = "text";

	nvlist_add_string(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_STRING, key);
	ATF_REQUIRE(nvlist_exists_string(nvl, key));

	cnvlist_free_string(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_string(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_nvlist);
ATF_TEST_CASE_BODY(cnvlist_free_nvlist)
{
	nvlist_t *nvl, *value, *result;
	const char *key, *subkey;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	value = nvlist_create(0);
	check_nvlist(value);

	key = "name";
	subkey = "subname";
	cookie = NULL;

	/* Add null to 'value' nvlist. */

	nvlist_add_null(value, subkey);
	nvlist_next(value, &type, &cookie);
	check_element(value, type, NV_TYPE_NULL, subkey);
	ATF_REQUIRE(nvlist_exists_null(value, subkey));

	/* Add 'value' nvlist. */

	cookie = NULL;
	nvlist_move_nvlist(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NVLIST, key);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, key));

	cnvlist_free_nvlist(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_nvlist(nvl, key));

	nvlist_destroy(value);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_binary);
ATF_TEST_CASE_BODY(cnvlist_free_binary)
{
	nvlist_t *nvl;
	const char *key;
	void *in_binary;
	const void *out_binary;
	void *cookie;
	int type;
	size_t in_size, out_size;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";
	in_size = 13;
	in_binary = malloc(in_size);
	ATF_REQUIRE(in_binary != NULL);
	memset(in_binary, 0xa5, in_size);

	nvlist_add_binary(nvl, key, in_binary, in_size);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BINARY, key);
	ATF_REQUIRE(nvlist_exists_binary(nvl, key));

	cnvlist_free_binary(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_binary(nvl, key));

	nvlist_destroy(nvl);
}

/* ATF cnvlist_take array tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_bool_array);
ATF_TEST_CASE_BODY(cnvlist_free_bool_array)
{
	nvlist_t *nvl;
	bool in_array[16];
	const bool *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = (i % 2 == 0);

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_bool_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_BOOL_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_bool_array(nvl, key));

	cnvlist_free_bool_array(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_bool_array(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_number_array);
ATF_TEST_CASE_BODY(cnvlist_free_number_array)
{
	nvlist_t *nvl;
	uint64_t in_array[16];
	const uint64_t *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = i;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_number_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NUMBER_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_number_array(nvl, key));

	cnvlist_free_number_array(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_number_array(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_string_array);
ATF_TEST_CASE_BODY(cnvlist_free_string_array)
{
	nvlist_t *nvl;
	const char *in_array[4] = {"inequality", "sucks", ".", ""};
	char **out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	cookie = NULL;
	key = "name";

	nvlist_add_string_array(nvl, key, in_array, 4);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_STRING_ARRAY, key);
	ATF_REQUIRE(nvlist_exists_string_array(nvl, key));

	cnvlist_free_string_array(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_string_array(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_nvlist_array);
ATF_TEST_CASE_BODY(cnvlist_free_nvlist_array)
{
	nvlist_t *testnvl[8];
	nvlist_t *nvl;
	void *cookie;
	size_t num_items;
	unsigned int i;
	int type;
	const char *somestr[8] = { "a", "b", "c", "d", "e", "f", "g", "h" };
	const char *key;

	for (i = 0; i < 8; i++) {
		testnvl[i] = nvlist_create(0);
		check_nvlist(testnvl[i]);
		nvlist_add_string(testnvl[i], "nvl/string", somestr[i]);

		cookie = NULL;
		nvlist_next(testnvl[i], &type, &cookie);
		check_element(testnvl[i], type, NV_TYPE_STRING, "nvl/string");
	}

	nvl = nvlist_create(0);
	check_nvlist(nvl);

	key = "nvl/nvlist";
	cookie = NULL;

	nvlist_add_nvlist_array(nvl, key, (const nvlist_t * const *)testnvl, 8);
	nvlist_next(nvl, &type, &cookie);
	check_element(nvl, type, NV_TYPE_NVLIST_ARRAY, key);

	cnvlist_free_nvlist_array(nvl, cookie);
	check_empty(nvl, key);
	ATF_REQUIRE(!nvlist_exists_nvlist_array(nvl, key));

	for (i = 0; i < 8; i++)
		nvlist_destroy(testnvl[i]);

	nvlist_destroy(nvl);
}

ATF_INIT_TEST_CASES(tp)
{
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_nvlist);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_descriptor);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_binary);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_nvlist_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_descriptor_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_bool);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_number);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_string);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_nvlist);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_binary);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_bool_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_number_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_string_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_nvlist_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_bool);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_number);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_string);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_nvlist);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_binary);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_bool_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_number_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_string_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_nvlist_array);
}
